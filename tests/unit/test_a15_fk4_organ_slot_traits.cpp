// tests/unit/test_a15_fk4_organ_slot_traits.cpp -- A15 / FK-4: das ORAKEL der ORGAN-SLOT-Fehlerraeume.
//
// FK-4 fuehrt den Organ-Fehlerraum an der SLOT-POSITION T0..T17 -- dort, wo binary_id, Stempel,
// Segment-Zeiten und CSV-Spalten indiziert sind. FK-5 (gelandet, f4ce89fe) fuehrt denselben Raum am
// C++-TYP der CRTP-Basis. Zwei Traeger derselben Aussage sind genau so lange in Ordnung, wie eine
// Wache sie aneinander bindet -- sonst driften sie.
//
// DIESE TU IST DIESE WACHE und darf dafuer ALLE Schichten sehen (Praezedenz woertlich in
// test_e24_c9_fk5_fehlerraum.cpp und test_w10_c4_zellwert_naht.cpp: "Ohne diese Zeile stuende die
// Gleichheit nur als Zusage im Kommentar."). Sie beweist:
//
//   (A) NAMENS-RUECKBINDUNG. slot_name[i] == experiment::kCompositionAxisNames[i], fuer alle 18 und IN
//       DIESER REIHENFOLGE. FK-4 muss die Namen spiegeln (measurement/ darf builder/ nicht sehen);
//       ohne diese Wache waere der Spiegel eine zweite Wahrheit.
//   (B) ZAHLEN-RUECKBINDUNG. kOrganSlotCount == abi::kOrganAxisCount == kCompositionAxisNames.size().
//       Drei Stellen sagen "18"; hier wird es EINMAL geprueft statt dreimal geglaubt.
//   (C) DIE ANTI-PARALLELSTRUKTUR-WACHE, der eigentliche Zweck: der D2-Satz JEDES Slots ist
//       DECKUNGSGLEICH zum error_classes()-Satz der zugehoerigen FK-5-CRTP-Basis. Die Bruecke ueber
//       die zwei Darstellungen (SampleStatus hier, string_view dort) ist die eine kanonische Funktion
//       sample_status_label(). Faellt diese Wache, haben FK-4 und FK-5 verschiedene Meinungen darueber,
//       was eine Organ-Achse hervorbringen kann -- und niemand wuesste, welche stimmt.
//   (D) NEGATIV-PROBEN. Fuer jede der drei organ-eigenen Invarianten ein Typ, der genau sie verletzt.
//       Ohne sie waere die Wache eine Tautologie.
//
// ASCII-only.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/axis_error_traits_organ.hpp>

#include <topics/organ_axis_error_classes.hpp> // FK-5: die Etiketten-Saetze an den CRTP-Basen

#include <cache_engine/abi/anatomy_version_stamp.hpp>          // abi::kOrganAxisCount (die 18)
#include <builder/experiment_tree/axis_path_serialization.hpp> // experiment::kCompositionAxisNames

#include <organ_axes/alloc/axis_06_allocator_strategy_base.hpp>
#include <organ_axes/cache_traversal/axis_03b_cache_traversal_base.hpp>
#include <organ_axes/concurrency_axis/axis_08_concurrency_strategy_base.hpp>
#include <organ_axes/filter_axis/axis_filter_strategy_base.hpp>
#include <organ_axes/index_organization/axis_01_index_organization_strategy_base.hpp>
#include <organ_axes/io_dispatch/axis_io_strategy_base.hpp>
#include <organ_axes/layout/axis_05_memory_layout_strategy_base.hpp>
#include <organ_axes/lookup/axis_03a_search_algo_base.hpp>
#include <organ_axes/mapping/axis_03m_mapping_base.hpp>
#include <organ_axes/migration_policy/axis_migration_strategy_base.hpp>
#include <organ_axes/node/axis_04_node_type_strategy_base.hpp>
#include <organ_axes/path_compression/axis_02_path_compression_strategy_base.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_strategy_base.hpp>
#include <organ_axes/prefetch_axis/axis_07_prefetch_strategy_base.hpp>
#include <organ_axes/serialization_axis/axis_10_serialization_strategy_base.hpp>
#include <organ_axes/value_handle_axis/axis_14_value_handle_strategy_base.hpp>
#include <organ_axes/axis_q1_queuing/axis_q1_queuing_base.hpp>
#include <organ_axes/axis_q2_queuing/axis_q2_queuing_strategy_base.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

namespace ms = ::comdare::cache_engine::measurement;
namespace tp = ::comdare::cache_engine::topics;
namespace ex = ::comdare::cache_engine::builder::experiment;
namespace ab = ::comdare::cache_engine::abi;

int  g_fail = 0;
void check(char const* was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

/// Derived-Platzhalter der Basis-Instanziierungen (Muster FK-5): error_classes() ist ein statischer
/// Member der BASIS und beruehrt Derived nicht -- der Typ darf unvollstaendig bleiben.
struct NieDefiniert;

/// Sind der FK-4-Satz (SampleStatus) und der FK-5-Satz (Etiketten) DECKUNGSGLEICH? Geprueft wird in
/// BEIDE Richtungen plus Kardinalitaet -- eine blosse Teilmengen-Pruefung wuerde einen Satz, der eine
/// Klasse ZUVIEL fuehrt, durchwinken.
template <class D2Satz, class LabelSatz>
[[nodiscard]] constexpr bool saetze_deckungsgleich(D2Satz const& d2, LabelSatz const& labels) noexcept {
    if (d2.size() != labels.size()) return false;
    for (auto const s : d2) { // FK-4 -> FK-5
        bool gefunden = false;
        for (std::string_view const l : labels)
            if (l == ms::sample_status_label(s)) gefunden = true;
        if (!gefunden) return false;
    }
    for (std::string_view const l : labels) { // FK-5 -> FK-4
        bool gefunden = false;
        for (auto const s : d2)
            if (l == ms::sample_status_label(s)) gefunden = true;
        if (!gefunden) return false;
    }
    return true;
}

} // namespace

// -- (D) Die drei Negativ-Typen. Jeder verletzt GENAU EINE organ-eigene Invariante. ---------------
namespace comdare::cache_engine::measurement {
namespace {
struct O1MitBauUrteil {}; // fuehrt eine D1-Domaene -- Bau-Urteil in der Mess-Spalte
struct O2OhneBoden {};    // fuehrt kein Failed -- behauptet, am Pruef-Dock nicht scheitern zu koennen
struct O3OhneName {};     // leerer slot_name -- macht die Namens-Rueckbindung zur Tautologie
} // namespace

template <>
struct AxisErrorTraits<O1MitBauUrteil> {
    static constexpr std::string_view slot_name  = "mit_bau_urteil";
    static constexpr auto             domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto             d1_klassen = std::array{CompilerCompilerErrorClass::ToolchainFehlt};
    static constexpr auto             d2_klassen = std::array<SampleStatus, 0>{};
};
template <>
struct AxisErrorTraits<O2OhneBoden> {
    static constexpr std::string_view slot_name  = "ohne_boden";
    static constexpr auto             domains    = std::array{ErrorDomain::Sample};
    static constexpr auto             d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto             d2_klassen = std::array{SampleStatus::NotApplicable};
};
template <>
struct AxisErrorTraits<O3OhneName> {
    static constexpr std::string_view slot_name  = "";
    static constexpr auto             domains    = std::array{ErrorDomain::Sample};
    static constexpr auto             d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto             d2_klassen = std::array{SampleStatus::Failed};
};

} // namespace comdare::cache_engine::measurement

int main() {
    using namespace ::comdare::cache_engine;

    std::cout << "== (B) ZAHLEN-RUECKBINDUNG: drei Stellen sagen 18 -- einmal geprueft statt dreimal geglaubt ==\n";
    {
        static_assert(ms::kOrganSlotCount == ab::kOrganAxisCount);
        static_assert(ms::kOrganSlotCount == ex::kCompositionAxisNames.size());
        check("kOrganSlotCount == abi::kOrganAxisCount", ms::kOrganSlotCount == ab::kOrganAxisCount);
        check("kOrganSlotCount == kCompositionAxisNames.size()",
              ms::kOrganSlotCount == ex::kCompositionAxisNames.size());
        std::cout << "  [INFO] kOrganSlotCount=" << ms::kOrganSlotCount
                  << " abi::kOrganAxisCount=" << ab::kOrganAxisCount
                  << " kCompositionAxisNames.size()=" << ex::kCompositionAxisNames.size() << "\n";
    }

    std::cout << "== (A)+(C) je Slot: Name gegen kCompositionAxisNames UND D2-Satz gegen die FK-5-Basis ==\n";
    {
        std::size_t n = 0;
        // Einzeln aufgezaehlt statt gefaltet: eine vergessene oder verschobene Achse faellt HIER auf,
        // und die Zeilen sind zugleich die Karte Slot -> CRTP-Basis.
        auto pruefe = [&n](std::size_t idx, std::string_view fk4_name, auto const& fk4_d2, auto const& fk5_labels) {
            bool const name_ok =
                (idx < ex::kCompositionAxisNames.size()) && (fk4_name == ex::kCompositionAxisNames[idx]);
            bool const satz_ok = saetze_deckungsgleich(fk4_d2, fk5_labels);
            bool const ok      = name_ok && satz_ok;
            std::cout << (ok ? "  [OK]  " : "  [ERR] ") << "T" << idx << " " << fk4_name << " -> Name "
                      << (name_ok ? "deckt" : "WEICHT AB") << ", FK-4/FK-5-Satz "
                      << (satz_ok ? "deckungsgleich" : "DIVERGENT") << " (" << fk4_d2.size() << " Klasse(n))\n";
            if (!ok) ++g_fail;
            ++n;
        };
#define FK4_PRUEFE(IDX, BASIS)                                                                                         \
    pruefe(IDX, ms::AxisErrorTraits<ms::OrganSlot<IDX>>::slot_name,                                                    \
           ms::AxisErrorTraits<ms::OrganSlot<IDX>>::d2_klassen, BASIS::error_classes())

        FK4_PRUEFE(0, lookup::SearchAlgoBase<NieDefiniert>);
        FK4_PRUEFE(1, cache_traversal::CacheTraversalBase<NieDefiniert>);
        FK4_PRUEFE(2, mapping::MappingBase<NieDefiniert>);
        FK4_PRUEFE(3, path_compression::PathCompressionStrategyBase<NieDefiniert>);
        FK4_PRUEFE(4, node::NodeTypeStrategyBase<NieDefiniert>);
        FK4_PRUEFE(5, layout::MemoryLayoutStrategyBase<NieDefiniert>);
        FK4_PRUEFE(6, alloc::AllocatorStrategyBase<NieDefiniert>);
        FK4_PRUEFE(7, prefetch_axis::PrefetchStrategyBase<NieDefiniert>);
        FK4_PRUEFE(8, concurrency_axis::ConcurrencyStrategyBase<NieDefiniert>);
        FK4_PRUEFE(9, serialization_axis::SerializationStrategyBase<NieDefiniert>);
        FK4_PRUEFE(10, value_handle_axis::ValueHandleStrategyBase<NieDefiniert>);
        FK4_PRUEFE(11, index_organization::IndexOrganizationStrategyBase<NieDefiniert>);
        FK4_PRUEFE(12, io_dispatch::IoStrategyBase<NieDefiniert>);
        FK4_PRUEFE(13, migration_policy::MigrationStrategyBase<NieDefiniert>);
        FK4_PRUEFE(14, filter_axis::FilterStrategyBase<NieDefiniert>);
        FK4_PRUEFE(15, queuing::axis_q1_queuing::BufferStrategyBase<NieDefiniert>);
        FK4_PRUEFE(16, queuing::axis_q2_queuing::FlushPolicyStrategyBase<NieDefiniert>);
        FK4_PRUEFE(17, persistence_target::PersistenceTargetStrategyBase<NieDefiniert>);
#undef FK4_PRUEFE
        check("GENAU 18 Organ-Slots aufgezaehlt", n == 18u);
        std::cout << "  [INFO] aufgezaehlt = " << n << "\n";
    }

    std::cout << "== (D) NEGATIV-PROBEN: die drei organ-eigenen Invarianten beissen einzeln ==\n";
    {
        // Drift-Richtung 2 (hinter dem Count). Sie steht als static_assert auch im Header; hier wird
        // sie SICHTBAR gemacht -- eine bestandene Compile-Wache schweigt sonst.
        static_assert(!ms::OrganSlotMitFehlerklassen<ms::OrganSlot<ms::kOrganSlotCount>>);
        check("Drift: OrganSlot<18> hat KEINEN Eintrag (der Count ist nicht zu klein)",
              !ms::OrganSlotMitFehlerklassen<ms::OrganSlot<ms::kOrganSlotCount>>);

        static_assert(!ms::OrganSlotMitFehlerklassen<ms::O1MitBauUrteil>);
        check("O1: ein Slot mit D1-Domaene wird abgelehnt (Bau-Urteil gehoert nie in die Mess-Spalte)",
              !ms::OrganSlotMitFehlerklassen<ms::O1MitBauUrteil>);

        static_assert(!ms::OrganSlotMitFehlerklassen<ms::O2OhneBoden>);
        check("O2: ein Slot ohne Realm-Boden (Failed) wird abgelehnt (am Pruef-Dock scheitert jeder)",
              !ms::OrganSlotMitFehlerklassen<ms::O2OhneBoden>);

        static_assert(!ms::OrganSlotMitFehlerklassen<ms::O3OhneName>);
        check("O3: ein Slot mit leerem Namen wird abgelehnt (sonst ist die Namens-Wache eine Tautologie)",
              !ms::OrganSlotMitFehlerklassen<ms::O3OhneName>);

        // GEGENPROBE: die Wache darf nicht blind alles ablehnen.
        check("und ein REALER Slot geht durch (die Wache lehnt nicht blind alles ab)",
              ms::OrganSlotMitFehlerklassen<ms::OrganSlot<0>>);

        // GEGENPROBE ZUR SATZ-VERGLEICHS-FUNKTION selbst: sie muss eine echte Abweichung auch melden.
        // Ohne diese Zeile koennte saetze_deckungsgleich() konstant true liefern und Block (C) waere
        // blind -- die 18 [OK] oben waeren dann 18 leere Zusicherungen.
        {
            constexpr auto zuwenig = std::array{ms::SampleStatus::Failed};
            check("Gegenprobe: {Failed} vs. der 2-elementige io_dispatch-Satz wird als DIVERGENT erkannt",
                  !saetze_deckungsgleich(zuwenig, io_dispatch::IoStrategyBase<NieDefiniert>::error_classes()));
            check("Gegenprobe: {Failed} vs. der 1-elementige search_algo-Satz gilt als deckungsgleich",
                  saetze_deckungsgleich(zuwenig, lookup::SearchAlgoBase<NieDefiniert>::error_classes()));
        }
    }

    std::cout << (g_fail == 0 ? "\nALLE PROBEN GRUEN\n" : "\nFEHLER\n");
    return g_fail == 0 ? 0 : 1;
}
