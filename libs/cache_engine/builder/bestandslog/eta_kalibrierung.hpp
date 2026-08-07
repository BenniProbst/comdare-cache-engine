#pragma once
// eta_kalibrierung.hpp -- A5 / F5-ETA-Paket.
//
// QUELLE (bindend): super docs/sessions/20260801-KONSOLIDIERT-gesamtarchitektur-lager-batch-eta-sha512.md,
// Abschnitt "F5: ETA-KALIBRIER-SPEZ". Dort stehen die Baupunkte (1) per-Binary-Timing, (2) Mini-Batch-
// Kalibrierung IM Slice, (3) Fortschreibung ueber das Rest-Fenster, (4) Wiederholung je Block,
// (5) avg_size-Konsumenten, (6) Kampagnen-Projektion -- und die Beweis-Anker.
//
// ABGRENZUNG ZU eta_estimator.hpp -- DER GRUND, WARUM ES ZWEI HEADER GIBT:
// eta_estimator.hpp RECHNET (estimate_eta_s, average_size_bytes, project_slice_eta_s). Er stellt nie die
// Frage, ob gerechnet werden DARF. Genau diese Frage ist der Gegenstand dieses Headers: ab wann traegt eine
// Kalibrierung, was gibt sie aus solange sie es nicht tut, und wann wird sie neu geschrieben. Die Arithmetik
// wird NICHT dupliziert -- sie wird von hier aus gerufen (eine Formel, eine Heimat).
//
// DIE ENTWURFS-ENTSCHEIDUNGEN, ausgeschrieben, weil sie das Ergebnis tragen:
//
// (A) BELASTBARKEIT -- ab wann, und was steht da solange nicht?
//     Schwelle = n_threads Messpunkte. Das ist KEINE gewaehlte Zahl, sondern die Doktrin-Zahl: der
//     Mini-Batch hat per LEDGER:3290 exakt die Groesse "maximale CPU-Thread-Zahl". Unterhalb davon gibt es
//     KEINE Zahl -- nicht eine kleine, nicht eine vorsichtige, sondern gar keine. Der Traeger dafuer ist
//     EtaAuskunft::belastbar, und die beiden Ausgabe-Formen sind:
//       * Draht (Bestandslog-Attribut eta_s): der LEERE String. Das ist die BESTEHENDE Konvention dieses
//         Feldes (reservation_lifecycle.hpp: r.eta_s = "" heisst "noch nicht kalibriert", has_usable_eta("")
//         ist false). KEIN neues Feld, KEIN Draht-Bump -- die honest-empty-Regel war hier schon Gesetz.
//       * Text (Log-/Testat-Zeile): "n/a". Dieselbe Aussage in der Form, die ein Mensch liest.
//     Was es ausdruecklich NICHT gibt: eine 0. Eine 0 in eta_s waere im Feld toedlich und nicht nur
//     unschoen -- is_takeable_by_eta(0, ...) ist SOFORT wahr, eine lebende Maschine wuerde enteignet.
//     Die Wache dagegen steht bereits (builder_registration.hpp A-1); dieser Header erzeugt die 0 gar nicht
//     erst.
//
// (B) AUSREISSER -- Median fuer die PROJEKTION, Summe fuer die BEOBACHTUNG.
//     Die beiden Faelle sind verschieden, und sie verschieden zu behandeln ist der ganze Punkt:
//       * Der vermessene Block ist eine BEOBACHTUNG. Sum(t_i)/N mit Untergrenze max(t_i) ist dort exakt
//         richtig -- ein Bau, der zehnmal so lange lief, hat diese Zeit WIRKLICH gekostet. Ihn
//         herauszumitteln hiesse, eine bereits bezahlte Sekunde zu leugnen. Hier bleibt die Doktrin-Formel
//         unveraendert (estimate_eta_s).
//       * Das Rest-Fenster ist eine PROJEKTION. Hier ist derselbe Ausreisser ein Hebel: bei n = 32 Punkten
//         zieht EIN Bau mit 10x der typischen Dauer das arithmetische Mittel um rund 28 Prozent hoch, und
//         diese 28 Prozent werden mit 4096 Rest-Binaries multipliziert. Der Median ruehrt sich dabei nicht.
//         Deshalb speist median_t_s() die Projektion, nicht der Mittelwert.
//     Gleitendes Fenster: NEIN als Ringpuffer, JA als Block-Schnitt. neuer_block() verwirft die Zeiten des
//     alten Blocks (LEDGER:3299 verlangt die Wiederholung je Block -- ein warm gelaufener ccache oder eine
//     thermisch gedrosselte Maschine sollen sich in der naechsten Zahl zeigen, nicht in einem ueber den
//     ganzen Lauf verschmierten Mittel). Was den Block UEBERLEBT, ist allein longest_seen_s: die
//     Untergrenze ist eine physikalische Aussage ueber die Bau-Last dieser Kampagne, kein Block-Datum. Sie
//     ist damit konservativ in die SICHERE Richtung -- eine zu KURZE ETA ist die gefaehrliche, weil sie den
//     Takeover einer lebenden Maschine verfrueht (is_takeable_by_eta misst gegen 1,5 x ETA).
//
// (C) RE-KALIBRIERUNG -- wann neu schreiben?
//     Zwei Ausloeser, und keiner davon ist "alle N Binaries":
//       * PFLICHT je Block (LEDGER:3299). Der Block-Wechsel ist der Takt der Doktrin.
//       * DAZWISCHEN nur bei relevanter Abweichung: kReKalibrierAbweichung (25 Prozent, s.u.). Der Grund
//         ist nicht Sparsamkeit an sich -- der ETA-Schreibvorgang ist laut ABNAHME-1 der EINZIGE lange
//         exklusive Lock-Fall des ganzen Systems. Ein zaehl-getriebener Trigger ("alle 100 Binaries")
//         schreibt bei ruhiger Bau-Last sinnlos oft und bei unruhiger zu selten; ein abweichungs-
//         getriebener schreibt genau dann, wenn sich die Aussage geaendert hat.
//     kReKalibrierAbweichung = 0.25 ist GEWAEHLT, NICHT GEMESSEN. Sie steht als benannte Konstante und als
//     Default-Parameter da, damit sie widerlegt und ersetzt werden kann, ohne den Code zu suchen. Sie als
//     hergeleitet auszugeben waere genau die Sorte Nachkommastelle, gegen die dieser Header gebaut ist.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, nur stdlib + die B4-Bestandslog-Header. Kein Runtime-
// Switch, kein std::variant, keine vtable. Die Klasse ist NICHT thread-sicher -- der Aufrufer (Build-
// Worker-Hook) haelt seinen eigenen Mutex; so bleibt sie zeit- und sperrfrei literal testbar.

#include "eta_estimator.hpp"
#include "reservation_lifecycle.hpp" // format_seconds: die EINE Zahl->Draht-Bruecke (nicht neu erfunden)

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional> // die Groesse eines FEHLGESCHLAGENEN Compiles existiert nicht -- nullopt, nicht 0
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Der Text-Marker fuer "nicht erhoben". Die Draht-Form derselben Aussage ist der LEERE String (s. Kopf (A)).
inline constexpr std::string_view kEtaUnbekannt = "n/a";

// Relative Abweichung, ab der zwischen zwei Bloecken neu geschrieben wird. GEWAEHLT, NICHT GEMESSEN.
inline constexpr double kReKalibrierAbweichung = 0.25;

// ---------------------------------------------------------------------------
// Die Auskunft: eine Zahl NUR mit ihrer Belastbarkeit zusammen. Die beiden Felder sind bewusst nicht
// trennbar -- wer eta_s liest, muss an belastbar vorbei. Genau das verhindert die stille erfundene Null.
// ---------------------------------------------------------------------------
struct EtaAuskunft {
    bool          belastbar      = false; // false => eta_s ist BEDEUTUNGSLOS, nicht "0"
    double        eta_s          = 0.0;
    std::uint64_t avg_size_bytes = 0;
    std::size_t   punkte         = 0; // Zeit-Messpunkte, die die ETA tragen
    std::size_t   noetige_punkte = 0; // wie viele es braucht (die Doktrin-Zahl n_threads)
    // Die GROESSEN-Grundlage wird GETRENNT gezaehlt, weil sie eine andere Menge ist: ein FEHLGESCHLAGENER
    // Compile liefert eine Dauer (die Zeit ist verbraucht), aber KEINE Binary. Beide in einen Zaehler zu
    // legen hiesse, avg_size mit Nullen zu verduennen -- eine erfundene Zahl auf dem Umweg ueber den
    // Mittelwert. groessen_punkte == 0 heisst: avg_size_bytes ist nicht erhoben.
    std::size_t groessen_punkte = 0;

    friend bool operator==(EtaAuskunft const&, EtaAuskunft const&) = default;
};

// Draht-Form (Bestandslog-Attribut eta_s): leer == nicht kalibriert. Das ist die BESTEHENDE Konvention
// des Feldes, kein neuer Vertrag -- has_usable_eta("") liefert false und der pro-forma-Zweig greift.
[[nodiscard]] inline std::string eta_draht(EtaAuskunft const& a) {
    return a.belastbar ? format_seconds(a.eta_s) : std::string{};
}

// Text-Form (Log-/Testat-Zeile): "n/a" statt einer Zahl, solange die Aussage nicht traegt.
[[nodiscard]] inline std::string eta_text(EtaAuskunft const& a) {
    return a.belastbar ? format_seconds(a.eta_s) : std::string{kEtaUnbekannt};
}

// Draht-Form der GROESSE -- eigener Befund, eigene Leere (s. EtaAuskunft::groessen_punkte).
[[nodiscard]] inline std::string avg_size_draht(EtaAuskunft const& a) {
    return (a.groessen_punkte > 0) ? std::to_string(a.avg_size_bytes) : std::string{};
}

// Die Kalibrierung EHRLICH in die Reservierung uebertragen.
//
// UNTERSCHIED ZU apply_calibration (B4, reservation_lifecycle.hpp): jenes schreibt BEIDE Felder
// unbedingt -- bei fehlender Grundlage also "0.000" und "0". Das war fuer den bisherigen EINEN Aufrufer
// richtig (er kalibriert post-hoc mit fertiger Wanduhr und fertigen Groessen, beides liegt vor). Fuer die
// LAUFENDE Kalibrierung stimmt es nicht mehr: dort kann die Zeit-Grundlage tragen, waehrend noch keine
// Binary fertig ist. Deshalb gilt die honest-empty-Regel hier JE FELD. apply_calibration bleibt
// unangetastet (bestehender Aufrufer, bestehende Tests).
inline void uebertrage_kalibrierung(BatchReservierung& r, EtaAuskunft const& a) {
    r.eta_s          = eta_draht(a);
    r.avg_size_bytes = avg_size_draht(a);
}

// Soll zwischen zwei Bloecken neu geschrieben werden? (s. Kopf (C))
//   * jetzt nicht belastbar        -> NEIN (eine Nicht-Aussage wird nie veroeffentlicht)
//   * noch nie geschrieben         -> JA   (die erste Kalibrierung ist immer eine Neuigkeit)
//   * sonst relative Abweichung    -> JA, wenn sie die Schwelle REISST (strikt groesser)
[[nodiscard]] inline bool eta_neu_schreiben(double zuletzt_s, EtaAuskunft const& jetzt,
                                            double schwelle = kReKalibrierAbweichung) noexcept {
    if (!jetzt.belastbar || !(jetzt.eta_s > 0.0) || !std::isfinite(jetzt.eta_s)) return false;
    if (!(zuletzt_s > 0.0) || !std::isfinite(zuletzt_s)) return true;
    return std::fabs(jetzt.eta_s - zuletzt_s) / zuletzt_s > schwelle;
}

// ---------------------------------------------------------------------------
// Der Sammler eines Bau-Blocks. Nimmt je fertigem Compile EINEN Punkt (Dauer + Groesse) entgegen und
// beantwortet daraus die drei Fragen des Kopfes. NICHT thread-sicher (s. Kopf).
// ---------------------------------------------------------------------------
class EtaKalibrierung {
public:
    // n_threads == 0 wird als 1 gelesen -- dieselbe Division-Schutz-Regel wie im eta_estimator, damit die
    // beiden Header nicht zwei Auslegungen derselben Null tragen.
    explicit EtaKalibrierung(unsigned n_threads) noexcept : n_threads_{(n_threads == 0) ? 1u : n_threads} {}

    // Ein abgeschlossener Compile. NUR echte Compiles gehoeren hier hinein: ein Lager-/Sidecar-Skip hat
    // nichts gebaut und ist kein Messpunkt (der Aufrufer filtert -- BuildResult::dauer_s ist genau deshalb
    // ein optional und keine 0). Nicht-endliche oder nicht-positive Dauern werden VERWORFEN statt als 0
    // gebucht; sie sind ein Uhren-Befund, kein Datum.
    //
    // size_bytes ist OPTIONAL und wird GETRENNT gefuehrt: ein fehlgeschlagener Compile hat Zeit gekostet
    // (er gehoert in die ETA), aber keine Binary hinterlassen (er gehoert NICHT in avg_size). Ihn dort mit
    // 0 zu buchen waere eine erfundene Zahl im Mittelwert.
    void beobachte(double t_s, std::optional<std::uint64_t> size_bytes) {
        if (!std::isfinite(t_s) || !(t_s > 0.0)) return;
        zeiten_.push_back(t_s);
        if (size_bytes) groessen_.push_back(*size_bytes);
        if (t_s > longest_seen_s_) longest_seen_s_ = t_s;
    }

    [[nodiscard]] std::size_t punkte() const noexcept { return zeiten_.size(); }
    [[nodiscard]] std::size_t groessen_punkte() const noexcept { return groessen_.size(); }
    [[nodiscard]] std::size_t noetige_punkte() const noexcept { return static_cast<std::size_t>(n_threads_); }
    [[nodiscard]] unsigned    threads() const noexcept { return n_threads_; }

    // Die EINE Belastbarkeits-Frage. Alles andere in diesem Header haengt daran.
    [[nodiscard]] bool belastbar() const noexcept { return zeiten_.size() >= noetige_punkte(); }

    // Untergrenze ueber den GANZEN Lauf (ueberlebt neuer_block(), s. Kopf (B)).
    [[nodiscard]] double longest_seen_s() const noexcept { return longest_seen_s_; }

    // Median der Block-Zeiten -- der ausreisser-feste Vertreter fuer die PROJEKTION. Bei gerader Anzahl
    // das arithmetische Mittel der beiden mittleren Werte (Standard-Definition, damit der Test literal ist).
    // Leere Menge -> 0.0; dieser Wert ist NIE eine Ausgabe, er ist durch belastbar() abgeschirmt.
    [[nodiscard]] double median_t_s() const {
        if (zeiten_.empty()) return 0.0;
        std::vector<double> kopie = zeiten_; // nth_element sortiert teilweise -- die Beobachtung bleibt unberuehrt
        std::size_t const   n     = kopie.size();
        std::size_t const   mitte = n / 2;
        std::nth_element(kopie.begin(), kopie.begin() + static_cast<std::ptrdiff_t>(mitte), kopie.end());
        double const oben = kopie[mitte];
        if (n % 2 == 1) return oben;
        // Der groesste Wert der unteren Haelfte ist nach nth_element das Maximum von [begin, mitte).
        double const unten = *std::max_element(kopie.begin(), kopie.begin() + static_cast<std::ptrdiff_t>(mitte));
        return (unten + oben) / 2.0;
    }

    // BEOBACHTUNG des vermessenen Blocks: die Doktrin-Formel, unveraendert (Summe, Untergrenze max(t_i)).
    [[nodiscard]] EtaAuskunft block_auskunft() const {
        EtaAuskunft a = rumpf();
        if (!a.belastbar) return a; // KEINE Zahl unterhalb der Schwelle -- auch keine vorsichtige
        a.eta_s = estimate_eta_s(std::span<double const>{zeiten_}, n_threads_);
        return a;
    }

    // PROJEKTION auf das Rest-Fenster: Median als Vertreter, longest_seen als harte Untergrenze.
    // rest == 0 (nichts mehr offen) ist eine ECHTE Null und keine fehlende Zahl -> belastbar mit eta_s 0.
    [[nodiscard]] EtaAuskunft rest_projektion(std::uint64_t rest) const {
        EtaAuskunft a = rumpf();
        if (!a.belastbar) return a;
        if (rest == 0) {
            a.eta_s = 0.0; // nichts offen == null Restzeit; das ist gemessen, nicht erfunden
            return a;
        }
        a.eta_s = project_slice_eta_s(median_t_s(), rest, n_threads_, longest_seen_s_);
        return a;
    }

    // Block-Schnitt (LEDGER:3299): die Zeiten des alten Blocks fallen, die Untergrenze bleibt (s. Kopf (B)).
    void neuer_block() noexcept {
        zeiten_.clear();
        groessen_.clear();
    }

private:
    // Der gemeinsame Kopf beider Auskuenfte: Zaehlwerk + Belastbarkeit + die Groessen-Seite (die von der
    // ETA-Schwelle UNABHAENGIG ist -- sie hat ihre eigene Grundlage, s. EtaAuskunft::groessen_punkte).
    [[nodiscard]] EtaAuskunft rumpf() const {
        EtaAuskunft a;
        a.punkte          = zeiten_.size();
        a.noetige_punkte  = noetige_punkte();
        a.belastbar       = belastbar();
        a.groessen_punkte = groessen_.size();
        if (!groessen_.empty()) a.avg_size_bytes = average_size_bytes(std::span<std::uint64_t const>{groessen_});
        return a;
    }

    unsigned                   n_threads_;
    std::vector<double>        zeiten_;
    std::vector<std::uint64_t> groessen_;
    double                     longest_seen_s_ = 0.0; // ueberlebt neuer_block() BEWUSST
};

// ---------------------------------------------------------------------------
// Baupunkt (6): die KAMPAGNEN-PROJEKTION.
//
// KEIN PRODUKTIONS-AUFRUFER (Stand A5) -- und das steht hier, damit es niemand erst wieder herausfinden
// muss. Genau diese Sorte Halb-Verdrahtung war der Anlass des ganzen Pakets: project_slice_eta_s stand
// jahrelang korrekt gerechnet und ungerufen da. Der Unterschied zu damals ist, dass der Grund hier
// BENANNT und nicht bloss unbemerkt ist:
//
//   Die Projektion braucht Posten je (MASCHINE x PERM). BatchReservierung traegt heute maschine,
//   threads, slice_begin/count, eta_s und avg_size_bytes -- aber KEIN Perm-Feld. Die Zuordnung einer
//   Kalibrierung zu ihrer System-Permutation ist im Bestandslog also gar nicht ablesbar. Sie
//   nachzuruesten ist ein syntax_version-Bump des Dokuments = Draht-Entscheid, kein Bau-Detail.
//
// Bis dahin ist dies eine geprueft rechnende Funktion mit ehrlicher Ausgabe, deren Eingabe der Aufrufer
// aus einer anderen Quelle beschaffen muss (z.B. dem Planer, der die Perm-Zuordnung ohnehin kennt).
// Sie erfindet sich diese Zuordnung ausdruecklich NICHT selbst.
//
// Eingabe sind die im Bestandslog liegenden Kalibrier-Posten je (Maschine x Perm). Jeder Posten sagt:
// "diese Maschine hat fuer diese Perm N Binaries in eta_s Sekunden gebaut". Daraus wird die Voll-Bau-Zeit
// hochgerechnet.
//
// DAS MODELL, ausgeschrieben, weil eine Projektion ohne benanntes Modell eine Behauptung ist:
//   * Auf EINER Maschine laufen die Perms NACHEINANDER  -> je Maschine wird summiert.
//   * Die Maschinen laufen NEBENEINANDER                 -> ueber die Maschinen wird das MAXIMUM genommen
//     (die Kampagne ist fertig, wenn die langsamste Maschine fertig ist).
//
// UNVOLLSTAENDIGE DATEN ERGEBEN KEINE SCHAETZUNG, SONDERN EINE UNTERE SCHRANKE. Fehlt auch nur eine
// (Maschine x Perm)-Zelle, ist die Summe je Maschine zu klein und damit das Maximum zu klein. Das Ergebnis
// wird dann als vollstaendig == false gefuehrt und ist als ">= x" zu lesen, nie als "x". Ein Posten ohne
// Bezugsgroesse (binaries_gemessen == 0) oder ohne brauchbare Zeit wird VERWORFEN und gezaehlt -- er geht
// nicht als 0 in den Mittelwert ein.
// ---------------------------------------------------------------------------
struct KampagnenPosten {
    std::string   maschine;                // Wirt, auf dem gemessen wurde
    std::string   perm;                    // System-Permutation
    double        eta_s             = 0.0; // gemessene Zeit fuer binaries_gemessen Stueck
    std::uint64_t binaries_gemessen = 0;   // die Bezugsgroesse -- ohne sie ist eta_s nicht skalierbar
    std::uint64_t avg_size_bytes    = 0;
};

struct KampagnenProjektion {
    bool          vollstaendig = false; // false => wandzeit_s ist eine UNTERE SCHRANKE, keine Schaetzung
    bool          belastbar    = false; // false => es gibt gar keine Aussage (kein einziger gueltiger Posten)
    double        wandzeit_s   = 0.0;   // max ueber die Maschinen (Modell s.o.)
    double        rechenzeit_s = 0.0;   // Summe ueber ALLE Maschinen (Flotten-Gesamtlast, eindeutig)
    std::uint64_t bytes        = 0;     // Speicher-Vorschau (Baupunkt 5): avg_size x Binaries der Kampagne
    std::size_t   zellen_ist   = 0;     // verwertbare (Maschine x Perm)-Posten
    std::size_t   zellen_soll  = 0;     // erwartete Zellen der Kampagne
    std::size_t   verworfen    = 0;     // Posten ohne Bezugsgroesse/ohne brauchbare Zeit

    friend bool operator==(KampagnenProjektion const&, KampagnenProjektion const&) = default;
};

// binaries_je_perm = die Zielgroesse EINER Perm (golden: 131072). zellen_soll = erwartete Maschine-x-Perm-
// Zellen (z.B. Maschinen x 12). Beide kommen vom Aufrufer -- dieser Header raet keine Kampagnen-Groesse.
[[nodiscard]] inline KampagnenProjektion projiziere_kampagne(std::span<KampagnenPosten const> posten,
                                                             std::uint64_t binaries_je_perm, std::size_t zellen_soll) {
    KampagnenProjektion p;
    p.zellen_soll = zellen_soll;

    // Je Maschine die Summe ihrer Perm-Zeiten. Kleine Kampagnen (< 100 Maschinen) -> lineare Suche statt
    // map: weniger Maschinerie, deterministische Reihenfolge, kein Allokator-Verhalten im Testpfad.
    std::vector<std::string> wirte;
    std::vector<double>      wirt_summe_s;
    std::uint64_t            groessen_summe = 0;

    for (auto const& q : posten) {
        // Ein Posten ohne Bezugsgroesse ist nicht skalierbar, einer ohne endliche positive Zeit ist keine
        // Messung. Beide werden gezaehlt und verworfen -- NICHT als 0 eingerechnet.
        if (q.binaries_gemessen == 0 || !std::isfinite(q.eta_s) || !(q.eta_s > 0.0)) {
            ++p.verworfen;
            continue;
        }
        double const je_binary_s = q.eta_s / static_cast<double>(q.binaries_gemessen);
        double const perm_s      = je_binary_s * static_cast<double>(binaries_je_perm);

        auto const it = std::find(wirte.begin(), wirte.end(), q.maschine);
        if (it == wirte.end()) {
            wirte.push_back(q.maschine);
            wirt_summe_s.push_back(perm_s);
        } else {
            wirt_summe_s[static_cast<std::size_t>(it - wirte.begin())] += perm_s;
        }
        groessen_summe += q.avg_size_bytes;
        ++p.zellen_ist;
    }

    if (p.zellen_ist == 0) return p; // belastbar bleibt false -- KEINE Zahl, nicht eine 0

    p.belastbar    = true;
    p.vollstaendig = (zellen_soll != 0) && (p.zellen_ist >= zellen_soll);
    p.rechenzeit_s = 0.0;
    for (double s : wirt_summe_s) {
        p.rechenzeit_s += s;
        if (s > p.wandzeit_s) p.wandzeit_s = s;
    }
    // Speicher-Vorschau: mittlere Binary-Groesse ueber die verwertbaren Posten x Binaries der GANZEN
    // Kampagne (zellen_soll Zellen a binaries_je_perm). Ohne zellen_soll gibt es keine Kampagnen-Groesse
    // -> dann bezieht sich die Zahl ehrlich nur auf die vorhandenen Zellen.
    std::uint64_t const avg          = groessen_summe / static_cast<std::uint64_t>(p.zellen_ist);
    std::size_t const   bezugszellen = (zellen_soll != 0) ? zellen_soll : p.zellen_ist;
    p.bytes                          = avg * binaries_je_perm * static_cast<std::uint64_t>(bezugszellen);
    return p;
}

// Die literale Ausgabe-Zeile (Baupunkt 6: "kein Erfolg ohne Ausgabe"). Sie traegt das Verhaeltnis
// zellen_ist/zellen_soll IMMER mit -- eine Projektion ohne ihren Nenner ist der Fehler, den sie verhindern
// soll. Unvollstaendig -> ">=" vor der Zahl. Gar keine Daten -> "n/a" und keine Zahl.
[[nodiscard]] inline std::string kampagnen_zeile(KampagnenProjektion const& p) {
    std::string out = "kampagne_eta_s=";
    if (!p.belastbar) {
        out += kEtaUnbekannt;
    } else {
        if (!p.vollstaendig) out += ">=";
        out += format_seconds(p.wandzeit_s);
    }
    out += " rechenzeit_s=";
    out += p.belastbar ? format_seconds(p.rechenzeit_s) : std::string{kEtaUnbekannt};
    out += " bytes=";
    out += p.belastbar ? std::to_string(p.bytes) : std::string{kEtaUnbekannt};
    out += " zellen=" + std::to_string(p.zellen_ist) + "/" + std::to_string(p.zellen_soll);
    out += " verworfen=" + std::to_string(p.verworfen);
    return out;
}

} // namespace comdare::cache_engine::builder::bestandslog
