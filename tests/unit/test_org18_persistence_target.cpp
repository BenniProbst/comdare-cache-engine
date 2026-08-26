// STRUKT-R ORG-18 (2026-07-26) -- Wachen der 18. Organ-Haupt-Achse persistence_target.
//
// Deckt die vier Pflicht-Nachweise dieses Increments ab:
//   §1 Q-1-ZUSATZ (Owner woertlich: "Die Achse wird per XML deaktiviert und das muss unterstuetzt sein"):
//      die DEAKTIVIERUNGS-KETTE ist end-to-end belegt -- CMake-option -> flags-Header -> Baustein::enabled
//      -> mp_filter -> EnabledTargets -> Katalog-Kardinalitaet -> Registry-XML.
//   §2 Katalog-Aritaet + die belegte mp_take_c-Falle (K17 MUSS 1 sein, solange die Achse 1 Wert hat).
//   §3 Alt-id-Normalisierung (Owner-Entscheid: Datei byte-unveraendert, nur Lese-Normalisierung).
//   §4 Concept-Erfuellung beider Observable-Huellen (persistence_target UND der nachgezogene io_dispatch).
//
// @task STRUKT-R Lane B / ORG-18

#include <gtest/gtest.h>

#include <organ_axes/io_dispatch/axis_io_dispatch_observable.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_observable.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_registry.hpp>
#include <builder/experiment_tree/axis_path_serialization.hpp>
#include <builder/experiment_tree/result_ingest.hpp>
#include <profile_facade/source_catalog.hpp>
#include <topics/io/topic_io_config_set.hpp>

#include <boost/mp11.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace mp  = boost::mp11;
namespace pt  = comdare::cache_engine::persistence_target;
namespace iod = comdare::cache_engine::io_dispatch;
namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;

// =============================================================================
// §1 -- Deaktivierungs-Kette (Q-1-ZUSATZ, Pflicht-Beweis)
// =============================================================================

TEST(Org18Deaktivierung, VollausbauZweiBausteineDavonEinerAktiv) {
    // Die Achse DEKLARIERT beide Bausteine (sie existiert vollstaendig, Owner-Entscheid Q-1 = FALL B) ...
    EXPECT_EQ(mp::mp_size<pt::AllTargets>::value, 2u) << "Vollausbau der Achse muss 2 Bausteine deklarieren";
    // ... aber der Bau permutiert nur den AKTIVEN Satz.
    EXPECT_EQ(mp::mp_size<pt::EnabledTargets>::value, 1u)
        << "Q-1 FALL B: disk_writeback ist per option() AUS -> genau 1 aktiver Baustein";
}

TEST(Org18Deaktivierung, DerDeaktivierteIstGenauDiskWritebackUndDerAktiveMemoryOnly) {
    // Nicht nur DASS einer fehlt, sondern WELCHER -- sonst koennte ein Flag-Dreher unbemerkt bleiben.
    static_assert(std::is_same_v<mp::mp_front<pt::EnabledTargets>, pt::MemoryOnlyTarget>,
                  "der aktive Baustein MUSS MemoryOnlyTarget sein (golden_wired-Durchreich-Wert)");
    EXPECT_TRUE(pt::MemoryOnlyTarget::enabled) << "memory_only ist der golden_wired-Wert und darf nie aus sein";
    EXPECT_FALSE(pt::DiskWritebackTarget::enabled) << "disk_writeback MUSS unter Q-1 FALL B deaktiviert sein";
    // Die Deaktivierung kommt aus dem CMake-generierten flags-Header, nicht aus einer Code-Konstante:
    EXPECT_EQ(pt::DiskWritebackTarget::enabled, pt::flags::disk_writeback_enabled)
        << "enabled MUSS aus dem flags-Header stammen (CMake-option-Kette), nicht hartkodiert sein";
    EXPECT_EQ(pt::MemoryOnlyTarget::enabled, pt::flags::memory_only_enabled);
}

TEST(Org18Deaktivierung, TopicIoTraegtBeideAchsenUndDerDefaultSatzBleibtIoDispatch) {
    using TCS = comdare::cache_engine::io::TopicConfigSet;
    static_assert(std::is_same_v<TCS::StaticAxisVariants_PT, pt::EnabledTargets>);
    // BYTE-NEUTRALITAETS-WACHE: der 1-Topic-Default darf sich NICHT auf die neue Achse verschoben haben,
    // sonst aendert jeder Alt-Nutzer des io-Topics still seinen Permutations-Satz.
    static_assert(std::is_same_v<TCS::StaticAxisVariants, TCS::StaticAxisVariants_IO>,
                  "der Default-Satz des io-Topics MUSS axis_io bleiben");
    SUCCEED();
}

TEST(Org18Deaktivierung, RegistryXmlSpiegeltDenAktivenSatz) {
    // Die generierte Organ-Registry reflektiert per Leitplanke NUR Enabled* (axis_registry_gen/main.cpp:9-17).
    // Konsequenz, die hier festgenagelt wird: die Achse erscheint mit GENAU EINEM Baustein, und der
    // deaktivierte Name taucht NICHT auf. Das ist der XML-seitige Beleg der Deaktivierung.
    std::ifstream in{COMDARE_ORG18_AXIS_REGISTRY_XML};
    ASSERT_TRUE(in) << "Registry-XML nicht lesbar: " << COMDARE_ORG18_AXIS_REGISTRY_XML;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string const xml = ss.str();

    EXPECT_NE(xml.find("<axis id=\"persistence_target\" slot=\"T17\""), std::string::npos)
        << "die 18. Achse muss als slot=\"T17\" in der Registry stehen";
    EXPECT_NE(xml.find("name=\"persistence_memory_only\""), std::string::npos);
    EXPECT_EQ(xml.find("persistence_disk_writeback"), std::string::npos)
        << "der DEAKTIVIERTE Baustein darf NICHT in der Registry stehen (Leitplanke: nur Enabled* wird "
           "reflektiert; ein Auftauchen hiesse, dass die option()-Kette nicht greift)";

    // Baustein-Zahl der Achse: genau 1 (nicht 2) -- die zweite Haelfte des XML-Belegs.
    std::size_t const p = xml.find("<axis id=\"persistence_target\"");
    ASSERT_NE(p, std::string::npos);
    std::size_t const e = xml.find('>', p);
    ASSERT_NE(e, std::string::npos);
    EXPECT_NE(xml.substr(p, e - p).find("baustein_count=\"1\""), std::string::npos)
        << "persistence_target muss baustein_count=\"1\" tragen (1 aktiv von 2 deklariert)";
}

// =============================================================================
// E-10/ORG-19 k2-KOEDER (T-1 rot-zuerst, 26.08.2026): die generierte Organ-Registry traegt den
// 19. Eintrag der NEUEN Kategorie organ_meta_meta (18+1-Form nach H-23 C.1 -- die 18 Kompositions-
// Achsen bleiben category="composition", der Meta-Meta-Eintrag ist ADDITIV, binary_id="never").
// Vor Schritt 1e ist dieser Test ROT (Eintrag fehlt in der committeten XML) = der Koeder-Biss.
TEST(Org19MetaMeta, RegistryXmlTraegtDenMetaMetaEintrag18Plus1) {
    std::ifstream in{COMDARE_ORG18_AXIS_REGISTRY_XML};
    ASSERT_TRUE(in) << "Registry-XML nicht lesbar: " << COMDARE_ORG18_AXIS_REGISTRY_XML;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string const xml = ss.str();
    EXPECT_NE(xml.find("<axis id=\"disk_io\" category=\"organ_meta_meta\""), std::string::npos)
        << "ORG-19-IO muss als 19. Eintrag der Kategorie organ_meta_meta in der Registry stehen "
           "(Praezedenz load_framework, measurement_axis_registry.xml:43)";
    EXPECT_NE(xml.find("binary_id=\"never\""), std::string::npos)
        << "der Meta-Meta-Eintrag permutiert NIE die binary_id (18+1: kCompositionAxisNames bleibt 18)";
}

// =============================================================================
// §2 -- Katalog-Aritaet + die belegte mp_take_c-Falle
// =============================================================================

TEST(Org18Katalog, RaumBleibtBei2Hoch17WeilK17AufEinsGepinntIst) {
    // Der Kern des Owner-Entscheids: die Achse existiert, ohne den Voll-Bau zu verdoppeln.
    EXPECT_EQ(tlz::catalog_axis_product<tlz::FullSourceCatalog>(), 131072u)
        << "Q-1 FALL B: 17 Achsen je 2 x persistence_target je 1 = 131072 (NICHT 262144)";
    EXPECT_EQ(tlz::catalog_axis_product<tlz::golden_320_catalog>(), 320u);
    EXPECT_EQ(tlz::catalog_axis_product<tlz::SmallSourceCatalog>(), 4u);
    // Der 18. Slot ist real vorhanden und einelementig -- genau die Bedingung, unter der mp_take_c<L,2>
    // ill-formed waere (boost_mp11 algorithm.hpp:430,447; CatalogAxes hat KEINEN min()-Schutz).
    EXPECT_EQ(mp::mp_size<tlz::FullSourceCatalog::L17>::value, 1u)
        << "K17 MUSS 1 sein, solange die Achse 1 aktiven Baustein hat";
}

TEST(Org18Katalog, StatischeLevelUndSegmentNameSindDieAchtzehnte) {
    auto const lv = tlz::catalog_static_levels<tlz::FullSourceCatalog>();
    ASSERT_EQ(lv.size(), 18u) << "der Katalog muss 18 statische Level pushen";
    EXPECT_EQ(lv.back().axis, "persistence_target") << "persistence_target ist der LETZTE Slot (T17-Anhang)";
    // Die zentrale Pfad-Konvention traegt denselben Namen an derselben Position (Round-Trip-Voraussetzung).
    ASSERT_EQ(ex::kCompositionAxisNames.size(), 18u);
    EXPECT_EQ(ex::kCompositionAxisNames.back(), "persistence_target");
}

// =============================================================================
// §3 -- Alt-id-Normalisierung (Datei byte-unveraendert, nur Lese-Normalisierung)
// =============================================================================

TEST(Org18AltIds, SiebzehnSegmentIdWirdAufDieAchtzehnSlotFormNormalisiert) {
    std::string const alt =
        "search_algo=k_ary/cache_traversal=linear_fanout/mapping=direct_placement/"
        "path_compression=path_compression_none/node_type=node4/"
        "memory_layout=memory_layout_cache_line_aligned/allocator=std_malloc/prefetch=prefetch_none/"
        "concurrency=concurrency_none/serialization=serialization_raw_binary/value_handle=value_handle_inline/"
        "index_organization=index_org_heap/io_dispatch=io_in_memory_only/migration_policy=migration_none/"
        "filter=filter_bloom/queuing_q1=no_buffer/queuing_q2=eager_flush";
    std::string const neu = ex::normalize_legacy_binary_id(alt);
    EXPECT_EQ(neu, alt + "/persistence_target=persistence_memory_only")
        << "eine Alt-id muss genau das memory_only-Segment gewinnen (inhaltlich korrekt: sie WAR memory_only)";
}

TEST(Org18AltIds, IdempotentUndShapeSchwanzBleibtHinten) {
    std::string const neu18 = "queuing_q2=eager_flush/persistence_target=persistence_memory_only";
    EXPECT_EQ(ex::normalize_legacy_binary_id(neu18), neu18) << "schon normalisierte id MUSS unveraendert bleiben";

    // Kritischer Fall: with_shape_segment haengt NACH der Komposition an. Blindes Anhaengen wuerde die
    // Segment-Ordnung zerstoeren; das Segment MUSS direkt hinter queuing_q2 eingeschoben werden.
    std::string const mit_shape = "queuing_q1=no_buffer/queuing_q2=eager_flush/btree_order=order_16";
    EXPECT_EQ(ex::normalize_legacy_binary_id(mit_shape),
              "queuing_q1=no_buffer/queuing_q2=eager_flush/persistence_target=persistence_memory_only/"
              "btree_order=order_16");
}

TEST(Org18AltIds, NichtKompositionsIdBleibtUnberuehrt) {
    // Ohne queuing_q2-Anker ist es keine SearchAlgorithm-Kompositions-id -> nicht anfassen (kein Raten).
    std::string const fremd = "irgendwas=anderes/noch_etwas=x";
    EXPECT_EQ(ex::normalize_legacy_binary_id(fremd), fremd);
    EXPECT_EQ(ex::normalize_legacy_binary_id(""), "");
}

TEST(Org18AltIds, AltMessdatenDateiIstByteUnveraendert) {
    // Messdaten-Doktrin: die CSV wird NIE veraendert. Dieser Pin faengt jede kuenftige "Anpassung" der Datei.
    // Belegte Ist-Werte (2026-07-26, vor UND nach ORG-18 identisch): 6748937 Bytes.
    std::ifstream in{COMDARE_ORG18_TIER150_CSV, std::ios::binary | std::ios::ate};
    ASSERT_TRUE(in) << "Alt-Mess-CSV nicht lesbar: " << COMDARE_ORG18_TIER150_CSV;
    EXPECT_EQ(static_cast<long long>(in.tellg()), 6748937LL)
        << "tier150_measurements.csv wurde veraendert -- Messdaten-Doktrin verletzt (nie aendern, nie loeschen)";
}

// =============================================================================
// §4 -- Concept-Erfuellung der Observable-Huellen
// =============================================================================

TEST(Org18Observable, BeideHuellenErfuellenIhrPermutationsConcept) {
    // persistence_target: die neue Huelle.
    static_assert(pt::concepts::CacheEnginePermutationStrategy<pt::ObservablePersistenceTarget<pt::MemoryOnlyTarget>>);
    static_assert(
        pt::concepts::CacheEnginePermutationStrategy<pt::ObservablePersistenceTarget<pt::DiskWritebackTarget>>);
    // io_dispatch: der ALT-DEFEKT-NACHZUG. Bis 2026-07-26 fehlten hier axis_tag/family_id/enabled durchgereicht
    // -> ObservableIoDispatch erfuellte sein eigenes Concept NICHT. Diese Wache haelt den Fix fest.
    static_assert(iod::concepts::CacheEnginePermutationStrategy<iod::ObservableIoDispatch<iod::InMemoryOnly>>,
                  "ObservableIoDispatch muss die volle Pflicht-Oberflaeche durchreichen (Memory-Direktive "
                  "observable_wrapper_must_forward_concept_members)");
    SUCCEED();
}

TEST(Org18Observable, EhrlichkeitsMarkeUndZaehlerLuegenNicht) {
    // Der Geraete-Pfad fehlt und sagt es -- maschinenlesbar, bis in die Huelle durchgereicht.
    static_assert(!pt::DiskWritebackTarget::has_device_writeback_path());
    static_assert(!pt::ObservablePersistenceTarget<pt::DiskWritebackTarget>::has_device_writeback_path());

    unsigned char buf[48 * 8]{};
    for (unsigned i = 0; i < sizeof(buf); ++i) buf[i] = static_cast<unsigned char>(i);

    // memory_only: echte Baseline (kein Rueckschreib-Pfad) -> ehrlich 0, KEIN "n/a", keine erfundene Zahl.
    EXPECT_EQ(pt::MemoryOnlyTarget::persistence_writeback_scan(buf, 8, 48), 0u);
    // disk_writeback: echte, strategie-abhaengige Staging-Arbeit -> != 0 (der Seg-Timer trennt die Bausteine).
    EXPECT_NE(pt::DiskWritebackTarget::persistence_writeback_scan(buf, 8, 48), 0u);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    pt::ObservablePersistenceTarget<pt::DiskWritebackTarget> obs{};
    (void)obs.observe_writeback(buf, 8, 48);
    auto const s = obs.statistics();
    EXPECT_EQ(s.writeback_rounds, 1u);
    EXPECT_EQ(s.records_staged, 8u);
    EXPECT_EQ(s.bytes_staged, 8u * 48u);
    EXPECT_EQ(s.device_flushes, 0u)
        << "device_flushes MUSS 0 bleiben, solange kein Geraete-Pfad existiert -- eine Zahl waere eine Messwert-Luege";

    // Gegenprobe memory_only: kein Pfad -> 0 gestagte Bytes, aber die Runde wird ehrlich gezaehlt.
    pt::ObservablePersistenceTarget<pt::MemoryOnlyTarget> obs_mem{};
    (void)obs_mem.observe_writeback(buf, 8, 48);
    auto const m = obs_mem.statistics();
    EXPECT_EQ(m.writeback_rounds, 1u);
    EXPECT_EQ(m.bytes_staged, 0u) << "memory_only stagt nichts -> 0, keine gerechnete Phantom-Zahl";
    EXPECT_EQ(m.device_flushes, 0u);
#endif
}
