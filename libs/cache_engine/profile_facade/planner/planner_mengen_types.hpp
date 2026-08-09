#pragma once
// planner_mengen_types.hpp -- check-size (2026-08-09): die MENGEN-RECHNUNG DES PLANERS, VOR DEM LAUF.
//
// SELBSTCHECK: diese Datei RECHNET und BERICHTET -- mehr nicht. Sie liest kein XML, faehrt keinen
// Director-Walk, kennt weder Registry noch Katalog. Jede Zahl, die sie multipliziert, kommt als Eingang
// herein. Das ist kein Stilentscheid, sondern die Bedingung dafuer, dass der Soll-Wert eines Tests aus
// einer ANDEREN Quelle stammen kann als die Rechnung selbst (T-3): ein Test darf hier von Hand
// gerechnete Zahlen einsetzen, ohne ein Profil, eine DLL oder einen Katalog zu brauchen.
//
// == WARUM ES DIESES KOMMANDO GIBT =================================================================
// Die Mess-Menge entscheidet drei Dinge VOR dem Lauf: ob die Kampagne in ihr Zeitfenster passt, ob die
// Mess-Arena den Lauf ueberhaupt fassen kann, und wie gross der Rueckschrieb wird. Wer sie erst am
// vollen Lauf erfaehrt, erfaehrt sie zu spaet -- der volle Lauf ist mehrtaegig.
//
// == DIE FALLE, GEGEN DIE HIER GEBAUT IST: DER FAKTOR 18 ===========================================
// Das T-15-Drift-Gate (harness/drift_gated_cell.hpp) misst DIESELBE Zelle bis zu reps*(max_reruns+1)
// mal -- mit den produktiven Vorgabewerten 3 und 5 sind das 18 Durchlaeufe, und JEDER schreibt in
// dieselbe monoton wachsende Arena. Eine Mengenrechnung, die EINEN Durchlauf annimmt, rechnet um bis
// zu Faktor 18 zu klein. Genau deshalb druckt dieser Bericht die Faktoren EINZELN: wer nur das Produkt
// sieht, kann einen fehlenden Faktor 18 nicht bemerken -- ein falsches Produkt sieht aus wie ein
// richtiges. Die Faktor-Zeilen sind das Produkt dieses Kommandos, nicht die Endzahl.
//
// == EIN BESTANDS-WIDERSPRUCH, DER HIER NICHT ENTSCHIEDEN WIRD =====================================
// mess_arena.hpp:174-176 nennt als Planer-Formel "n_ops * zeilen_je_op(aktive Ebenen) * 2
// (Sicherheitsfaktor 2)". checkpoint_speicher.hpp:77-80 nennt fuer denselben Faktor "bis zu 18".
// Beide Stellen stammen vom selben Tag (09.08.2026) und widersprechen sich um Faktor 9. DIESE Datei
// waehlt keine Seite: sie rechnet mit dem Drift-Gate-Faktor, den ihr der Aufrufer aus dem Profil-XML
// reicht, und druckt ihn samt Nenner -- damit ist im Bericht sichtbar, welche Lesart gerade gilt.
// Die Aufloesung des Widerspruchs gehoert dem Eigentuemer von measure_storage/, nicht diesem Kommando.
//
// == WAS HIER NICHT ENTSCHIEDEN WIRD: DAS ARENA-FENSTER ============================================
// checkpoint_measure ist heute in KEINE Mess-Schleife verdrahtet. Damit steht nicht fest, ob eine
// Arena einen Op-Batch, eine 4096er-Scheibe oder eine ganze Zelle ueberdauert. Dieses Kommando rechnet
// die Arena darum auf die KLEINSTE Einheit, die der Bestand definiert -- einen Op-Batch aus n_ops Ops
// einer (Binary x Einstellung) -- und gibt die Multiplikatoren darueber (Scheiben, Binaries je Perm,
// Zellen) als EIGENE Faktor-Zeilen aus, statt sie stillschweigend hineinzumultiplizieren. Eine
// Endzahl "Arena-Bytes der Kampagne" waere eine Erfindung ueber eine ungeklaerte Lebensdauer.

#include <type_traits>                                     // is_same_v -- Wache an der KapazitaetRechnung-Zuweisung
#include <builder/measure_storage/checkpoint_speicher.hpp> // kapazitaet_zeilen_rechnen + MessCheckpointZeile

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::planner {

namespace ms_ = ::comdare::cache_engine::builder::measure_storage;

// ---------------------------------------------------------------------------------------------------------------
// Die HERKUNFT einer Zahl -- neben JEDER Menge, weil eine Menge ohne Herkunft nicht pruefbar ist.
// ---------------------------------------------------------------------------------------------------------------
enum class MengenArt : std::uint8_t {
    Exakt = 0,   ///< aus dem deterministischen Director-Walk gezaehlt
    Xml,         ///< im Profil-XML ausdruecklich deklariert
    Default,     ///< NICHT deklariert -- der eingebaute Rueckfall traegt die Zahl
    Konstante,   ///< compile-hart im Bestand (static_assert-gewacht)
    Umgebung,    ///< aus einer Umgebungsvariablen
    Gerechnet,   ///< Produkt/Summe der Faktoren darueber
    Geschaetzt,  ///< der Aufrufer hat sie geschaetzt/kalibriert eingereicht
    Unbestimmbar ///< es gibt sie heute nicht -- und sie wird auch nicht erfunden
};

[[nodiscard]] constexpr char const* art_wort(MengenArt a) noexcept {
    switch (a) {
        case MengenArt::Exakt: return "exakt";
        case MengenArt::Xml: return "xml";
        case MengenArt::Default: return "default";
        case MengenArt::Konstante: return "konstante";
        case MengenArt::Umgebung: return "env";
        case MengenArt::Gerechnet: return "gerechnet";
        case MengenArt::Geschaetzt: return "GESCHAETZT";
        case MengenArt::Unbestimmbar: return "unbestimmbar";
    }
    return "?";
}

/// Eine Zeile des Berichts: WAS, WIEVIEL, WOHER -- nie eine nackte Zahl.
struct MengenFaktor {
    std::string name;
    std::string wert; ///< als Text, damit "n/a" und Zahlen dieselbe Spalte teilen
    MengenArt   art = MengenArt::Unbestimmbar;
    std::string nenner; ///< die Grundgesamtheit/Fundstelle -- gehoert in die AUSGABE, nicht in den Kopf
};

// ---------------------------------------------------------------------------------------------------------------
// Das URTEIL eines Deckels. "Unbestimmbar" ist ausdruecklich NICHT "haelt".
// ---------------------------------------------------------------------------------------------------------------
enum class Deckelurteil : std::uint8_t {
    KeinDeckel = 0, ///< der Aufrufer hat keinen gesetzt -- es wird nur berichtet
    Haelt,          ///< Menge <= Deckel, gerechnet
    Gerissen,       ///< Menge > Deckel
    Unbestimmbar    ///< Deckel verlangt, aber die Menge ist nicht berechenbar -> gilt als NICHT bestanden
};

[[nodiscard]] constexpr char const* urteil_wort(Deckelurteil u) noexcept {
    switch (u) {
        case Deckelurteil::KeinDeckel: return "kein_deckel";
        case Deckelurteil::Haelt: return "haelt";
        case Deckelurteil::Gerissen: return "GERISSEN";
        case Deckelurteil::Unbestimmbar: return "unbestimmbar";
    }
    return "?";
}

// ---------------------------------------------------------------------------------------------------------------
// Der EINGANG -- alles, was von aussen kommt. Diese Datei holt nichts selbst.
// ---------------------------------------------------------------------------------------------------------------
struct MengenEingang {
    // -- aus dem deterministischen Director-Walk (exakt) --------------------------------------------------------
    std::uint64_t perm_count         = 0;     ///< PlanHeader.perm_count -- |opt x simd| JE Mess-Kombination
    std::uint64_t combo_count        = 0;     ///< PlanHeader.measurement_combo_count
    std::uint64_t zellen_gezaehlt    = 0;     ///< begin_perm-Aufrufe des Walks -- die GEGENPROBE zum Produkt
    std::uint64_t profile_axis_count = 0;     ///< PlanHeader.profile_axis_count (s. Warnung in mengen_rechnen)
    bool          ebene_macro        = false; ///< PlanMeasurementCombo.tooling enthaelt "macro"
    bool          ebene_micro        = false; ///< PlanMeasurementCombo.tooling enthaelt "micro"
    bool          tooling_leer       = true;  ///< tooling leer == volles Angebot (dann gelten beide Ebenen)

    // -- aus dem Profil-XML bzw. dem eingebauten Rueckfall ------------------------------------------------------
    std::uint64_t n_ops            = 0;     ///< Ops je (Binary x Einstellung)
    bool          n_ops_aus_xml    = false; ///< false => der Wert ist ein Default, kein Profil-Entscheid
    std::uint64_t drift_reps       = 3;     ///< <drift_gate reps>
    std::uint64_t drift_max_reruns = 5;     ///< <drift_gate max_reruns>
    bool          drift_aus_xml    = false;

    // -- aus der Umgebung / dem compile-harten Bestand ----------------------------------------------------------
    std::uint64_t binaries_je_perm = 0; ///< COMDARE_GN_TOTAL (golden: 131072)
    bool          binaries_aus_env = false;
    std::uint64_t batch_korn       = 0; ///< planner::kGnBatchSlice -- der Aufrufer reicht sie, damit hier
                                        ///< KEIN viertes 4096-Literal entsteht (Korn-Wache!)

    // -- Aufrufer-Kalibrierung und Deckel ------------------------------------------------------------------------
    double        sekunden_je_op = 0.0; ///< 0 == keine Kalibrierung => die Dauer bleibt n/a
    std::uint64_t deckel_bytes   = 0;   ///< 0 == kein Speicher-Deckel
    double        deckel_tage    = 0.0; ///< 0 == kein Zeit-Deckel

    // -- Herkunfts-Angaben fuer den Kopf --------------------------------------------------------------------------
    std::string profile_id;
    std::string source_kind; ///< "thesis" | "experiment"
};

// ---------------------------------------------------------------------------------------------------------------
// Das ERGEBNIS.
// ---------------------------------------------------------------------------------------------------------------
struct MessMengenSicht {
    bool                      erhoben = false;
    std::string               grund; ///< wenn !erhoben: warum -- nie leer bei !erhoben
    std::vector<MengenFaktor> faktoren;

    std::uint64_t zellen                  = 0;
    std::uint64_t zeilen_je_op            = 0;
    std::uint64_t drift_faktor            = 0;
    std::uint64_t zeilen_je_op_batch      = 0; ///< == kapazitaet_zeilen_rechnen(n_ops, zeilen_je_op, drift_faktor)
    std::uint64_t arena_bytes_je_op_batch = 0;
    std::uint64_t ops_gesamt              = 0; ///< Kampagnen-Ops (nur wenn alle Faktoren bekannt)

    bool   dauer_bekannt = false;
    double dauer_s       = 0.0;
    double dauer_tage    = 0.0;

    bool ueberlauf = false; ///< eine Multiplikation waere ueber uint64 gelaufen -> KEINE Zahl gedruckt

    Deckelurteil speicher_urteil = Deckelurteil::KeinDeckel;
    Deckelurteil zeit_urteil     = Deckelurteil::KeinDeckel;

    /// true == der Lauf darf so nicht starten (ein verlangter Deckel haelt nicht oder ist unbestimmbar).
    [[nodiscard]] bool abgelehnt() const noexcept {
        return ueberlauf || speicher_urteil == Deckelurteil::Gerissen ||
               speicher_urteil == Deckelurteil::Unbestimmbar || zeit_urteil == Deckelurteil::Gerissen ||
               zeit_urteil == Deckelurteil::Unbestimmbar;
    }
};

// ---------------------------------------------------------------------------------------------------------------
// Ueberlauf-sichere Multiplikation.
//
// WARUM HIER UND NICHT DORT: kapazitaet_zeilen_rechnen (checkpoint_speicher.hpp:84) multipliziert drei
// uint64 OHNE Wache -- bei genuegend grossen Faktoren laeuft sie stillschweigend um und liefert eine
// KLEINE Zahl. Eine zu kleine Kapazitaet ist genau der Fehler, den dieses Kommando verhindern soll.
// Die Funktion wird NICHT geaendert (fremdes Modul); stattdessen prueft check-size die Faktoren VOR dem
// Aufruf und meldet "ueberlauf", statt eine umgelaufene Zahl zu drucken.
[[nodiscard]] constexpr bool mul_sicher(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0u && b > (std::numeric_limits<std::uint64_t>::max() / a)) return false;
    out = a * b;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Der DRIFT-FAKTOR -- die Zahl, die eine naive Rechnung um bis zu 18 danebenlegt.
//
// GATE AUS ist NICHT Faktor 0 und nicht Faktor reps: drift_gated_cell.hpp:146 sagt woertlich
// "GATE AUS (cfg.reps < 2): GENAU EIN Aufruf von measure_cell()". Also 1.
// GATE AN: reps Proben je Versuch, bis zu max_reruns+1 Versuche => reps * (max_reruns + 1).
[[nodiscard]] constexpr std::uint64_t drift_faktor_rechnen(std::uint64_t reps, std::uint64_t max_reruns) noexcept {
    if (reps < 2u) return 1u;
    return reps * (max_reruns + 1u);
}

// ---------------------------------------------------------------------------------------------------------------
// Die ZEILEN JE OP -- 2 je aktiver Mess-Ebene, die Mikro-Ebene einmal JE ACHSE.
//
// Quelle der Form: checkpoint_speicher.hpp:75-76 ("2 (das Makro-Paar) plus 2 je aktiver Achse").
// Quelle der aktiven Ebenen: PlanMeasurementCombo.tooling {wallclock/macro/micro} aus dem Walk --
// NICHT geraten. Leeres tooling == volles Angebot (experiment_plan_director.hpp:109), dann zaehlen beide.
[[nodiscard]] constexpr std::uint64_t zeilen_je_op_rechnen(bool macro_aktiv, bool micro_aktiv,
                                                           std::uint64_t achsen) noexcept {
    std::uint64_t z = 0u;
    if (macro_aktiv) z += 2u;
    if (micro_aktiv) z += 2u * achsen;
    return z;
}

// ---------------------------------------------------------------------------------------------------------------
// Kleine Text-Helfer. Bewusst ohne <format>: die Bau-Matrix faehrt acht Distributionen, und dieser
// Header wird von der Planer-App wie vom Standalone-Test eingebunden.
// ---------------------------------------------------------------------------------------------------------------
namespace mengen_detail {

[[nodiscard]] inline std::string sekunden_text(double s) {
    if (!(s >= 0.0) || s != s) return "n/a"; // NaN faellt durch beide Vergleiche
    char      puffer[64];
    int const n = std::snprintf(puffer, sizeof(puffer), "%.3f", s);
    return (n > 0) ? std::string(puffer, static_cast<std::size_t>(n)) : std::string{"n/a"};
}

[[nodiscard]] inline std::string breit(std::string const& s, std::size_t n) {
    std::string out = s;
    while (out.size() < n) out.push_back(' ');
    return out;
}

} // namespace mengen_detail

// ---------------------------------------------------------------------------------------------------------------
// DIE RECHNUNG.
//
// Reihenfolge ist Absicht: erst die Faktoren EINZELN samt Nenner, dann erst das Produkt. Jede
// Multiplikation ist ueberlauf-gewacht; bricht eine, wird KEINE Endzahl gedruckt, sondern "ueberlauf".
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline MessMengenSicht mengen_rechnen(MengenEingang const& e) {
    MessMengenSicht s{};
    auto            fak = [&s](char const* name, std::string wert, MengenArt art, char const* nenner) {
        s.faktoren.push_back(MengenFaktor{name, std::move(wert), art, nenner});
    };

    // -- Vorbedingungen: lieber ehrlich nichts sagen als eine Zahl aus Luecken bauen -----------------------------
    if (e.perm_count == 0u || e.combo_count == 0u) {
        s.grund = "Der Director-Walk lieferte keine Zellen (perm_count=" + std::to_string(e.perm_count) +
                  ", combo_count=" + std::to_string(e.combo_count) + ") -- ohne Zellen gibt es keine Mess-Menge.";
        return s;
    }
    if (e.n_ops == 0u) {
        s.grund = "n_ops ist 0 -- weder im Profil deklariert noch vom Aufrufer aufgeloest. Ohne Ops je "
                  "(Binary x Einstellung) ist die Arena nicht bemessbar.";
        return s;
    }
    if (e.batch_korn == 0u) {
        s.grund = "batch_korn ist 0 -- der Aufrufer hat planner::kGnBatchSlice nicht gereicht.";
        return s;
    }

    // == 1. DIE ZELLEN ===========================================================================================
    fak("perm_je_combo", std::to_string(e.perm_count), MengenArt::Exakt,
        "PlanHeader.perm_count -- |opt x simd| aus dem Director-Walk");
    fak("mess_kombinationen", std::to_string(e.combo_count), MengenArt::Exakt,
        "PlanHeader.measurement_combo_count -- <measurement_tooling><combo>");
    if (!mul_sicher(e.perm_count, e.combo_count, s.zellen)) {
        s.ueberlauf = true;
        s.grund     = "perm_count * combo_count laeuft ueber uint64";
        return s;
    }
    fak("zellen", std::to_string(s.zellen), MengenArt::Gerechnet, "perm_je_combo * mess_kombinationen");
    // GEGENPROBE aus einer zweiten Zaehlung desselben Walks: die begin_perm-Aufrufe. Stimmen Produkt und
    // Zaehlung nicht ueberein, ist eine der beiden Zahlen falsch -- und der Bericht sagt es, statt zu waehlen.
    fak("zellen_gezaehlt", std::to_string(e.zellen_gezaehlt), MengenArt::Exakt,
        (e.zellen_gezaehlt == s.zellen) ? "begin_perm-Aufrufe des Walks -- deckt sich mit dem Produkt"
                                        : "begin_perm-Aufrufe des Walks -- WEICHT VOM PRODUKT AB (s. HINWEISE)");

    // == 2. DIE BINARIES UNTER EINER ZELLE =======================================================================
    fak("binaries_je_perm", e.binaries_je_perm != 0u ? std::to_string(e.binaries_je_perm) : std::string{"n/a"},
        e.binaries_aus_env ? MengenArt::Umgebung : MengenArt::Default,
        "COMDARE_GN_TOTAL -- Voll-Bau 131072 (experiment_plan_director.hpp:1316)");
    fak("batch_korn", std::to_string(e.batch_korn), MengenArt::Konstante,
        "planner::kGnBatchSlice (experiment_plan_director.hpp:667, dreifach static_assert-gewacht)");
    if (e.binaries_je_perm != 0u) {
        fak("scheiben_je_perm", std::to_string(e.binaries_je_perm / e.batch_korn), MengenArt::Gerechnet,
            "binaries_je_perm / batch_korn");
    }

    // == 3. DIE ZEILEN JE OP =====================================================================================
    bool const macro_aktiv = e.tooling_leer || e.ebene_macro;
    bool const micro_aktiv = e.tooling_leer || e.ebene_micro;
    fak("ebene_macro", macro_aktiv ? "an" : "aus", MengenArt::Exakt,
        e.tooling_leer ? "PlanMeasurementCombo.tooling LEER == volles Angebot" : "PlanMeasurementCombo.tooling");
    fak("ebene_micro", micro_aktiv ? "an" : "aus", MengenArt::Exakt,
        e.tooling_leer ? "PlanMeasurementCombo.tooling LEER == volles Angebot" : "PlanMeasurementCombo.tooling");
    fak("achsen", std::to_string(e.profile_axis_count), MengenArt::Xml,
        "PlanHeader.profile_axis_count -- DEKLARIERTE Profil-Achsen (Gleichsetzung mit den instrumentierten "
        "Mess-Achsen ist eine ANNAHME, s. HINWEISE)");
    // Auch das Mikro-Paar wird gewacht: ein umgelaufenes 2*achsen ergaebe ein ZU KLEINES zeilen_je_op,
    // und ein zu kleines zeilen_je_op ist eine zu kleine Arena -- derselbe Fehler wie ein zu kleiner
    // Sicherheitsfaktor, nur eine Ebene tiefer.
    if (std::uint64_t verworfen = 0u; micro_aktiv && !mul_sicher(2u, e.profile_axis_count, verworfen)) {
        s.ueberlauf = true;
        s.grund     = "2 * achsen laeuft ueber uint64 -- zeilen_je_op waere zu klein";
        return s;
    }
    s.zeilen_je_op = zeilen_je_op_rechnen(macro_aktiv, micro_aktiv, e.profile_axis_count);
    fak("zeilen_je_op", std::to_string(s.zeilen_je_op), MengenArt::Gerechnet,
        "2 je aktiver Makro-Ebene + 2 je Achse bei aktiver Mikro-Ebene (checkpoint_speicher.hpp:75-76)");

    // == 4. DER DRIFT-FAKTOR -- die Zahl, um die eine naive Rechnung danebenliegt =================================
    fak("drift_reps", std::to_string(e.drift_reps), e.drift_aus_xml ? MengenArt::Xml : MengenArt::Default,
        "<drift_gate reps> bzw. DriftGateConfig::reps=3 (drift_gated_cell.hpp:100)");
    fak("drift_max_reruns", std::to_string(e.drift_max_reruns), e.drift_aus_xml ? MengenArt::Xml : MengenArt::Default,
        "<drift_gate max_reruns> bzw. DriftGateConfig::max_reruns=5 (drift_gated_cell.hpp:120)");
    s.drift_faktor = drift_faktor_rechnen(e.drift_reps, e.drift_max_reruns);
    fak("drift_faktor", std::to_string(s.drift_faktor), MengenArt::Gerechnet,
        (e.drift_reps < 2u) ? "GATE AUS (reps<2) => genau EIN Durchlauf (drift_gated_cell.hpp:146)"
                            : "reps * (max_reruns + 1) -- JEDER Durchlauf schreibt in DIESELBE Arena");

    // == 5. n_ops ================================================================================================
    fak("n_ops", std::to_string(e.n_ops), e.n_ops_aus_xml ? MengenArt::Xml : MengenArt::Default,
        e.n_ops_aus_xml ? "<comdare_thesis_profile> n_ops (xml_config_parser.hpp:194)"
                        : "ExperimentRunArgs::n_ops=10000 (profile_run_facade.hpp:195) -- NICHT im Profil erklaert");

    // == 6. DIE ARENA JE OP-BATCH ================================================================================
    // Ueberlauf-Wache VOR dem Aufruf: kapazitaet_zeilen_rechnen multipliziert ungewacht.
    std::uint64_t zwischen = 0u;
    if (!mul_sicher(e.n_ops, s.zeilen_je_op, zwischen) || !mul_sicher(zwischen, s.drift_faktor, zwischen)) {
        s.ueberlauf = true;
        s.grund     = "n_ops * zeilen_je_op * drift_faktor laeuft ueber uint64 -- kapazitaet_zeilen_rechnen "
                      "wuerde stillschweigend eine ZU KLEINE Zahl liefern";
        return s;
    }
    // DIE VERDRAHTUNG: die vorhandene Bestands-Funktion wird GERUFEN, nicht nachgebaut.
    //
    // 09.08.2026 NACHGEZOGEN (clang-Runde 2, erster Fund): kapazitaet_zeilen_rechnen liefert seit MS-1
    // KEIN nacktes uint64 mehr, sondern KapazitaetRechnung{zeilen, darstellbar}. Der Grund steht an der
    // Funktion selbst -- in nacktem uint64 wickelte die Formel still, und `(2^63 + 1, 2, 1)` ergab den
    // Wert 2: eine vollkommen plausible Kapazitaet aus einer masslosen Anforderung.
    // Diese Zuweisungsstelle stammt aus einem ANDEREN Zweig und wurde beim Merge nicht mitgezogen.
    // GCC uebersetzte den Baum trotzdem, clang nicht -- genau die Klasse, fuer die Runde 2 existiert.
    //
    // `darstellbar` wird hier NICHT ein zweites Mal geprueft, und das ist Absicht: die mul_sicher-Wache
    // sechs Zeilen darueber hat den Ueberlauf bereits abgefangen und mit Grund gemeldet. Eine zweite
    // Pruefung waere ein zweites Orakel fuer dieselbe Frage -- der static_assert unten haelt die
    // Annahme fest, statt sie zu wiederholen.
    auto const kapazitaet = ms_::kapazitaet_zeilen_rechnen(e.n_ops, s.zeilen_je_op, s.drift_faktor);
    static_assert(std::is_same_v<decltype(kapazitaet.zeilen), std::uint64_t>,
                  "KapazitaetRechnung::zeilen muss uint64 bleiben -- sonst bricht diese Zuweisung still.");
    s.zeilen_je_op_batch = kapazitaet.zeilen;
    fak("zeilen_je_op_batch", std::to_string(s.zeilen_je_op_batch), MengenArt::Gerechnet,
        "measure_storage::kapazitaet_zeilen_rechnen(n_ops, zeilen_je_op, drift_faktor)");
    fak("byte_je_zeile", std::to_string(sizeof(ms_::MessCheckpointZeile)), MengenArt::Konstante,
        "sizeof(MessCheckpointZeile) -- static_assert-gewacht auf 32 (mess_arena.hpp:99)");
    if (!mul_sicher(s.zeilen_je_op_batch, sizeof(ms_::MessCheckpointZeile), s.arena_bytes_je_op_batch)) {
        s.ueberlauf = true;
        s.grund     = "zeilen_je_op_batch * byte_je_zeile laeuft ueber uint64";
        return s;
    }
    fak("arena_bytes_je_op_batch", std::to_string(s.arena_bytes_je_op_batch), MengenArt::Gerechnet,
        "zeilen_je_op_batch * byte_je_zeile -- der RAM EINES Mess-Prozesses");

    // == 7. DIE KAMPAGNEN-OPS ====================================================================================
    if (e.binaries_je_perm != 0u) {
        std::uint64_t ops = 0u;
        if (!mul_sicher(e.n_ops, e.binaries_je_perm, ops) || !mul_sicher(ops, s.zellen, ops) ||
            !mul_sicher(ops, s.drift_faktor, ops)) {
            s.ueberlauf = true;
            s.grund     = "n_ops * binaries_je_perm * zellen * drift_faktor laeuft ueber uint64";
            return s;
        }
        s.ops_gesamt = ops;
        fak("ops_gesamt", std::to_string(s.ops_gesamt), MengenArt::Gerechnet,
            "n_ops * binaries_je_perm * zellen * drift_faktor -- die GEMESSENEN Ops der Kampagne");
    } else {
        fak("ops_gesamt", "n/a", MengenArt::Unbestimmbar, "ohne COMDARE_GN_TOTAL gibt es keine Kampagnen-Groesse");
    }

    // == 8. DIE DAUER ============================================================================================
    // EHRLICH: vor dem Lauf gibt es keine gemessene Zeit. projiziere_kampagne (eta_kalibrierung.hpp:297)
    // braucht KampagnenPosten aus echten Laeufen; die existieren hier nicht. Eine Dauer aus dem Nichts waere
    // genau der Stellvertreter, gegen den GOAL v8 gebaut ist. Darum: nur mit AUSDRUECKLICHER Kalibrierung.
    if (e.sekunden_je_op > 0.0 && s.ops_gesamt != 0u) {
        s.dauer_bekannt = true;
        s.dauer_s       = static_cast<double>(s.ops_gesamt) * e.sekunden_je_op;
        s.dauer_tage    = s.dauer_s / 86400.0;
        fak("sekunden_je_op", mengen_detail::sekunden_text(e.sekunden_je_op), MengenArt::Geschaetzt,
            "--sekunden-je-op=<f> vom Aufrufer -- NICHT gemessen, NICHT aus dem Bestand");
        fak("dauer_s", mengen_detail::sekunden_text(s.dauer_s), MengenArt::Geschaetzt,
            "ops_gesamt * sekunden_je_op -- so belastbar wie die Kalibrierung darueber");
        fak("dauer_maschinentage", mengen_detail::sekunden_text(s.dauer_tage), MengenArt::Geschaetzt,
            "dauer_s / 86400 -- EINE Maschine; eine Flotte teilt sie, dieser Bericht modelliert das nicht");
    } else {
        fak("sekunden_je_op", "n/a", MengenArt::Unbestimmbar,
            "keine --sekunden-je-op-Kalibrierung gereicht; projiziere_kampagne braucht gemessene "
            "KampagnenPosten, die es VOR dem Lauf nicht gibt");
        fak("dauer_maschinentage", "n/a", MengenArt::Unbestimmbar, "ohne sekunden_je_op nicht berechenbar");
    }

    // == 9. DIE DECKEL ===========================================================================================
    // VORLAEUFIGER ENTSCHEID (fail-closed, 2026-08-09, Owner-Entscheid OFFEN): zeilen_je_op == 0 (Tooling
    // ohne Makro- und ohne Mikro-Ebene, z.B. wallclock-only) rechnet die Arena zu NULL Bytes. Ein Byte-
    // Deckel, der dann "haelt", haelt nicht WEIL die Kampagne passt, sondern WEIL nichts gerechnet wurde --
    // ununterscheidbar von einem Tooling-Fehlparse. Der Bestand sichert "wallclock-only == null
    // Checkpoint-Zeilen" nirgends zu (checkpoint_measure ist unverdrahtet, s. Kopf). Bis der Eigentuemer
    // diese Zusicherung trifft, faellt ein VERLANGTER Byte-Deckel hier UNBESTIMMBAR -- dieselbe
    // fail-closed-Richtung wie beim Zeit-Deckel darunter. Ohne verlangten Deckel bleibt der Bericht frei.
    if (e.deckel_bytes != 0u) {
        s.speicher_urteil = (s.zeilen_je_op == 0u)                          ? Deckelurteil::Unbestimmbar
                            : (s.arena_bytes_je_op_batch <= e.deckel_bytes) ? Deckelurteil::Haelt
                                                                            : Deckelurteil::Gerissen;
    }
    if (e.deckel_tage > 0.0) {
        // Unbestimmbar ist NICHT "haelt": ein verlangter Deckel ohne Zahl faellt fail-closed durch.
        s.zeit_urteil = !s.dauer_bekannt                  ? Deckelurteil::Unbestimmbar
                        : (s.dauer_tage <= e.deckel_tage) ? Deckelurteil::Haelt
                                                          : Deckelurteil::Gerissen;
    }

    s.erhoben = true;
    return s;
}

// ---------------------------------------------------------------------------------------------------------------
// DER BERICHT -- das eigentliche Produkt dieses Kommandos.
//
// Die Faktor-Tabelle steht VOR jeder Endzahl, und jede Zeile traegt ihren Nenner. Wer nur ein Produkt
// druckt, macht einen fehlenden Faktor unsichtbar -- der Faktor-18-Fehler sieht als Endzahl voellig
// gesund aus.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline std::string mengen_bericht(MessMengenSicht const& s, MengenEingang const& e) {
    std::string o;
    o += "check-size -- MENGEN-VORSCHAU des Planers (rein rechnend: baut KEINE DLL, misst NICHT)\n";
    o += "profil=" + (e.profile_id.empty() ? std::string{"?"} : e.profile_id) +
         " wurzel=" + (e.source_kind.empty() ? std::string{"?"} : e.source_kind) + "\n\n";

    // ABGEBROCHEN heisst NICHT "nichts sagen": die Faktoren, die bis zum Abbruch erhoben wurden, sind
    // genau das, was den Abbruch erklaert. Sie werden darum AUCH hier gedruckt -- wer nur "Ueberlauf"
    // liest, weiss nicht, welcher Faktor zu gross war.
    if (!s.erhoben && s.faktoren.empty()) {
        o += "ERHEBUNG NICHT MOEGLICH\n  " + s.grund + "\n";
        return o;
    }
    if (!s.erhoben) {
        o += "ERHEBUNG ABGEBROCHEN\n  " + s.grund + "\n\n";
        o += "die bis zum Abbruch erhobenen Faktoren:\n";
    }

    o += mengen_detail::breit("FAKTOR", 26) + mengen_detail::breit("MENGE", 22) + mengen_detail::breit("ART", 14) +
         "NENNER (woher die Zahl kommt)\n";
    o += std::string(120, '-') + "\n";
    for (auto const& f : s.faktoren) {
        o += mengen_detail::breit(f.name, 26) + mengen_detail::breit(f.wert, 22) +
             mengen_detail::breit(art_wort(f.art), 14) + f.nenner + "\n";
    }

    // Bei Abbruch endet der Bericht hier: Deckel-Urteile und eine Schlusszeile ueber Zahlen, die es nicht
    // gibt, waeren eine Aussage ohne Gegenstand.
    if (!s.erhoben) {
        o += "\ncheck_size erhoben=nein grund_klasse=" + std::string{s.ueberlauf ? "ueberlauf" : "eingang"} +
             " urteil=ABGELEHNT\n";
        return o;
    }

    o += "\nDECKEL\n";
    o += "  speicher: " + std::string{urteil_wort(s.speicher_urteil)};
    if (e.deckel_bytes == 0u) {
        o += "  (kein --max-bytes gesetzt -- es wird nur berichtet)";
    } else if (s.speicher_urteil == Deckelurteil::Unbestimmbar) {
        o += "  (zeilen_je_op=0 -- die Arena rechnet zu NULL Bytes; ob das null Checkpoint-Zeilen BEDEUTET\n"
             "            oder ein Tooling-Fehlparse ist, sichert der Bestand nicht zu -> fail-closed)";
    } else {
        o += "  (arena_bytes_je_op_batch=" + std::to_string(s.arena_bytes_je_op_batch) +
             " gegen --max-bytes=" + std::to_string(e.deckel_bytes) + ")";
    }
    o += "\n  zeit:     " + std::string{urteil_wort(s.zeit_urteil)};
    if (e.deckel_tage > 0.0) {
        o += "  (dauer_maschinentage=" +
             (s.dauer_bekannt ? mengen_detail::sekunden_text(s.dauer_tage) : std::string{"n/a"}) +
             " gegen --max-tage=" + mengen_detail::sekunden_text(e.deckel_tage) + ")";
    } else {
        o += "  (kein --max-tage gesetzt -- es wird nur berichtet)";
    }
    o += "\n";

    // == HINWEISE -- was hier GESCHAETZT statt gemessen ist, ausdruecklich und vollstaendig ======================
    o += "\nHINWEISE -- was an dieser Rechnung NICHT gemessen ist\n";
    o += "  1. achsen: PlanHeader.profile_axis_count zaehlt die im Profil DEKLARIERTEN Achsen. Ob das\n"
         "     dieselbe Menge ist wie die INSTRUMENTIERTEN Mess-Achsen, ist im Bestand nicht festgelegt --\n"
         "     observer_registry/ und measurement_matrix/ sind heute leere Stuempfe. Die Zahl ist gemessen,\n"
         "     die GLEICHSETZUNG ist eine Annahme.\n";
    o += "  2. arena-fenster: checkpoint_measure ist in KEINE Mess-Schleife verdrahtet. arena_bytes_je_op_batch\n"
         "     bezieht sich darum auf einen Op-Batch (n_ops Ops einer Binary x Einstellung), nicht auf eine\n"
         "     Scheibe und nicht auf eine ganze Zelle. Die Multiplikatoren darueber stehen als eigene Zeilen.\n";
    o += "  3. dauer: ";
    o += s.dauer_bekannt ? "beruht ausschliesslich auf der vom Aufrufer gereichten --sekunden-je-op. Der\n"
                           "     Bestand liefert VOR dem Lauf keine Kalibrierung (projiziere_kampagne braucht\n"
                           "     gemessene KampagnenPosten).\n"
                         : "nicht berechnet -- es wurde keine --sekunden-je-op gereicht.\n";
    o += "  4. widerspruch im bestand: mess_arena.hpp:174-176 nennt Sicherheitsfaktor 2,\n"
         "     checkpoint_speicher.hpp:77-80 nennt bis zu 18. Dieser Bericht rechnet mit dem Drift-Gate-Faktor\n"
         "     oben (Zeile drift_faktor) und entscheidet den Widerspruch NICHT.\n";
    if (e.zellen_gezaehlt != s.zellen) {
        o += "  5. ZELL-DIVERGENZ: das Produkt (" + std::to_string(s.zellen) + ") und die Walk-Zaehlung (" +
             std::to_string(e.zellen_gezaehlt) +
             ") stimmen NICHT ueberein. Eine der beiden Zahlen ist falsch;\n"
             "     dieser Bericht waehlt keine Seite.\n";
    }

    // Maschinen-lesbare Schlusszeile, Muster kampagnen_zeile (eta_kalibrierung.hpp:350): Nenner IMMER dabei.
    o += "\ncheck_size zellen=" + std::to_string(s.zellen) + "/" + std::to_string(e.zellen_gezaehlt) +
         " zeilen_je_op=" + std::to_string(s.zeilen_je_op) + " drift_faktor=" + std::to_string(s.drift_faktor) +
         " zeilen_je_op_batch=" + std::to_string(s.zeilen_je_op_batch) +
         " arena_bytes=" + std::to_string(s.arena_bytes_je_op_batch) +
         " ops_gesamt=" + (s.ops_gesamt != 0u ? std::to_string(s.ops_gesamt) : std::string{"n/a"}) +
         " tage=" + (s.dauer_bekannt ? mengen_detail::sekunden_text(s.dauer_tage) : std::string{"n/a"}) +
         " speicher=" + urteil_wort(s.speicher_urteil) + " zeit=" + urteil_wort(s.zeit_urteil) +
         " urteil=" + (s.abgelehnt() ? "ABGELEHNT" : "ok") + "\n";
    return o;
}

} // namespace comdare::cache_engine::planner
