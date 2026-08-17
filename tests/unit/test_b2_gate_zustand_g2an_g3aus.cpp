// tests/unit/test_b2_gate_zustand_g2an_g3aus.cpp -- B2-ABSCHLUSS (15.08.2026): DER GATE-ZUSTAND
// "G2 AN / G3 AUS" ALS KOMPILAT, nicht nur als Abbildung.
//
// DUAL-REVIEW-FUND (Codex Fund 3, am Objekt verifiziert): bis zu dieser TU wurde der reale
// Gate-Zustand der [wallclock,macro]-Binary NIRGENDS kompiliert -- /usr/bin/grep -rn SEGMENT_TIMING
// ueber tests/unit/CMakeLists.txt + .gitlab-ci.yml + CMakeLists.txt == EXIT 1 (0 Treffer in 3 von 3
// Bau-Rezepten). test_b2_mess_gate_trennung prueft ausdruecklich NUR die reine Abbildung ("kein
// Makro-Zustand dieser TU geht in die Pruefung ein"); ein Revert der drei G3-Flaechen in
// anatomy/abi_adapter.hpp (~1765/1797/1818) zurueck auf das G2-Gate bliebe dort gruen.
//
// DIESE TU IST DIE GEGENMASSNAHME. Sie wird mit EXAKT dem Define-Satz uebersetzt, den
// mess_achsen_defines("[wallclock,macro]") fuer eine Tier-TU emittiert, plus dem Perm-Marker und dem
// CT-Einbau der CEB-Stufe (KON37-01):
//     -DCOMDARE_MEASUREMENT_ON=1  -DCOMDARE_CE_ENABLE_STATISTICS=1  -DCOMDARE_CE_ENABLE_SEGMENT_TIMING=0
//     -DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1  -DCOMDARE_MEASUREMENT_TOOLING_MACRO=1
//     -DCOMDARE_EXPERIMENT_MODE_ON=1  -DCOMDARE_MEASUREMENT_COMBO_CT="[wallclock,macro]"
// Sie inkludiert den ECHTEN Adapter und INSTANZIIERT ihn (ein uninstanziierter Template-Rumpf waere
// kein Bau-Beweis) -- die MCE24-/a8s4-Bauform dieser Testdatei-Familie.
//
//   K1 COMPILE  Ableitung + Glied: die explizite 0 schlaegt die G2-Vererbung; das neunte
//               Preimage-Glied dieser TU ist literal "mg=m1;s1;st0;x1;tw1;tm1;tmi0".
//   K2 LAUFZEIT Emission == TU-Zustand: mess_achsen_defines_for_legend("[wallclock,macro]") ist
//               EXAKT der Define-Satz dieser TU (Vektor-Gleichheit, Reihenfolge inklusive).
//   K3 LAUFZEIT live-Kette der CEB-Seite: live_mess_observer_ausstattung() == true,
//               live_mess_feinkorn_ausstattung() == false, live_mess_gates_glied() == TU-Glied.
//               (Env == CT-Combo, wie im echten Lauf; die Widerspruchs-Wache verlangt das.)
//   K4 LAUFZEIT Segment-Flaechen TOT am ECHTEN, GETRIEBENEN Tier: Observer (G2) fuellt real,
//               alle seg_ns == 0, T17-Zeile unberuehrt, filled_axis_count zaehlt T17 NICHT.
//               Ein Revert der drei G3-Flaechen auf G2 macht diese Haelfte ROT (Timer liefen wieder).
//   K5 LAUFZEIT E2E cfg -> row -> CSV ueber die PRODUKTIONS-Naht lazy_row_mess_ausstattung_uebernehmen
//               (Dual-Review Fund 4): [wallclock,macro]-Polaritaet aus der live-Kette (seg_* n/a,
//               Observer-Zellen echt), [all]-Gegenpolaritaet aus der Legenden-Abbildung (seg_* Zahlen).
//
// Die try_compile-NEGATIVPROBE (G3=1 ohne G2 MUSS am #error der Ableitung scheitern) wohnt nicht hier,
// sondern im Configure-Schritt: tests/unit/CMakeLists.txt, Block "B2-ABSCHLUSS NEGATIVPROBE" (mit
// K13-Kontrolle: dieselbe Probe-TU MIT G2 muss bauen).
//
// ASCII-only. Standalone int main() (kein gtest).

#include <builder/codegen/all_axes_umbrella.hpp>

#include <anatomy/abi_adapter.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_memory_only.hpp>

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>
#include <profile_facade/mess_achsen_naht.hpp>

#include <cache_engine/abi/mess_gates_glied.hpp>
#include <cache_engine/measurement/axis_error.hpp>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace an  = ::comdare::cache_engine::anatomy;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace pf  = ::comdare::cache_engine::profile_facade;
namespace cea = ::comdare::cache_engine::abi;
namespace cem = ::comdare::cache_engine::measurement;

// ---------------------------------------------------------------------------------------------------
// K1 -- COMPILE-BEWEISE: der Zustand dieser TU ist der bestellte, und der Stempel sagt ihn wahr.
// ---------------------------------------------------------------------------------------------------
static_assert(COMDARE_CE_ENABLE_SEGMENT_TIMING == 0,
              "B2-ABSCHLUSS K1: die EXPLIZITE 0 des G3-Gates muss die G2-Vererbung der Ableitung "
              "(mess_gate_segment_timing.hpp) schlagen -- sonst ist [wallclock,macro] kein eigener Zustand.");
static_assert(cea::kMessGatesTuStatisticsOn && !cea::kMessGatesTuSegmentTimingOn,
              "B2-ABSCHLUSS K1: diese TU MUSS G2 an und G3 wirksam aus tragen (der nie kompilierte Zustand).");
static_assert(cea::kMessGatesTuMeasurementOn && cea::kMessGatesTuExperimentModeOn,
              "B2-ABSCHLUSS K1: G1 und der Perm-Marker gehoeren zum realen [wallclock,macro]-Tier-Zustand.");
static_assert(cea::kMessGatesTuToolingWallclock && cea::kMessGatesTuToolingMacro && !cea::kMessGatesTuToolingMicro,
              "B2-ABSCHLUSS K1: die Deklarations-Defines dieser TU muessen exakt {wallclock,macro} sein.");
static_assert(cea::kMessGatesTuGlied == std::string_view{"mg=m1;s1;st0;x1;tw1;tm1;tmi0"},
              "B2-ABSCHLUSS K1: das neunte Preimage-Glied dieser TU traegt nicht die [wallclock,macro]-Form "
              "(s1 mit st0) -- der Stempel wuerde einen anderen Gate-Zustand behaupten, als der Bau faehrt.");

// ---------------------------------------------------------------------------------------------------
// Die Prueffall-Komposition -- EIGENE Komposition dieser TU (dokumentierte Bauform, s.
// test_a8s4_konstitutiv_kette.cpp: gleiche Achsen-Wahl, store-traversierbarer Fall).
// ---------------------------------------------------------------------------------------------------
namespace comdare::cache_engine::compositions {
struct B2GateZustandComposition {
    using search_algo      = traversal::axis_03a_search_algo::LinearScanSearchAlgo;
    using cache_traversal  = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping          = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type        = nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>;
    using memory_layout    = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
        memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>;
    using allocator     = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch      = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency   = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization = serialization::axis_10_serialization::ObservableSerialization<
        serialization::axis_10_serialization::RawBinarySerialization>;
    using telemetry = telemetry::axis_11_telemetry::ObservableTelemetry<telemetry::axis_11_telemetry::LeafOnlyCounter>;
    using value_handle                         = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                                  = hardware::axis_09_isa::Amd64Isa;
    using index_organization                   = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch                          = io::axis_io::InMemoryOnly;
    using migration_policy                     = migration::axis_migration::NoMigration;
    using filter                               = filter::axis_filter::BloomFilter;
    using queuing_q1                           = queuing::axis_q1_queuing::NoBuffer;
    using queuing_q2                           = queuing::axis_q2_queuing::LazyFlush;
    using persistence_target                   = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;
    static constexpr std::string_view paper_id = "B2-ABSCHLUSS (Gate-Zustands-Probe G2an/G3aus)";
    static constexpr std::string_view paper_title =
        "B2 Gate-Trennung -- der [wallclock,macro]-Zustand als instanziiertes Kompilat";
    static constexpr std::string_view name = "B2GateZustandComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::B2GateZustandComposition",
                                        "tests/unit/test_b2_gate_zustand_g2an_g3aus.cpp");
};
} // namespace comdare::cache_engine::compositions

using Comp = ::comdare::cache_engine::compositions::B2GateZustandComposition;

namespace {

int g_fail = 0;

void chk(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] std::vector<std::string> split_semicolon(std::string const& line) {
    std::vector<std::string> out;
    std::string              cur;
    for (char const c : line) {
        if (c == ';') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

[[nodiscard]] std::size_t column_index(std::vector<std::string> const& header, std::string_view name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return i;
    return header.size();
}

[[nodiscard]] std::string cell(std::vector<std::string> const& header, std::vector<std::string> const& cells,
                               std::string_view name) {
    std::size_t const i = column_index(header, name);
    if (i >= header.size() || i >= cells.size()) return {};
    return cells[i];
}

/// Die 13 Observer-Spalten aus test_m1hb_observer_zellen_ehrlich -- dieselbe namentliche Liste,
/// damit K5 zellgenau an denselben Zellen haengt wie die m1hb-Wache.
constexpr char const* kObserverSpalten[] = {"search_lookup", "hit",         "miss",         "insert",    "erase",
                                            "peak",          "bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt",
                                            "fail",          "obs_axes",    "fill"};

/// Eine Mess-Zeile mit durchgehend NICHT-NULL Quellen (m1hb-Bauform): nur so ist die 0-vs-n/a-Luege
/// ueberhaupt sichtbar. Die Ausstattungs-Felder werden hier NICHT gesetzt -- genau das ist der Punkt:
/// sie kommen in K5 ausschliesslich ueber die Produktions-Naht lazy_row_mess_ausstattung_uebernehmen.
[[nodiscard]] ex::LazyMeasuredRow basis_row() {
    ex::LazyMeasuredRow row;
    row.binary_id  = "b2_gatezustand_probe";
    row.setting_id = row.binary_id;
    row.total_ns   = 1'000'000;
    row.n_ops      = 1000;
    row.timed_ops  = 2000;
    for (std::size_t k = 0; k < row.op_lat.size(); ++k) {
        row.op_lat[k].n       = 1000 + k;
        row.op_lat[k].p50_ns  = 100 + static_cast<std::int64_t>(k);
        row.op_lat[k].p99_ns  = 900 + static_cast<std::int64_t>(k);
        row.op_lat[k].p999_ns = 9000 + static_cast<std::int64_t>(k);
    }
    for (std::size_t t = 0; t < std::size(row.unified.seg_ns); ++t)
        row.unified.seg_ns[t] = 1000 + static_cast<std::int64_t>(t);
    row.unified.seg_framework_ns = 555;
    row.unified.seg_run_total_ns = 44444;
    for (std::size_t t = 0; t < std::size(row.unified.axis_stats); ++t)
        for (std::size_t f = 0; f < std::size(row.unified.axis_stats[t]); ++f)
            row.unified.axis_stats[t][f] = 100 * (t + 1) + f + 1;
    row.unified.filled_axis_count         = std::size(row.unified.axis_stats);
    row.observer.search_lookup_count      = 4711;
    row.observer.search_hit_count         = 4001;
    row.observer.search_miss_count        = 710;
    row.observer.search_insert_count      = 256;
    row.observer.search_erase_count       = 17;
    row.observer.search_peak_occupancy    = 256;
    row.observer.alloc_bytes_allocated    = 65536;
    row.observer.alloc_bytes_in_use       = 32768;
    row.observer.alloc_allocation_count   = 128;
    row.observer.alloc_deallocation_count = 64;
    row.observer.alloc_failure_count      = 3;
    row.observer.observable_axis_count    = 9;
    row.observer.tier_fill_level          = 256;
    return row;
}

constexpr std::string_view kLegende   = "[wallclock,macro]";
constexpr std::string_view kGliedSoll = "mg=m1;s1;st0;x1;tw1;tm1;tmi0";

} // namespace

int main() {
    // Env == CT-Combo, wie im echten [wallclock,macro]-Lauf -- die Widerspruchs-Wache der EINEN
    // Aufloesung (resolve_live_measurement_combo_legend) verlangt beide Kanaele synchron. VOR dem
    // ersten live_*-Aufruf gesetzt (::setenv-Bauform wie test_45_parallel_measure_loop).
    ::setenv("COMDARE_MEASUREMENT_COMBO", "[wallclock,macro]", 1);

    std::cout << "== B2-ABSCHLUSS: Gate-Zustand G2-an/G3-aus als Kompilat (K1 compile-bewiesen) ==\n";
    try {
        // ---- K2: Emission == TU-Zustand (Vektor-Gleichheit, Reihenfolge inklusive) ----------------
        std::vector<std::string> const soll = {
            "-DCOMDARE_MEASUREMENT_ON=1",
            "-DCOMDARE_CE_ENABLE_STATISTICS=1",
            "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=0",
            "-DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1",
            "-DCOMDARE_MEASUREMENT_TOOLING_MACRO=1",
        };
        auto const emittiert = pf::mess_achsen_defines_for_legend(kLegende);
        std::cout << "  K2 emittierte Defines: " << emittiert.size() << " von " << soll.size() << " erwartet\n";
        chk("K2 mess_achsen_defines_for_legend(" + std::string{kLegende} +
                ") == EXAKT der Define-Satz dieser TU (K1 bindet jeden davon an den Makro-Zustand)",
            emittiert == soll);

        // ---- K3: die live-Kette der CEB-Seite (dieselbe Aufloesung wie cfg-Schreiber im Run-Eintritt)
        chk("K3 live_mess_observer_ausstattung() == true (G2 ist da)", pf::live_mess_observer_ausstattung());
        chk("K3 live_mess_feinkorn_ausstattung() == false (G3 ist AUS-entschieden)",
            !pf::live_mess_feinkorn_ausstattung());
        std::string const live_glied = pf::live_mess_gates_glied();
        std::cout << "  K3 live_mess_gates_glied() = " << live_glied << "\n";
        chk("K3 Host-Vorhersage == TU-Wahrheit (live-Glied == kMessGatesTuGlied dieser TU)",
            live_glied == std::string{cea::kMessGatesTuGlied} && live_glied == std::string{kGliedSoll});

        // ---- K4: Segment-Flaechen TOT am ECHTEN, GETRIEBENEN Tier --------------------------------
        using Anatomy = an::SearchAlgorithmAnatomy<Comp>;
        using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;
        Adapter tier;
        auto*   drv = dynamic_cast<an::IDriveableTier*>(static_cast<an::IAnatomyBase*>(&tier));
        auto*   obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
        chk("K4 IDriveableTier vorhanden (Gattungs-Antrieb)", drv != nullptr);
        chk("K4 IObservableTier vorhanden (G2 ist einkompiliert)", obs != nullptr);
        if (drv == nullptr || obs == nullptr) {
            std::cout << "== B2-ABSCHLUSS: ABBRUCH (Interfaces fehlen) ==\n";
            return 1;
        }
        constexpr std::uint64_t kN = 1024; // Keys < 65536 (LinearScanSearchAlgo key_type=uint16)
        for (std::uint64_t i = 0; i < kN; ++i) (void)drv->tier_insert(i, i * 7u + 1u);
        std::uint64_t treffer = 0;
        for (std::uint64_t i = 0; i < kN; ++i) {
            std::uint64_t v = 0;
            if (drv->tier_lookup(i, &v) && v == i * 7u + 1u) ++treffer;
        }
        chk("K4 Gattungs-Kern real getrieben (alle Lookups treffen)", treffer == kN);

        an::ComdareTierObserverSnapshot snap{};
        obs->tier_observe(&snap);
        std::cout << "  K4 filled_axis_count = " << snap.filled_axis_count << " (T17-Deckel: 17)\n";
        chk("K4 Observer (G2) LEBT: fill_observer_v3 hat Achsen-Zeilen befuellt (filled_axis_count > 0)",
            snap.filled_axis_count > 0);
        chk("K4 T17 wird NICHT fortgeschrieben (filled_axis_count <= 17 -- SCHRITT 3 liegt hinter G3)",
            snap.filled_axis_count <= 17);
        std::size_t seg_nicht_null = 0;
        for (std::size_t t = 0; t < std::size(snap.seg_ns); ++t)
            if (snap.seg_ns[t] != 0) ++seg_nicht_null;
        std::cout << "  K4 seg_ns != 0: " << seg_nicht_null << " von " << std::size(snap.seg_ns) << "\n";
        chk("K4 ALLE seg_ns == 0 (fill_segment_timing_v3 ist NICHT einkompiliert -- die G3-Flaeche ist tot; "
            "ein Revert der drei Guards auf G2 macht genau diese Zeile rot)",
            seg_nicht_null == 0);
        chk("K4 seg_framework_ns == 0 und seg_run_total_ns == 0 (kein Segment-Lauf-Rahmen)",
            snap.seg_framework_ns == 0 && snap.seg_run_total_ns == 0);
        // Index 17 == kPathBDrivenAxisPersistenceTarget (abi_adapter.hpp) -- die NUR-Pfad-B-getriebene
        // T17-Zeile entsteht ausschliesslich im Segment-Lauf und muss hier leer bleiben.
        std::size_t t17_nicht_null = 0;
        for (std::size_t f = 0; f < std::size(snap.axis_stats[17]); ++f)
            if (snap.axis_stats[17][f] != 0) ++t17_nicht_null;
        chk("K4 T17-Zeile (axis_stats[17], persistence_target) ist vollstaendig 0 (Pfad-B-Fenster tot)",
            t17_nicht_null == 0);

        // ---- K5: E2E cfg -> row -> CSV ueber die PRODUKTIONS-Naht --------------------------------
        std::vector<std::string> const header = split_semicolon(ex::lazy_csv_header());
        std::vector<std::string>       seg_spalten;
        for (auto const& name : header)
            if (name.rfind("seg_", 0) == 0) seg_spalten.push_back(name);
        std::string const na{cem::sample_status_token(cem::SampleStatus::SourceUnavailable)};
        std::cout << "  K5 seg-Spalten im Header: " << seg_spalten.size() << ", n/a-Token: '" << na << "'\n";
        chk("K5 NENNER: es gibt seg-Spalten im Header (sonst prueft K5 nichts)", !seg_spalten.empty());

        // [wallclock,macro]-Polaritaet: die Ausstattung kommt aus der LIVE-Kette dieser TU (K3),
        // die Uebernahme in die Zeile aus der EINEN Produktions-Naht (Dual-Review Fund 4).
        ex::LazyRunConfig cfg;
        cfg.mess_observer_ausstattung = pf::live_mess_observer_ausstattung();
        cfg.mess_feinkorn_ausstattung = pf::live_mess_feinkorn_ausstattung();
        ex::LazyMeasuredRow zeile_ma  = basis_row();
        ex::lazy_row_mess_ausstattung_uebernehmen(zeile_ma, cfg, /*gemessen_unified_real=*/true);
        chk("K5 Naht-Semantik [wallclock,macro]: unified_real true, seg_real false",
            zeile_ma.unified_real && !zeile_ma.seg_real);
        std::vector<std::string> const ma = split_semicolon(ex::format_csv_row(zeile_ma));
        chk("K5 STRUKTUR: Zeile und Header haben dieselbe Zellenzahl", ma.size() == header.size());
        std::size_t seg_na = 0, seg_null_luege = 0;
        for (auto const& name : seg_spalten) {
            std::string const v = cell(header, ma, name);
            if (v == na) ++seg_na;
            if (v == "0") ++seg_null_luege;
        }
        std::cout << "  K5 [wallclock,macro]: seg-n/a = " << seg_na << " von " << seg_spalten.size()
                  << ", Null-Luegen = " << seg_null_luege << "\n";
        chk("K5 [wallclock,macro]: ALLE seg-Zellen n/a (ehrlich, ueber die Produktions-Naht erreicht)",
            seg_na == seg_spalten.size() && seg_null_luege == 0);
        std::size_t obs_zahlen = 0;
        for (char const* name : kObserverSpalten) {
            std::string const v = cell(header, ma, name);
            if (!v.empty() && v != na && v != "0") ++obs_zahlen;
        }
        chk("K5 [wallclock,macro]: alle 13 Observer-Zellen bleiben echte Zahlen (G2-Block unberuehrt)",
            obs_zahlen == 13);
        chk("K5 [wallclock,macro]: total_ns bleibt gueltig (kein zeilenweiter Ersatz)",
            cell(header, ma, "total_ns") != na && !cell(header, ma, "total_ns").empty());

        // [all]-Gegenpolaritaet: dieselbe Naht, Ausstattung aus der reinen Legenden-Abbildung --
        // ein Renderer/eine Naht, die IMMER n/a schriebe, waere sonst gruen.
        auto const        menge_all = pf::mess_tooling_menge_from_legend("[all]");
        ex::LazyRunConfig cfg_all;
        cfg_all.mess_observer_ausstattung = pf::mess_menge_hat_observer_gate(menge_all);
        cfg_all.mess_feinkorn_ausstattung = pf::mess_menge_hat_feinkorn_gate(menge_all);
        ex::LazyMeasuredRow zeile_all     = basis_row();
        ex::lazy_row_mess_ausstattung_uebernehmen(zeile_all, cfg_all, /*gemessen_unified_real=*/true);
        chk("K5 Naht-Semantik [all]: unified_real true, seg_real true", zeile_all.unified_real && zeile_all.seg_real);
        std::vector<std::string> const alle       = split_semicolon(ex::format_csv_row(zeile_all));
        std::size_t                    seg_zahlen = 0;
        for (auto const& name : seg_spalten) {
            std::string const v = cell(header, alle, name);
            if (!v.empty() && v != na && v != "0") ++seg_zahlen;
        }
        std::cout << "  K5 [all]: seg-Zellen mit echter Zahl = " << seg_zahlen << " von " << seg_spalten.size() << "\n";
        chk("K5 GEGENPROBE [all]: ALLE seg-Zellen tragen Zahlen (die Naht kann auch AN)",
            seg_zahlen == seg_spalten.size());
        chk("K5 BYTE-BILANZ: die beiden Polaritaeten rendern verschieden (der Test vergleicht wirklich zwei)",
            ma != alle);
    } catch (std::exception const& e) {
        std::cout << "  [ERR] unerwartete Ausnahme: " << e.what() << "\n";
        ++g_fail;
    }

    std::cout << "== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
