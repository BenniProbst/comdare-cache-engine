// tests/unit/test_e24_c9_fk5_fehlerraum.cpp -- E-24 C9 / FK-5: das ORAKEL des Organ-Achsen-FEHLERRAUMS.
//
// FK-5 (A15 EBENE 4) traegt die Owner-Direktive vom 17.07.2026 ("Fehlerklassen und Behandlung sind fuer
// ALLE Achsen -> Unterachsen -> Algorithmen Pflicht") auf die Algorithmen-Ebene. Die Erzwingung sitzt an
// den 18 Organ-Haupt-Achsen-CRTP-Basen, in genau dem Ctor, der schon algo_version erzwingt (K2-Muster).
//
// DIESE TU IST DIE CROSS-LAYER-NAHT und beweist die vier Aussagen, die man sonst nur behaupten kann:
//
//   (A) EINE WAHRHEITSQUELLE. Der Fehlerraum-Header liegt im topics/-Dach und fuehrt die D2-Etiketten als
//       string_view, weil KEINE Datei unter axes//topics/ heute cache_engine/measurement/ zieht (Layering,
//       am Ist erhoben). Genau deshalb MUSS hier verwacht werden, dass die Etiketten zeichenfolgen-gleich
//       zu measurement::sample_status_label() sind -- sonst waeren es zwei Wahrheiten. Diese TU darf beide
//       Schichten sehen; die Praezedenz dafuer steht woertlich in test_w10_c4_zellwert_naht.cpp.
//   (B) VOLLSTAENDIGKEIT. Alle 18 Basen tragen einen nicht-leeren Fehlerraum -- einzeln aufgezaehlt, damit
//       eine vergessene Achse hier auffaellt und nicht erst, wenn jemand sie baut.
//   (C) NEGATIV-PROBE. Ein Typ OHNE error_classes() erfuellt das Concept NICHT, und einer mit LEEREM Satz
//       ebenfalls nicht. Ohne diese beiden Zeilen waere die Wache eine Tautologie.
//   (D) DOMAENEN-TRENNUNG. Der Fehlerraum enthaelt NIE "mess_ok" (Ok ist der Normalfall, kein Fehler) und
//       keine D1-Etiketten (Planer-/Compile-Zeit ist eine andere Domaene).
//
// ASCII-only.

#include <cache_engine/measurement/axis_error.hpp> // die EINE Taxonomie-Quelle (measurement-Schicht)
#include <topics/organ_axis_error_classes.hpp>     // der Fehlerraum-Traeger (topics-Schicht)

#include <axes/alloc/axis_06_allocator_strategy_base.hpp>
#include <axes/cache_traversal/axis_03b_cache_traversal_base.hpp>
#include <axes/concurrency_axis/axis_08_concurrency_strategy_base.hpp>
#include <axes/filter_axis/axis_filter_strategy_base.hpp>
#include <axes/index_organization/axis_01_index_organization_strategy_base.hpp>
#include <axes/io_dispatch/axis_io_strategy_base.hpp>
#include <axes/layout/axis_05_memory_layout_strategy_base.hpp>
#include <axes/lookup/axis_03a_search_algo_base.hpp>
#include <axes/mapping/axis_03m_mapping_base.hpp>
#include <axes/migration_policy/axis_migration_strategy_base.hpp>
#include <axes/node/axis_04_node_type_strategy_base.hpp>
#include <axes/path_compression/axis_02_path_compression_strategy_base.hpp>
#include <axes/persistence_target/axis_persistence_target_strategy_base.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_strategy_base.hpp>
#include <axes/serialization_axis/axis_10_serialization_strategy_base.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_strategy_base.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_base.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_strategy_base.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace tp = ::comdare::cache_engine::topics;
namespace ms = ::comdare::cache_engine::measurement;

int  g_fail = 0;
void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

/// Der Derived-Platzhalter der Basis-Instanziierungen. error_classes() ist ein statischer Member der BASIS
/// und beruehrt Derived nicht -- der Typ darf deshalb unvollstaendig bleiben. Genau das ist der Punkt: die
/// Deklaration wird OHNE eine einzige echte Achsen-Variante nachgewiesen, die TU bleibt schlank und
/// haengt an keiner Registry.
struct NieDefiniert;

/// (C) Negativ-Proben: der eine Typ hat gar keinen Fehlerraum, der andere einen leeren.
struct OhneFehlerraum {};
struct MitLeeremFehlerraum {
    [[nodiscard]] static constexpr auto error_classes() noexcept { return std::array<std::string_view, 0>{}; }
};

/// Ein Fehlerraum-Satz enthaelt ein bestimmtes Etikett?
template <class Set>
[[nodiscard]] constexpr bool enthaelt(Set const& s, std::string_view label) noexcept {
    for (std::string_view const e : s)
        if (e == label) return true;
    return false;
}

} // namespace

int main() {
    std::cout << "== (A) EINE WAHRHEITSQUELLE: topics-Etiketten == measurement::sample_status_label ==\n";
    {
        // Ohne diese drei Zeilen waere der topics-Header eine zweite Wahrheit ueber dieselben Zeichenfolgen.
        static_assert(tp::kOrganErrMessFehler == ms::sample_status_label(ms::SampleStatus::Failed));
        static_assert(tp::kOrganErrNichtAnwendbar == ms::sample_status_label(ms::SampleStatus::NotApplicable));
        static_assert(tp::kOrganErrQuelleNichtVerfuegbar ==
                      ms::sample_status_label(ms::SampleStatus::SourceUnavailable));
        check("mess_fehler            == sample_status_label(Failed)",
              tp::kOrganErrMessFehler == ms::sample_status_label(ms::SampleStatus::Failed));
        check("nicht_anwendbar        == sample_status_label(NotApplicable)",
              tp::kOrganErrNichtAnwendbar == ms::sample_status_label(ms::SampleStatus::NotApplicable));
        check("quelle_nicht_verfuegbar == sample_status_label(SourceUnavailable)",
              tp::kOrganErrQuelleNichtVerfuegbar == ms::sample_status_label(ms::SampleStatus::SourceUnavailable));
        std::cout << "  [INFO] Etiketten: '" << tp::kOrganErrMessFehler << "' / '" << tp::kOrganErrNichtAnwendbar
                  << "' / '" << tp::kOrganErrQuelleNichtVerfuegbar << "'\n";
    }

    std::cout << "== (B) VOLLSTAENDIGKEIT: alle 18 Organ-Haupt-Achsen-Basen tragen einen nicht-leeren Satz ==\n";
    {
        // Einzeln aufgezaehlt statt ueber eine Liste gefaltet: eine vergessene Achse faellt hier auf, und
        // die Zeilen sind zugleich die Karte, welche Achse welchen Satz fuehrt.
        std::size_t n      = 0;
        auto        zaehle = [&n](char const* achse, auto const& satz, std::size_t soll) {
            bool const ok = (satz.size() == soll) && enthaelt(satz, tp::kOrganErrMessFehler);
            std::cout << (ok ? "  [OK]  " : "  [ERR] ") << achse << " -> " << satz.size() << " Klasse(n)\n";
            if (!ok) ++g_fail;
            ++n;
        };
        using namespace ::comdare::cache_engine;
        zaehle("search_algo", lookup::SearchAlgoBase<NieDefiniert>::error_classes(), 1);
        zaehle("cache_traversal", cache_traversal::CacheTraversalBase<NieDefiniert>::error_classes(), 1);
        zaehle("mapping", mapping::MappingBase<NieDefiniert>::error_classes(), 1);
        zaehle("path_compression", path_compression::PathCompressionStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("node_type", node::NodeTypeStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("memory_layout", layout::MemoryLayoutStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("allocator", alloc::AllocatorStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("prefetch", prefetch_axis::PrefetchStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("serialization", serialization_axis::SerializationStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("value_handle", value_handle_axis::ValueHandleStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("index_organization", index_organization::IndexOrganizationStrategyBase<NieDefiniert>::error_classes(),
               1);
        zaehle("filter", filter_axis::FilterStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("queuing_q1", queuing::axis_q1_queuing::BufferStrategyBase<NieDefiniert>::error_classes(), 1);
        zaehle("queuing_q2", queuing::axis_q2_queuing::FlushPolicyStrategyBase<NieDefiniert>::error_classes(), 1);
        // Die vier Achsen mit strukturellem n/a (je am Qualitaets-Parameter-Katalog belegt).
        zaehle("concurrency [T8 1-Thread]", concurrency_axis::ConcurrencyStrategyBase<NieDefiniert>::error_classes(),
               2);
        zaehle("io_dispatch [T12 Simulation]", io_dispatch::IoStrategyBase<NieDefiniert>::error_classes(), 2);
        zaehle("migration_policy [T13 decide-only]",
               migration_policy::MigrationStrategyBase<NieDefiniert>::error_classes(), 2);
        zaehle("persistence_target [T17 honest-0]",
               persistence_target::PersistenceTargetStrategyBase<NieDefiniert>::error_classes(), 2);
        check("GENAU 18 Organ-Haupt-Achsen-Basen aufgezaehlt", n == 18u);
        std::cout << "  [INFO] aufgezaehlt = " << n << "\n";
    }

    std::cout << "== (C) NEGATIV-PROBE: die Wache ist keine Tautologie ==\n";
    {
        static_assert(!tp::OrganAxisMitFehlerklassen<OhneFehlerraum>);
        static_assert(!tp::OrganAxisMitFehlerklassen<MitLeeremFehlerraum>);
        static_assert(
            tp::OrganAxisMitFehlerklassen<::comdare::cache_engine::io_dispatch::IoStrategyBase<NieDefiniert>>);
        check("Typ OHNE error_classes() erfuellt das Concept NICHT", !tp::OrganAxisMitFehlerklassen<OhneFehlerraum>);
        check("Typ mit LEEREM Fehlerraum erfuellt das Concept NICHT",
              !tp::OrganAxisMitFehlerklassen<MitLeeremFehlerraum>);
        check("eine reale Achsen-Basis erfuellt es",
              tp::OrganAxisMitFehlerklassen<::comdare::cache_engine::io_dispatch::IoStrategyBase<NieDefiniert>>);
    }

    std::cout << "== (D) DOMAENEN-TRENNUNG: kein Ok, keine D1-Etiketten im Fehlerraum ==\n";
    {
        // "mess_ok" ist der Normalfall und gehoert NICHT in den Fehlerraum -- sonst zaehlte jede Achse eine
        // Klasse, die nie ein Fehler ist, und die Nicht-Leerheits-Wache waere wertlos.
        check("Boden enthaelt kein 'mess_ok'",
              !enthaelt(tp::kOrganAxisErrorFloor, ms::sample_status_label(ms::SampleStatus::Ok)));
        check("Boden+n/a enthaelt kein 'mess_ok'",
              !enthaelt(tp::kOrganAxisErrorFloorPlusNa, ms::sample_status_label(ms::SampleStatus::Ok)));
        // D1 ist die Planer-/Compile-Zeit-Domaene. Ein D1-Etikett im D2-Fehlerraum waere genau die
        // Domaenen-Vermengung, gegen die axis_error.hpp gebaut wurde (RF-3-Lehre).
        bool d1_frei = true;
        for (std::size_t i = 0; i < ms::kCompilerCompilerErrorClassCount; ++i) {
            auto const d1 = ms::error_class_label(static_cast<ms::CompilerCompilerErrorClass>(i));
            if (enthaelt(tp::kOrganAxisErrorFloorPlusNa, d1)) d1_frei = false;
        }
        check("kein D1-Etikett (alle 7 geprueft) steht im D2-Fehlerraum", d1_frei);
        std::cout << "  [INFO] D1-Klassen geprueft = " << ms::kCompilerCompilerErrorClassCount << "\n";
    }

    std::cout << (g_fail == 0 ? "\nALLE PROBEN GRUEN\n" : "\nFEHLER\n");
    return g_fail == 0 ? 0 : 1;
}
