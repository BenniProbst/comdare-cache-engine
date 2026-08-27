// tests/unit/test_e10_organ_meta_meta_anlage.cpp -- E-10/ORG-19 SCHRITT-1-GATE (k1-KOEDER, T-1 rot-zuerst;
// Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19.md Schritt 1 + VERIFY-Fixes FIX-1/FIX-7(b), 26.08.2026).
//
// GEGENSTAND: die ANLAGE der ersten Organ-Meta-Meta-Achse ORG-19-IO (disk_io) -- Typ + Concept +
// Vollmengen-Liste abi::OrganMetaMetas + Comp-Selektions-Trait ueber die 18 vorhandenen Slot-Aliase
// (D-1: KEIN 19. Comp-Member, prt-art-Vertrag bleibt 18 Aliase) + Carrier-Bindung als Single-Source
// (kCarrierValues[0] == DiskWritebackTarget::name()) + die FIX-1-Byteform (UNGEWRAPPTE Entry-Form fuer
// die 1d-Tabelle; der EINE Renderer wrappt genau EINMAL).
//
// T-1/K13: VOR Schritt 1a/1c uebersetzt diese TU NICHT (die beiden neuen Header fehlen) = der
// Koeder-Biss; NACH Schritt 1 ist sie gruen. Der Zwischenstand 1b (OrganMetaMetas = Vollmenge
// typ-global) ist hier ABSICHTLICH nicht gepinnt ueber organ_stamp_line -- die Stempel-JE-COMP-Pins
// leben in k9 (Schritt 4); dieser Test bleibt im S1-S4-Korridor gruen (F-V1a-Auflage).

#include <gtest/gtest.h>

#include <organ_axes/organ_meta_meta/axis_disk_io_organ_meta_meta.hpp> // 1a (k1: vor dem Bau FEHLT sie = rot)
#include <cache_engine/abi/organ_meta_meta_selection.hpp>              // 1c (k1: vor dem Bau FEHLT sie = rot)

#include <cache_engine/abi/anatomy_version_stamp.hpp>      // abi::OrganMetaMetas (Vollmenge, 1b)
#include <cache_engine/measurement/meta_meta_identity.hpp> // subsumes_v-Kreuzprobe (Halbordnungs-Quelle)
#include <compositions/prt_art_reference.hpp>              // PrtArtComposition (18 Slot-Aliase, MemoryOnly)
#include <organ_axes/persistence_target/axis_persistence_target_disk_writeback.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_memory_only.hpp>
#include <topics/organ_meta_meta_axis.hpp>

#include <string>
#include <string_view>

namespace {

namespace abi  = ::comdare::cache_engine::abi;
namespace cet  = ::comdare::cache_engine::topics;
namespace meas = ::comdare::cache_engine::measurement;
namespace omm  = ::comdare::cache_engine::organ_meta_meta;
namespace pt   = ::comdare::cache_engine::persistence_target;
using ::comdare::cache_engine::compositions::PrtArtComposition;

// -- (1) TYP + CONCEPT + REALM-DISKRIMINATOR (Muster ProofOrganMetaMeta, striktheit-Guard) ---------
static_assert(cet::OrganMetaMetaAxisConcept<omm::DiskIoOrganMetaMeta>);
static_assert(omm::DiskIoOrganMetaMeta::axis_kind() == cet::AxisKind::organ_meta_meta);
static_assert(omm::DiskIoOrganMetaMeta::axis_kind() != cet::AxisKind::organ,
              "eine Organ-Meta-Meta ist KEINE Organ-HAUPT-Achse (18+1: kCompositionAxisNames bleibt 18)");
static_assert(omm::DiskIoOrganMetaMeta::do_axis_label() == std::string_view{"disk_io"});
static_assert(omm::DiskIoOrganMetaMeta::sub_axis_label() == std::string_view{"disk_io"},
              "Konvention: die Familie spannt ihre gleichnamige Unter-Achse (external_utils-Vorbild)");
static_assert(omm::DiskIoOrganMetaMeta::axis_code_version == std::string_view{"1.0.0.c"});

// FIX-7(b): BEIDE Versions-Wachen DIREKT asserten -- die cpu_pflicht-Wache am Renderer-Engpass ist
// flag-gated (COMDARE_VERSION_HW_FLAG_ENFORCE) und darf nicht die einzige Absicherung der 'c'-Pflicht
// sein (Flag-Grammatik v2: 'c' = CPU-Basis, dieselbe Zerlegung wie beim System-Vorbild "[simd=...]").
static_assert(meas::meta_meta_version_wohlgeformt<omm::DiskIoOrganMetaMeta>());
static_assert(meas::meta_meta_version_cpu_pflicht<omm::DiskIoOrganMetaMeta>());

// -- (2) VOLLMENGE (1b): abi::OrganMetaMetas traegt ORG-19-IO als erstes Glied ---------------------
static_assert(abi::OrganMetaMetas::size == 1,
              "Vollmenge aller bekannten Organ-Meta-Metas: heute GENAU ORG-19-IO (disk_io)");
static_assert(abi::OrganMetaMetas::contains<omm::DiskIoOrganMetaMeta>);

// -- (3) CARRIER-BINDUNG = SINGLE-SOURCE (D-1) -----------------------------------------------------
static_assert(omm::DiskIoOrganMetaMeta::kCarrierAxis == std::string_view{"persistence_target"});
static_assert(omm::DiskIoOrganMetaMeta::kCarrierValues.size() == 1);
static_assert(omm::DiskIoOrganMetaMeta::kCarrierValues[0] == pt::DiskWritebackTarget::name(),
              "Bindung == Single-Source: der Traeger-Wert ist DiskWritebackTarget::name()");
static_assert(omm::DiskIoOrganMetaMeta::kSubAxisOptions.size() == 1);
static_assert(omm::DiskIoOrganMetaMeta::kSubAxisOptions[0] == std::string_view{"staging"},
              "D-5/KN-3: heute GENAU 'staging' (DiskWritebackTarget misst nur Staging, "
              "has_device_writeback_path()==false); 'device' folgt mit dem Geraetepfad");
static_assert(omm::DiskIoOrganMetaMeta::genus_kanal_token() == std::string_view{"disk_io"},
              "D-7: Kanal-Token DEKLARIERT am Typ (Registrierung/Bau des Kanals nach Trigger, KON110-06)");

// -- (4) COMP-SELEKTIONS-TRAIT (1c): ueber die 18 Slot-Aliase, KEIN 19. Member ---------------------
// TestCompDisk = die prt_art-Slots mit persistence_target = DiskWritebackTarget (heute NICHT baubar,
// enabled=false -- der Typ existiert vollstaendig, Q-1 Fall B; genau dafuer ist die Selektion da).
struct TestCompDisk {
    using search_algo        = PrtArtComposition::search_algo;
    using cache_traversal    = PrtArtComposition::cache_traversal;
    using mapping            = PrtArtComposition::mapping;
    using path_compression   = PrtArtComposition::path_compression;
    using node_type          = PrtArtComposition::node_type;
    using memory_layout      = PrtArtComposition::memory_layout;
    using allocator          = PrtArtComposition::allocator;
    using prefetch           = PrtArtComposition::prefetch;
    using concurrency        = PrtArtComposition::concurrency;
    using serialization      = PrtArtComposition::serialization;
    using value_handle       = PrtArtComposition::value_handle;
    using index_organization = PrtArtComposition::index_organization;
    using io_dispatch        = PrtArtComposition::io_dispatch;
    using migration_policy   = PrtArtComposition::migration_policy;
    using filter             = PrtArtComposition::filter;
    using queuing_q1         = PrtArtComposition::queuing_q1;
    using queuing_q2         = PrtArtComposition::queuing_q2;
    using persistence_target = pt::DiskWritebackTarget;
};

static_assert(abi::organ_meta_metas_of_t<PrtArtComposition>::size == 0,
              "MemoryOnly-Comp: KEIN Meta-Meta-Traeger -> leerer Anhang (Flotte byte-neutral, N-1)");
static_assert(abi::organ_meta_metas_of_t<TestCompDisk>::size == 1,
              "IO-Comp: genau ORG-19-IO ueber den persistence_target-Slot");
static_assert(abi::organ_meta_metas_of_t<TestCompDisk>::contains<omm::DiskIoOrganMetaMeta>);

// -- (5) VOLLMENGEN-WACHE + DUPLIKATFREI (FIX-1 iii) + MUTANTEN-PROBE (Schritt-5-Koeder) -----------
static_assert(abi::organ_meta_meta_subsumiert<abi::OrganMetaMetas, abi::organ_meta_metas_of_t<TestCompDisk>>::value,
              "jede Comp-Selektion ist Teilmenge der Vollmenge abi::OrganMetaMetas");
static_assert(abi::organ_meta_metas_sind_duplikatfrei<abi::organ_meta_metas_of_t<TestCompDisk>>::value,
              "FIX-1 (iii): zwei Slots duerfen dasselbe Glied nicht doppelt stempeln");
// Mutanten-Probe (K13, requires-Form statt #if): eine test-lokale Meta-Meta AUSSERHALB der Vollmenge
// darf die Vollmengen-Wache NICHT passieren -- die Wache muss beissen koennen.
struct LokaleProbeMetaMeta final : cet::OrganMetaMetaAxis<LokaleProbeMetaMeta> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "probe_mm"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "probe_mm"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
};
// Die Probe ist selbst WOHLGEFORMT (Concept + FIX-7(b)-Versions-Wachen wie ORG-19-IO oben): sie faellt NUR an
// der Vollmengen-Wache -- K13: der Biss kommt aus dem richtigen Grund, nicht aus einer kaputten Probe. (S6-
// Warnungs-Review 27.08.2026: clang-22 -Wunused-const-variable an axis_code_version; die Wachen WERTEN es aus.)
static_assert(cet::OrganMetaMetaAxisConcept<LokaleProbeMetaMeta>);
static_assert(meas::meta_meta_version_wohlgeformt<LokaleProbeMetaMeta>());
static_assert(meas::meta_meta_version_cpu_pflicht<LokaleProbeMetaMeta>());
static_assert(!abi::organ_meta_meta_subsumiert<abi::OrganMetaMetas, meas::MetaMetaMembers<LokaleProbeMetaMeta>>::value,
              "Biss-Beweis: ein Glied ausserhalb der Vollmenge faellt an der Wache");
// Kreuzprobe gegen die Halbordnungs-Quelle (meta_meta_identity.hpp subsumes_v via mp_list-Sicht):
static_assert(meas::subsumes_v<abi::OrganMetaMetas::as<meas::MetaMetaSet>,
                               abi::organ_meta_metas_of_t<TestCompDisk>::as<meas::MetaMetaSet>>,
              "dieselbe Halbordnung wie subsumes_v (boost-freie Wache == mp11-Quelle)");

// -- (6) BYTEFORM (FIX-1): Tabelle traegt UNGEWRAPPT, der EINE Renderer wrappt EINMAL --------------
TEST(E10OrganMetaMetaAnlage, SuffixUndEntryFormen) {
    // MemoryOnly: leere Selektion -> leerer Anhang (kein "[]"; wrap_meta_meta_group-Leerfall).
    EXPECT_EQ(abi::organ_meta_meta_suffix_for_variant<pt::MemoryOnlyTarget>(), "");
    EXPECT_EQ(abi::organ_meta_meta_entries_for_variant<pt::MemoryOnlyTarget>(), "");
    // IO-Traeger: gewrappte Form == System-Vorbild "[simd=code@1.0.0.c]" mit disk_io.
    EXPECT_EQ(abi::organ_meta_meta_suffix_for_variant<pt::DiskWritebackTarget>(), "[disk_io=code@1.0.0.c]");
    // FIX-1 (i): die 1d-Tabelle traegt die UNGEWRAPPTE Entry-Form (4b wrappt GENAU EINMAL).
    EXPECT_EQ(abi::organ_meta_meta_entries_for_variant<pt::DiskWritebackTarget>(), "disk_io=code@1.0.0.c");
    // Wrap-Aequivalenz: EIN Renderer, EINE Grammatik (keine zweite Byte-Form, O-8-Schritt-12-Falle).
    EXPECT_EQ(std::string("[") + abi::organ_meta_meta_entries_for_variant<pt::DiskWritebackTarget>() + "]",
              abi::organ_meta_meta_suffix_for_variant<pt::DiskWritebackTarget>());
}

// -- (7) 18+1-NENNER-ANKER: die Kompositions-Welt bleibt 18 ----------------------------------------
TEST(E10OrganMetaMetaAnlage, KompositionsNennerBleibt18) {
    EXPECT_EQ(abi::kOrganAxisCount, 18u) << "kCompositionAxisNames bleibt 18 (H-23 C.1; 18+1-Schnitt)";
    SUCCEED();
}

} // namespace
