#pragma once
// checkpoint_measure.hpp -- DAS ZENTRALE MESSINSTRUMENT DES SYSTEMS (Owner-KERN 09.08.2026).
//
// == DER AUFTRAG, WOERTLICH ========================================================================
//     "checkpoint measure ist das ZENTRALE MESSINSTRUMENT DES SYSTEMS, es MUSS GEBAUT WERDEN.
//      Sonst gibt es keine Messungen."
//
// Der Wellenplan par.7 fuehrte diesen Posten unter "WAS FAELLT" mit der Notiz "OV-2b bestaetigt -> W7".
// DIESE BESTAETIGUNG HAT ES NIE GEGEBEN; der Owner hat den Streichgrund am 09.08.2026 zurueckgewiesen.
// Der Posten ist NICHT rutschfaehig. Damit faellt auch die Textzusage "vier gemessen, fuenfte
// spezifiziert" -- es sind FUENF.
//
// == WAS DIESE DATEI IST -- UND WAS SIE AUSDRUECKLICH NICHT IST ====================================
// Sie ist das INSTRUMENT ueber der gelandeten MS-1-Substanz. Die zwei Arenen (mess_arena.hpp,
// stapel_arena.hpp, zusammengefuehrt in checkpoint_speicher.hpp) sind fertig, gemessen und belegt --
// sie werden hier ANGESCHLOSSEN, nicht umgebaut. Bis heute hatten sie NULL Produktions-Aufrufer
// (nachgemessen am Objekt: nur ihr eigener Header und test_ms1 nennen sie). Eine Substanz ohne
// Instrument ist eine Datenstruktur, kein Messgeraet -- genau das war der Befund.
//
// Sie ist NICHT der Kanal. Der Kanal (wer das Instrument beauftragt und wohin die Werte gehen) steht
// in mess/mess_naht.hpp und am Genus-Interface. Diese Trennung ist die Lehre aus dem Sidecar-Befund:
// wer misst, und wer entscheidet WAS gemessen wird, duerfen nicht dieselbe Klasse sein.
//
// == WO ES LEBT ====================================================================================
// IM TIER-BINARY. Dort wird gemessen; der Owner verlangt ausdruecklich, dass das Signal BEIM
// Tier-Binary ankommt. Beauftragt wird es von der CEB ueber das Dock -- init() vor dem Fenster,
// flush() JE ARENA GETRENNT danach. Ein Instrument, das im Host laege und in das Tier hineingriffe,
// waere wieder ein Sidecar.
//
// == DIE DREI REGELN DES MESSFENSTERS (SOLL-Design R1..R3), hier gebaut ============================
//   R1  KEIN TEXT im heissen Pfad: der Checkpoint nimmt einen DESKRIPTOR-INDEX, keine Zeichenkette.
//       Welcher Ort das ist, loest die statische Tabelle beim Auswerten auf -- nach dem Fenster.
//   R2  KEINE BELEGUNG im heissen Pfad: beide Arenen stehen seit init(); anhaengen() und hinauf()
//       rechnen, vergleichen, speichern. Mehr nicht.
//   R3  KEIN I/O im heissen Pfad: geschrieben wird erst im flush, und der laeuft nach dem Fenster.
//
// == SELBSTCHECK ===================================================================================
//   ZUSICHERT: dass die zwei Arenen GETRENNT beauftragt werden (zwei flush-Namen, zwei Befund-Typen,
//              kein Sammelaufruf) und dass eine Ebene ohne einkompiliertes Instrument nicht
//              aufrufbar ist (requires EbeneEingebaut an der Steuerzeile).
//   ZUSICHERT NICHT: dass irgendjemand das Instrument auch BENUTZT -- Anwesenheit ist keine Aussage.
//              Das ist Sache der Abnahme (tests/unit/test_ck1_messkette_koeder.cpp), die einen frisch
//              gewuerfelten Wert durch die ganze Kette schickt und am fernen Ende nachrechnet.
//
// ASCII-only.

#include <builder/measure_storage/checkpoint_speicher.hpp>
#include <mess/konfiguration.hpp>

#include <chrono>
#include <cstdint>
#include <span>

#if defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

namespace comdare::cache_engine::builder::measure_storage {

namespace mess = ::comdare::cache_engine::mess;

// ==================================================================================================
// DIE ZEITQUELLE -- billig, monoton, EINMAL je Lauf verankert
// ==================================================================================================
//
// mess_arena.hpp beschreibt das Feld als "billiger monotoner Zaehler, einmal je Lauf gegen die
// Systemzeit verankert". Genau das ist hier gebaut, und die Begruendung gehoert dazu:
//
// Im Fenster wird der ROHE ZAEHLER genommen (auf x86-64: rdtsc, ~10 Zyklen, kein Aufruf in eine
// fremde Bibliothek). Die Umrechnung in Nanosekunden passiert NACH dem Fenster, beim Auswerten, ueber
// den einen Anker. Der Grund ist die Fehlerklasse, um die sich dieses ganze Paket dreht: eine
// Umrechnung im Fenster kostet Zeit, und diese Zeit faellt in genau die Groesse, die gemessen wird.
//
// EHRLICH BENANNT: rdtsc ist ohne Serialisierung nicht ordnungstreu gegen umliegende Befehle, und die
// Frequenz-Invarianz (constant_tsc) ist eine Eigenschaft der Maschine, keine der Norm. Das ist fuer
// den Zweck hier tragbar, weil die Zeit NICHT der gemessene Gegenstand dieser Abnahme ist -- der
// Messwert reist als eigenes Feld. Wo die ZEIT der Gegenstand ist, gehoert eine serialisierende
// Variante her; das ist ein benanntes Folgepaket und keine stille Annahme dieser Datei.
[[nodiscard]] inline std::uint64_t zeit_ticks_jetzt() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    return static_cast<std::uint64_t>(__rdtsc());
#else
    // Ersatzweg: ein monotoner Zaehler, der auf jeder Plattform existiert. Teurer, aber ehrlich --
    // und er wird auf der 8er-Docker-Matrix (x86-64) nicht gefahren.
    return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
#endif
}

/// ProzessAnker -- was EINMAL je Lauf feststeht und deshalb nicht in jede Zeile gehoert.
///
/// SOLL-Design par.3/par.6: der Prozessname wird einmal gesetzt, der Zeit-Anker einmal je Lauf
/// genommen. Beides in jede 32-Byte-Zeile zu kopieren waere Platzverschwendung in der Struktur, die
/// am haeufigsten geschrieben wird -- und es waere eine Kopie, die veralten kann.
struct ProzessAnker {
    std::uint64_t ticks_bei_start = 0; // der Zaehlerstand zum Ankerzeitpunkt
    std::uint64_t ns_bei_start    = 0; // dieselbe Zeit in Nanosekunden, aus der Systemuhr

    /// Nimmt BEIDE Zeitstaende im selben Atemzug. Die Reihenfolge ist bewusst: erst die teure
    /// Systemuhr, dann der billige Zaehler. So faellt die Dauer des Uhr-Aufrufs VOR den Ankerpunkt
    /// und nicht zwischen die beiden Staende -- der Versatz, den die Umrechnung spaeter erbt, ist
    /// damit so klein wie er ohne serialisierende Befehle werden kann.
    [[nodiscard]] static ProzessAnker setzen() noexcept {
        ProzessAnker a{};
        a.ns_bei_start = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        a.ticks_bei_start = zeit_ticks_jetzt();
        return a;
    }
};

// ==================================================================================================
// DIE MASSE -- was der Planer vorgibt, und was hier NICHT geraten wird
// ==================================================================================================

/// MessMasse -- die Dimensionierung BEIDER Arenen, vom Planer.
///
/// SELBSTCHECK: hier steht KEINE Formel. Die Kapazitaets-Formel lebt an genau einer Stelle
/// (kapazitaet_zeilen_rechnen in checkpoint_speicher.hpp) und wird vom Planer gerufen. Wer sie hier
/// noch einmal aufschriebe, haette zwei Wahrheiten ueber die Kapazitaet -- und die zweite veraltete.
struct MessMasse {
    std::uint64_t   kapazitaet_zeilen = 0;                   // Mess-Arena: erwartete EREIGNISZAHL
    std::uint32_t   tiefe_plaetze     = 0;                   // Stapel-Arena: VERSCHACHTELUNGSTIEFE
    VorabBeruehrung vorab             = VorabBeruehrung::Ja; // Seitenfehler in den Aufbau ziehen
};

// ==================================================================================================
// DIE FLUSH-BEFUNDE -- JE ARENA EINER, mit Absicht kein gemeinsamer Obertyp
// ==================================================================================================
//
// Owner 09.08.2026: der flush wird "JE ARENA GETRENNT" beauftragt. Die Trennung ist hier nicht bloss
// eine Namensfrage: die beiden Arenen tragen VERSCHIEDENE FEHLERKLASSEN (Datenverlust gegen
// Programmierfehler), und checkpoint_speicher.hpp haelt ausdruecklich fest, dass sie sich keine
// Meldung teilen duerfen. Ein gemeinsamer Obertyp waere genau die Zusammenfuehrung, die dort
// verboten ist -- deshalb gibt es ihn nicht, und deshalb gibt es auch keinen Sammel-flush().

/// Ergebnis von flush_mess -- die Datenverlust-Klasse.
struct FlushBefundMess {
    UeberlaufBefund befund{};          // Kapazitaet, Versuche, belegt, verloren -- mit Nenner
    std::uint64_t   uebergeben    = 0; // Zeilen, die die Senke bekommen hat
    bool            senke_bedient = false;

    /// Vollstaendig heisst: die Senke wurde bedient UND es ging nichts verloren.
    [[nodiscard]] constexpr bool vollstaendig() const noexcept { return senke_bedient && !befund.hat_verlust(); }
};

/// Ergebnis von flush_stapel -- die Programmierfehler-Klasse. KEIN Feld "verloren", mit Absicht.
///
/// SOLL-Design par.7: ein EIN ohne AUS ist eine REGRESSION und wird GEMELDET, nicht aufgeraeumt. Der
/// Befund traegt deshalb die offenen Eintraege SELBST heraus (Station, Ebene, Tiefe) und nicht nur
/// ihre Anzahl -- eine Meldung, die sagt DASS etwas offen blieb, aber nicht WELCHES, laesst genau die
/// Diagnose offen, fuer die der Stapel ueberhaupt gefuehrt wird.
struct FlushBefundStapel {
    StapelBefund befund{}; // offen, hoechststand, zu_tief, unpaarige_aus -- mit Nenner
    bool         senke_bedient = false;

    /// AUSGEGLICHEN heisst: alle drei Fehlerklassen leer. Delegiert an StapelBefund::sauber(), damit
    /// die Bedingung an EINER Stelle lebt -- eine zweite Abschrift veraltete lautlos.
    [[nodiscard]] constexpr bool ausgeglichen() const noexcept { return senke_bedient && befund.sauber(); }
};

// ==================================================================================================
// DIE SENKEN -- Concept-Guard, keine vtable, kein std::function
// ==================================================================================================
//
// Eine Senke ist ein GEWOEHNLICHER Typ mit einer noexcept-Methode. Sie erbt nichts und zahlt keine
// vtable; die Auswahl erledigt der Uebersetzer. Der flush laeuft ohnehin NACH dem Fenster -- die
// Zero-Cost-Frage stellt sich hier nicht, wohl aber die nach dem stillen Fehlgriff: mit Concept faellt
// eine verfehlte Flaeche an der Instanziierung auf, ohne sie waehlte der Uebersetzer stumm die
// falsche Ueberladung.

/// Die Mess-Senke bekommt Zeilen UND Verlust in EINEM Objekt (AusleseErgebnis) -- sie kann den
/// Verlust ignorieren, aber nicht, ohne es zu tun.
template <class S>
concept MessSenkeConcept = requires(S s, AusleseErgebnis const& e) {
    { s.mess_aufnehmen(e) } noexcept;
};

/// Die Stapel-Senke bekommt den Befund UND die offen gebliebenen Eintraege.
template <class S>
concept StapelSenkeConcept = requires(S s, StapelBefund const& b, std::span<StapelEintrag const> o) {
    { s.stapel_aufnehmen(b, o) } noexcept;
};

// ==================================================================================================
// DAS INSTRUMENT
// ==================================================================================================

/// CheckpointMeasure<MK> -- das zentrale Messinstrument, je Mess-Konfiguration monomorphisiert.
///
/// `requires aktiv<MK>` ist die erste der drei Riegel gegen den stillen Nulllauf: eine
/// CheckpointMeasure<Leer> ist NICHT BILDBAR. Ein Kompilat ohne Messung kann das Instrument nicht
/// einmal BENENNEN -- es gibt also nichts, was still nichts tun koennte. Das ist der Unterschied
/// zwischen "der Zweig wird nicht genommen" und "der Zweig existiert nicht".
template <mess::KonfigConcept MK>
    requires mess::aktiv<MK>
class CheckpointMeasure {
public:
    using konfiguration_t = MK;

    /// Der ABI-Tag dieser Monomorphisierung -- reist in den Modul-Deskriptor und wird vom Dock
    /// gegen den eigenen verglichen, BEVOR gemessen wird.
    static constexpr std::uint64_t tag = MK::tag();

    CheckpointMeasure() noexcept                           = default;
    CheckpointMeasure(CheckpointMeasure const&)            = delete;
    CheckpointMeasure& operator=(CheckpointMeasure const&) = delete;

    // ----------------------------------------------------------------------------------------------
    // init -- DIE GLOBALE INITIALISIERUNG BEIDER ARENEN, VOR dem Messfenster
    // ----------------------------------------------------------------------------------------------
    //
    // Owner 09.08.2026: "Eine globale Initialisierung der BEIDEN ARENEN von checkpoint_measure zum
    // init() geht daher nicht wie geplant" -- Folge 3 der Kausalkette. Sie geht jetzt, weil MK durch
    // die Kaskade kommt und das Instrument damit im Tier-Binary existiert.
    //
    // Hier ist Belegung ERLAUBT und erwuenscht: das ist der Aufbau, nicht das Fenster. Mit
    // VorabBeruehrung::Ja wandern auch die Seitenfehler hierher -- sonst laegen sie mitten in der
    // Messung.
    //
    // Der Befund kommt JE ARENA getrennt zurueck (AufbauBefund{mess, stapel}); ein nacktes `a && b`
    // liesse offen, WELCHE der beiden fehlte, und die beiden haben voellig verschiedene Ursachen.
    [[nodiscard]] AufbauBefund init(MessMasse const& masse) noexcept {
        anker_ = ProzessAnker::setzen();
        return speicher_aufbauen(speicher_, masse.kapazitaet_zeilen, masse.tiefe_plaetze, masse.vorab);
    }

    // ----------------------------------------------------------------------------------------------
    // DIE EINE UNIFORME STEUERZEILE (SOLL-Design par.2)
    // ----------------------------------------------------------------------------------------------
    //
    // EIN Aufruf fuer alle Ebenen und beide Richtungen. Ebene und Richtung sind TEMPLATE-Parameter,
    // also compile-time -- mess_arena.hpp haelt ausdruecklich fest: "hier steht KEIN switch. Ebene und
    // Richtung werden vom Aufrufer als constexpr-Wert uebergeben und in EIN Byte gefaltet".
    //
    // `requires EbeneEingebaut<MK, E>` ist der Riegel gegen die STILLE NULL: ein Mikro-Checkpoint in
    // einer CEB-Version ohne Mikro-Instrument ist ein UEBERSETZUNGSFEHLER, kein Aufruf ins Leere.
    // Ohne diese Klausel liefe genau der Fall, den das Haus als teuerste Fehlerklasse fuehrt -- eine
    // Reihe voller ehrlicher Nullen, die wie Messwerte aussehen.
    //
    // WAS HIER PASSIERT: ein Zaehlerstand lesen, eine 32-Byte-Zeile bauen, anhaengen, den Stapel um
    // eins bewegen. Keine Belegung (R2), kein Text (R1), kein I/O (R3), kein Zweig ueber die Ebene.
    template <MessEbene E, Richtung R>
        requires mess::EbeneEingebaut<MK, E>
    void checkpoint(std::uint32_t deskriptor_ix, std::uint64_t messwert, std::uint16_t thread_nr) noexcept {
        MessCheckpointZeile zeile{};
        zeile.zeit_ticks    = zeit_ticks_jetzt();
        zeile.messwert      = messwert;
        zeile.deskriptor_ix = deskriptor_ix;
        zeile.thread_nr     = thread_nr;
        zeile.tag           = tag_bauen(E, R);

        std::uint64_t const platz = speicher_.mess.anhaengen(zeile);

        // Die Stapel-Buchhaltung: "wo bin ich gerade?". Sie ist compile-time verzweigt, nicht zur
        // Laufzeit -- R ist ein Template-Parameter, also bleibt je Instanziierung genau ein Zweig
        // uebrig. Der Rueckgabewert wird bewusst verworfen: BEIDE Fehlerfaelle (zu tief, unpaariges
        // AUS) sind in den Zaehlern des Stapels bereits verbucht und erscheinen im Befund. Ihn hier
        // zu pruefen hiesse, im Messfenster zu verzweigen und zu melden -- also genau das zu tun,
        // was R3 verbietet. Gemeldet wird nach dem Fenster, im flush.
        if constexpr (R == Richtung::Ein) {
            StapelEintrag e{};
            e.log_index     = platz;
            e.deskriptor_ix = deskriptor_ix;
            e.thread_nr     = thread_nr;
            e.ebene         = static_cast<std::uint8_t>(E);
            (void)speicher_.stapel.hinauf(e);
        } else {
            (void)speicher_.stapel.herunter(nullptr);
        }
    }

    // ----------------------------------------------------------------------------------------------
    // flush -- JE ARENA GETRENNT. Zwei Namen, zwei Befunde, NIE ein Sammelaufruf.
    // ----------------------------------------------------------------------------------------------
    //
    // Owner 09.08.2026, Folge 4 der Kausalkette: "Die CEB kann nach einem Durchlauf ueber den Observer
    // KEINEN flush() der Messwerte (JE ARENA GETRENNT) beauftragen und uebermitteln."
    //
    // Warum es keinen flush_beides() gibt, obwohl er bequem waere: er waere die gemeinsame Meldung,
    // die checkpoint_speicher.hpp ausdruecklich ausschliesst. Sobald ein Aufrufer beide Befunde in
    // einem Ausdruck bekaeme, entstuende der Griff `if (!flush_beides()) melde("Messung kaputt")` --
    // und damit waere die Unterscheidung zwischen Datenverlust und Programmierfehler wieder weg.

    /// Uebergibt die MESS-Arena an die Senke. Laeuft NACH dem Fenster (R3: hier ist I/O erlaubt).
    /// Die Arena wird dabei NICHT zurueckgesetzt: sie waechst monoton ueber den ganzen Lauf, und ein
    /// Ruecksetzen vernichtete die Verlust-Buchhaltung, die `versuche - kapazitaet` exakt macht.
    template <MessSenkeConcept S>
    [[nodiscard]] FlushBefundMess flush_mess(S& senke) noexcept {
        AusleseErgebnis const erg = speicher_.mess.auslesen();
        senke.mess_aufnehmen(erg);
        return FlushBefundMess{erg.befund, static_cast<std::uint64_t>(erg.zeilen.size()), true};
    }

    /// Uebergibt die STAPEL-Arena an die Senke -- den Befund UND die offen gebliebenen Eintraege.
    ///
    /// Der Stapel wird hier ebenfalls NICHT geleert. Ein Stapel, der sich am Ende selbst aufraeumt,
    /// hat die Regression vernichtet, die er haette anzeigen sollen (stapel_arena.hpp, woertlich).
    template <StapelSenkeConcept S>
    [[nodiscard]] FlushBefundStapel flush_stapel(S& senke) noexcept {
        StapelBefund const                   b      = speicher_.stapel.befund();
        std::span<StapelEintrag const> const offene = speicher_.stapel.offene();
        senke.stapel_aufnehmen(b, offene);
        return FlushBefundStapel{b, true};
    }

    // ----------------------------------------------------------------------------------------------
    // Auskunft -- fuer die Abnahme und die Dock-Diagnose, NICHT fuer den heissen Pfad
    // ----------------------------------------------------------------------------------------------
    [[nodiscard]] ProzessAnker const&       anker() const noexcept { return anker_; }
    [[nodiscard]] CheckpointSpeicher const& speicher() const noexcept { return speicher_; }

private:
    CheckpointSpeicher speicher_{}; // MS-1: Mess-Arena (Append) + Stapel-Arena (LIFO), getrennte Seiten
    ProzessAnker       anker_{};
};

} // namespace comdare::cache_engine::builder::measure_storage
