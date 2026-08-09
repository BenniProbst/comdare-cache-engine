#pragma once
// lager_baum_writer.hpp -- LB-2/LB-3-Kern (F9-Paketschnitt): der BAUM-WRITER beider Realm-Wurzeln.
//
// Er komponiert genau zwei Quellen und erfindet keine dritte:
//   * lager_pfad_grammatik.hpp -- die kv-Grammatik, die Wachen, die Praefix-Semantik (LB-0, L5),
//   * die BINDENDEN Ordnungen des Hauses -- abi::kSystemAxisOrder (die eine System-Achsen-Ordnung,
//     L22) und experiment::kCompositionAxisNames (die 18 Organ-Achsen der Komposition).
// Genau HIER wird die Layer-Kante bewusst gezogen (Doktrin wie im Kopf von
// artifact_cache_transport.hpp): die Grammatik selbst bleibt experiment_tree-frei, der Writer sieht
// beide Seiten. Wer nur die Grammatik braucht, zieht diese Kante nicht mit.
//
// K1 (Owner-Entscheid 09.08.2026) -- DIE DREI GEMEINSAMEN WURZELEBENEN, VOR BEIDEN REALMS:
//
//   1  gattung=<token>    Ebene 1 der Anatomie (Map/Container/Graph -- anatomy::AnatomyGattung)
//   2  genus=<token>      Ebene 2 der Anatomie (SearchAlgorithm/Set/Sequence/Adapter/View)
//   3  realm=<binaries|messdaten>
//
// Owner verbatim: die Hybrid-Gattung ist "einfach eine weitere Gattung+Genus, die parallel zu allen
// anderen Gattung+Genus in den beiden wurzel-Ordner-Ebenen des Lagerbaumes mit einsortiert wird" --
// und praezisierend: "Binary-Ordner und Messung-Ordner branchen unter Gattung->Genus->Binary/Messung
// ->REST wie gehabt." Genau das ist der Bau: DREI Ebenen vorne, der REST beider Kaskaden unveraendert.
//
// WARUM DER REALM AUS DER WURZEL IN DIE KASKADE WANDERT: bis K1 war "binaries"/"messdaten" KEINE
// Ebene, sondern der wurzel_-Konstruktor-String des Writers -- ein Wert, der an kv_kette() VORBEI
// lief und deshalb weder Zeichenklasse noch Laenge noch irgendeine Wache sah. Die Owner-Ordnung
// verlangt Gattung VOR Binary; ein Praefix, das im Konstruktor-String steckt, kann aber nichts vor
// sich haben. Der Realm wird damit zwangslaeufig zur echten, geprueften Ebene, und wurzel_ ist ab
// jetzt genau das, was der Name sagt: die Wurzel des LAGERS, nicht die eines Realms.
//
// DIE ZWEI REALM-KASKADEN (Owner-KERN 26.07. Section 4-6, A9-Doc 6.1/6.2, Lage-Dossier Nr. 25) --
// jeweils UNTER den drei Wurzelebenen oben, in sich unveraendert ("REST wie gehabt"):
//
// Messdaten-Realm (MessdatenRealmPolicy):
//   1     mess=<tooling>+load_framework=on|off                (Mess-Kombinatorik BEIDER Mess-Haupt-Achsen)
//   2     EIN Ordner = GESAMTE System-Haupt-Rekombination, Meta-Metas HINTEN (D-09)
//   3-7   die 5 ENGLISCHEN Organ-Gruppen-Ordner GESCHACHTELT in Speicherhierarchie-Reihenfolge
//   8     Haupt-Blatt (vollstaendige Haupt-Achsen-Bindung)
//   9-11  mess_unter -> system_unter -> organ_unter (nur soweit noetig)
//   Blatt Datei nach blatt_dateiname() -- Datum+Uhrzeit+Unter-Achsen-Variablen IM NAMEN,
//         Haupt-Achsen als Ordner + Metadaten (A-15: Unter-Achsen nie im Stempel, wohl im Dateinamen)
//
// Binaries-Realm (BinariesRealmPolicy):
//   1     System-Achse DIREKT als Wurzel (+ Meta-Metas hinten)
//   2-6   die 5 Organ-Gruppen-Ordner
//   7     Mess-Typ als LETZTER/tiefster Haupt-Achsen-Typ (D-12)
//   ABNAHME-5: die CEB-Binaries liegen in den BLAETTERN der System-Achsen-Knoten, also am Uebergang
//   zu den Organ-Achsen -- ceb_blatt_ebenen() liefert genau diesen Praefix (Ebene 1).
//   LED-68b: das Test-Log liegt NEBEN der Binary (test_log_neben()), nicht in einem Log-Baum.
//
// BLATT-IDENTITAET = v6-FINGERPRINT (F7-Konvergenz): Skip-Marke, minio-Key, Bestandslog-key_sha512 und
// Baum-Blatt sind DASSELBE Preimage. bestandslog_index.hpp::derive_key_from_lines rechnet ueber
// abi::anatomy_fingerprint_preimage (EIN '\n'-Separator, feste Glied-Anzahl) -- identisch zu
// abi::anatomy_fingerprint_hex. Dieser Writer rechnet NICHTS nach: blatt_segment_aus_fingerprint()
// nimmt den fertigen 128-hex entgegen und prueft nur seine FORM. Ein zweiter Preimage waere genau die
// Drift, die A13-M3 geschlossen hat.
//
// L3 (Manager) ist mit K1 AUFGEHOBEN: bis dahin verbot die Grammatik jeden "hybrid"-Token auf jeder
// Ebene. K1 sortiert Hybrid stattdessen als regulaere Gattung+Genus in die Wurzelebenen ein, also ist
// das Verbot entfallen (Begruendung im Kopf von lager_pfad_grammatik.hpp). AN SEINE STELLE TRITT die
// Wurzel-Wache dieser Datei: eine Gattung ohne Lager-Token bricht den BAU (consteval, s.u.), und ein
// Genus, das nicht zu seiner Gattung gehoert, wird zur Laufzeit klassifiziert abgewiesen. Der
// Unterschied ist der Gegenstand: L3 verbot einen NAMEN, K1 prueft eine ZUGEHOERIGKEIT.
//
// CT/RT-GRENZE: die Realm-Wahl ist eine CT-POLICY (Template-Parameter + Concept-Guard, GoF
// Factory/Strategy im Muster der BestandKeyPolicy-Factory, bestandslog_factory.hpp:36-63) -- KEIN
// Runtime-Switch, KEIN std::variant, keine vtable. Die Gruppen-Tabelle und ihre Drift-Wachen sind CT.
// Die konkreten Pfade, das Verzeichnis-Anlegen und das Schreiben sind RT.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + die zwei genannten Quellen.

#include "../experiment_tree/axis_path_serialization.hpp" // experiment::kCompositionAxisNames (18 Organ-Achsen)
#include "lager_pfad_grammatik.hpp"

#include <anatomy/anatomy_base.hpp>               // K1: AnatomyGattung/AnatomyGenus + gattung_of (Ebene 2 -> 1)
#include <cache_engine/abi/system_axis_order.hpp> // abi::kSystemAxisOrder (die EINE System-Ordnung, L22)

#include <array>
#include <cstdint>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// ---------------------------------------------------------------------------
// Die zwei Realm-Wurzeln. Ein ENUM als NAME (Testat/Diagnose), NICHT als Schalter: gewaehlt wird
// ueber die Policy zur Compile-Zeit.
// ---------------------------------------------------------------------------
enum class LagerRealm { binaries, messdaten };

[[nodiscard]] constexpr std::string_view to_string(LagerRealm r) noexcept {
    switch (r) {
        case LagerRealm::binaries: return "binaries";
        case LagerRealm::messdaten: return "messdaten";
    }
    return "binaries";
}

// ---------------------------------------------------------------------------
// K1 -- DIE ZWEI WURZELEBENEN: Gattung -> Genus.
//
// Die Achsen-Namen der drei gemeinsamen Wurzelebenen. Konstanten, damit kein Konsument ein Literal
// nachbildet (dieselbe Disziplin wie kBlattAchse weiter unten).
// ---------------------------------------------------------------------------
inline constexpr std::string_view kGattungAchse = "gattung";
inline constexpr std::string_view kGenusAchse   = "genus";
inline constexpr std::string_view kRealmAchse   = "realm";

// ---------------------------------------------------------------------------
// DIE LAGER-TOKEN -- eine EIGENE, EINGEFRORENE Tabelle, NICHT gattung_name()/genus_name().
//
// WARUM NICHT sanitisiere_wert(anatomy::gattung_name(g)): das waere die naheliegende Abkuerzung und
// sie ist eine Falle. Zwei Gruende, beide am Objekt belegbar:
//
//  (1) DIE ENUM-ETIKETTEN AENDERN SICH. E-24 C7-1 hat am 04.08.2026 den Enumerator SearchAlgorithm
//      nach Map umbenannt (anatomy_base.hpp, Kommentar an AnatomyGattung) -- eine reine Etiketten-
//      Korrektur, ausdruecklich erlaubt, WEIL nichts eingefroren war. Ein Ordnername ist aber ab dem
//      ersten eingelagerten Blatt eingefroren: die naechste solche Umbenennung wuerde jeden
//      bestehenden Ast verwaisen lassen. Diese Tabelle ist genau die Naht dazwischen -- der C++-Name
//      darf sich weiter bewegen, der Lager-Token nicht.
//  (2) sanitisiere_wert("SearchAlgorithm") ergaebe "searchalgorithm" (die Funktion senkt nur Gross-
//      auf Kleinbuchstaben, sie fuegt keine Trenner ein). Der Token unten heisst bewusst
//      "search_algorithm" -- lesbar, und in der Achsen-Zeichenklasse des Hauses.
//
// ABGRENZUNG ZU E-24 C7-6: dort wurde entschieden, KEINEN Gattungs-String auf den ABI-DRAHT zu legen
// (kein vtable-Member, kein Wire-Feld, Begruendung u.a. "Etiketten reisen in Experiment-Logs"). Das
// bleibt unberuehrt: hier entsteht kein Draht-String, sondern ein HOST-SEITIGER Ordnername, den der
// Owner mit K1 ausdruecklich angeordnet hat. Die .so exportiert weiterhin nur genus().
//
// EIN UNBEKANNTER WERT LIEFERT LEER, nicht "unknown": ein Ersatzname waere ein schreibbarer Ordner
// und damit genau der stille Stellvertreter, den wir nicht wollen -- alles landete unter
// gattung=unknown und der Baum saehe gesund aus. Leer ist nicht schreibbar und wird unten gefangen.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string_view lager_gattung_token(anatomy::AnatomyGattung g) noexcept {
    switch (g) {
        case anatomy::AnatomyGattung::Map: return "map";
        case anatomy::AnatomyGattung::Container: return "container";
        case anatomy::AnatomyGattung::Graph: return "graph";
        // HY-A1 (09.08.2026), NACHGETRAGEN 09.08.2026 (Warnungs-Runde 1, Klasse REGRESSION): die
        // vierte Ebene-1-Kategorie. Sie kam mit HY-A1 in anatomy_base.hpp an, ohne dass diese Tabelle
        // mitwuchs -- genau der Fall, den die WACHE 1 unten compile-hart abfangen soll. Der Token folgt
        // der Hausform der Tabelle (snake_case des Enumerator-Namens, wie "search_algorithm").
        case anatomy::AnatomyGattung::HeuristikAdapter: return "heuristik_adapter";
    }
    return {};
}

[[nodiscard]] constexpr std::string_view lager_genus_token(anatomy::AnatomyGenus g) noexcept {
    switch (g) {
        case anatomy::AnatomyGenus::SearchAlgorithm: return "search_algorithm";
        case anatomy::AnatomyGenus::Set: return "set";
        case anatomy::AnatomyGenus::Sequence: return "sequence";
        case anatomy::AnatomyGenus::Adapter: return "adapter";
        case anatomy::AnatomyGenus::View: return "view";
        // HY-A1: das Reroute-Genus der Gattung HeuristikAdapter. Es ist ein KLASSIFIKATIONS-Wert und
        // sortiert das Hybrid-Artefakt im Lagerbaum ein; genus() einer Hybrid-Binary liefert ihn nie
        // (Owner-Entscheid E-1, Weg C). Fuer den Lagerbaum zaehlt aber genau die Klassifikation --
        // deshalb braucht er hier einen Token und nicht etwa den geerbten Ziel-Genus-Token.
        case anatomy::AnatomyGenus::FunctionInterfaceReroute: return "function_interface_reroute";
    }
    return {};
}

namespace detail {

/// Ist v ein ECHTER Enumerator? ORAKEL AUS FREMDER QUELLE (T-3): gattung_name()/genus_name() sind die
/// kanonischen Namens-Funktionen der Anatomie und liefern fuer jeden Nicht-Enumerator "Unknown". Wir
/// fragen also die Anatomie, nicht uns selbst -- eine Tabelle, die sich selbst als vollstaendig
/// bescheinigt, beweist nichts.
[[nodiscard]] constexpr bool ist_gattung_enumerator(anatomy::AnatomyGattung g) noexcept {
    return anatomy::gattung_name(g) != std::string_view{"Unknown"};
}
[[nodiscard]] constexpr bool ist_genus_enumerator(anatomy::AnatomyGenus g) noexcept {
    return anatomy::genus_name(g) != std::string_view{"Unknown"};
}

/// WACHE 1 -- TOTALITAET IN BEIDE RICHTUNGEN, ueber den GESAMTEN Wertebereich des Basistyps
/// (uint8_t -> 256 Werte; das ist die Grundgesamtheit dieser Wache, nicht die 3 bzw. 5 von heute).
/// Hinrichtung:  jeder Enumerator hat einen Token  -> eine NEUE Gattung ohne Token bricht den BAU.
/// Rueckrichtung: kein Nicht-Enumerator hat einen Token -> ein Token fuer einen entfernten Wert
///                bleibt nicht als Leiche liegen.
[[nodiscard]] consteval bool lager_gattung_tokens_sind_total() {
    for (int v = 0; v <= 255; ++v) {
        auto const g = static_cast<anatomy::AnatomyGattung>(static_cast<std::uint8_t>(v));
        if (ist_gattung_enumerator(g) != !lager_gattung_token(g).empty()) return false;
    }
    return true;
}

[[nodiscard]] consteval bool lager_genus_tokens_sind_total() {
    for (int v = 0; v <= 255; ++v) {
        auto const g = static_cast<anatomy::AnatomyGenus>(static_cast<std::uint8_t>(v));
        if (ist_genus_enumerator(g) != !lager_genus_token(g).empty()) return false;
    }
    return true;
}

/// WACHE 2 -- jeder Token ist als Ordner-Segment SCHREIBBAR (Werte-Zeichenklasse der Grammatik).
/// Ohne sie faellt ein neuer Token erst zur Laufzeit auf, und zwar als wert_zeichenklasse tief in
/// einer Kaskade -- weit weg von der Zeile, die ihn eingefuehrt hat.
[[nodiscard]] consteval bool lager_wurzel_tokens_sind_grammatik_rein() {
    for (int v = 0; v <= 255; ++v) {
        auto const ga = static_cast<anatomy::AnatomyGattung>(static_cast<std::uint8_t>(v));
        if (ist_gattung_enumerator(ga) && !ist_wert_rein(lager_gattung_token(ga))) return false;
        auto const ge = static_cast<anatomy::AnatomyGenus>(static_cast<std::uint8_t>(v));
        if (ist_genus_enumerator(ge) && !ist_wert_rein(lager_genus_token(ge))) return false;
    }
    // Die Realm-Ebene laeuft ab K1 durch dieselbe Grammatik und braucht deshalb dieselbe Zusage.
    return ist_wert_rein(to_string(LagerRealm::binaries)) && ist_wert_rein(to_string(LagerRealm::messdaten)) &&
           ist_achse_rein(kGattungAchse) && ist_achse_rein(kGenusAchse) && ist_achse_rein(kRealmAchse);
}

/// WACHE 3 -- die Token sind PAARWEISE VERSCHIEDEN. Das ist die schaerfste der drei: zwei Gattungen
/// mit demselben Token wuerden ihre Baeume STILL ineinander schieben. Nichts wuerde klappern, die
/// Pfade blieben wohlgeformt, und zwei Messreihen laegen unter derselben Adresse.
///
/// UMBAU 09.08.2026 (Warnungs-Runde 2, clang; Klasse REGRESSION am WERKZEUG-BUDGET). Die erste Form
/// lief als Doppelschleife ueber alle 256x255/2 Wertepaare und rief in JEDEM Paar die Enumerator-
/// Orakel (gattung_name/genus_name + string_view-Vergleich gegen "Unknown") erneut -- zusammen ueber
/// 2,6 Mio. constexpr-Schritte. GCCs Budget (-fconstexpr-ops-limit, Default 33.554.432) schluckt
/// das; clangs Budget (-fconstexpr-steps, Default 1.048.576) NICHT: "constexpr evaluation hit
/// maximum step limit" -- der Bau bricht, und zwar NUR unter clang. Ein Budget-Flag zu erhoehen
/// waere der Stellvertreter (Signal weg, Kosten bleiben). Stattdessen sammelt SCHRITT 1 die Tokens
/// der echten Enumeratoren EINMAL ein (2x256 Orakel-Aufrufe), und SCHRITT 2 vergleicht nur noch die
/// eingesammelten Tokens paarweise. Die ZUSAGE ist byte-identisch dieselbe: verglichen werden exakt
/// die Paare (a<b) echter Enumeratoren je Ebene -- nur eben ohne die 65.280-fache Orakel-Wiederholung.
[[nodiscard]] consteval bool lager_wurzel_tokens_sind_unterscheidbar() {
    std::array<std::string_view, 256> ga_toks{};
    std::array<std::string_view, 256> ge_toks{};
    std::size_t                       ga_n = 0;
    std::size_t                       ge_n = 0;
    for (int v = 0; v <= 255; ++v) {
        auto const ga = static_cast<anatomy::AnatomyGattung>(static_cast<std::uint8_t>(v));
        if (ist_gattung_enumerator(ga)) ga_toks[ga_n++] = lager_gattung_token(ga);
        auto const ge = static_cast<anatomy::AnatomyGenus>(static_cast<std::uint8_t>(v));
        if (ist_genus_enumerator(ge)) ge_toks[ge_n++] = lager_genus_token(ge);
    }
    for (std::size_t a = 0; a < ga_n; ++a)
        for (std::size_t b = a + 1; b < ga_n; ++b)
            if (ga_toks[a] == ga_toks[b]) return false;
    for (std::size_t a = 0; a < ge_n; ++a)
        for (std::size_t b = a + 1; b < ge_n; ++b)
            if (ge_toks[a] == ge_toks[b]) return false;
    return true;
}

/// WACHE 4 -- jedes Genus haengt an einer Gattung, die selbst einen Token hat. Ohne sie koennte ein
/// neues Genus in eine Gattung zeigen, die im Lager gar nicht existiert -- die Wurzelebene 1 waere
/// dann unbaubar, obwohl Ebene 2 sauber aussieht.
[[nodiscard]] consteval bool jedes_genus_haengt_an_einer_lagerbaren_gattung() {
    for (int v = 0; v <= 255; ++v) {
        auto const ge = static_cast<anatomy::AnatomyGenus>(static_cast<std::uint8_t>(v));
        if (!ist_genus_enumerator(ge)) continue;
        if (lager_gattung_token(anatomy::gattung_of(ge)).empty()) return false;
    }
    return true;
}

} // namespace detail

static_assert(detail::lager_gattung_tokens_sind_total(),
              "K1: JEDE anatomy::AnatomyGattung braucht genau einen Lager-Token und jeder Token genau eine "
              "Gattung. Wer eine Gattung hinzufuegt (etwa die Hybrid-Gattung HEURISTIK-ADAPTER aus E-1), traegt "
              "sie HIER nach -- dann, und nur dann, ist sie im Lagerbaum einsortierbar. Genau das ist der Sinn "
              "des K1-Umbaus: kein Sonderfall je Gattung, aber auch kein stilles Durchrutschen.");
static_assert(detail::lager_genus_tokens_sind_total(), "K1: dasselbe fuer jedes anatomy::AnatomyGenus (Ebene 2).");
static_assert(detail::lager_wurzel_tokens_sind_grammatik_rein(),
              "K1: jeder Wurzel-Token und jeder Realm-Name muss der Werte-Zeichenklasse [a-z0-9._-] genuegen, "
              "jeder Wurzel-Achsen-Name der Achsen-Klasse [a-z0-9_] -- sonst ist die Ebene nicht schreibbar.");
static_assert(detail::lager_wurzel_tokens_sind_unterscheidbar(),
              "K1: zwei Gattungen (oder zwei Genera) mit demselben Lager-Token wuerden ihre Baeume still "
              "verschmelzen. Der Token ist die Adresse -- er muss injektiv sein.");
static_assert(detail::jedes_genus_haengt_an_einer_lagerbaren_gattung(),
              "K1: gattung_of(genus) muss auf eine Gattung zeigen, die selbst einen Lager-Token hat.");

// ---------------------------------------------------------------------------
// LagerWurzelPaar -- die zwei Wurzelebenen als EIN Wert.
//
// KEIN VORGABE-KONSTRUKTOR, mit Absicht. Ein Default waere zwangslaeufig Map/SearchAlgorithm (die
// Enum-Werte 0) -- also eine ECHTE, plausible, falsche Antwort: jeder Aufrufer, der das Feld
// vergisst, lagerte still unter gattung=map/genus=search_algorithm ein. Genau diese Klasse Fehler
// faellt nie auf. Ohne Vorgabe-Konstruktor bricht stattdessen JEDE Spec-Konstruktion, die das Paar
// nicht nennt, beim UEBERSETZEN -- das ist der laute Bruch, den K1 ausdruecklich will (ein
// Migrationszwang fuer alte Pfade besteht NICHT, wohl aber die Pflicht, dass der Bruch sichtbar ist).
// ---------------------------------------------------------------------------
struct LagerWurzelPaar {
    anatomy::AnatomyGattung gattung;
    anatomy::AnatomyGenus   genus;

    LagerWurzelPaar() = delete;
    constexpr LagerWurzelPaar(anatomy::AnatomyGattung ga, anatomy::AnatomyGenus ge) noexcept : gattung{ga}, genus{ge} {}

    friend bool operator==(LagerWurzelPaar const&, LagerWurzelPaar const&) = default;
};

/// Die Zugehoerigkeit von Ebene 2 zu Ebene 1 -- die EINE Wahrheit dazu ist anatomy::gattung_of.
/// Sie wird hier NICHT nachgebaut (Riss-2-Lehre, wie bei pruefe_system_ordnung gegen kSystemAxisOrder).
[[nodiscard]] constexpr bool wurzel_paar_ist_stimmig(LagerWurzelPaar const& w) noexcept {
    return anatomy::gattung_of(w.genus) == w.gattung;
}

// ---------------------------------------------------------------------------
// Fehlerklassen des Writers. Sie sind GETRENNT von LagerPfadFehler: die Grammatik urteilt ueber
// Zeichen und Laengen, der Writer ueber ORDNUNG und STRUKTUR. Ein Kaskaden-Ergebnis traegt beide,
// damit ein Testat nie raten muss, welche Schicht abgelehnt hat.
// ---------------------------------------------------------------------------
enum class LagerBaumFehler : int {
    ok = 0,
    grammatik,                 // eine Ebene verstiess gegen die kv-Grammatik (Detail: pfad_fehler)
    wurzel_token_fehlt,        // K1: Gattung/Genus ohne Lager-Token (kein Enumerator der Anatomie)
    gattung_genus_unvereinbar, // K1: gattung_of(genus) != gattung -- das Genus haengt woanders
    system_achse_unbekannt,    // eine System-Achse steht nicht in abi::kSystemAxisOrder
    system_achsen_ordnung,     // System-Achsen nicht in der bindenden Ordnung (L22)
    organ_achse_unbekannt,     // eine Organ-Achse steht nicht in experiment::kCompositionAxisNames
    organ_achse_doppelt,       // dieselbe Organ-Achse zweimal geliefert
    organ_achse_fehlt,         // eine der 18 Organ-Achsen fehlt -> ein Gruppen-Ordner waere unvollstaendig
    ebene_fehlt,               // eine PFLICHT-Ebene ist leer
    fingerprint_form,          // Blatt-Fingerprint ist kein 128-stelliges Klein-Hex
    ablage_verzeichnis,        // Verzeichnis-Anlegen schlug fehl
    ablage_datei,              // Datei-Schreiben schlug fehl
};

[[nodiscard]] constexpr std::string_view to_string(LagerBaumFehler f) noexcept {
    switch (f) {
        case LagerBaumFehler::ok: return "ok";
        case LagerBaumFehler::grammatik: return "grammatik";
        case LagerBaumFehler::wurzel_token_fehlt: return "wurzel_token_fehlt";
        case LagerBaumFehler::gattung_genus_unvereinbar: return "gattung_genus_unvereinbar";
        case LagerBaumFehler::system_achse_unbekannt: return "system_achse_unbekannt";
        case LagerBaumFehler::system_achsen_ordnung: return "system_achsen_ordnung";
        case LagerBaumFehler::organ_achse_unbekannt: return "organ_achse_unbekannt";
        case LagerBaumFehler::organ_achse_doppelt: return "organ_achse_doppelt";
        case LagerBaumFehler::organ_achse_fehlt: return "organ_achse_fehlt";
        case LagerBaumFehler::ebene_fehlt: return "ebene_fehlt";
        case LagerBaumFehler::fingerprint_form: return "fingerprint_form";
        case LagerBaumFehler::ablage_verzeichnis: return "ablage_verzeichnis";
        case LagerBaumFehler::ablage_datei: return "ablage_datei";
    }
    return "ok";
}

// ---------------------------------------------------------------------------
// Die 5 ORGAN-GRUPPEN in Speicherhierarchie-Reihenfolge (LB-SESSION 26.07. Section 5). Die Namen sind
// ENGLISCH und nummeriert, damit ein `ls` sie in fachlicher Reihenfolge zeigt und nicht alphabetisch.
//
// KEINE ZWEITE ACHSEN-QUELLE: die Gruppen ordnen die 18 Achsen aus experiment::kCompositionAxisNames
// nur NEU -- die Drift-Wache unten beweist compile-time, dass die Vereinigung der Gruppen exakt diese
// 18 ist (jede genau einmal). Wer eine Achse umbenennt/hinzufuegt/entfernt, bricht HIER.
// ---------------------------------------------------------------------------
inline constexpr std::size_t kOrganGruppenCount = 5;

inline constexpr std::array<std::string_view, kOrganGruppenCount> kOrganGruppenNamen{
    {"01_read_path", "02_layout", "03_placement", "04_execution", "05_write_path_io"}};

inline constexpr std::array<std::string_view, 2> kOrganGruppe01{{"search_algo", "cache_traversal"}};
inline constexpr std::array<std::string_view, 5> kOrganGruppe02{
    {"node_type", "memory_layout", "path_compression", "filter", "serialization"}};
inline constexpr std::array<std::string_view, 5> kOrganGruppe03{
    {"mapping", "allocator", "value_handle", "index_organization", "migration_policy"}};
inline constexpr std::array<std::string_view, 2> kOrganGruppe04{{"concurrency", "prefetch"}};
inline constexpr std::array<std::string_view, 4> kOrganGruppe05{
    {"queuing_q1", "queuing_q2", "io_dispatch", "persistence_target"}};

/// Die Achsen EINER Gruppe (Index 0..4). Die Reihenfolge INNERHALB der Gruppe ist die Tabelle oben --
/// nicht die Aufruf-Reihenfolge des Hosts. Nur so ist ein Ordnername reproduzierbar.
[[nodiscard]] constexpr std::span<std::string_view const> organ_gruppen_achsen(std::size_t idx) noexcept {
    switch (idx) {
        case 0: return kOrganGruppe01;
        case 1: return kOrganGruppe02;
        case 2: return kOrganGruppe03;
        case 3: return kOrganGruppe04;
        case 4: return kOrganGruppe05;
        default: return {};
    }
}

namespace detail {

[[nodiscard]] consteval bool organ_gruppen_decken_die_komposition() {
    std::array<int, experiment::kCompositionAxisNames.size()> gesehen{};
    std::size_t                                               summe = 0;
    for (std::size_t g = 0; g < kOrganGruppenCount; ++g) {
        auto const achsen = organ_gruppen_achsen(g);
        summe += achsen.size();
        for (auto const a : achsen) {
            bool gefunden = false;
            for (std::size_t i = 0; i < experiment::kCompositionAxisNames.size(); ++i)
                if (experiment::kCompositionAxisNames[i] == a) {
                    ++gesehen[i];
                    gefunden = true;
                    break;
                }
            if (!gefunden) return false; // Gruppen-Achse, die es in der Komposition nicht gibt
        }
    }
    if (summe != experiment::kCompositionAxisNames.size()) return false;
    for (int n : gesehen)
        if (n != 1) return false; // jede Komposition-Achse GENAU EINMAL
    return true;
}

// K1: beide Wachen trugen bis 09.08.2026 zusaetzlich "&& !traegt_hybrid_token(...)" -- den L3-Anteil.
// Mit L3 faellt genau dieser Konjunkt weg, NICHT die Wache: die Zeichenklassen-Zusage war und bleibt
// ihr eigentlicher Gegenstand. Bewusst nicht durch eine Ersatz-Bedingung aufgefuellt: es gibt keinen
// Grund mehr, einer Organ-Achse den Namen "hybrid_*" zu verbieten, und eine Wache ohne Grund ist
// spaeter genau die Zeile, die niemand zu entfernen wagt.
[[nodiscard]] consteval bool organ_gruppen_namen_sind_grammatik_rein() {
    for (auto const n : kOrganGruppenNamen)
        if (!ist_achse_rein(n)) return false;
    return true;
}

[[nodiscard]] consteval bool organ_achsen_sind_grammatik_rein() {
    for (auto const a : experiment::kCompositionAxisNames)
        if (!ist_achse_rein(a)) return false;
    for (auto const a : abi::kSystemAxisOrder)
        if (!ist_achse_rein(a)) return false;
    return true;
}

} // namespace detail

static_assert(detail::organ_gruppen_decken_die_komposition(),
              "Die 5 Organ-Gruppen sind eine UMSORTIERUNG der 18 Komposition-Achsen, keine zweite Liste: ihre "
              "Vereinigung muss experiment::kCompositionAxisNames genau einmal je Achse treffen. Wer eine "
              "Organ-Haupt-Achse hinzufuegt/entfernt/umbenennt, zieht die Gruppen-Tabelle hier nach.");
static_assert(detail::organ_gruppen_namen_sind_grammatik_rein(),
              "Die Organ-Gruppen-Ordnernamen muessen der Achsen-Zeichenklasse [a-z0-9_] genuegen.");
static_assert(detail::organ_achsen_sind_grammatik_rein(),
              "Jede Organ- und System-Haupt-Achse muss als Ordner-Segment schreibbar sein (Zeichenklasse).");

// ---------------------------------------------------------------------------
// Owning-Paar fuer die Spezifikationen. KvPaar traegt Views; die Spec haelt die Strings.
// ---------------------------------------------------------------------------
struct AchsenWert {
    std::string achse;
    std::string wert;

    friend bool operator==(AchsenWert const&, AchsenWert const&) = default;
};

namespace detail {

[[nodiscard]] inline std::vector<KvPaar> als_kv(std::span<AchsenWert const> v) {
    std::vector<KvPaar> out;
    out.reserve(v.size());
    for (auto const& a : v) out.push_back(KvPaar{a.achse, a.wert});
    return out;
}

/// System-Ebene: Haupt-Achsen ZUERST (in der bindenden Ordnung), Meta-Metas HINTEN (D-09).
/// Die Ordnungs-Wache liest abi::kSystemAxisOrder und bildet sie NICHT nach (Riss-2-Lehre).
[[nodiscard]] inline LagerBaumFehler pruefe_system_ordnung(std::span<AchsenWert const> system) {
    std::size_t letzter = 0;
    bool        erster  = true;
    for (auto const& a : system) {
        std::size_t const idx = abi::system_axis_order_index(a.achse);
        if (idx >= abi::kSystemAxisOrderCount) return LagerBaumFehler::system_achse_unbekannt;
        if (!erster && idx <= letzter) return LagerBaumFehler::system_achsen_ordnung;
        letzter = idx;
        erster  = false;
    }
    return LagerBaumFehler::ok;
}

} // namespace detail

/// 128-stelliges Klein-Hex? Die EINE Form-Pruefung des Blatt-Fingerprints (der WERT kommt fertig vom
/// Fingerprint-Weg -- hier wird nichts nachgerechnet, s. Kopf).
[[nodiscard]] constexpr bool ist_fingerprint_hex(std::string_view hex) noexcept {
    if (hex.size() != 128) return false;
    for (char c : hex)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

/// Das Haupt-Blatt-Segment aus dem v6-Fingerprint: "blatt=<128-hex>". Der Name ist damit exakt die
/// Lager-Identitaet -- dieselbe, unter der der Eintrag im Bestandslog steht (key_sha512).
inline constexpr std::string_view kBlattAchse = "blatt";

[[nodiscard]] inline std::optional<AchsenWert> blatt_segment_aus_fingerprint(std::string_view hex) {
    if (!ist_fingerprint_hex(hex)) return std::nullopt;
    return AchsenWert{std::string{kBlattAchse}, std::string{hex}};
}

/// LED-68b: das Test-Log liegt NEBEN der Binary, unter demselben Blatt -- nicht in einem eigenen
/// Log-Baum. EINE Ableitung, damit Schreiber und Sucher denselben Namen benutzen.
inline constexpr std::string_view kTestLogSuffix = ".test.log";

[[nodiscard]] inline std::string test_log_neben(std::string_view binary_name) {
    return std::string{binary_name} + std::string{kTestLogSuffix};
}

// ---------------------------------------------------------------------------
// Ergebnis einer Kaskaden-Berechnung: die Ebenen von der Wurzel abwaerts + die Fehlerklassen +
// die VOLL-KETTEN gekuerzter Ebenen (nie stilles Kuerzen: der Aufrufer legt sie ins Knoten-Log).
// ---------------------------------------------------------------------------
struct KaskadeErgebnis {
    std::vector<std::string> ebenen;
    LagerBaumFehler          fehler      = LagerBaumFehler::ok;
    LagerPfadFehler          pfad_fehler = LagerPfadFehler::ok;
    // Je gekuerzter Ebene ein Paar (Kurzname, Voll-Kette). Leer == nichts gekuerzt.
    std::vector<std::pair<std::string, std::string>> gekuerzt;

    [[nodiscard]] bool        ok() const noexcept { return fehler == LagerBaumFehler::ok; }
    [[nodiscard]] std::string pfad() const { return pfad_join(ebenen); }
};

namespace detail {

/// Eine kv-Ebene anhaengen; uebersetzt den Grammatik-Fehler in die Writer-Klasse und haelt die
/// Voll-Kette fest.
[[nodiscard]] inline bool ebene_anhaengen(KaskadeErgebnis& k, PfadErgebnis const& e) {
    if (!e.ok()) {
        k.fehler      = LagerBaumFehler::grammatik;
        k.pfad_fehler = e.fehler;
        return false;
    }
    if (e.gekuerzt) k.gekuerzt.emplace_back(e.text, e.voll_kette);
    k.ebenen.push_back(e.text);
    return true;
}

/// Die 18 Organ-Achsen in die 5 Gruppen-Ordner giessen. Die Reihenfolge INNERHALB einer Gruppe ist die
/// Tabelle (nicht die Aufruf-Reihenfolge) -> derselbe Satz Werte ergibt immer denselben Ordnernamen.
[[nodiscard]] inline LagerBaumFehler organ_gruppen_ebenen(KaskadeErgebnis& k, std::span<AchsenWert const> organ) {
    // Zuordnung Achse -> Wert, mit Doppel-/Fremd-Wache. Linear ueber 18 Namen: kein Hot-Path.
    std::array<std::string_view, experiment::kCompositionAxisNames.size()> werte{};
    std::array<bool, experiment::kCompositionAxisNames.size()>             belegt{};
    for (auto const& a : organ) {
        std::size_t idx = experiment::kCompositionAxisNames.size();
        for (std::size_t i = 0; i < experiment::kCompositionAxisNames.size(); ++i)
            if (experiment::kCompositionAxisNames[i] == a.achse) {
                idx = i;
                break;
            }
        if (idx == experiment::kCompositionAxisNames.size()) return LagerBaumFehler::organ_achse_unbekannt;
        if (belegt[idx]) return LagerBaumFehler::organ_achse_doppelt;
        belegt[idx] = true;
        werte[idx]  = a.wert;
    }
    for (bool b : belegt)
        if (!b) return LagerBaumFehler::organ_achse_fehlt;

    for (std::size_t g = 0; g < kOrganGruppenCount; ++g) {
        std::vector<KvPaar> paare;
        for (auto const achse : organ_gruppen_achsen(g))
            for (std::size_t i = 0; i < experiment::kCompositionAxisNames.size(); ++i)
                if (experiment::kCompositionAxisNames[i] == achse) {
                    paare.push_back(KvPaar{achse, werte[i]});
                    break;
                }
        if (!ebene_anhaengen(k, gruppen_segment(kOrganGruppenNamen[g], paare))) return k.fehler;
    }
    return LagerBaumFehler::ok;
}

/// K1 -- DIE DREI GEMEINSAMEN WURZELEBENEN: gattung -> genus -> realm. EINE Funktion fuer BEIDE
/// Realms; sie ist der Grund, warum Hybrid spaeter ohne Sonderfall einsortierbar ist (der Weg kennt
/// keine einzelne Gattung, nur die Tabelle).
///
/// REIHENFOLGE DER WACHEN, bewusst wie in pruefe_kv: erst ANWESENHEIT (hat der Wert ueberhaupt einen
/// Token?), dann BEZIEHUNG (haengt das Genus an dieser Gattung?). Andersherum meldete ein Aufruf mit
/// einem voellig unbekannten Genus "unvereinbar" -- eine Aussage ueber die Beziehung, obwohl der
/// Gegenstand selbst fehlt. Die Fehlerklasse soll die ERSTE Ursache nennen.
[[nodiscard]] inline LagerBaumFehler wurzel_ebenen(KaskadeErgebnis& k, LagerWurzelPaar const& w, LagerRealm r) {
    auto const gattung_token = lager_gattung_token(w.gattung);
    auto const genus_token   = lager_genus_token(w.genus);
    if (gattung_token.empty() || genus_token.empty()) return LagerBaumFehler::wurzel_token_fehlt;
    if (!wurzel_paar_ist_stimmig(w)) return LagerBaumFehler::gattung_genus_unvereinbar;

    // Die drei Grammatik-Ausgaenge darunter sind KONSTRUKTIV UNERREICHBAR (Lens-Befund 09.08.):
    // jeder Token ist per static_assert ist_wert_rein, jede der drei Achsen ist_achse_rein, und der
    // laengste Wert (genus=search_algorithm, 22 B) liegt weit unter kMaxKomponenteBytes. kv_kette
    // hat keine weitere Fehlerquelle. Sie bleiben als Form-Invariante des ebene_anhaengen-Musters
    // stehen; einen Eingang, der sie erreicht, gibt es nicht (Mutanten dort ueberleben per
    // Definition -- das ist kein Test-Loch, sondern tote Grammatik-Vorsicht).
    std::array<KvPaar, 1> const g{KvPaar{kGattungAchse, gattung_token}};
    if (!ebene_anhaengen(k, kv_kette(g))) return k.fehler;
    std::array<KvPaar, 1> const n{KvPaar{kGenusAchse, genus_token}};
    if (!ebene_anhaengen(k, kv_kette(n))) return k.fehler;
    std::array<KvPaar, 1> const s{KvPaar{kRealmAchse, to_string(r)}};
    if (!ebene_anhaengen(k, kv_kette(s))) return k.fehler;
    return LagerBaumFehler::ok;
}

/// System-Ebene: Haupt-Achsen in bindender Ordnung + Meta-Metas HINTEN.
[[nodiscard]] inline LagerBaumFehler system_ebene(KaskadeErgebnis& k, std::span<AchsenWert const> system,
                                                  std::span<AchsenWert const> meta_metas) {
    if (system.empty()) return LagerBaumFehler::ebene_fehlt;
    if (auto const f = pruefe_system_ordnung(system); f != LagerBaumFehler::ok) return f;
    std::vector<KvPaar> paare = als_kv(system);
    for (auto const& m : meta_metas) paare.push_back(KvPaar{m.achse, m.wert});
    if (!ebene_anhaengen(k, kv_kette(paare))) return k.fehler;
    return LagerBaumFehler::ok;
}

/// Eine OPTIONALE Unter-Ebene ("mess_unter=..."): leer -> die Ebene entfaellt (A9-Doc 6.1: "nur soweit
/// noetig"). Die Form ist die des Gruppen-Segments, damit Unter-Ebenen und Gruppen-Ordner nicht in
/// zwei Formen auseinanderlaufen.
[[nodiscard]] inline LagerBaumFehler unter_ebene(KaskadeErgebnis& k, std::string_view name,
                                                 std::span<AchsenWert const> werte) {
    if (werte.empty()) return LagerBaumFehler::ok;
    if (!ebene_anhaengen(k, gruppen_segment(name, als_kv(werte)))) return k.fehler;
    return LagerBaumFehler::ok;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Die zwei Spezifikationen (RT-Daten; die Struktur ist CT).
// ---------------------------------------------------------------------------
// Die Ebenen-Nummern sind seit K1 die des VOLLEN Baumes: 1 gattung, 2 genus, 3 realm, dann der Rest.
struct MessdatenBaumSpec {
    LagerWurzelPaar         wurzel;       // Ebenen 1-2: Gattung -> Genus (K1; ohne Vorgabe, s.o.)
    std::vector<AchsenWert> mess;         // Ebene 4: Mess-Tooling + load_framework
    std::vector<AchsenWert> system;       // Ebene 5: die System-Haupt-Achsen (bindende Ordnung)
    std::vector<AchsenWert> meta_metas;   // Ebene 5: Meta-Metas, HINTEN angehaengt (D-09)
    std::vector<AchsenWert> organ;        // Ebenen 6-10: alle 18 Organ-Haupt-Achsen
    std::vector<AchsenWert> haupt_blatt;  // Ebene 11: die Blatt-Identitaet (z.B. blatt=<128-hex>)
    std::vector<AchsenWert> mess_unter;   // Ebene 12 (leer -> entfaellt)
    std::vector<AchsenWert> system_unter; // Ebene 13 (leer -> entfaellt)
    std::vector<AchsenWert> organ_unter;  // Ebene 14 (leer -> entfaellt)
};

struct BinariesBaumSpec {
    LagerWurzelPaar         wurzel;     // Ebenen 1-2: Gattung -> Genus (K1; ohne Vorgabe, s.o.)
    std::vector<AchsenWert> system;     // Ebene 4: System-Achse (+ Meta-Metas)
    std::vector<AchsenWert> meta_metas; // Ebene 4, hinten
    std::vector<AchsenWert> organ;      // Ebenen 5-9: alle 18 Organ-Haupt-Achsen
    std::vector<AchsenWert> mess_typ;   // Ebene 10: Mess-Typ als TIEFSTER Haupt-Achsen-Typ (D-12)
};

// DIE WACHE ZUM LAUTEN BRUCH (K1, Lens-Auflage 09.08.): "keine Spec aus der Zeit vor K1 uebersetzt
// noch" war bis hier nur Disziplin -- eine gestrichene Zeile (LagerWurzelPaar() = delete) haette
// den stillen Default gattung=map/genus=search_algorithm wiederhergestellt, ohne dass ein Test
// faellt. Die Zusicherung gehoert an die SPEC, nicht ans Paar: ein Default-Member-Initializer in
// der Spec stellte denselben stillen Default eine Ebene hoeher wieder her und liefe an einer
// Paar-Wache vorbei (nachgemessen: die enge Fassung liess genau das durch).
static_assert(!std::is_default_constructible_v<MessdatenBaumSpec> && !std::is_default_constructible_v<BinariesBaumSpec>,
              "K1: keine Spec darf ohne genanntes Wurzel-Paar konstruierbar sein.");

// ---------------------------------------------------------------------------
// Der CT-Vertrag einer Realm-Policy (Concept-Guard statt vtable -- CRTP/Concept-Doktrin, GoF Strategy).
// ---------------------------------------------------------------------------
template <typename P>
concept LagerRealmPolicy = requires(typename P::Spec const& s) {
    { P::realm() } -> std::same_as<LagerRealm>;
    { P::kaskade(s) } -> std::same_as<KaskadeErgebnis>;
};

struct MessdatenRealmPolicy {
    using Spec = MessdatenBaumSpec;

    [[nodiscard]] static constexpr LagerRealm realm() noexcept { return LagerRealm::messdaten; }

    [[nodiscard]] static KaskadeErgebnis kaskade(Spec const& s) {
        KaskadeErgebnis k;
        if (auto const f = detail::wurzel_ebenen(k, s.wurzel, realm()); f != LagerBaumFehler::ok) {
            // Die Bedingung unterscheidet nur in den konstruktiv unerreichbaren Grammatik-
            // Ausgaengen von wurzel_ebenen (s. dort); auf allen erreichbaren Pfaden ist sie
            // aequivalent zur bedingungslosen Zuweisung. Sie steht hier der Form wegen.
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (s.mess.empty()) {
            k.fehler = LagerBaumFehler::ebene_fehlt;
            return k;
        }
        if (!detail::ebene_anhaengen(k, kv_kette(detail::als_kv(s.mess)))) return k;
        if (auto const f = detail::system_ebene(k, s.system, s.meta_metas); f != LagerBaumFehler::ok) {
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (auto const f = detail::organ_gruppen_ebenen(k, s.organ); f != LagerBaumFehler::ok) {
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (s.haupt_blatt.empty()) {
            k.fehler = LagerBaumFehler::ebene_fehlt;
            return k;
        }
        if (!detail::ebene_anhaengen(k, kv_kette(detail::als_kv(s.haupt_blatt)))) return k;
        if (auto const f = detail::unter_ebene(k, "mess_unter", s.mess_unter); f != LagerBaumFehler::ok) return k;
        if (auto const f = detail::unter_ebene(k, "system_unter", s.system_unter); f != LagerBaumFehler::ok) return k;
        if (auto const f = detail::unter_ebene(k, "organ_unter", s.organ_unter); f != LagerBaumFehler::ok) return k;
        return k;
    }
};

struct BinariesRealmPolicy {
    using Spec = BinariesBaumSpec;

    [[nodiscard]] static constexpr LagerRealm realm() noexcept { return LagerRealm::binaries; }

    [[nodiscard]] static KaskadeErgebnis kaskade(Spec const& s) {
        KaskadeErgebnis k;
        if (auto const f = detail::wurzel_ebenen(k, s.wurzel, realm()); f != LagerBaumFehler::ok) {
            // Schwester der Messdaten-Policy: Bedingung nur der Form wegen, s. Kommentar dort.
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (auto const f = detail::system_ebene(k, s.system, s.meta_metas); f != LagerBaumFehler::ok) {
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (auto const f = detail::organ_gruppen_ebenen(k, s.organ); f != LagerBaumFehler::ok) {
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (s.mess_typ.empty()) {
            k.fehler = LagerBaumFehler::ebene_fehlt;
            return k;
        }
        if (!detail::ebene_anhaengen(k, kv_kette(detail::als_kv(s.mess_typ)))) return k;
        return k;
    }

    /// ABNAHME-5: der Praefix, in dessen BLAETTERN die CEB-Binaries liegen -- das ist der
    /// System-Achsen-Knoten am Uebergang zu den Organ-Achsen, seit K1 also die Ebenen 1-4
    /// (gattung, genus, realm, system). Eine CEB gehoert keiner Organ-Permutation an; sie unter einen
    /// Organ-Ordner zu legen waere eine erfundene Bindung.
    ///
    /// SCHWESTERPFLICHT (T-6): diese Funktion baut ihren Ebenen-Satz EIGENSTAENDIG, nicht als Praefix
    /// von kaskade(). Wer die Wurzelebenen nur dort einzieht und hier vergisst, verliert genau die
    /// Praefix-Eigenschaft, die ABNAHME-5 zusichert -- und zwar LEISE: beide Pfade blieben
    /// wohlgeformt, nur laege die CEB nicht mehr ueber ihren Tiers. Der Test dazu vergleicht deshalb
    /// pfad_praefix_passt(voll, ceb) und nicht bloss die Laengen.
    [[nodiscard]] static KaskadeErgebnis ceb_blatt_ebenen(Spec const& s) {
        KaskadeErgebnis k;
        if (auto const f = detail::wurzel_ebenen(k, s.wurzel, realm()); f != LagerBaumFehler::ok) {
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
            return k;
        }
        if (auto const f = detail::system_ebene(k, s.system, s.meta_metas); f != LagerBaumFehler::ok)
            if (k.fehler == LagerBaumFehler::ok) k.fehler = f;
        return k;
    }
};

static_assert(LagerRealmPolicy<MessdatenRealmPolicy>, "MessdatenRealmPolicy muss den CT-Vertrag erfuellen");
static_assert(LagerRealmPolicy<BinariesRealmPolicy>, "BinariesRealmPolicy muss den CT-Vertrag erfuellen");
static_assert(MessdatenRealmPolicy::realm() != BinariesRealmPolicy::realm(),
              "Die zwei Realm-Wurzeln sind DISJUNKT (Owner-E2 'Realm-Trennung') -- eine Policy, die denselben "
              "Realm meldet wie die andere, hat den Schnitt verloren.");

// ---------------------------------------------------------------------------
// ABLAGE-NAHT -- vier Verben auf dem Datei-System bzw. auf einem Fake im Test. Dieselbe
// Injektions-Bauform wie BestandTransport (bestandslog_lock.hpp): der Writer haengt an seiner
// Abstraktion, nicht an std::filesystem. Kein Hot-Path -> std::function ist zulaessig.
// ---------------------------------------------------------------------------
struct BaumAblage {
    std::function<bool(std::string const& pfad)>                            verzeichnis_anlegen;
    std::function<bool(std::string const& pfad, std::string const& inhalt)> datei_schreiben;
    std::function<std::optional<std::string>(std::string const& pfad)>      datei_lesen;
    std::function<bool(std::string const& pfad)>                            existiert;
    // Loeschen (idempotent: fehlend == ok). Fuenftes Verb, weil der Knoten-Lock (LB-1) sein
    // <doc>.lock-Nebenobjekt wieder abraeumen muss -- dieselbe Vier-Verben-Naht wie BestandTransport
    // plus genau dieses eine. Ein Writer ohne Loeschen bliebe auf jedem Lock sitzen.
    std::function<bool(std::string const& pfad)> datei_entfernen;
};

/// Die reale Bindung an std::filesystem. Fehler reisen als false (die Aufrufer klassifizieren sie als
/// ablage_verzeichnis/ablage_datei) -- kein Wurf aus der Buchhaltungs-Schicht in den Bau.
[[nodiscard]] inline BaumAblage make_filesystem_ablage() {
    BaumAblage a;
    a.verzeichnis_anlegen = [](std::string const& pfad) -> bool {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path{pfad}, ec);
        if (ec) return false;
        return std::filesystem::is_directory(std::filesystem::path{pfad}, ec);
    };
    a.datei_schreiben = [](std::string const& pfad, std::string const& inhalt) -> bool {
        std::ofstream os(pfad, std::ios::binary | std::ios::trunc);
        if (!os) return false;
        os.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
        os.flush();
        return static_cast<bool>(os);
    };
    a.datei_lesen = [](std::string const& pfad) -> std::optional<std::string> {
        std::ifstream is(pfad, std::ios::binary);
        if (!is) return std::nullopt;
        std::string inhalt((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        return inhalt;
    };
    a.existiert = [](std::string const& pfad) -> bool {
        std::error_code ec;
        return std::filesystem::exists(std::filesystem::path{pfad}, ec);
    };
    a.datei_entfernen = [](std::string const& pfad) -> bool {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path{pfad}, ec);
        return !ec; // idempotent: eine fehlende Datei ist kein Fehler (remove meldet dann nur false)
    };
    return a;
}

// ---------------------------------------------------------------------------
// Ergebnis einer Einlagerung.
// ---------------------------------------------------------------------------
struct EinlagerungErgebnis {
    LagerBaumFehler          fehler      = LagerBaumFehler::ok;
    LagerPfadFehler          pfad_fehler = LagerPfadFehler::ok;
    std::string              knoten_pfad; // das Blatt-VERZEICHNIS (ohne Datei)
    std::string              blatt_pfad;  // knoten_pfad + '/' + Datei
    std::vector<std::string> knoten;      // ALLE angelegten Knoten von der Wurzel abwaerts (depth-first)
    std::vector<std::pair<std::string, std::string>> gekuerzt; // (Kurzname, Voll-Kette) je gekuerzter Ebene

    [[nodiscard]] bool ok() const noexcept { return fehler == LagerBaumFehler::ok; }
};

// ---------------------------------------------------------------------------
// LagerBaumWriter<Policy> -- EIN Writer je Realm, per CT-Policy gewaehlt. Kein Runtime-Switch:
// make_binaries_baum_writer()/make_messdaten_baum_writer() sind zwei verschiedene TYPEN.
//
// wurzel_ IST SEIT K1 DIE LAGER-WURZEL, NICHT DIE REALM-WURZEL. Vorher hing der Realm hier drin
// (Aufrufer reichten ".../binaries" bzw. ".../messdaten"); seit K1 ist er Ebene 3 der Kaskade und
// entsteht aus Policy::realm(). Ein Aufrufer, der weiterhin ".../binaries" reicht, baut den
// Realm-Ordner doppelt. Das bricht nicht beim Uebersetzen -- was aber JEDEN Aufrufer zwingt, hier
// vorbeizukommen, ist die Spec: LagerWurzelPaar hat keinen Vorgabe-Konstruktor, also uebersetzt
// keine einzige Spec-Konstruktion aus der Zeit vor K1 mehr.
//
// DEPTH-FIRST-EINLAGERUNG (ABNAHME-2): die Knoten entstehen von der Wurzel abwaerts, jeder Knoten
// wird einzeln gemeldet -- genau die Liste, die das Knoten-Log (complete-heuristik.log, LB-1)
// fortschreibt. Ein Blatt wird NIE geschrieben, bevor sein Ast vollstaendig existiert.
// ---------------------------------------------------------------------------
template <LagerRealmPolicy Policy>
class LagerBaumWriter {
public:
    using Spec = typename Policy::Spec;

    LagerBaumWriter(BaumAblage ablage, std::string wurzel) : ablage_{std::move(ablage)}, wurzel_{std::move(wurzel)} {}

    [[nodiscard]] static constexpr LagerRealm realm() noexcept { return Policy::realm(); }
    [[nodiscard]] std::string const&          wurzel() const noexcept { return wurzel_; }

    /// Nur rechnen, nichts anlegen (Resolver-Weg fuer A9: "A9 legt NIE selbst Knoten an, sondern
    /// bezieht das Blatt-Verzeichnis vom Baum-Writer/Resolver", A9-Doc 6.1).
    [[nodiscard]] KaskadeErgebnis kaskade(Spec const& s) const { return Policy::kaskade(s); }

    /// Der volle Blatt-Verzeichnis-Pfad unter der Wurzel (leer, wenn die Kaskade nicht gebaut werden konnte).
    [[nodiscard]] std::string knoten_pfad(Spec const& s) const {
        auto const k = Policy::kaskade(s);
        if (!k.ok()) return {};
        return objekt_key(wurzel_, k.pfad());
    }

    /// Depth-first einlagern: Ast anlegen, dann das Blatt schreiben.
    [[nodiscard]] EinlagerungErgebnis einlagern(Spec const& s, std::string const& blatt_name,
                                                std::string const& inhalt) const {
        EinlagerungErgebnis erg;
        auto const          k = Policy::kaskade(s);
        erg.gekuerzt          = k.gekuerzt;
        if (!k.ok()) {
            erg.fehler      = k.fehler;
            erg.pfad_fehler = k.pfad_fehler;
            return erg;
        }
        if (blatt_name.empty()) {
            erg.fehler = LagerBaumFehler::ebene_fehlt;
            return erg;
        }
        std::string lauf = wurzel_;
        if (!lauf.empty() && ablage_.verzeichnis_anlegen && !ablage_.verzeichnis_anlegen(lauf)) {
            erg.fehler = LagerBaumFehler::ablage_verzeichnis;
            return erg;
        }
        for (auto const& ebene : k.ebenen) {
            lauf = objekt_key(lauf, ebene);
            if (!ablage_.verzeichnis_anlegen || !ablage_.verzeichnis_anlegen(lauf)) {
                erg.fehler      = LagerBaumFehler::ablage_verzeichnis;
                erg.knoten_pfad = lauf;
                return erg;
            }
            erg.knoten.push_back(lauf);
        }
        erg.knoten_pfad = lauf;
        erg.blatt_pfad  = objekt_key(lauf, blatt_name);
        if (!ablage_.datei_schreiben || !ablage_.datei_schreiben(erg.blatt_pfad, inhalt)) {
            erg.fehler = LagerBaumFehler::ablage_datei;
            return erg;
        }
        return erg;
    }

    [[nodiscard]] BaumAblage const& ablage() const noexcept { return ablage_; }

private:
    BaumAblage  ablage_;
    std::string wurzel_;
};

// Factory Pattern (benannt) -- die zwei Realms als eigene TYPEN, nie als Parameter.
[[nodiscard]] inline LagerBaumWriter<BinariesRealmPolicy> make_binaries_baum_writer(BaumAblage  ablage,
                                                                                    std::string wurzel) {
    return LagerBaumWriter<BinariesRealmPolicy>{std::move(ablage), std::move(wurzel)};
}

[[nodiscard]] inline LagerBaumWriter<MessdatenRealmPolicy> make_messdaten_baum_writer(BaumAblage  ablage,
                                                                                      std::string wurzel) {
    return LagerBaumWriter<MessdatenRealmPolicy>{std::move(ablage), std::move(wurzel)};
}

} // namespace comdare::cache_engine::builder::bestandslog
