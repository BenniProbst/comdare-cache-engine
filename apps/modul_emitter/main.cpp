// STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, Weiche A) -- comdare-modul-emitter CLI: das Werkzeug der
// L4-Andock-Flaeche (builder/codegen/modul_emitter.hpp + hybrid_modul_emitter.hpp) als Kommandozeile.
// Es emittiert die GESCHLOSSENE Menge der C2-VERTRAGSPAARE (H-23 L3: "je Gattung ein Vertrags-Testpaar:
// emittierte Form kompiliert+laedt / stempellos faellt mit 13"): je Gattung SearchAlgorithm, Set,
// Sequence, View, Adapter EIN gestempeltes + EIN stempelloses Modul, fuer den Hybrid ZWEI gestempelte
// (Ziel SearchAlgorithm, Ziel Set -- die beiden deklarierten Reroute-Ziele) + EIN stempelloses = 13.
//
// WER ES RUFT: cmake/modul_emitter.cmake (comdare_stempel2_vertragspaar_fixtures) als BUILD-ZEIT-Custom-
// Command; die 13 Erzeugnisse werden als SHARED-Module gebaut und von test_stempel2_vertragspaare ueber den
// ECHTEN dlopen-Weg geladen (gestempelt -> status_ok, stempellos -> status 13 literal). Der produktive
// Weg der Emission ist die CEB-Bau-Naht (perm_compile zur Laufzeit) -- diese CLI ist ihr Beweis-Treiber,
// so wie apps/adhoc_emitter der R5.G-Skalierungs-Beleg ist.
//
// DIE KOMPOSITIONEN SIND REAL, WO DAS OBJEKT ES HERGIBT: alle SearchAlgorithm-Achsen-Slots tragen DIESELBEN
// Registry-Typen wie der R5.G-Pilot (apps/adhoc_emitter/main.cpp) und stempeln ueber ihre echten
// name()/algo_version. Die genus-EIGENEN Achsen von Sequence/View/Adapter sind am Objekt nicht stempelbar
// (Begruendung in stempel2_fixture_slots.hpp) -> dort tragen stampbare Huellen der Anatomie-Defaults das
// Paar. Die Set-Gattung braucht KEINE Huelle (13 SA-Achsen).
//
// F-17 (Owner 26.08.2026, FINAL-f17 = C): die beiden Hybrid-Fixtures werden aus XML-Fragmenten mit
// max_docks=1 bzw. max_docks=32 gespeist -- die Zahl reist im Emissions-Deskriptor, NIE im Quelltext; die
// CLI prueft das selbst (rc 3 bei Verstoss) und der Unit-Test test_stempel2_modul_emitter pinnt es.
//
// AUSGABE: <out_dir>/<fixture>/<Loader-Pattern-Datei>.cpp je Fixture + <out_dir>/manifest.txt (eine Zeile je
// Fixture: name TAB relpfad TAB gestempelt|stempellos TAB genus TAB makro). Pfade auf stdout, Bilanz auf
// stderr. rc 0 nur bei exakt 13 geschriebenen Dateien.

#include <builder/codegen/all_axes_umbrella.hpp> // die realen Achsen-Registries (dieselben wie der R5.G-Pilot)
#include <builder/codegen/hybrid_modul_emitter.hpp>
#include <builder/codegen/modul_emitter.hpp>

#include <anatomy/adapter_anatomy.hpp>
#include <anatomy/sequence_composition.hpp>
#include <anatomy/set_composition.hpp>
#include <anatomy/view_composition.hpp>
#include <hybrid/hybrid_config_xml.hpp>
#include <modul_emitter/stempel2_fixture_slots.hpp>

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace ce  = ::comdare::cache_engine;
namespace ana = ::comdare::cache_engine::anatomy;
namespace cg  = ::comdare::cache_engine::builder::codegen;
namespace hy  = ::comdare::cache_engine::hybrid;
namespace fx  = ::comdare::cache_engine::stempel2_fixture;
namespace mp  = ::boost::mp11;

namespace {

// Die realen Registry-Typen -- WOERTLICH die Belegung des R5.G-Pilots (apps/adhoc_emitter/main.cpp), damit
// die SearchAlgorithm-Fixture und der Pilot dieselbe Stempel-Quelle haben.
using SA  = ce::traversal::axis_03a_search_algo::Array256SearchAlgo;
using CT  = ce::traversal::axis_03b_cache_traversal::LinearFanout;
using MP  = ce::traversal::axis_03m_mapping::DirectPlacement;
using PC  = ce::nodes::axis_02_path_compression::PathCompressionNone;
using NT  = ce::nodes::axis_04_node_type::Node256NodeType;
using ML  = ce::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout;
using AL  = ce::allocator::axis_06_allocator::StdMalloc;
using PF  = ce::prefetch::axis_07_prefetch::NonePrefetch;
using CC  = ce::concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
using SE  = ce::serialization::axis_10_serialization::RawBinarySerialization;
using VH  = ce::value_handle::axis_14_value_handle::InlineValueHandle;
using IO  = ce::search_engine::axis_01_index_organization::IotIndexOrganization;
using IOD = ce::io::axis_io::InMemoryOnly;
using MG  = ce::migration::axis_migration::NoMigration;
using FL  = ce::filter::axis_filter::BloomFilter;
using Q1  = ce::queuing::axis_q1_queuing::NoBuffer;
using Q2  = ce::queuing::axis_q2_queuing::LazyFlush;
using PT  = ce::io::axis_persistence_target::MemoryOnlyTarget;

/// SearchAlgorithm: die 18-Slot-AdHocComposition (Reihenfolge == adhoc_macro_args == organ_stamp_line).
using SaComp = ana::AdHocComposition<SA, CT, MP, PC, NT, ML, AL, PF, CC, SE, VH, IO, IOD, MG, FL, Q1, Q2, PT>;
/// Set: 13 Slots, ALLE aus SearchAlgorithm-Achsen -> real stempelbar ohne Huelle.
using SetComp = ana::SetComposition<SA, CT, PC, NT, ML, AL, PF, CC, SE, IO, IOD, MG, FL>;
/// Sequence: 8 geteilte reale Slots + stampbare Huelle des Growth-Defaults.
using SequenceComp = ana::SequenceComposition<ML, AL, PF, CC, SE, VH, IOD, MG, fx::DoublingGrowthStampbar>;
/// View: 2 geteilte reale Slots + stampbare Huellen der drei View-Defaults.
using ViewComp =
    ana::ViewComposition<ML, VH, fx::DynamicExtentStampbar, fx::LayoutRightStampbar, fx::DefaultAccessorStampbar>;
/// Adapter: 10 geteilte/delegierte reale Slots + stampbare Huelle des inner_container-Defaults.
using AdapterComp = ana::AdapterComposition<SA, CT, ML, AL, PF, CC, SE, VH, IOD, MG, fx::DequeInnerStampbar>;

// Die Stempelbarkeit der vier Fixture-Kompositionen ist COMPILE-ZEIT-Beweis, kein Laufzeit-Zufall: faellt
// eine dieser Zeilen, emittierte der Emitter das Paar stempellos -- und die dlopen-Probe faende status 13
// statt status_ok (genau die Aussage, die die Vertragspaare tragen).
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::SearchAlgorithm, SaComp>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::Set, SetComp>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::Sequence, SequenceComp>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::View, ViewComp>);
static_assert(cg::kGenusStampbar<ana::AnatomyGenus::Adapter, AdapterComp>);

/// FesteListenEngine<Cs...> -- ein Engine mit GESCHLOSSENER Kompositions-Liste (Muster fullcov::SampleEngine
/// des R5.G-Pilots): for_each_composition_type ueber genau diese Typen. Die Vertragspaare brauchen EINE
/// Permutation je Gattung, keinen Permutations-Raum.
template <class... Cs>
struct FesteListenEngine {
    [[nodiscard]] static constexpr std::size_t count() noexcept { return sizeof...(Cs); }
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        (v.template operator()<Cs>(), ...);
    }
};

/// Die Kompositions-Header der emittierten Container-TUs (Muster shape_include der Shaped-Schwester): der
/// Umbrella deklariert alle realen Registry-Typen, die Fixture-Datei die Huellen.
inline constexpr std::array<std::string_view, 2> kContainerIncludes{"builder/codegen/all_axes_umbrella.hpp",
                                                                    "modul_emitter/stempel2_fixture_slots.hpp"};

struct ManifestZeile {
    std::string fixture;
    std::string relpfad;
    bool        gestempelt;
    std::string genus;
    std::string makro;
};

/// schreibe(dir, dateiname, quelle) -- eine Quelldatei anlegen; liefert den Pfad.
std::filesystem::path schreibe(std::filesystem::path const& dir, std::string const& dateiname,
                               std::string const& quelle) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::filesystem::path const f = dir / dateiname;
    std::ofstream               out(f, std::ios::trunc);
    out << quelle;
    return f;
}

std::string relativ(std::filesystem::path const& out_dir, std::filesystem::path const& f) {
    return std::filesystem::relative(f, out_dir).generic_string();
}

/// Das Container-/SA-Paar EINER Gattung: gestempelt ueber emit_modules (Organ-/System-Zeile gespeist),
/// stempellos ueber die Kernfunktion ohne Organ-Zeile (Diagnose-Klasse, nicht ladbar seit A-11).
template <ana::AnatomyGenus G, class Comp>
void gattungs_paar(std::filesystem::path const& out_dir, std::string const& kurz, std::vector<ManifestZeile>& man,
                   std::vector<std::filesystem::path>& alle) {
    cg::ModulEmitterStrategie const&        s        = *cg::modul_strategie_fuer(G);
    std::span<std::string_view const> const includes = (G == ana::AnatomyGenus::SearchAlgorithm)
                                                           ? std::span<std::string_view const>{}
                                                           : std::span<std::string_view const>{kContainerIncludes};

    cg::GattungsModulEmitter<G, FesteListenEngine<Comp>> const emitter{includes};
    auto const gestempelt = emitter.emittieren(out_dir / (kurz + "_gestempelt"));
    for (auto const& f : gestempelt) {
        alle.push_back(f);
        man.push_back({kurz + "_gestempelt", relativ(out_dir, f), true, std::string{ana::genus_name(G)},
                       std::string{s.define_makro}});
    }

    std::string const quelle = cg::render_modul_source(s, 0, cg::modul_macro_args<G, Comp>(), {}, {}, {}, includes);
    std::filesystem::path const f =
        schreibe(out_dir / (kurz + "_stempellos"), std::string{s.datei_praefix} + "0.cpp", quelle);
    alle.push_back(f);
    man.push_back({kurz + "_stempellos", relativ(out_dir, f), false, std::string{ana::genus_name(G)},
                   std::string{s.define_makro}});
}

/// Die Hybrid-Speisung aus einem XML-Fragment -- der ECHTE Parser-Weg (hybrid_config_xml.hpp), keine
/// handgebaute Konfiguration: so reist ein Planer-Datum wirklich in den Emitter.
hy::HybridTierConfig hybrid_konfig_aus_xml(std::string_view xml) {
    hy::HybridTierParseErgebnis const e = hy::parse_hybrid_tier_xml(xml);
    if (!e.has_value()) {
        std::cerr << "comdare-modul-emitter: Hybrid-XML-Fragment nicht lesbar (hybrid_status="
                  << hy::hybrid_status_name(hy::letzter_parse_status(e)) << ")\n";
        std::exit(4);
    }
    return *e;
}

int emit_alle(std::filesystem::path const& out_dir) {
    std::vector<ManifestZeile>         man;
    std::vector<std::filesystem::path> alle;

    gattungs_paar<ana::AnatomyGenus::SearchAlgorithm, SaComp>(out_dir, "sa", man, alle);
    gattungs_paar<ana::AnatomyGenus::Set, SetComp>(out_dir, "set", man, alle);
    gattungs_paar<ana::AnatomyGenus::Sequence, SequenceComp>(out_dir, "sequence", man, alle);
    gattungs_paar<ana::AnatomyGenus::View, ViewComp>(out_dir, "view", man, alle);
    gattungs_paar<ana::AnatomyGenus::Adapter, AdapterComp>(out_dir, "adapter", man, alle);

    // HYBRID -- gespeist aus XML (Planer-Datum max_docks = 1 bzw. 32, F-17), Ziel = die zwei deklarierten
    // Reroute-Ziele; das stempellose Erzeugnis ueber die Kernfunktion ohne Organ-Zeile.
    hy::HybridTierConfig const cfg_sa = hybrid_konfig_aus_xml(
        R"(<hybrid_tier enabled="true" genus="SearchAlgorithm"><dock_array max_docks="1"/></hybrid_tier>)");
    hy::HybridTierConfig const cfg_set =
        hybrid_konfig_aus_xml(R"(<hybrid_tier enabled="true" genus="Set"><dock_array max_docks="32"/></hybrid_tier>)");
    hy::HybridTierConfig cfg_sa_32 = cfg_sa;
    cfg_sa_32.max_docks            = hy::kHybridNodeObergrenzeDefault;

    // F-17-Selbstprobe des Werkzeugs: nur max_docks verschieden -> byte-identische Quelle, kein Dock-Token.
    cg::HybridModulEmission const e1  = cg::hybrid_modul_emission(cfg_sa, 0);
    cg::HybridModulEmission const e32 = cg::hybrid_modul_emission(cfg_sa_32, 0);
    if (!e1.emittiert() || !e32.emittiert() || e1.quelle != e32.quelle ||
        !cg::hybrid_erzeugnis_traegt_keine_dockzahl(e1.quelle) || e1.dock_deckel_geplant != 1 ||
        e32.dock_deckel_geplant != hy::kHybridNodeObergrenzeDefault) {
        std::cerr << "comdare-modul-emitter: F-17-VERSTOSS -- die Dock-Zahl beeinflusst den Hybrid-Quelltext "
                     "oder reist nicht im Deskriptor (status "
                  << cg::hybrid_emission_status_name(e1.status) << "/" << cg::hybrid_emission_status_name(e32.status)
                  << ")\n";
        return 3;
    }

    struct HybridFall {
        char const*                 kurz;
        hy::HybridTierConfig const* cfg;
    };
    for (HybridFall const& h : {HybridFall{"hybrid_sa", &cfg_sa}, HybridFall{"hybrid_set", &cfg_set}}) {
        cg::HybridModulEmission     d;
        std::filesystem::path const dir = out_dir / (std::string{h.kurz} + "_gestempelt");
        auto const                  f   = cg::emit_hybrid_module(*h.cfg, dir, 0, {}, &d);
        if (!f) {
            std::cerr << "comdare-modul-emitter: Hybrid-Fixture " << h.kurz << " nicht emittiert (status "
                      << cg::hybrid_emission_status_name(d.status) << ")\n";
            return 5;
        }
        alle.push_back(*f);
        man.push_back({std::string{h.kurz} + "_gestempelt", relativ(out_dir, *f), true,
                       std::string{ana::genus_name(d.ziel_genus)},
                       std::string{cg::kHybridModulStrategie.define_makro}});
    }
    {
        cg::HybridModulEmission const ohne = cg::render_hybrid_module_source(cfg_sa, 0); // keine Organ-Zeile
        if (!ohne.emittiert() || cg::erzeugnis_ist_gestempelt(ohne.quelle)) {
            std::cerr << "comdare-modul-emitter: die stempellose Hybrid-Diagnose-Emission traegt einen Stempel\n";
            return 6;
        }
        std::filesystem::path const f = schreibe(
            out_dir / "hybrid_stempellos", std::string{cg::kHybridModulStrategie.datei_praefix} + "0.cpp", ohne.quelle);
        alle.push_back(f);
        man.push_back({"hybrid_stempellos", relativ(out_dir, f), false, std::string{ana::genus_name(cfg_sa.genus)},
                       std::string{cg::kHybridModulStrategie.define_makro}});
    }

    // Manifest + stdout-Pfade.
    {
        std::ofstream m(out_dir / "manifest.txt", std::ios::trunc);
        for (auto const& z : man)
            m << z.fixture << '\t' << z.relpfad << '\t' << (z.gestempelt ? "gestempelt" : "stempellos") << '\t'
              << z.genus << '\t' << z.makro << '\n';
    }
    for (auto const& f : alle) std::cout << f.string() << "\n";
    std::cerr
        << "comdare-modul-emitter: " << alle.size() << " Vertragspaar-Modul-.cpp geschrieben (SOLL 13; "
        << "5 Gattungen x {gestempelt, stempellos} + Hybrid 2 gestempelt + 1 stempellos), manifest.txt geschrieben.\n";
    return alle.size() == 13u ? 0 : 2;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: comdare-modul-emitter <output-dir>\n"
                     "  Emittiert die 13 C2-Vertragspaar-Fixtures (Stempel Teil 2, Weiche A) je Gattung nach\n"
                     "  <output-dir>/<fixture>/<Loader-Pattern>.cpp + <output-dir>/manifest.txt.\n";
        return 1;
    }
    return emit_alle(std::filesystem::path{argv[1]});
}
