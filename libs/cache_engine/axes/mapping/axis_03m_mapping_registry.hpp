#pragma once
// V41.F.6.1 axis_03m_mapping Registry (W6-Pattern)

#include <axes/mapping/axis_03m_mapping_flags.hpp>

#include "axis_03m_mapping_direct_placement.hpp"
#include "axis_03m_mapping_pool_relative.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::mapping {

namespace mp = boost::mp11;

using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    DirectPlacement, PoolRelative
    // -- SCOPE-LIMITATION (T2 mapping, Diplomarbeit) -----------------------------------------------
    // mapping ist eine KOMPOSITIONS-Achse: mp_size(EnabledStrategies) geht als Faktor in
    // all_axes_matrix_count() == PROD enabled_count == 137.594.142.720.000 (Gate-1; K-7-Rename,
    // registry_to_axis_levels.hpp:114) UND in den mixed-radix StaticBinaryView
    // (golden_fullpilot_320_binary_ids.txt pinnt mapping=direct_placement). Ein HINZUFUEGEN von
    // MP03 PermutationIndexed / MP04 HashedOffset als Registry-WERT hebt mp_size 2->4, verdoppelt
    // die eingefrorene Gate-1-Kardinalitaet und schneidet die golden-320-Radix neu -> BEWUSSTER
    // Gate-1/golden-Neuschnitt, NUR mit expliziter User-GO (sonst TABU-Bruch). Bis dahin misst T2
    // die ENGERE Groesse "Indirektions-CM" (Adress-Aufloesungs-Tiefe je resolve: Direct=1,
    // PoolRelative=2, via MappingStatistics::total_indirection_steps -> POD-Slot axis_stats[2][6])
    // ueber die 2 Basis-Werte. MP03/MP04 bleiben Roadmap.
    >;

template <typename M>
using is_enabled = mp::mp_bool<M::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0, "axis_03m_mapping: at least one strategy must be enabled");

} // namespace comdare::cache_engine::mapping
