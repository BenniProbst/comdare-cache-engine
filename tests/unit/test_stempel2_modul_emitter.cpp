// test_stempel2_modul_emitter -- STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, WEICHE A; H-23 Teil B L2/L3/L4):
// die L4-ANDOCK-FLAECHE (builder/codegen/modul_emitter.hpp) und der L2-HYBRID-EMITTER
// (hybrid_modul_emitter.hpp) auf der HOST-Seite -- Textform, Strategie-Daten, Argument-Bildung, Organ-Zeilen,
// D1-Eingang, Emitter-Vertrag, Hybrid-Zustaende, XML-Speisung und die F-17-Wache.
//
// WAS DIESE TU BEWEIST
// (A) Die Strategie-Liste ist geschlossen (Index == Genus, 6 Eintraege) und fail-closed (nullptr fuer eine
//     Kennung ohne Strategie).
// (B) DIE EINE Rendering-Kernfunktion: Container-Form byte-genau (Kopfzeile, ABI-Include, Zusatz-Includes,
//     DEFINE-Makro, ANGEHAENGTER Stempel-Traeger + Stempel-Zeile NACH dem Makro -- Weiche A), 2-arg/3-arg-
//     Weiche (MESS, SYSTEM, ORGAN), stempellos ohne Organ-Zeile; SearchAlgorithm = byte-gleiche DELEGATION an
//     render_adhoc_module_source (kein zweiter SA-Emitter).
// (C) Argument-Bildung und Organ-Zeile je Container-Gattung (13/9/5/11 Slots, Alias-Reihenfolge der
//     Kompositionen), Stempelbarkeits-Praedikat je Genus.
// (D) Eingang = das D1-Verdict (genus_build_verdict), nicht eine eigene Aritaets-Tabelle.
// (E) Der Emitter je Gattung schreibt je Komposition eine Datei im Loader-Pattern; stampbare Kompositionen
//     tragen die ECHTEN Zeilen, andere bleiben stempellos (Diagnose-Klasse) -- ueber die REALE
//     SetPermutationEngine (for_each_composition_type, E-24 C2) UND eine feste Liste.
// (F) Hybrid: alle Emissions-Zustaende werden ERZEUGT (nicht nur benannt), die Speisung aus dem echten
//     XML-Parser traegt Ziel-Genus und Deckel, die Organ-Zeile ist das Reroute-Ziel, und F-17: zwei
//     Konfigurationen, die sich nur in max_docks unterscheiden, erzeugen BYTE-IDENTISCHE Quellen ohne
//     Dock-Token (Koeder: ein eingeschmuggeltes Dock-Token faellt der Wache auf).
//
// WAS SIE NICHT KANN: sie kompiliert und laedt kein Erzeugnis -- das ist test_stempel2_vertragspaare
// (echter dlopen-Weg ueber die 13 CLI-Fixtures, stempellos = status 13 literal).

#include <builder/codegen/hybrid_modul_emitter.hpp>
#include <builder/codegen/modul_emitter.hpp>

#include <anatomy/adapter_anatomy.hpp>
#include <anatomy/sequence_composition.hpp>
#include <anatomy/set_composition.hpp>
#include <anatomy/set_permutation_engine.hpp>
#include <anatomy/view_composition.hpp>
#include <hybrid/hybrid_config_xml.hpp>

#include "comdare_test_tmp.hpp"

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace cg    = ::comdare::cache_engine::builder::codegen;
namespace cex   = ::comdare::cache_engine::builder::experiment;
namespace ana   = ::comdare::cache_engine::anatomy;
namespace hy    = ::comdare::cache_engine::hybrid;
namespace ceabi = ::comdare::cache_engine::abi; // NICHT "abi": <cxxabi.h> (via gtest) belegt den global
namespace mp    = ::boost::mp11;

namespace {

// --------------------------------------------------------------------------------------------
// Stampbare MOCK-Slots (Muster: die Mock-Composition-Probe von abi::organ_stamp_line<Comp>): jeder Slot
// traegt einen EIGENEN Namen, damit die Alias-REIHENFOLGE der Organ-Zeile beobachtbar ist.
// --------------------------------------------------------------------------------------------
inline constexpr std::string_view kMockNamen[13] = {"m0", "m1", "m2", "m3",  "m4",  "m5", "m6",
                                                    "m7", "m8", "m9", "m10", "m11", "m12"};
template <int I>
struct MockSlot {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return kMockNamen[I]; }
    static constexpr std::string_view               algo_version = "1.2.3.c";
};

using MockSet =
    ana::SetComposition<MockSlot<0>, MockSlot<1>, MockSlot<2>, MockSlot<3>, MockSlot<4>, MockSlot<5>, MockSlot<6>,
                        MockSlot<7>, MockSlot<8>, MockSlot<9>, MockSlot<10>, MockSlot<11>, MockSlot<12>>;
using MockSeq  = ana::SequenceComposition<MockSlot<0>, MockSlot<1>, MockSlot<2>, MockSlot<3>, MockSlot<4>, MockSlot<5>,
                                          MockSlot<6>, MockSlot<7>, MockSlot<8>>;
using MockView = ana::ViewComposition<MockSlot<0>, MockSlot<1>, MockSlot<2>, MockSlot<3>, MockSlot<4>>;
using MockAdapter =
    ana::AdapterComposition<MockSlot<0>, MockSlot<1>, MockSlot<2>, MockSlot<3>, MockSlot<4>, MockSlot<5>, MockSlot<6>,
                            MockSlot<7>, MockSlot<8>, MockSlot<9>, MockSlot<10>>;
using IntSet = ana::SetComposition<int, int, int, int, int, int, int, int, int, int, int, int, int>;

/// FesteListenEngine -- Engine mit geschlossener Kompositions-Liste (dasselbe Idiom wie im CLI).
template <class... Cs>
struct FesteListenEngine {
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        (v.template operator()<Cs>(), ...);
    }
};

/// Die REALE Set-Engine (E-24 C2) mit EINER Permutation aus Mock-Slots: jeder TopicConfigSet traegt genau
/// eine Variante -> for_each_composition_type liefert genau MockSet.
template <int I>
struct MockTopic {
    using StaticAxisVariants = mp::mp_list<MockSlot<I>>;
};
using MockSetEngine = ana::SetPermutationEngine<MockTopic<0>, MockTopic<1>, MockTopic<2>, MockTopic<3>, MockTopic<4>,
                                                MockTopic<5>, MockTopic<6>, MockTopic<7>, MockTopic<8>, MockTopic<9>,
                                                MockTopic<10>, MockTopic<11>, MockTopic<12>>;

std::size_t zaehle(std::string_view s, std::string_view t) {
    std::size_t n = 0;
    for (std::size_t p = s.find(t); p != std::string_view::npos; p = s.find(t, p + t.size())) ++n;
    return n;
}

std::string lies(std::filesystem::path const& f) {
    std::ifstream      in(f);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

std::filesystem::path frisches_tmp(std::string const& etikett) {
    std::filesystem::path const d = ::comdare::test::user_tmp_dir() / ("stempel2_" + etikett);
    std::error_code             ec;
    std::filesystem::remove_all(d, ec);
    std::filesystem::create_directories(d, ec);
    return d;
}

// CT-Pins des Vertrags (die Andock-Flaeche ist ein Concept -- beide Traeger erfuellen es).
static_assert(cg::ModulEmitterVertrag<cg::GattungsModulEmitter<ana::AnatomyGenus::Set, MockSetEngine>>);
static_assert(cg::ModulEmitterVertrag<cg::GattungsModulEmitter<ana::AnatomyGenus::Set, FesteListenEngine<IntSet>>>);
static_assert(cg::ModulEmitterVertrag<cg::HybridModulEmitter>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::Set, MockSet>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::Sequence, MockSeq>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::View, MockView>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::Adapter, MockAdapter>);
static_assert(!cg::kGenusStampbar<ana::AnatomyGenus::Set, IntSet>,
              "int-Slots tragen kein name()/algo_version -> Diagnose-Klasse, stempellos.");

} // namespace

// (A) -----------------------------------------------------------------------------------------
TEST(Stempel2ModulEmitter, StrategienGeschlossenIndexTreuUndFailClosed) {
    ASSERT_EQ(cg::kAlleModulStrategien.size(), 6u);
    for (std::size_t i = 0; i < cg::kAlleModulStrategien.size(); ++i) {
        auto const  g = static_cast<ana::AnatomyGenus>(i);
        auto const* s = cg::modul_strategie_fuer(g);
        ASSERT_NE(s, nullptr) << "Genus " << i;
        EXPECT_EQ(s->genus, g);
        EXPECT_EQ(s->makro_aritaet, cex::genus_build_slot_count(g) == 32 ? 1u : cex::genus_build_slot_count(g))
            << "Aritaet == Slot-Zahl der Bau-Bindung (Hybrid: 1 = ZielGenus, 32 ist der Deckel)";
        EXPECT_TRUE(std::string_view{s->define_makro}.starts_with("COMDARE_DEFINE_"));
        EXPECT_TRUE(std::string_view{s->datei_praefix}.starts_with("comdare_anatomy_perm_"));
    }
    EXPECT_EQ(cg::modul_strategie_fuer(static_cast<ana::AnatomyGenus>(99)), nullptr)
        << "eine Gattungs-Kennung aus XML/Planer ist ein int, kein Beweis -- fail-closed";
    EXPECT_EQ(cg::kHybridModulStrategie.makro_aritaet, 1u) << "F-17: das Hybrid-Makro nimmt GENAU das Ziel-Genus";
}

// (B) -----------------------------------------------------------------------------------------
TEST(Stempel2ModulEmitter, KernfunktionContainerFormByteGenau) {
    std::array<std::string_view, 1> const inc{"a/b.hpp"};
    std::string const                     mit =
        cg::render_modul_source(cg::kSetModulStrategie, 7, "A,\n    B", "kern=x@1.0.0.c", "sys=code@1.0.0.c", {}, inc);
    std::string const soll = "// AUTO-GENERATED by modul_emitter (Stempel Teil 2, Weiche A) -- Gattung Set -- "
                             "Permutation 7 -- DO NOT EDIT\n"
                             "#include <cache_engine/abi/set_module_abi_v1.hpp>\n"
                             "#include <a/b.hpp>\n"
                             "\n"
                             "COMDARE_DEFINE_SET_MODULE(\n    A,\n    B)\n"
                             "#include <cache_engine/abi/anatomy_module_abi_v1.hpp>\n"
                             "COMDARE_ANATOMY_VERSION_STAMP(\"sys=code@1.0.0.c\", \"kern=x@1.0.0.c\")\n";
    EXPECT_EQ(mit, soll);
    // Weiche A, positional: der Stempel steht NACH dem DEFINE-Makro, nie davor, nie im Makro.
    EXPECT_LT(mit.find("COMDARE_DEFINE_SET_MODULE("), mit.find("COMDARE_ANATOMY_VERSION_STAMP("));
    EXPECT_TRUE(cg::erzeugnis_ist_gestempelt(mit));
    EXPECT_TRUE(cg::erzeugnis_traegt_define_makro(mit, "COMDARE_DEFINE_SET_MODULE"));

    std::string const ohne = cg::render_modul_source(cg::kSetModulStrategie, 7, "A,\n    B", {}, {}, {}, inc);
    EXPECT_EQ(ohne, "// AUTO-GENERATED by modul_emitter (Stempel Teil 2, Weiche A) -- Gattung Set -- "
                    "Permutation 7 -- DO NOT EDIT\n"
                    "#include <cache_engine/abi/set_module_abi_v1.hpp>\n"
                    "#include <a/b.hpp>\n"
                    "\n"
                    "COMDARE_DEFINE_SET_MODULE(\n    A,\n    B)\n");
    EXPECT_FALSE(cg::erzeugnis_ist_gestempelt(ohne));
    EXPECT_EQ(ohne.find("anatomy_module_abi_v1.hpp"), std::string::npos)
        << "ohne Organ-Zeile wird auch der Stempel-Traeger nicht inkludiert";

    // Jede Gattung nutzt DIESELBE Form mit ihren Strategie-Daten.
    for (cg::ModulEmitterStrategie const* s : {&cg::kSequenceModulStrategie, &cg::kViewModulStrategie,
                                               &cg::kAdapterModulStrategie, &cg::kHybridModulStrategie}) {
        std::string const q = cg::render_modul_source(*s, 0, "X", "o=x@1.0.0.c", "s=code@1.0.0.c");
        EXPECT_NE(q.find("#include <" + std::string{s->abi_include} + ">\n"), std::string::npos) << s->gattung_etikett;
        EXPECT_NE(q.find(std::string{s->define_makro} + "(\n    X)\n"), std::string::npos) << s->gattung_etikett;
        EXPECT_TRUE(cg::erzeugnis_ist_gestempelt(q)) << s->gattung_etikett;
    }
}

TEST(Stempel2ModulEmitter, StempelWeicheZweiArgDreiArgUndFolgeMessSystemOrgan) {
    std::string const zwei = cg::render_modul_source(cg::kViewModulStrategie, 1, "T", "o=x@1.0.0.c", "s=code@1.0.0.c");
    EXPECT_NE(zwei.find("COMDARE_ANATOMY_VERSION_STAMP(\"s=code@1.0.0.c\", \"o=x@1.0.0.c\")\n"), std::string::npos);
    EXPECT_EQ(zwei.find("COMDARE_ANATOMY_VERSION_STAMP_M("), std::string::npos);
    std::string const drei = cg::render_modul_source(cg::kViewModulStrategie, 1, "T", "o=x@1.0.0.c", "s=code@1.0.0.c",
                                                     "measurement_tooling=wallclock@1.0.0.c");
    EXPECT_NE(drei.find("COMDARE_ANATOMY_VERSION_STAMP_M(\"measurement_tooling=wallclock@1.0.0.c\", "
                        "\"s=code@1.0.0.c\", \"o=x@1.0.0.c\")\n"),
              std::string::npos)
        << "S-6a: Argument-Folge MESS, SYSTEM, ORGAN";
    // Die Anhaengung selbst, isoliert: leer bei leerer Organ-Zeile.
    std::string leer;
    cg::append_anatomy_version_stamp(leer, "", "s", "m");
    EXPECT_TRUE(leer.empty());
}

TEST(Stempel2ModulEmitter, SearchAlgorithmIstByteGleicheDelegation) {
    std::string const args = "::a::A,\n    ::b::B";
    for (auto const& [o, s, m] : std::vector<std::array<std::string_view, 3>>{
             {"", "", ""},
             {"organ=x@1.0.0.c", "sys=code@1.0.0.c", ""},
             {"organ=x@1.0.0.c", "sys=code@1.0.0.c", "measurement_tooling=macro@1.0.0.c"}}) {
        EXPECT_EQ(cg::render_modul_source(cg::kSearchAlgorithmModulStrategie, 5, args, o, s, m),
                  cg::render_adhoc_module_source(5, args, o, s, m))
            << "kein zweiter SA-Emitter: die Bytes der bestehenden Emission bleiben unangetastet";
    }
}

// (C) -----------------------------------------------------------------------------------------
TEST(Stempel2ModulEmitter, MakroArgumenteJeGattungInAliasReihenfolge) {
    std::string const set = cg::set_macro_args<MockSet>();
    EXPECT_EQ(zaehle(set, ",\n    "), 12u) << "13 Set-Slots = 12 Fugen";
    EXPECT_LT(set.find("MockSlot<0>"), set.find("MockSlot<12>"));
    EXPECT_EQ((cg::modul_macro_args<ana::AnatomyGenus::Set, MockSet>()), set);
    EXPECT_EQ(zaehle(cg::sequence_macro_args<MockSeq>(), ",\n    "), 8u) << "9 Sequence-Slots";
    EXPECT_EQ(zaehle(cg::view_macro_args<MockView>(), ",\n    "), 4u) << "5 View-Slots";
    EXPECT_EQ(zaehle(cg::adapter_macro_args<MockAdapter>(), ",\n    "), 10u) << "11 Adapter-Slots";
    EXPECT_EQ(zaehle(cg::set_macro_args<IntSet>(), "int"), 13u);
    // Dieselbe Fuge wie adhoc_macro_args (DRY im Textformat).
    EXPECT_NE(set.find(",\n    "), std::string::npos);
}

TEST(Stempel2ModulEmitter, OrganZeilenJeGattungTragenAlleSlotsInAliasReihenfolge) {
    EXPECT_EQ(cg::set_organ_stamp_line<MockSet>(),
              "search_algo=m0@1.2.3.c;cache_traversal=m1@1.2.3.c;path_compression=m2@1.2.3.c;node_type=m3@1.2.3.c;"
              "memory_layout=m4@1.2.3.c;allocator=m5@1.2.3.c;prefetch=m6@1.2.3.c;concurrency=m7@1.2.3.c;"
              "serialization=m8@1.2.3.c;index_organization=m9@1.2.3.c;io_dispatch=m10@1.2.3.c;"
              "migration_policy=m11@1.2.3.c;filter=m12@1.2.3.c");
    EXPECT_EQ(cg::sequence_organ_stamp_line<MockSeq>(),
              "memory_layout=m0@1.2.3.c;allocator=m1@1.2.3.c;prefetch=m2@1.2.3.c;concurrency=m3@1.2.3.c;"
              "serialization=m4@1.2.3.c;value_handle=m5@1.2.3.c;io_dispatch=m6@1.2.3.c;migration_policy=m7@1.2.3.c;"
              "growth_policy=m8@1.2.3.c");
    EXPECT_EQ(cg::view_organ_stamp_line<MockView>(),
              "memory_layout=m0@1.2.3.c;value_handle=m1@1.2.3.c;extent_policy=m2@1.2.3.c;layout_policy=m3@1.2.3.c;"
              "accessor_policy=m4@1.2.3.c");
    EXPECT_EQ(cg::adapter_organ_stamp_line<MockAdapter>(),
              "search_algo=m0@1.2.3.c;cache_traversal=m1@1.2.3.c;memory_layout=m2@1.2.3.c;allocator=m3@1.2.3.c;"
              "prefetch=m4@1.2.3.c;concurrency=m5@1.2.3.c;serialization=m6@1.2.3.c;value_handle=m7@1.2.3.c;"
              "io_dispatch=m8@1.2.3.c;migration_policy=m9@1.2.3.c;inner_container=m10@1.2.3.c");
    // Die Weiche ueber das Genus liefert dieselben Zeilen.
    EXPECT_EQ((cg::modul_organ_stamp_line<ana::AnatomyGenus::Set, MockSet>()), cg::set_organ_stamp_line<MockSet>());
    EXPECT_EQ((cg::modul_organ_stamp_line<ana::AnatomyGenus::Adapter, MockAdapter>()),
              cg::adapter_organ_stamp_line<MockAdapter>());
    // Entry-Zahl == Makro-Aritaet je Gattung (Stempel-Blindstellen-Wache, hier als Laufzeit-Gegenprobe).
    EXPECT_EQ(zaehle(cg::set_organ_stamp_line<MockSet>(), ";") + 1, cg::kSetModulStrategie.makro_aritaet);
    EXPECT_EQ(zaehle(cg::sequence_organ_stamp_line<MockSeq>(), ";") + 1, cg::kSequenceModulStrategie.makro_aritaet);
    EXPECT_EQ(zaehle(cg::view_organ_stamp_line<MockView>(), ";") + 1, cg::kViewModulStrategie.makro_aritaet);
    EXPECT_EQ(zaehle(cg::adapter_organ_stamp_line<MockAdapter>(), ";") + 1, cg::kAdapterModulStrategie.makro_aritaet);
}

// F-1 (Fremd-Refutation 27.08.2026): die Container-/Hybrid-Organ-Zeilen sind von der SearchAlgorithm-VOLLMENGE
// abi::OrganMetaMetas ENTKOPPELT. E-10/ORG-19 (bau/e10-38a2-org19, landet VOR diesem Zug) fuellt jene Vollmenge
// (DiskIoOrganMetaMeta) und waehlt JE COMP ueber den persistence_target-Slot -- keine Container-Anatomie fuehrt
// diesen Slot, der Hybrid auch nicht. Eine Kopplung an die Vollmenge haette nach der E-10-Landung jede dieser
// Zeilen stumm um ';[disk_io=...]' verlaengert (Segmentzahl != Aritaet, Blindstelle). Koeder-Beweis im Beweisort
// ~/backups-workflow/20260826-stempel-teil2/refutation/ (koeder-f1.diff simuliert die Vollmenge: rot-f1 = 3 Tests
// rot mit '[simd=code@1.0.0.c]'-Anhang; nach der Entkopplung gruen unter demselben Koeder).
TEST(Stempel2ModulEmitter, ContainerUndHybridOrganZeilenSindVonDerSaVollmengeEntkoppelt) {
    for (std::string const& l : {cg::set_organ_stamp_line<MockSet>(), cg::sequence_organ_stamp_line<MockSeq>(),
                                 cg::view_organ_stamp_line<MockView>(), cg::adapter_organ_stamp_line<MockAdapter>()}) {
        EXPECT_EQ(l.find('['), std::string::npos) << "Klammer-Anhang aus einer fremden Vollmenge: " << l;
        EXPECT_EQ(l.find(']'), std::string::npos) << l;
    }
    // Die Zeile IST die Achsen-Zeile (build_axis_version_stamp_line), ohne jeden Anhang -- literal gepinnt.
    EXPECT_EQ(cg::view_organ_stamp_line<MockView>(),
              "memory_layout=m0@1.2.3.c;value_handle=m1@1.2.3.c;extent_policy=m2@1.2.3.c;layout_policy=m3@1.2.3.c;"
              "accessor_policy=m4@1.2.3.c");
    hy::HybridTierConfig cfg;
    cfg.enabled   = true;
    cfg.genus     = ana::AnatomyGenus::Set;
    cfg.max_docks = 1;
    EXPECT_EQ(cg::hybrid_organ_stamp_line(cfg), "reroute_ziel=Set@1.0.0.c")
        << "die Hybrid-Organ-Zeile traegt genau das Reroute-Ziel, keinen Meta-Meta-Anhang";
}

// (D) -----------------------------------------------------------------------------------------
TEST(Stempel2ModulEmitter, EingangIstDasD1Verdict) {
    EXPECT_TRUE(
        cg::modul_emission_zulassung(ana::AnatomyGenus::Set, cg::kSetModulStrategie.makro_aritaet).zugelassen());
    auto const falsch = cg::modul_emission_zulassung(ana::AnatomyGenus::Set, cg::kSequenceModulStrategie.makro_aritaet);
    EXPECT_FALSE(falsch.zugelassen());
    EXPECT_EQ(cex::genus_build_cell(falsch), std::string_view{"nicht_gebaut"});
    EXPECT_FALSE(cg::modul_emission_zulassung(static_cast<ana::AnatomyGenus>(99), 1).zugelassen())
        << "ungebundene Kennung: Bindungs-Fehler, kein Emitter-Fall";
    // Die Zulassung IST das Verdict der Admission -- kein Nachbau.
    EXPECT_EQ(cg::modul_emission_zulassung(ana::AnatomyGenus::View, 5).build_status,
              cex::genus_build_verdict(ana::AnatomyGenus::View, 5).build_status);
}

TEST(Stempel2ModulEmitter, ErzeugnisPraedikateSindPositional) {
    EXPECT_TRUE(
        cg::erzeugnis_ist_gestempelt("COMDARE_DEFINE_SET_MODULE(int)\nCOMDARE_ANATOMY_VERSION_STAMP(\"s\", \"o\")\n"));
    EXPECT_FALSE(cg::erzeugnis_ist_gestempelt("COMDARE_DEFINE_SET_MODULE(int)\n"));
    EXPECT_FALSE(
        cg::erzeugnis_ist_gestempelt("COMDARE_ANATOMY_VERSION_STAMP(\"s\", \"o\")\nCOMDARE_DEFINE_SET_MODULE(int)\n"))
        << "ein Stempel VOR dem Makro ist nicht die Weiche-A-Form";
    EXPECT_FALSE(cg::erzeugnis_ist_gestempelt("COMDARE_ANATOMY_VERSION_STAMP(\"s\", \"o\")\n"))
        << "ohne DEFINE kein Erzeugnis";
    EXPECT_TRUE(cg::erzeugnis_traegt_define_makro("x COMDARE_DEFINE_VIEW_MODULE(a)", "COMDARE_DEFINE_VIEW_MODULE"));
    EXPECT_FALSE(cg::erzeugnis_traegt_define_makro("x COMDARE_DEFINE_VIEW_MODULE_X(a)", "COMDARE_DEFINE_VIEW_MODULE"));
}

// (E) -----------------------------------------------------------------------------------------
TEST(Stempel2ModulEmitter, EmitterSchreibtJeKompositionEineDateiImLoaderPattern) {
    std::filesystem::path const dir = frisches_tmp("emitter_set");

    // Die REALE Set-Engine: genau EINE Permutation aus Mock-Slots -> gestempelte Quelle mit echten Zeilen.
    cg::GattungsModulEmitter<ana::AnatomyGenus::Set, MockSetEngine> const real{};
    auto const                                                            files = real.emittieren(dir / "real");
    ASSERT_EQ(files.size(), 1u);
    EXPECT_EQ(files[0].filename().string(), "comdare_anatomy_perm_auto_set_0.cpp");
    std::string const q0 = lies(files[0]);
    EXPECT_TRUE(cg::erzeugnis_ist_gestempelt(q0));
    EXPECT_NE(q0.find("\"" + cg::set_organ_stamp_line<MockSet>() + "\""), std::string::npos)
        << "die Organ-Zeile des Erzeugnisses IST set_organ_stamp_line<C>";
    EXPECT_NE(q0.find("\"" + ceabi::system_stamp_line() + "\""), std::string::npos)
        << "die System-Zeile des Erzeugnisses IST abi::system_stamp_line()";
    EXPECT_EQ(q0, cg::render_modul_source(cg::kSetModulStrategie, 0, cg::set_macro_args<MockSet>(),
                                          cg::set_organ_stamp_line<MockSet>(), ceabi::system_stamp_line()));

    // Feste Liste {int-Set (Diagnose), Mock-Set (stampbar)} mit Zusatz-Includes: Index-Folge + Weiche.
    std::array<std::string_view, 1> const inc{"probe/kompositions_header.hpp"};
    cg::GattungsModulEmitter<ana::AnatomyGenus::Set, FesteListenEngine<IntSet, MockSet>> const liste{inc};
    auto const zwei = liste.emittieren(dir / "liste");
    ASSERT_EQ(zwei.size(), 2u);
    EXPECT_EQ(zwei[0].filename().string(), "comdare_anatomy_perm_auto_set_0.cpp");
    EXPECT_EQ(zwei[1].filename().string(), "comdare_anatomy_perm_auto_set_1.cpp");
    std::string const diag = lies(zwei[0]);
    EXPECT_FALSE(cg::erzeugnis_ist_gestempelt(diag))
        << "int-Slots: Diagnose-Klasse, stempellos (nicht ladbar seit A-11)";
    EXPECT_NE(diag.find("#include <probe/kompositions_header.hpp>\n"), std::string::npos);
    EXPECT_TRUE(cg::erzeugnis_ist_gestempelt(lies(zwei[1])));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

// (F) -----------------------------------------------------------------------------------------
TEST(Stempel2HybridEmitter, ZustaendeWerdenErzeugtNichtNurBenannt) {
    hy::HybridTierConfig cfg; // Default: enabled=false
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status,
              cg::HybridEmissionStatus::zweig_nicht_angefordert);
    EXPECT_TRUE(cg::render_hybrid_module_source(cfg, 0, "o", "s").quelle.empty());

    cfg.enabled = true;
    cfg.genus   = ana::AnatomyGenus::View; // ABI-sichtbar, aber kein deklariertes Reroute-Ziel (HY-A2)
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status,
              cg::HybridEmissionStatus::ziel_genus_nicht_deklariert);
    cfg.genus = ana::AnatomyGenus::FunctionInterfaceReroute; // Gate S1: nie ein Ziel
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status,
              cg::HybridEmissionStatus::ziel_genus_nicht_deklariert);

    cfg.genus     = ana::AnatomyGenus::SearchAlgorithm;
    cfg.max_docks = 0;
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status,
              cg::HybridEmissionStatus::max_docks_ausserhalb_deckel);
    cfg.max_docks = hy::kHybridNodeObergrenzeDefault + 1;
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status,
              cg::HybridEmissionStatus::max_docks_ausserhalb_deckel);
    cfg.max_docks = hy::kHybridNodeObergrenzeDefault; // der Deckel selbst ist zulaessig (inklusiv, KON28-03)
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status, cg::HybridEmissionStatus::ok);

    cfg.docks.push_back(hy::DockContractDescriptor{255, cfg.genus});
    EXPECT_EQ(cg::render_hybrid_module_source(cfg, 0, "o", "s").status,
              cg::HybridEmissionStatus::dock_vertrag_ungueltig);
    cfg.docks.clear();

    for (auto const s : cg::kAlleHybridEmissionStatus) EXPECT_NE(cg::hybrid_emission_status_name(s), "unknown");
    EXPECT_EQ(cg::hybrid_emission_status_name(cg::HybridEmissionStatus::ok), "ok");
}

TEST(Stempel2HybridEmitter, SpeisungAusXmlUndStempelAngehaengtImHandFixtureMuster) {
    hy::HybridTierParseErgebnis const e = hy::parse_hybrid_tier_xml(
        R"(<hybrid_tier enabled="true" genus="Set"><dock_array max_docks="4"/></hybrid_tier>)");
    ASSERT_TRUE(e.has_value()) << hy::hybrid_status_name(hy::letzter_parse_status(e));
    cg::HybridModulEmission const em = cg::hybrid_modul_emission(*e, 3);
    ASSERT_TRUE(em.emittiert()) << cg::hybrid_emission_status_name(em.status);
    EXPECT_EQ(em.ziel_genus, ana::AnatomyGenus::Set);
    EXPECT_EQ(em.dock_deckel_geplant, 4u) << "das Planer-Datum reist im Deskriptor";
    EXPECT_EQ(cg::hybrid_organ_stamp_line(*e), "reroute_ziel=Set@1.0.0.c");
    std::string const soll = "// AUTO-GENERATED by modul_emitter (Stempel Teil 2, Weiche A) -- Gattung Hybrid -- "
                             "Permutation 3 -- DO NOT EDIT\n"
                             "#include <hybrid/hybrid_module_abi_v1.hpp>\n"
                             "\n"
                             "COMDARE_DEFINE_HYBRID_MODULE(\n    ::comdare::cache_engine::anatomy::AnatomyGenus::Set)\n"
                             "#include <cache_engine/abi/anatomy_module_abi_v1.hpp>\n"
                             "COMDARE_ANATOMY_VERSION_STAMP(\"" +
                             ceabi::system_stamp_line() + "\", \"reroute_ziel=Set@1.0.0.c\")\n";
    EXPECT_EQ(em.quelle, soll);
    // Das ist die Form der Hand-Fixture hybrid_tier_module_set.cpp: ABI-Include, EIN Makro-Argument (das
    // Ziel-Genus als FQ-Enumerator), danach Stempel-Traeger + 2-arg-Stempel.
    EXPECT_LT(em.quelle.find("COMDARE_DEFINE_HYBRID_MODULE("), em.quelle.find("COMDARE_ANATOMY_VERSION_STAMP("));
    EXPECT_EQ(cg::genus_enumerator_fq(ana::AnatomyGenus::SearchAlgorithm),
              "::comdare::cache_engine::anatomy::AnatomyGenus::SearchAlgorithm");
    EXPECT_TRUE(cg::genus_enumerator_fq(ana::AnatomyGenus::FunctionInterfaceReroute).empty());
}

TEST(Stempel2HybridEmitter, F17DockzahlIstPlanerDatumUndNieImQuelltext) {
    hy::HybridTierConfig eins;
    eins.enabled                     = true;
    eins.genus                       = ana::AnatomyGenus::SearchAlgorithm;
    eins.max_docks                   = 1;
    hy::HybridTierConfig voll        = eins;
    voll.max_docks                   = hy::kHybridNodeObergrenzeDefault;
    cg::HybridModulEmission const e1 = cg::hybrid_modul_emission(eins, 0);
    cg::HybridModulEmission const e2 = cg::hybrid_modul_emission(voll, 0);
    ASSERT_TRUE(e1.emittiert());
    ASSERT_TRUE(e2.emittiert());
    EXPECT_EQ(e1.quelle, e2.quelle) << "F-17: nur max_docks verschieden -> BYTE-IDENTISCHE Quelle";
    EXPECT_EQ(e1.dock_deckel_geplant, 1u);
    EXPECT_EQ(e2.dock_deckel_geplant, hy::kHybridNodeObergrenzeDefault);
    EXPECT_TRUE(cg::hybrid_erzeugnis_traegt_keine_dockzahl(e1.quelle));
    EXPECT_EQ(e1.quelle.find("32"), std::string::npos) << "der Deckel 32 ist keine Emissions-Konstante";
    // KOEDER: ein eingeschmuggeltes Dock-Token faellt der Wache auf (sonst waere sie wertlos).
    EXPECT_FALSE(cg::hybrid_erzeugnis_traegt_keine_dockzahl(e1.quelle + "// max_docks=32\n"));
    EXPECT_FALSE(cg::hybrid_erzeugnis_traegt_keine_dockzahl(e1.quelle + "static_assert(MaxDocks == 1);\n"));
    EXPECT_FALSE(cg::hybrid_erzeugnis_traegt_keine_dockzahl("COMDARE_DEFINE_HYBRID_MODULE(x, /*docks*/ 4)"));
}

TEST(Stempel2HybridEmitter, EmitSchreibtDateiUndDerVertragsTraegerLiefertPfade) {
    std::filesystem::path const dir = frisches_tmp("emitter_hybrid");
    hy::HybridTierConfig        cfg;
    cfg.enabled   = true;
    cfg.genus     = ana::AnatomyGenus::SearchAlgorithm;
    cfg.max_docks = 2;

    cg::HybridModulEmission d;
    auto const              f = cg::emit_hybrid_module(cfg, dir, 0, {}, &d);
    ASSERT_TRUE(f.has_value()) << cg::hybrid_emission_status_name(d.status);
    EXPECT_EQ(f->filename().string(), "comdare_anatomy_perm_auto_hybrid_0.cpp");
    EXPECT_EQ(lies(*f), d.quelle);
    EXPECT_EQ(d.dock_deckel_geplant, 2u);

    cg::HybridModulEmitter const traeger{cfg, 1};
    auto const                   pfade = traeger.emittieren(dir / "traeger");
    ASSERT_EQ(pfade.size(), 1u);
    EXPECT_EQ(pfade[0].filename().string(), "comdare_anatomy_perm_auto_hybrid_1.cpp");
    EXPECT_TRUE(cg::erzeugnis_ist_gestempelt(lies(pfade[0])));

    cfg.enabled = false;
    cg::HybridModulEmission d2;
    EXPECT_FALSE(cg::emit_hybrid_module(cfg, dir, 5, {}, &d2).has_value());
    EXPECT_EQ(d2.status, cg::HybridEmissionStatus::zweig_nicht_angefordert);
    EXPECT_TRUE((cg::HybridModulEmitter{cfg, 5}.emittieren(dir / "aus").empty()))
        << "Zweig nicht angefordert: kein Erzeugnis";

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
