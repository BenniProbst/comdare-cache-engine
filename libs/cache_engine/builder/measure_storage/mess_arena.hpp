#pragma once
// mess_arena.hpp -- MeasureStorage (2026-08-09): DIE ERSTE DER ZWEI ARENEN -- die MESS-ARENA.
//
// SELBSTCHECK: diese Datei kennt WEDER die Mess-Ebene NOCH das Modul NOCH die Funktion, in der der
// Checkpoint steht. Sie kennt nur Zeilen und einen Index. Die Frage "wo bin ich gerade?" beantwortet
// die ANDERE Arena (stapel_arena.hpp) -- das ist keine Arbeitsteilung aus Geschmack, sondern die vom
// Owner am 09.08.2026 vorgegebene Trennung. Wer hier eine Verschachtelungs-Tiefe oder einen
// Aufrufer sucht, ist in der falschen Datei.
//
// == WAS DIESE ARENA IST (Owner-KERN 09.08.2026, woertlich) =========================================
//     "es gibt eine Arena fuer die Messergebnisse mit nur append und Auswertung zum Schluss"
//
//   Zugriff:    NUR ANHAENGEN. Kein Ueberschreiben, kein Ring, kein Loeschen.
//   Wachstum:   MONOTON ueber den ganzen Lauf. Sie wird NIE zurueckgesetzt.
//   Dimension:  die erwartete EREIGNISZAHL (Millionen), nicht die Verschachtelungstiefe.
//   Auswertung: ZUM SCHLUSS, nach dem Lauf.
//   Inhalt:     POD-Zeilen, keine Verweise.
//
// KEIN RING, ausdruecklich (Deep-Research-Punkt 2): ein Ring loest das Problem "alter Inhalt darf
// weg" -- das existiert hier nicht. Der Owner verlangt "nie ueberschreiben", und es gibt keinen
// nebenlaeufigen Leser. Perfetto, LTTng, Tracy und moodycamel konvergieren unabhaengig auf dasselbe
// lineare Muster; LTTng entartet in seinem Discard-Modus funktional exakt zu "linear plus
// Verlustzaehler" -- und genau das ist hier gebaut.
//
// == DIE UEBERLAUF-POLITIK -- UND WARUM SIE EINE EIGENE IST ========================================
// Eine volle Mess-Arena bedeutet DATENVERLUST. Das ist eine ANDERE Fehlerklasse als ein voller
// Stapel (der bedeutet zu tiefe Verschachtelung, also einen Programmierfehler) -- die beiden duerfen
// sich keine Meldung teilen. Deshalb hat jede Arena ihren eigenen Befund-Typ und ihren eigenen
// Meldetext, und es gibt keine gemeinsame Oberklasse, ueber die sie zusammenfielen.
//
// Drei Wege waeren denkbar gewesen, zwei sind verworfen:
//   - HARTER ABBRUCH: verstoesst gegen die Fehlerklassen-Doktrin (nie abbrechen) und wirft eine
//     mehrtaegige Kampagne wegen einer Kapazitaets-Fehlschaetzung weg.
//   - STILLES VERWERFEN: die schlimmste Variante. Die Messung sieht vollstaendig aus und ist es
//     nicht; niemand kann das spaeter noch feststellen.
//   - GEBAUT: ZAEHLEN, MELDEN, WEITERMESSEN. Der Versuchs-Zaehler laeuft UEBER die Kapazitaet
//     hinaus weiter, damit `verloren = versuche - kapazitaet` exakt ist und nicht geschaetzt. Der
//     heisse Pfad tut dafuer nichts Zusaetzliches -- er zaehlt ohnehin hoch.
//
// DAMIT "MELDEN" NICHT ZU "STILL" ENTARTET: das Auslesen gibt die Zeilen NICHT nackt heraus. Es
// gibt ein AusleseErgebnis, in dem die Zeilen und der Ueberlauf-Befund im SELBEN Objekt stecken.
// Ein Konsument, der die Daten nimmt, hat den Verlust zwangslaeufig in der Hand. Er kann ihn
// ignorieren -- aber nicht, ohne es zu tun.

#include "mess_speicher_kanon.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <type_traits>

namespace comdare::cache_engine::builder::measure_storage {

// == Die Mess-Ebene und die Richtung -- beide compile-time, nie ein Laufzeit-Schalter =============
//
// SELBSTCHECK: hier steht KEIN switch. Ebene und Richtung werden vom Aufrufer als constexpr-Wert
// uebergeben und in EIN Byte gefaltet; der heisse Pfad liest sie nie, er kopiert sie nur mit.
enum class MessEbene : std::uint8_t { Compare = 0, Macro = 1, Micro = 2, Reserviert = 3 };
enum class Richtung : std::uint8_t { Ein = 0, Aus = 1 };

/// tag = 2 Bit Ebene (0..1) + 1 Bit Richtung (2). Bits 3..7 bleiben frei.
[[nodiscard]] constexpr std::uint8_t tag_bauen(MessEbene e, Richtung r) noexcept {
    return static_cast<std::uint8_t>((static_cast<std::uint8_t>(e) & 0x03u) |
                                     static_cast<std::uint8_t>((static_cast<std::uint8_t>(r) & 0x01u) << 2u));
}
[[nodiscard]] constexpr MessEbene ebene_aus_tag(std::uint8_t tag) noexcept {
    return static_cast<MessEbene>(tag & 0x03u);
}
[[nodiscard]] constexpr Richtung richtung_aus_tag(std::uint8_t tag) noexcept {
    return static_cast<Richtung>((tag >> 2u) & 0x01u);
}

// == Die Zeile -- 32 Byte, POD ====================================================================
//
// DER NAME IST DER DRITTE, UND DAS MIT ABSICHT. Im Haus sind bereits vergeben:
//   measurement::MeasurementRecord              (32 B, include/cache_engine/measurement/)
//   benchmark_suite::MeasurementRecord32         (32 B, libs/test_infra/benchmark_suite/)
// Ein dritter "MeasurementRecord" waere eine Verwechslungsquelle in einem Haus, in dem beide
// bestehenden Namen 32 Byte gross sind und keiner von beiden hierher gehoert.
//
// 32 Byte ist keine gegriffene Groesse: zwei Zeilen je Cacheline, und dieselbe Zeilenbreite fuehrt
// Tracy extern -- die Hausgroesse ist damit stand-der-technik-konform.
struct alignas(32) MessCheckpointZeile {
    std::uint64_t zeit_ticks = 0; // billiger monotoner Zaehler, einmal je Lauf gegen die Systemzeit verankert
    std::uint64_t messwert   = 0; // der aufgenommene Wert (Bedeutung traegt der Deskriptor)

    // RESERVE, ehrlich benannt: die Deskriptor-Konsolidierung (Soll-Design OP-1/OP-2) ist NICHT
    // entschieden. Byte-Layout eines Feldes zu erfinden, dessen Bedeutung offen ist, waere ein
    // ABI-Bruch auf Vorrat. Heute immer 0.
    std::uint64_t reserviert_64 = 0;

    std::uint32_t deskriptor_ix = 0; // Index in die statische Deskriptor-Tabelle (Ziel, Ebene, Achse)
    std::uint16_t thread_nr     = 0; // bleibt im Satz, auch wenn Measure 1-Faden ist (Debug=parallel)
    std::uint8_t  tag           = 0; // tag_bauen(Ebene, Richtung)
    std::uint8_t  reserviert_8  = 0;
};

static_assert(sizeof(MessCheckpointZeile) == 32, "Mess-Zeile muss exakt 32 Byte sein (2 je Cacheline)");
static_assert(alignof(MessCheckpointZeile) == 32, "Mess-Zeile muss 32-Byte ausgerichtet sein");
static_assert(std::is_trivially_copyable_v<MessCheckpointZeile>, "Mess-Zeile muss trivially copyable sein");
static_assert(std::is_standard_layout_v<MessCheckpointZeile>, "Mess-Zeile muss standard_layout sein");

// == Der heisse Zustand -- eigene Cacheline, trivially copyable ====================================
//
// WARUM DAS EIN EIGENER TYP IST: genau diese drei Felder fasst der heisse Pfad an. Als eigener Typ
// tragen sie die Zusicherungen unten, und zwar HIER und nur hier.
//
// WAS DER trivially_copyable-ASSERT LEISTET -- und was nicht (praezisiert 09.08.2026). Er faengt
// die FEHLERKLASSE des Bestands-Defekts, wenn sie IN DIESEM TYP auftritt: ThreadArena
// (include/cache_engine/measurement/thread_arena.hpp:35) haelt einen std::vector als Member, und ein
// Vektor-Member zerstoert trivially_copyable. Wer denselben Griff hier hineintraegt, faellt beim
// Uebersetzen auf. Er faengt NICHT den Defekt in ThreadArena selbst -- eine Zusicherung in dieser
// Datei kann ueber eine fremde Datei nichts aussagen. Die frueher hier stehende Formulierung
// ("faengt den BESTANDS-DEFEKT") war genau diese Verwechslung; der Bestand bleibt sichtbar ueber
// Abschnitt (1) von test_ms1, der ihn MISST.
struct alignas(kCacheLineBytes) MessArenaHeiss {
    MessCheckpointZeile* basis      = nullptr;
    std::uint64_t        kapazitaet = 0;
    std::uint64_t        versuche   = 0; // laeuft UEBER die Kapazitaet weiter -- daher ist der Verlust exakt
};

static_assert(std::is_trivially_copyable_v<MessArenaHeiss>, "heisser Zustand muss trivially copyable sein");
static_assert(std::is_standard_layout_v<MessArenaHeiss>, "heisser Zustand muss standard_layout sein");
static_assert(alignof(MessArenaHeiss) == kCacheLineBytes, "heisser Zustand muss auf einer Cacheline beginnen");
// GENAU EINE Cacheline, nicht "ein Vielfaches davon". Der frueher hier stehende Assert
// (sizeof % kCacheLineBytes == 0) KONNTE NIE FEUERN: die Norm verlangt, dass sizeof ein Vielfaches
// von alignof ist, und alignof ist eine Zeile darueber bereits auf kCacheLineBytes festgenagelt --
// die Zeile war damit von der Zeile darueber schon impliziert. Diese hier feuert: ein viertes Feld
// im heissen Zustand macht sizeof zu 128, und dann teilt sich der heisse Pfad zwei Linien statt
// einer. Genau das soll auffallen.
static_assert(sizeof(MessArenaHeiss) == kCacheLineBytes, "der heisse Zustand muss in GENAU eine Cacheline passen");

// == Der Ueberlauf-Befund der MESS-Arena -- Fehlerklasse DATENVERLUST ==============================
struct UeberlaufBefund {
    std::uint64_t kapazitaet = 0; // wie viele Zeilen Platz hatten
    std::uint64_t versuche   = 0; // wie viele angeboten wurden (kann groesser sein)
    std::uint64_t belegt     = 0; // wie viele wirklich liegen (min(versuche, kapazitaet))
    std::uint64_t verloren   = 0; // versuche - kapazitaet, sonst 0

    [[nodiscard]] constexpr bool hat_verlust() const noexcept { return verloren != 0u; }
};

// == Der Rueckgabe-Vertrag der Meldezeilen -- zwei Zahlen, nicht eine ==============================
//
// BEFUND, der diesen Typ erzwingt (09.08.2026): beide befund_zeile()-Ueberladungen gaben den
// snprintf-Rueckgabewert unveraendert weiter und versprachen im Doxygen-Satz "geschriebene Zeichen".
// snprintf liefert aber die Zahl, die WAERE geschrieben worden. Nachstellbar in test_ms1, Abschnitt
// (15): auf einem 16-Byte-Puffer meldet die Verlust-Zeile benoetigt = 123, im Puffer stehen 15
// Zeichen. Mit [[nodiscard]] laedt eine solche Zahl zum Weiterrechnen ein (puffer + rc), und das
// zeigte 108 Byte hinter das Puffer-Ende.
//
// ENTSCHIEDEN ist die dritte Moeglichkeit, nicht die zwei naheliegenden. Nur die Doku zu
// korrigieren liesse die Falle stehen; nur die geschriebene Laenge zurueckzugeben verschwiege das
// ABSCHNEIDEN, und eine abgeschnittene Diagnose ist die schlimmste Sorte Diagnose -- sie sieht
// vollstaendig aus. Zurueck kommen deshalb BEIDE Zahlen in EINEM Objekt, nach demselben Muster wie
// AusleseErgebnis weiter unten: wer die Laenge nimmt, hat das Abschneiden zwangslaeufig in der Hand.
struct MeldungsLaenge {
    std::size_t geschrieben  = 0;     // Zeichen IM PUFFER, ohne Abschluss-Null -- immer < n
    std::size_t benoetigt    = 0;     // Zeichen, die noetig gewesen waeren, ohne Abschluss-Null
    bool        formatfehler = false; // snprintf hat negativ gemeldet (Kodierungsfehler)

    [[nodiscard]] constexpr bool abgeschnitten() const noexcept { return formatfehler || benoetigt > geschrieben; }
};

/// Rechnet den snprintf-Rueckgabewert in den ehrlichen Vertrag um. EIN snprintf-Aufruf genuegt:
/// sein Rueckgabewert IST `benoetigt`, und was davon im Puffer steht, ergibt sich aus n.
[[nodiscard]] constexpr MeldungsLaenge meldung_verbuchen(int roh, std::size_t n) noexcept {
    if (roh < 0) return MeldungsLaenge{0u, 0u, true};
    std::size_t const benoetigt   = static_cast<std::size_t>(roh);
    std::size_t const geschrieben = (n == 0u) ? 0u : (benoetigt < n ? benoetigt : n - 1u);
    return MeldungsLaenge{geschrieben, benoetigt, false};
}

/// Schreibt die Verlust-Meldung in einen vom Aufrufer gestellten Puffer.
///
/// KEIN ostream, KEIN Text-Objekt: die Meldung muss auch dann formulierbar sein, wenn gerade nichts
/// belegt werden darf.
///
/// Rueckgabe: siehe MeldungsLaenge -- `geschrieben` sind die Zeichen IM Puffer, `benoetigt` die
/// noetigen. Ein Aufruf mit n == 0 (puffer darf dann nullptr sein) schreibt nichts und liefert
/// trotzdem `benoetigt` -- so laesst sich der Puffer ausmessen, bevor er gestellt wird.
///
/// Der Text nennt ZAHL UND NENNER ("N von M"), nie nur "Ueberlauf" -- eine Meldung ohne Nenner
/// laesst offen, ob zwei oder zwei Millionen Zeilen fehlen.
[[nodiscard]] inline MeldungsLaenge befund_zeile(UeberlaufBefund const& b, char* puffer, std::size_t n) noexcept {
    if (puffer == nullptr) n = 0u; // snprintf erlaubt (nullptr, 0) ausdruecklich -- nur diesen Fall
    int const roh =
        !b.hat_verlust()
            ? std::snprintf(puffer, n, "mess_arena: kein Verlust (belegt %llu von %llu Zeilen)",
                            static_cast<unsigned long long>(b.belegt), static_cast<unsigned long long>(b.kapazitaet))
            : std::snprintf(puffer, n,
                            "mess_arena: DATENVERLUST -- ueberlauf_verloren = %llu von %llu angebotenen Zeilen "
                            "(Kapazitaet %llu). Die Messung ist unvollstaendig.",
                            static_cast<unsigned long long>(b.verloren), static_cast<unsigned long long>(b.versuche),
                            static_cast<unsigned long long>(b.kapazitaet));
    return meldung_verbuchen(roh, n);
}

/// AusleseErgebnis -- Zeilen und Befund in EINEM Objekt.
///
/// Damit "melden" nicht zu "still" entartet: es gibt keinen Weg, an die Zeilen zu kommen, ohne den
/// Verlust in derselben Hand zu haben.
struct AusleseErgebnis {
    std::span<MessCheckpointZeile const> zeilen{};
    UeberlaufBefund                      befund{};
};

// == DIE MESS-ARENA ================================================================================
class alignas(kCacheLineBytes) MessArena {
public:
    /// Rueckgabe von anhaengen(), wenn die Zeile NICHT mehr untergebracht werden konnte.
    static constexpr std::uint64_t kUeberlaufSlot = ~static_cast<std::uint64_t>(0);

    MessArena() noexcept = default;

    MessArena(MessArena const&)            = delete;
    MessArena& operator=(MessArena const&) = delete;

    /// AUFBAU, vor dem Messfenster. Hier ist Belegung erlaubt -- und nur hier.
    ///
    /// Dimensionierung: kapazitaet_zeilen kommt aus der Planer-Formel
    ///   n_ops * zeilen_je_op(aktive Ebenen) * arena_gesamt_faktor
    /// (kapazitaet_zeilen_rechnen, checkpoint_speicher.hpp; der Gesamt-Faktor = drift * paar *
    /// t15b_retry steht als eigene Planer-Zeile in planner_mengen_types.hpp -- Vorgabewerte
    /// 12*2*5 = 120). KORRIGIERT 20.08.2026 (D.7-Entscheid, KON92 C7): hier stand "* 2
    /// (Sicherheitsfaktor 2)" -- eine STALE Fruehfassung, die dem verdrahteten Drift-Faktor um
    /// Faktor 9 widersprach; die Formel-Wahrheit liegt bei checkpoint_speicher/planner_mengen_types,
    /// nicht hier. Die Zahl gehoert NICHT hierher -- diese Klasse nimmt entgegen, was ihr
    /// gesagt wird, und meldet, wenn es zu wenig war. Wer die Formel hier einbaute, haette zwei
    /// Wahrheiten ueber die Kapazitaet.
    [[nodiscard]] bool reservieren(std::uint64_t kapazitaet_zeilen, VorabBeruehrung vorab) noexcept {
        // BEIDE Teile des Zustands gehen zuerst weg, und zwar BEDINGUNGSLOS. Vorher kehrte die
        // Funktion bei kapazitaet_zeilen == 0 um, BEVOR res_ angefasst wurde: der alte Block blieb
        // dann abgebildet, waehrend heiss_ (und damit jede Zugriffsfunktion) schon "leer" sagte --
        // ein Zustand, den das Objekt nach aussen falsch beschreibt. freigeben() ist idempotent.
        heiss_ = MessArenaHeiss{};
        res_.freigeben();
        if (kapazitaet_zeilen == 0u) return false;
        std::size_t const bytes = static_cast<std::size_t>(kapazitaet_zeilen) * sizeof(MessCheckpointZeile);
        // RUECKRECHNUNGS-WAECHTER. Die Multiplikation kann ueberlaufen, und ein Ueberlauf ist hier
        // still: gemessen lieferte reservieren(2^59 + 1) TRUE und kapazitaet() == 576460752303423489
        // auf einem 4096-Byte-Block -- die Arena haette 18 Trillionen Zeilen zugesagt und 128 Platz
        // gehabt. Der erste Checkpoint schriebe hinter die Abbildung.
        //
        // Die Rueckrechnung ist EXAKT, nicht heuristisch: `bytes` ist (kapazitaet * 32) mod 2^64,
        // also 32 * (kapazitaet mod 2^59); die Division durch 32 liefert kapazitaet mod 2^59. Die
        // Gleichheit unten gilt damit genau fuer kapazitaet < 2^59 -- also genau fuer die Werte, die
        // NICHT ueberlaufen. Sie weist keinen gueltigen Wert ab und laesst keinen ungueltigen durch.
        // Der Vergleich laeuft in uint64, damit er auch auf einer 32-Bit-Plattform traegt, wo schon
        // der static_cast auf size_t abschneidet.
        if (static_cast<std::uint64_t>(bytes / sizeof(MessCheckpointZeile)) != kapazitaet_zeilen) return false;
        if (!res_.reservieren(bytes, vorab)) return false;
        heiss_.basis      = static_cast<MessCheckpointZeile*>(res_.basis());
        heiss_.kapazitaet = kapazitaet_zeilen;
        heiss_.versuche   = 0;
        return true;
    }

    // [MS-HEISS]
    /// Haengt EINE Zeile an. Der einzige Aufruf im Messfenster.
    ///
    /// Was hier passiert: ein Hochzaehlen, ein Vergleich, ein Speichern. Keine Belegung, keine
    /// Umlagerung, keine Sperre, kein Aufruf in eine fremde Bibliothek.
    ///
    /// Der Zaehler steigt AUCH BEI UEBERLAUF weiter. Das ist die ganze Verlust-Buchhaltung: kein
    /// zweiter Zaehler, kein Zweig, der im Normalfall Arbeit kostet.
    [[nodiscard]] std::uint64_t anhaengen(MessCheckpointZeile const& zeile) noexcept {
        std::uint64_t const platz = heiss_.versuche;
        ++heiss_.versuche;
        if (platz >= heiss_.kapazitaet) return kUeberlaufSlot;
        heiss_.basis[platz] = zeile;
        return platz;
    }
    // [MS-HEISS-ENDE]

    [[nodiscard]] std::uint64_t kapazitaet() const noexcept { return heiss_.kapazitaet; }
    [[nodiscard]] std::uint64_t versuche() const noexcept { return heiss_.versuche; }
    [[nodiscard]] std::uint64_t belegt() const noexcept {
        return heiss_.versuche < heiss_.kapazitaet ? heiss_.versuche : heiss_.kapazitaet;
    }
    [[nodiscard]] std::uint64_t verloren() const noexcept {
        return heiss_.versuche > heiss_.kapazitaet ? heiss_.versuche - heiss_.kapazitaet : 0u;
    }

    [[nodiscard]] UeberlaufBefund befund() const noexcept {
        return UeberlaufBefund{heiss_.kapazitaet, heiss_.versuche, belegt(), verloren()};
    }

    /// Auswertung ZUM SCHLUSS. Zeilen und Verlust untrennbar in einem Objekt.
    [[nodiscard]] AusleseErgebnis auslesen() const noexcept {
        std::span<MessCheckpointZeile const> s{};
        if (heiss_.basis != nullptr) { s = std::span<MessCheckpointZeile const>{heiss_.basis, belegt()}; }
        return AusleseErgebnis{s, befund()};
    }

    /// Die Adresse der Nutzlast -- nur fuer die Beleg-Fuehrung (Seiten- und Cacheline-Trennung).
    [[nodiscard]] void const* nutzlast_basis() const noexcept { return heiss_.basis; }

    /// Die Adresse des HEISSEN Zustands -- der Teil, der bei jedem Checkpoint geschrieben wird.
    [[nodiscard]] void const* heisse_basis() const noexcept { return &heiss_; }

private:
    MessArenaHeiss     heiss_{}; // eigene Cacheline, Offset 0
    SeitenReservierung res_{};   // kalt: nur Aufbau und Abbau
};

static_assert(alignof(MessArena) == kCacheLineBytes, "Mess-Arena muss auf einer Cacheline beginnen");
static_assert(std::is_standard_layout_v<MessArena>, "Mess-Arena muss standard_layout sein (offsetof-Beleg)");
// ERSATZLOS GESTRICHEN (09.08.2026): `sizeof(MessArena) % kCacheLineBytes == 0`. Der Assert konnte
// nie feuern -- sizeof ist per Norm ein Vielfaches von alignof, und alignof ist eine Zeile darueber
// festgenagelt; er war von seinem eigenen Nachbarn impliziert. Was er zusichern SOLLTE -- dass der
// kalte Teil (res_) die heisse Cacheline nicht mitbenutzt -- steht seit heute anderswo und traegt
// dort wirklich: sizeof(MessArenaHeiss) == kCacheLineBytes (oben) legt die heisse Linie fest,
// standard_layout plus "heiss_ ist erstes Mitglied" setzt sie auf Offset 0 (zur Laufzeit gemessen
// in test_ms1, Abschnitt (8)), und der Abstand der beiden Arenen sichert checkpoint_speicher.hpp.

} // namespace comdare::cache_engine::builder::measure_storage
