#pragma once
// V41.F.6.1 axis_03m_mapping CRTP-Basis (2026-05-26)

#include "concepts/axis_03m_mapping_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>               // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum

#include <axes/alloc/axis_06_allocator_exgen.hpp> // A8-S5-03: Versorger-Achse der Slot-Tabellen (s.u.)

#include <type_traits>

namespace comdare::cache_engine::mapping {

/// A8-S5 Familie 03_placement (Dossier 20260803-a8_f2 Abschn. 3.4, Schnitt-Regel): die Slot-Tabelle der
/// 03m-Varianten bezieht ihren Speicher NICHT mehr ueber den Default-Allokator, sondern ueber das
/// Allokator-ACHSEN-Interface (axis_06). DIESE Zeile ist die EINE Stelle, die den Versorger benennt --
/// beide Varianten (DirectPlacement/PoolRelative) und der Form-B-Ausweis der S5-Gate-Wache lesen sie,
/// damit kein zweiter Name driften kann.
///
/// WARUM ExgenAllocator: das ist exakt der Versorger, den die bereits konformen Pool-Stores des Repos
/// als Achsen-Default fuehren (btree_node_pool_store.hpp:56, tree_node_pool_store.hpp, surf_fst_map_pool_store.hpp).
/// Die 03m-Varianten sind is_thread_safe()==false, also single-threaded -- die Sub-Achse AA4 des Exgen
/// (Single-Threaded Specialized) passt zum Organ. Bei abgeschaltetem Vendor-Flag faellt die Strategie
/// intern auf portable_aligned_alloc zurueck: derselbe libc-Heap wie vorher, aber ueber das Achsen-Interface.
///
/// ABHAENGIGKEITSRICHTUNG (Dossier 3.3): mapping -> alloc. Der Allokator ist die UNTERSTE Versorger-Achse;
/// axes/alloc/ zieht keinen 03m-Header -> kein Zyklus.
///
/// OFFEN (bewusst NICHT hier entschieden): ob das Organ statt des Achsen-Defaults den Allokator DER
/// KOMPOSITION bekommt (Kompositions-Rebind). Das verlangt einen Template-Kopf + Namens-Alias an jeder
/// Nicht-Template-Variante und ist als eigener Entscheid der S5-Scheibe 01c vorgemerkt (S5-Planung,
/// LEDGER-Nachtrag 04.08. abend-4: "01c-Mechanik Default-Arg-Template+Alias VERSUS Kompositions-Rebind").
using mapping_slot_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

/**
 * @brief MappingBase — CRTP-Basis fuer 03m-Wrapper
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
template <typename Derived>
class MappingBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden: die Schluessel-Abbildung ist auf jedem Pfad wirksam.
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloor;
    }

protected:
    MappingBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");

        // FK-5 (E-24 C9): der Fehlerraum-Zwilling der Versions-Wache darueber -- existiert die
        // Deklaration, und ist sie nicht leer? Beides bricht compile-hart MIT dem Typ-Namen.
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<Derived>();
        static_assert(concepts::MappingVariant<Derived>,
                      "Pflicht: Derived muss MappingVariant erfuellen "
                      "(register_slot/resolve_offset/reverse_lookup/mapped_count/clear)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + "
                      "is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

} // namespace comdare::cache_engine::mapping
