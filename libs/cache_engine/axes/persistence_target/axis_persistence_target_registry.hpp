#pragma once
// STRUKT-R ORG-18 axis_persistence_target Zentrale Registry
// Vorbild 1:1 axes/io_dispatch/axis_io_registry.hpp.
//
// AllTargets = deklarierter Vollausbau der Achse; EnabledTargets = das, was der Bau tatsaechlich
// permutiert (mp_filter ueber T::enabled, gespeist aus CMake option() -> flags-Header).
// Owner-Entscheid Q-1 = FALL B: mit disk_writeback=OFF ist EnabledTargets 1-elementig.
//
// ACHTUNG Falle (Bauplan §4 Q-1, belegt): mp_take_c<L,2> auf einer 1-elementigen Liste ist ill-formed
// (cmake/third_party/boost_mp11/include/boost/mp11/algorithm.hpp:430,447) und CatalogAxes hat KEINEN
// min()-Schutz. Wer diese Achse in source_catalog.hpp verdrahtet, MUSS bei disk_writeback=OFF K17=1
// schreiben -- ein uniformes <2,...,2> bricht bereits im TU.

#include <axes/persistence_target/axis_persistence_target_flags.hpp>

#include "axis_persistence_target_memory_only.hpp"
#include "axis_persistence_target_disk_writeback.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::persistence_target {

namespace mp = boost::mp11;

using AllTargets = mp::mp_list<MemoryOnlyTarget, DiskWritebackTarget>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledTargets = mp::mp_filter<is_enabled, AllTargets>;

static_assert(mp::mp_size<EnabledTargets>::value > 0,
              "Axis persistence_target: mindestens ein Baustein muss enabled sein");

// Der golden_wired-Wert der Achse ist MemoryOnlyTarget (Session-Doc §5: "memory_only (Default,
// golden_wired, Durchreich-Semantik)"). Er darf daher NIE per option() abgeschaltet werden -- sonst
// verliert jede bestehende Komposition ihren Durchreich-Wert.
static_assert(MemoryOnlyTarget::enabled,
              "COMDARE_AXIS_PERSISTENCE_ENABLE_MEMORY_ONLY=OFF ist unzulaessig: memory_only ist der "
              "golden_wired-Durchreich-Wert jeder bestehenden Komposition (Session-Doc §5).");

} // namespace comdare::cache_engine::persistence_target
