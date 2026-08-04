// A8-S5 Familie 03_placement (mapping + alloc + value_handle + index_organization + migration_policy)
// -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp (Pilot 04_execution); DIESE TU
// besteht aus den Typ-Listen der Familie, dem literalen Lauf-Ausweis -- und, weil die Familie Form B nutzt,
// zusaetzlich aus dem REALEN VERDRAHTUNGS-BELEG (unten Block (4)/(5)).
//
// WAS GEPINNT WIRD: kein Organ dieser Familie fuehrt Speicher an der Allokator-Achse vorbei. Die Pruefung
// laeuft auf TYP-EBENE gegen die REALEN Kompositions-Typen -- die Listen kommen aus den Achsen-Registries
// (AllStrategies / AllHandles / AllOrganizations / AllMigrations), NICHT aus einer handgepflegten Aufzaehlung.
// Waechst eine Achse um eine Strategie, waechst die Wache mit.
//
// GEPRUEFTE EBENEN:
//   (1) die STRATEGIE-Typen selbst        -- mapping/value_handle/index_org/migration aus den Registries
//   (2) die ORGAN-HUELLEN der Komposition -- ObservableValueHandle<S>/ObservableIndexOrg<S>/ObservableMigration<S>
//                                            (genau die Member-Typen vh_organ_/idx_organ_/mig_organ_ des
//                                            ABI-Adapters; mapping haelt der Adapter NACKT -> schon in (1))
//   (3) die inneren SLOT-BACKINGS         -- real_slot_t<S> je value_handle-Strategie (der Scrub-Gegenstand)
//   (4) LAUFZEIT-Beleg der Verdrahtung    -- Form B behauptet nicht nur, sie ist am Objekt nachgewiesen
//   (5) LAUFZEIT-Beleg des R1-Mementos    -- die Kopie rebindet, sie erbt keinen fremden/Default-Allokator
//
// WARUM (4)/(5) HIER STEHEN MUESSEN (Form-B-Grenze, s5_family_alloc_conformance.hpp:31): das Praedikat prueft,
// dass ein DEKLARIERTER allocator_type das Achsen-Concept erfuellt -- nicht, dass die Member-Allokation real
// darueber laeuft. Familie 04 kam mit Form A (heap-frei) aus und brauchte den Beleg nicht; Familie 03 ist die
// erste, die Form B WIRKLICH benutzt. Ohne (4)/(5) waere die gruene Wache genau die Sorte Erfolgsmeldung, vor
// der die Form-B-Grenze warnt.
//
// SONDERROLLE DER ALLOKATOR-ACHSE (axes/alloc/): sie ist der VERSORGER, nicht ein Verbraucher. "Bezieht ihren
// Speicher ueber die Allokator-Achse" auf sie selbst anzuwenden waere zirkulaer -- und faktisch falsch: der
// PoolResourceAllocator BESITZT bestimmungsgemaess eine eigene Speicher-Ressource (std::pmr::unsynchronized_
// pool_resource, axis_06_allocator_pool_resource.hpp:205). Ihr Konformitaets-Kriterium ist deshalb ein anderes
// und wird hier als solches geprueft: JEDE Strategie der Achse erfuellt AllocatorStrategy, ist also ein
// gueltiger Versorger (Block (0)). Der Familien-grep hatte in axes/alloc/ ohnehin nur EINEN Treffer, und der
// ist die Adapter-DOKUMENTATION selbst (axis_06_allocator_strategy_base.hpp:152) -- kein Scrub-Objekt.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <axes/alloc/axis_06_allocator_registry.hpp>
#include <axes/index_organization/axis_01_index_organization_observable.hpp>
#include <axes/index_organization/axis_01_index_organization_registry.hpp>
#include <axes/mapping/axis_03m_mapping_registry.hpp>
#include <axes/migration_policy/axis_migration_observable.hpp>
#include <axes/migration_policy/axis_migration_registry.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_observable.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_real_slot.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_registry.hpp>

// Form-(B)-REFERENZ am realen Repo-Typ (Pilot-Vorbild): der B-Baum-Knoten-Pool fuehrt seinen unbounded
// Knoten-Speicher ueber die Allokator-Achse (StdAllocatorAdapter-Rebind + COW-Memento). Er gehoert NICHT zu
// dieser Familie und wird hier nur GELESEN -- er ist der Anker, an dem der Scrub dieser Familie Mass nimmt.
#include <axes/lookup/composable/btree_node_pool_store.hpp>

#include <boost/mp11.hpp>

#include <cstdio>
#include <string_view>

namespace mp   = boost::mp11;
namespace s5   = ::comdare::cache_engine::tests::s5;
namespace mapx = ::comdare::cache_engine::mapping;
namespace vhx  = ::comdare::cache_engine::value_handle_axis;
namespace idxx = ::comdare::cache_engine::index_organization;
namespace migx = ::comdare::cache_engine::migration_policy;
namespace alx  = ::comdare::cache_engine::alloc;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

// -- (1) Strategie-Typen der Familie, direkt aus den Achsen-Registries ----------------------------
using Family03Strategies =
    mp::mp_append<mapx::AllStrategies, vhx::AllHandles, idxx::AllOrganizations, migx::AllMigrations>;

// -- (2) Organ-Huellen, wie der ABI-Adapter sie als Member haelt -----------------------------------
//    mapping haelt der Adapter NACKT (map_organ_ = Composition::mapping) -> bereits in (1) enthalten.
template <class S>
using ValueHandleOrganOf = vhx::ObservableValueHandle<S>;
template <class S>
using IndexOrgOrganOf = idxx::ObservableIndexOrg<S>;
template <class S>
using MigrationOrganOf = migx::ObservableMigration<S>;
using Family03Organs   = mp::mp_append<mp::mp_transform<ValueHandleOrganOf, vhx::AllHandles>,
                                       mp::mp_transform<IndexOrgOrganOf, idxx::AllOrganizations>,
                                       mp::mp_transform<MigrationOrganOf, migx::AllMigrations>>;

// -- (3) Innere Slot-Backings (der Scrub-Gegenstand dieser Scheibe) --------------------------------
template <class S>
using RealSlotOf          = vhx::real_slot_t<S>;
using Family03SlotBacking = mp::mp_transform<RealSlotOf, vhx::AllHandles>;

// -- (0) Die Versorger-Achse: anderes Kriterium (s. Kopf-Kommentar) --------------------------------
template <class A>
using is_valid_supplier = mp::mp_bool<::comdare::cache_engine::alloc::concepts::AllocatorStrategy<A>>;

// -- Anti-Leerlauf: eine leere Liste macht JEDE Alles-Aussage wahr ---------------------------------
static_assert(mp::mp_size<Family03Strategies>::value > 0,
              "S5-03: die Familien-Strategie-Liste ist LEER -- eine leere Liste macht jede Alles-Aussage wahr "
              "und die Wache wertlos (Registry nicht eingebunden?).");
static_assert(mp::mp_size<alx::AllVendors>::value > 0,
              "S5-03: die Allokator-Registry ist LEER -- dann pruefte Block (0) nichts.");
static_assert(mp::mp_size<Family03SlotBacking>::value == mp::mp_size<vhx::AllHandles>::value,
              "S5-03: zu jeder value_handle-Strategie gehoert genau ein Slot-Backing -- die Backing-Liste ist "
              "aus der Strategie-Liste abgeleitet, nicht handgepflegt.");
static_assert(mp::mp_size<Family03Organs>::value == mp::mp_size<vhx::AllHandles>::value +
                                                        mp::mp_size<idxx::AllOrganizations>::value +
                                                        mp::mp_size<migx::AllMigrations>::value,
              "S5-03: zu jeder gehuellten Familien-Strategie gehoert genau eine Organ-Huelle.");

// -- Die Wache selbst (Typ-Ebene) -----------------------------------------------------------------
static_assert(s5::family_alloc_conform_v<Family03Strategies>,
              "S5-03: eine Strategie der Familie 03_placement fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family03Organs>,
              "S5-03: eine Organ-Huelle der Familie 03_placement fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family03SlotBacking>,
              "S5-03: ein value_handle-Slot-Backing fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(mp::mp_all_of<alx::AllVendors, is_valid_supplier>::value,
              "S5-03: eine Strategie der Allokator-Achse erfuellt AllocatorStrategy nicht mehr -- dann waere sie "
              "kein gueltiger Versorger, und der Form-B-Zweig der ganzen Scheibe stuende auf Sand.");

// Die beiden Versorger, die der Scrub dieser Scheibe wirklich verdrahtet hat (EINE Quelle je Achse).
static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<mapx::mapping_slot_allocator_t>,
              "S5-03: der in axis_03m_mapping_base.hpp benannte Versorger erfuellt das Achsen-Concept nicht.");
static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<vhx::value_handle_slot_allocator_t>,
              "S5-03: der in axis_14_value_handle_real_slot.hpp benannte Versorger erfuellt das Concept nicht.");

// Form (B) traegt an echtem Bestand (Pilot-Vorbild, Referenz-Anker ausserhalb der Familie).
static_assert(s5::AxisAllocatorBoundOrgan<lkc::BTreeNodePoolStore<>>,
              "S5-Gate-Muster: der Form-(B)-Zweig findet am realen Referenz-Muster keinen allocator_type mehr -- "
              "dann pruefte die Wache nur noch Heap-Freiheit und liesse den Adapter-Weg ungedeckt.");
// ... und er traegt jetzt auch INNERHALB der Familie (das ist das Neue dieser Scheibe gegenueber dem Pilot).
static_assert(s5::AxisAllocatorBoundOrgan<mapx::DirectPlacement> && s5::AxisAllocatorBoundOrgan<mapx::PoolRelative>,
              "S5-03: die mapping-Varianten haben ihren Form-B-Ausweis (allocator_type) verloren.");
static_assert(s5::AxisAllocatorBoundOrgan<vhx::PoolValueSlot<false>> &&
                  s5::AxisAllocatorBoundOrgan<vhx::PoolValueSlot<true>> &&
                  s5::AxisAllocatorBoundOrgan<vhx::ChainValueSlot>,
              "S5-03: ein value_handle-Slot-Backing hat seinen Form-B-Ausweis (allocator_type) verloren.");
// EmptyRealSlot (Inline, M3-Pin) bleibt die STAERKERE Form A -- der Scrub hat den Nullpunkt nicht beschwert.
static_assert(s5::HeapFreeOrgan<vhx::EmptyRealSlot>,
              "S5-03: das Inline-Backing ist nicht mehr heap-frei -- der M3-Pin waere nicht mehr messneutral.");

// -- Literaler Lauf-Ausweis (kein Erfolgs-Haken ohne Ausgabe) -------------------------------------
int g_fail = 0;

void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

template <class T>
void report_organ(char const* stufe, std::string_view label) {
    bool const ok = s5::FamilyAllocConform<T>;
    std::printf("  [%s] %-8s %-46.*s Form: %s\n", ok ? " ok " : "FAIL", stufe, static_cast<int>(label.size()),
                label.data(), s5::family_alloc_form<T>());
    if (!ok) ++g_fail;
}

template <class List>
void report_list(char const* stufe) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using T = typename decltype(id)::type;
        report_organ<T>(stufe, T::name());
    });
}

/// Die Slot-Backings tragen selbst kein name() (sie sind Struktur, keine Strategie) -- beschriftet wird
/// deshalb ueber die Strategie, die das Backing compile-zeit-selektiert hat (real_slot_selector).
void report_slot_backings() {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, vhx::AllHandles>>([&](auto id) {
        using S = typename decltype(id)::type;
        report_organ<vhx::real_slot_t<S>>("SLOT", S::name());
    });
}

/// (4) VERDRAHTUNGS-BELEG: ein frisches Organ hat 0 Achsen-Allokationen; nach echter Nutzung > 0.
/// Damit ist belegt, dass die Member-Allokation WIRKLICH ueber die Versorger-Achse laeuft und nicht
/// ueber einen Default-Allokator neben einer huebschen using-Zeile (Form-B-Grenze).
template <class Mapping>
void probe_mapping_wiring(char const* label) {
    Mapping    m{};
    auto const before = m.mapping_allocator_statistics();
    for (std::uint32_t s = 0; s < 256; ++s)
        m.register_slot(static_cast<typename Mapping::slot_index_type>(s),
                        static_cast<typename Mapping::offset_type>(s) * 64u);
    auto const after = m.mapping_allocator_statistics();
    std::printf("     %-16s alloc_cnt %llu -> %llu   bytes_in_use %llu   mapped %llu\n", label,
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use),
                static_cast<unsigned long long>(m.mapped_count()));
    tr("(4) frisches mapping-Organ hat noch KEINE Achsen-Allokation (ehrlicher Nullpunkt)",
       before.allocation_count == 0);
    tr("(4) nach 256 register_slot laeuft der Tabellen-Speicher REAL ueber die Achse (alloc_cnt > 0)",
       after.allocation_count > 0);
    tr("(4) die Achse haelt den Speicher auch (bytes_in_use > 0)", after.total_bytes_in_use > 0);
    tr("(4) und die Tabelle traegt die Daten wirklich (mapped_count == 256)", m.mapped_count() == 256u);
}

/// (5) MEMENTO-BELEG (der eigentliche Grund, warum diese Familie eine eigene Probe braucht):
/// Die R1-Kopie muss auf das EIGENE allocator_ rebinden. Beweis ohne Zeiger-Introspektion, rein aus
/// den Achsen-Zaehlern:
///   (a) die Kopie allein darf die Zaehler der QUELLE nicht bewegen -> sie hat NICHT aus der Quelle alloziert,
///   (b) eine Mutation der KOPIE bewegt die Zaehler der Kopie, aber weiterhin nicht die der Quelle
///       -> die Vektoren der Kopie haengen am Allokator der Kopie (und nicht an einem Default-Allokator,
///          sonst blieben auch die Zaehler der Kopie stehen),
///   (c) und der Inhalt ist bit-exakt uebernommen (operator==) -- der Memento-Vertrag selbst.
template <class Backing>
void probe_memento_rebind(char const* label) {
    Backing src{};
    for (std::uint64_t k = 0; k < 512; ++k) src.store_value(k, k * 7u + 1u);
    auto const src_before = src.slot_allocator_statistics();

    Backing    cp{src}; // R1-Memento-Kopie (genau das tut saved_vh_.emplace(vh_organ_))
    auto const src_after_copy = src.slot_allocator_statistics();
    auto const cp_after_copy  = cp.slot_allocator_statistics();

    for (std::uint64_t k = 512; k < 1024; ++k) cp.store_value(k, k * 7u + 1u); // Mutation NUR der Kopie
    auto const src_after_mut = src.slot_allocator_statistics();
    auto const cp_after_mut  = cp.slot_allocator_statistics();

    std::printf("     %-18s src alloc_cnt %llu -> %llu (nach Kopie) -> %llu (nach Mutation der Kopie)\n", label,
                static_cast<unsigned long long>(src_before.allocation_count),
                static_cast<unsigned long long>(src_after_copy.allocation_count),
                static_cast<unsigned long long>(src_after_mut.allocation_count));
    std::printf("     %-18s cp  alloc_cnt %llu (nach Kopie) -> %llu (nach Mutation)   slots src/cp %llu/%llu\n", label,
                static_cast<unsigned long long>(cp_after_copy.allocation_count),
                static_cast<unsigned long long>(cp_after_mut.allocation_count),
                static_cast<unsigned long long>(src.slot_count()), static_cast<unsigned long long>(cp.slot_count()));

    tr("(5a) die Kopie hat NICHT aus dem Allokator der Quelle alloziert (Quell-Zaehler unveraendert)",
       src_after_copy.allocation_count == src_before.allocation_count);
    tr("(5b) Mutation der Kopie bewegt den Allokator der KOPIE (Kopie-Zaehler steigt)",
       cp_after_mut.allocation_count > cp_after_copy.allocation_count);
    tr("(5b) Mutation der Kopie laesst die Quelle unberuehrt (Quell-Zaehler weiterhin unveraendert)",
       src_after_mut.allocation_count == src_before.allocation_count);
    tr("(5b) und die Quelle behaelt ihren Inhalt (512 Slots, keine geteilte Struktur)", src.slot_count() == 512u);

    Backing fresh{};
    for (std::uint64_t k = 0; k < 512; ++k) fresh.store_value(k, k * 7u + 1u);
    tr("(5c) Memento-Vertrag: gleicher Inhalt vergleicht bit-exakt gleich (operator==)", fresh == src);
    Backing assigned{};
    assigned = src; // der Rueckspiel-Pfad (vh_organ_ = *saved_vh_)
    tr("(5c) Rueckspiel per copy-assign ist inhaltsgleich (operator==)", assigned == src);
    tr("(5c) und der Rueckspiel-Empfaenger hat aus SEINEM Allokator alloziert",
       assigned.slot_allocator_statistics().allocation_count > 0);
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 03_placement -- Allokator-Achsen-Konformitaet ==\n");
    std::printf("   Achsen: mapping (T2) . allocator (T6, Versorger) . value_handle (T10) . index_organization (T11)"
                " . migration_policy (T13)\n");

    std::printf("-- (1) Strategie-Typen aus den Achsen-Registries --\n");
    report_list<Family03Strategies>("STRAT");
    std::printf("-- (2) Organ-Huellen (Member-Typen des ABI-Adapters) --\n");
    report_list<Family03Organs>("ORGAN");
    std::printf("-- (3) value_handle-Slot-Backings, beschriftet mit ihrer Strategie (Scrub-Gegenstand) --\n");
    report_slot_backings();

    std::printf("-- (4) LAUFZEIT: reale Verdrahtung der Form-B-Organe (mapping) --\n");
    probe_mapping_wiring<mapx::DirectPlacement>("direct_placement");
    probe_mapping_wiring<mapx::PoolRelative>("pool_relative");

    std::printf("-- (5) LAUFZEIT: R1-Memento rebindet auf den EIGENEN Allokator (value_handle) --\n");
    probe_memento_rebind<vhx::PoolValueSlot<false>>("PoolValueSlot<f>");
    probe_memento_rebind<vhx::PoolValueSlot<true>>("PoolValueSlot<t>");
    probe_memento_rebind<vhx::ChainValueSlot>("ChainValueSlot");

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Strategien: %zu   Organ-Huellen: %zu   Slot-Backings: %zu   Allokator-Versorger: %zu\n",
                static_cast<std::size_t>(mp::mp_size<Family03Strategies>::value),
                static_cast<std::size_t>(mp::mp_size<Family03Organs>::value),
                static_cast<std::size_t>(mp::mp_size<Family03SlotBacking>::value),
                static_cast<std::size_t>(mp::mp_size<alx::AllVendors>::value));
    std::printf("  index_organization + migration_policy: Familien-grep 0 Code-Treffer -- kein Scrub noetig,\n");
    std::printf("  aber von (1)/(2) mitgepinnt, damit ein kuenftiger Container dort sofort auffaellt.\n");

    std::printf("== test_s5_03_placement_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
