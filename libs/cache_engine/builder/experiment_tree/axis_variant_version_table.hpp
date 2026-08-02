#pragma once
// Inkrementeller Tier-Binary-Cache (Bauplan §2+§4, 2026-07-18) — compile-time {axis,variant -> algo_version}-Tabelle.
//
// Der Rebuild-/Neu-Mess-Selektor braucht je Binary eine deterministische Organ-Algorithmus-Signatur (algo_sig).
// Diese Tabelle liefert das Fundament: sie reflektiert die 17 KOMPOSITIONS-Achsen (kCompositionAxisNames-Reihenfolge,
// axis_path_serialization.hpp) aus DENSELBEN Registry-Enabled-Listen wie registry_to_axis_levels.hpp (axes26-Aliase,
// BR-1, REGISTRY-getrieben statt string-getrieben) in Tripel {axis, W::name(), W::algo_version}. compose_algo_signature
// (axis_path_serialization.hpp) schlaegt hier je (axis,value) die Version nach und baut die Flach-Signatur.
//
// **DIES IST DIE UNIVERSELLE COMPILE-ZEIT-DURCHSETZUNG (Bauplan §2 „Concept erzwingt version()"):** das mp_for_each
// greift W::algo_version je registrierter Variante ab — fehlt einer Variante das Member, ist der Zugriff ill-formed und
// die Kompilation bricht MIT DEM TYP-NAMEN. Eine Organ-Variante ohne algo_version kann so nicht unbemerkt in eine
// Registry gelangen. Ergaenzend traegt JEDE der 17 Kompositions-StrategyBases (search_algo..queuing_q2) im CRTP-Ctor
// einen fokussierten `static_assert(requires { Derived::algo_version; })` (Bauplan §2, Exemplar
// axis_06_allocator_strategy_base.hpp) — der Concept-Guard je Konstruktion. Bewusst NICHT an der gemeinsamen Wurzel
// topics::OrganAxis: die traegt auch die System-Achse ISA und die 4 Shape-Achsen, die KEIN algo_version fuehren
// (Organ- vs System-/Shape-Provenienz strikt getrennt) — ein Wurzel-Guard wuerde die faelschlich brechen.
//
// System-Achsen (telemetry/isa/page_type/simd_extension/general_hardware) und die 4 Shape-Achsen bleiben AUSSEN: sie
// tragen KEINE algo_sig (Organ- vs System-Provenienz strikt getrennt; Shapes sind Default-OFF-Anhang). C++23, header-
// only; heap-schwer (inkludiert ALLE Kompositions-Registries via registry_to_axis_levels.hpp) — nur dort einbinden,
// wo die Registries ohnehin praesent sind (Facade / dedizierter Test), NIE in einen schlanken TU.

#include "axis_path_serialization.hpp" // kCompositionAxisNames (Slot-Reihenfolge der algo_sig)
#include "registry_to_axis_levels.hpp" // axes26::T* Registry-Aliase (dieselbe Quelle wie build_all_axis_levels)

#include <cache_engine/abi/anatomy_version_stamp.hpp>  // A13-M2: abi::OrganMetaMetas (Organ-Meta-Meta-Single-Source)
#include <cache_engine/abi/meta_meta_stamp_suffix.hpp> // A13-M2: Klammer-Anhang der Meta-Metas (Owner-Q1)
#include <cache_engine/measurement/algo_semver.hpp>    // W12-A: algo_semver_string (X.Y.Z-Voll-Form, NUR fuer Stempel)

#include <axes/alloc/alloc_hw_config.hpp>       // kAllocHwSubaxisVersion (Sub-Achsen-Werteset, Bauplan §2)
#include <axes/cacheline/cacheline_config.hpp>  // kCacheLineSubaxisVersion
#include <axes/cacheline/node_width_config.hpp> // kNodeWidthSubaxisVersion

#include <boost/mp11.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

/// Ein Tabellen-Eintrag: (Kompositions-Achsen-Name, Varianten-name(), algo_version).
struct AxisVariantVersion {
    std::string_view axis;    ///< kCompositionAxisNames-Slot (z.B. "search_algo")
    std::string      variant; ///< W::name() (z.B. "bst")
    std::string      version; ///< W::algo_version (z.B. "v1.0.0"; A13-M1b: kuenftig "v1.0.0c")
};

/// mp_for_each ueber eine Registry-Enabled-Liste -> {axis, W::name(), W::algo_version} je Variante. mp_identity
/// vermeidet das Default-Konstruieren der Wrapper (nur der Typ wird benoetigt — analog reflect_names, axis_reflect.hpp).
/// Der W::algo_version-Zugriff ist die harte Compile-Zeit-Durchsetzung (siehe Datei-Kopf).
template <class List>
inline void reflect_versions(std::string_view axis, std::vector<AxisVariantVersion>& out) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        // A1 (G2-4a, W12-A, 2026-07-23): Concept-Guard der X.Y.Z-Disziplin -- JEDE registrierte Organ-Variante MUSS eine
        // PARSBARE algo_version tragen. Eine Fehlform (Kurzform "v1.2", Tippfehler, ohne 'v') parst zum Sentinel und
        // bricht hier compile-time MIT dem Typ-Namen. Ergaenzt den bestehenden requires-Existenz-Guard (Datei-Kopf) um
        // die WOHLGEFORMTHEIT -- unparsbare Versionen koennen ab jetzt nicht mehr unbemerkt in eine Registry gelangen.
        // A13-M1-Auflage K-5: die Wache prueft das x/y/z-TRIPEL (is_sentinel), NICHT den Struct-Vergleich gegen
        // AlgoSemVer{} -- sonst koennte eine Flag-Sentinel-Form die Wache umgehen.
        // A13-M1b (Owner-Q3 02.08.2026): die zulaessige Form ist "vX.Y.Z[HWFLAG[e]]" -- die Kurzform "vN" faellt seit
        // dem Rueckbau ebenfalls in diese Wache (sie ist jetzt Sentinel).
        static_assert(!::comdare::cache_engine::measurement::parse_algo_semver(W::algo_version).is_sentinel(),
                      "algo_version unparsbar (Sentinel): erlaubt ist NUR \"vX.Y.Z\" mit optional GENAU EINEM "
                      "Hardware-Flag (c/g/f/n) und danach optional 'e' -- die Kurzform \"vN\" ist verboten "
                      "(Owner-Q3 02.08.2026), A10-X.Y.Z-Disziplin");
        // A13-M1 (Owner-Entscheid E2 vom 02.08.2026): das 'e'-Suffix markiert experimentelle Achsen-Algorithmen
        // AUS EINEM PRUEFLING. Die ce-EIGENEN Registry-Varianten sind der stabile Bestand und duerfen es nie tragen
        // -- sonst waeren die ce-Stempel/Fingerprints/Lager-Keys nicht mehr beweisbar golden-neutral. Ein 'e' in
        // einer ce-Registry bricht hier compile-time MIT dem Typ-Namen.
        static_assert(!::comdare::cache_engine::measurement::parse_algo_semver(W::algo_version).experimental,
                      "algo_version experimentell ('e'-Suffix): das 'e' ist AUSSCHLIESSLICH die Pruefling-Markierung "
                      "(Owner-E2 02.08.2026) -- ce-eigene Registry-Varianten tragen es NIE");
        // A13-M1b, WACHE "wenn Flag vorhanden, dann GENAU EIN -- und im CPU-Scope 'c'" (Owner-Q3 02.08.2026).
        // Die Zweiteilung ist Absicht und deckt die Uebergangs-Phase sauber ab:
        //   * MEHRFACH-/Fremd-/Gross-Flags ("v1.0.0cg", "v1.0.0ec", "v1.0.0C", "v1.0.0x") sind schon grammatisch
        //     unparsbar und laufen in die Sentinel-Wache oben -- "genau EIN Flag" ist damit durchgesetzt.
        //   * Diese Wache HIER faengt den verbleibenden Fall: ein WOHLGEFORMTES, aber im CPU-only-Scope falsches
        //     Flag ("v1.0.0g"/"v1.0.0f"/"v1.0.0n"). Sie ist NICHT tautologisch -- eine ce-Registry-Variante mit
        //     "v1.0.0g" bricht hier compile-time MIT dem Typ-Namen, waehrend der flaglose Bestand ("v1.0.0")
        //     bewusst durchgeht (Uebergangs-Toleranz bis zum M2/M3-Migrations-Commit).
        //   * Die restliche Owner-Pflicht -- dass ueberhaupt ein Flag DA sein MUSS -- ist der ENFORCE-Block unten.
        static_assert(!::comdare::cache_engine::measurement::parse_algo_semver(W::algo_version).has_hardware_flag() ||
                          ::comdare::cache_engine::measurement::version_satisfies_cpu_only_policy(W::algo_version),
                      "algo_version mit FALSCHEM Hardware-Flag: zulaessig ist im CPU-only-Scope GENAU 'c' (bzw. 'ce') "
                      "-- g/f/n sind reserviert und werden hier nicht produziert (Owner-Q3 02.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
        // A13-M1b SCHARFSCHALTUNG (Owner-Q3: "Wir produzieren nur CPU code, daher muessen alle Versionen mit 'c'
        // oder 'ce' enden"). GEBAUT, aber per Define ausgeschaltet, solange der Bestand flaglos ist: das Define geht
        // im MIGRATIONS-COMMIT des A13-M2/M3-Neuanker-Fensters auf ON -- im selben Commit, der die 122
        // W::algo_version-Literale von "v1.0.0" auf "v1.0.0c" zieht. Die Wachen-LOGIK selbst
        // (ce_owned_version_satisfies_cpu_enforce) ist immer kompiliert und in algo_semver.hpp CT-bewiesen; das
        // Define schaltet nur ihre ANWENDUNG auf jede Registry-Variante.
        // B12 (Codex-Review 02.08.2026): der ENFORCE-Zweig prueft cpu UND !experimental ueber DIESELBE
        // Politik-Funktion wie die drei Nicht-Organ-Registries. version_satisfies_cpu_only_policy allein liesse
        // "v1.0.0ce" durch ('ce' erfuellt die CPU-Politik) -- hier faengt es heute schon die ungated 'e'-Wache
        // oben ab, aber die Politik darf nicht davon ABHAENGEN, dass die zweite Wache daneben steht.
        static_assert(::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(W::algo_version),
                      "algo_version ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version auf "
                      "'c' enden und darf NIE experimentell sein (Owner-Q3/E2 02.08.2026) -- "
                      "COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif
        out.push_back(AxisVariantVersion{axis, std::string{W::name()}, std::string{W::algo_version}});
    });
}

/// Baut die {axis,variant->version}-Tabelle ueber GENAU die 17 Kompositions-Achsen (kCompositionAxisNames-Reihenfolge
/// = algo_sig-Slot-Reihenfolge). Registry-getrieben (axes26-Aliase). Der Alias-Fahrplan spiegelt exakt
/// append_organ_core_axis_levels() + append_composition_tail_axis_levels() (die q1/q2-Slots) — OHNE die build-only-/
/// System-Achsen und OHNE die Shape-Achsen (die tragen keine algo_sig).
[[nodiscard]] inline std::vector<AxisVariantVersion> build_axis_variant_version_table() {
    std::vector<AxisVariantVersion> t;
    reflect_versions<axes26::T00_search_algo>("search_algo", t);
    reflect_versions<axes26::T01_cache_traversal>("cache_traversal", t);
    reflect_versions<axes26::T02_mapping>("mapping", t);
    reflect_versions<axes26::T03_path_compression>("path_compression", t);
    reflect_versions<axes26::T04_node_type>("node_type", t);
    reflect_versions<axes26::T05_memory_layout>("memory_layout", t);
    reflect_versions<axes26::T06_allocator>("allocator", t);
    reflect_versions<axes26::T07_prefetch>("prefetch", t);
    reflect_versions<axes26::T08_concurrency>("concurrency", t);
    reflect_versions<axes26::T09_serialization>("serialization", t);
    reflect_versions<axes26::T11_value_handle>("value_handle", t);
    reflect_versions<axes26::T13_index_organization>("index_organization", t);
    reflect_versions<axes26::T14_io_dispatch>("io_dispatch", t);
    reflect_versions<axes26::T15_migration_policy>("migration_policy", t);
    reflect_versions<axes26::T16_filter>("filter", t);
    reflect_versions<axes26::T20_queuing_q1>("queuing_q1", t);
    reflect_versions<axes26::T21_queuing_q2>("queuing_q2", t);
    // STRUKT-R ORG-18: 18. Organ-Haupt-Achse. Reihenfolge = kCompositionAxisNames (T17 hinter queuing_q2).
    reflect_versions<axes26::T26_persistence_target>("persistence_target", t);
    return t;
}

/// Nachschlag der algo_version einer (axis, variant)-Kombination. Nicht gefunden -> leerer view (der Aufrufer
/// compose_algo_signature emittiert dann den Sentinel @v0 statt zu raten). Lineare Suche ueber die kleine Tabelle.
[[nodiscard]] inline std::string_view lookup_algo_version(std::vector<AxisVariantVersion> const& table,
                                                          std::string_view axis, std::string_view variant) {
    for (AxisVariantVersion const& e : table)
        if (e.axis == axis && e.variant == variant) return e.version;
    return std::string_view{};
}

/// Die globalen Sub-Achsen-Werteset-Versionen (Bauplan §2): eine Werteset-ERWEITERUNG (z.B. neuer CacheLineConfig-
/// Wert) aendert das serialisierte Bit-Layout, ohne dass eine Varianten-algo_version bumpt -> muss in die algo_sig,
/// sonst wuerde eine layout-geaenderte Binary STILL reused. Sie sind BUILD-global (nicht per-Variante) -> als fester
/// Schwanz an jede algo_sig gehaengt; ein Bump invalidiert konsequenterweise ALLE Binaries (grobkoernig, aber korrekt
/// und selten). Deterministisch, plattform-stabil.
[[nodiscard]] inline std::string sub_axis_valueset_segment() {
    return "sub=cacheline@v" + std::to_string(::comdare::cache_engine::cacheline::kCacheLineSubaxisVersion) +
           ",node_width@v" + std::to_string(::comdare::cache_engine::cacheline::kNodeWidthSubaxisVersion) +
           ",alloc_hw@v" + std::to_string(::comdare::cache_engine::alloc::kAllocHwSubaxisVersion);
}

/// compose_algo_signature(axes, table) — die deterministische Organ-Algorithmus-Signatur EINER Binary. Iteriert die
/// kCompositionAxisNames-Slots in FESTER Reihenfolge (plattform-stabil, unabhaengig von der spec.axes-Reihenfolge),
/// sucht je Slot den (axis,value) in spec.axes und schlaegt dessen algo_version in der Tabelle nach. Format je Slot
/// "<axis>=<variant>@<version>", per ';' gejoint (Vorbild perm.algos-Sidecar, Bauplan §1), abgeschlossen vom
/// Sub-Achsen-Werteset-Schwanz. Nicht-Kompositions-Achsen (Shapes) und in spec.axes fehlende Slots werden
/// uebersprungen; eine unbekannte (axis,value)-Kombination -> @v0-Sentinel (statt zu raten). Der Rebuild-/Neu-Mess-
/// Selektor vergleicht diese Signatur String-gleich gegen den .algos-Sidecar -> nur Binaries mit geaenderter Signatur
/// (= eine gebumpte Variante im 18-Tupel ODER ein Werteset-Bump) werden neu gebaut/gemessen; die binary_id bleibt
/// unberuehrt (die Version lebt ausschliesslich im Sidecar).
[[nodiscard]] inline std::string compose_algo_signature(std::vector<std::pair<std::string, std::string>> const& axes,
                                                        std::vector<AxisVariantVersion> const&                  table) {
    std::string out;
    for (std::string_view const slot : kCompositionAxisNames) {
        for (auto const& [ax, val] : axes) {
            if (ax != slot) continue;
            std::string_view const ver = lookup_algo_version(table, slot, val);
            if (!out.empty()) out += ';';
            out += slot;
            out += '=';
            out += val;
            out += '@';
            // A13-M1b: HIER bleibt die Kurzform "v0" stehen. Dieser Zweig ist die ROHE .algos-Signatur --
            // eine SEPARATE, byte-eingefrorene Welt (der Sidecar-Vergleich ist String-gleich). Der
            // Owner-Q3-Dreistelligkeits-Rueckbau betrifft die VERSIONS-Grammatik (algo_semver), nicht diesen
            // Signatur-Token; ein Wechsel auf "v0.0.0" waere hier ein Byte-Bruch ohne Nutzen.
            out += ver.empty() ? std::string_view{"v0"} : ver;
            break;
        }
    }
    if (!out.empty()) out += ';';
    out += sub_axis_valueset_segment();
    return out;
}

/// compose_organ_stamp_line(axes, table) -- die kOrganAxisVersionLine "achse=variant@X.Y.Z;..." (Bau W12-A,
/// Section 43, geteilter Emitter-Helfer). Der SHARED-Helfer, den BEIDE Emitter-Pfade (lazy + Katalog) aufrufen
/// -> beide injizieren byte-identisch -> die 320-Round-Trip-Wache bleibt STRIKT (kein modulo).
///
/// LOEST DEN METADATEN-BLOCKER: die Version stammt aus der TABELLE (Registry-Wrapper name->algo_version), NICHT
/// aus den Composition-Strategie-Typen (die tragen kein name()/algo_version). SEPARATE Welt zur .algos-Sig
/// (compose_algo_signature bleibt byte-identisch): hier X.Y.Z-Voll-Form (algo_semver_string), NUR Haupt-Achsen
/// (Section 42.b: kein Sub-Achsen-Schwanz), kanonische kCompositionAxisNames-Ordnung (Entscheid W12-A-5).
/// Unbekannte (axis,value) -> "@0.0.0" (== der @v0-Sentinel des .algos-Pfads, in X.Y.Z). Determiniert.
[[nodiscard]] inline std::string compose_organ_stamp_line(std::vector<std::pair<std::string, std::string>> const& axes,
                                                          std::vector<AxisVariantVersion> const& table) {
    std::string out;
    for (std::string_view const slot : kCompositionAxisNames) {
        for (auto const& [ax, val] : axes) {
            if (ax != slot) continue;
            std::string_view const ver = lookup_algo_version(table, slot, val);
            if (!out.empty()) out += ';';
            out += slot;
            out += '=';
            out += val;
            out += '@';
            // A13-M1b: dreistelliger Sentinel (Owner-Q3). Byte-neutral -- "v0" und "v0.0.0" rendern beide
            // "0.0.0"; nach dem Kurzform-Rueckbau ist "v0.0.0" aber der ABSICHTLICHE statt der zufaellige Weg.
            out += ::comdare::cache_engine::measurement::algo_semver_string(ver.empty() ? std::string_view{"v0.0.0"}
                                                                                        : ver);
            break;
        }
    }
    // A13-M2 (OP-11-Rueckbau, Owner-E2): der Organ-Meta-Meta-Klammer-Anhang ANS ENDE -- an BEIDEN
    // Organ-Zeilen-Quellen (hier der REALE Emitter-Pfad, in abi::organ_stamp_line<Comp>() der Mock-Pfad).
    // Ein Anhang nur auf einer Seite waere exakt die O-8-Schritt-12-Falle ("uebersehener dritter
    // Ableitungsweg"). abi::OrganMetaMetas ist heute LEER -> die Zeile bleibt BYTE-IDENTISCH (no-op), belegt
    // durch die unveraenderten 320er-Round-Trip- und CRC-Anker.
    ::comdare::cache_engine::abi::append_meta_meta_suffix(
        out, ::comdare::cache_engine::abi::meta_meta_stamp_suffix_from_members<
                 ::comdare::cache_engine::abi::OrganMetaMetas>());
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
