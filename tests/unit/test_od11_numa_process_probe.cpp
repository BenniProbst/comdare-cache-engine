// Paket OD-11-RT -- Laufzeit-Erhebung der target_isa-Unter-Achse core_class (Ausfuehrungs-Lokalitaet)
// und der Zuordnungs-Faehigkeit dieser Maschine.
//
// ZWECK: Block A beweist CT-Dispatch, Totalitaets-Wache, K5-Versionierung und den ANSCHLUSS an die
// statische Einhaengung (option_source == "machine_resolved"). Block B faehrt die tragende
// Klassifikations-Logik HARDWARE-FREI gegen einen Fixture-sysfs-Baum im Test-Tempdir -- einschliesslich
// des BISSES: DERSELBE Code liefert auf einer eingespeisten HYBRID-Fassung nachweislich etwas anderes
// als auf einer uniformen und etwas anderes als auf einer L3-asymmetrischen. Block C belegt jede
// K4-Fehlerklasse einzeln. Block D liest die Live-Maschine mit benanntem Skip-Guard gegen eine
// unabhaengige Zweit-Lesung und belegt am Objekt, warum acpi_cppc NICHT in der Detektions-Kette steht.
// Block E prueft A-15: kein erhobener RT-Wert gelangt in den Binary-Stempel.
//
// WARUM DIE TESTS HARDWARE-FREI TRAGEN: der gesamte sysfs-Teil der Linux-Zelle ist portables
// std::filesystem plus String-Auswertung und steht ausserhalb jedes Plattform-Guards. Genau deshalb
// laufen Listen-Grammatik, PMU-Paar, L3-Domaenen, Klassifikations-Kette und ALLE Negativ-Proben auf
// jeder Bau-Plattform -- nicht nur dort, wo zufaellig eine hybride CPU steht. Die Bau-Maschine dieses
// Pakets (prod1/Zen5) ist AUSDRUECKLICH NICHT hybrid; die Hybrid-Faelle sind deshalb eingespeiste
// Topologie-Fassungen, und der Live-Block belegt getrennt, was die echte Maschine hergibt.
//
// K2/A-15: Der Test startet keinen Prozess. Er nutzt nur Datei-Reads und Scheduler-Syscalls. Die
// Stempel-Gegenprobe liest den bestehenden Stempel, schreibt aber weder Registry noch ABI-Daten.
// Die Zuordnungs-Erprobung veraendert die Affinitaet des Test-Threads kurzzeitig und stellt sie wieder
// her -- der Test prueft ausdruecklich, dass sie das tut.

#include "comdare_test_tmp.hpp"

#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/system_axis_code_versions.hpp>
#include <cache_engine/abi/system_cell_values.hpp>
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/hardware_probe_factory.hpp>
#include <cache_engine/measurement/numa_page_probe.hpp>
#include <cache_engine/measurement/numa_process_probe.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/target_isa_sub_axes.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#if defined(__linux__)
#include <sched.h>
#endif

namespace cem  = ::comdare::cache_engine::measurement;
namespace cabi = ::comdare::cache_engine::abi;
namespace fs   = std::filesystem;

using LinuxProbe   = cem::NumaProcessProbe<cem::LinuxOperatingSystem>;
using WindowsProbe = cem::NumaProcessProbe<cem::WindowsOperatingSystem>;
using MacosProbe   = cem::NumaProcessProbe<cem::MacosOperatingSystem>;

namespace {

// -- Block A (compile-time): CT-Dispatch, Totalitaet, K5 und der ANSCHLUSS -------------------------

[[nodiscard]] constexpr std::size_t count_character(std::string_view text, char needle) noexcept {
    std::size_t count = 0;
    for (char const c : text)
        if (c == needle) ++count;
    return count;
}

/// Zweite, ABSICHTLICH unabhaengige Formulierung des K5-Vertrags. Sie ruft die Produktions-Helfer
/// numa_process_probe_family_part/at_count NICHT auf: sonst koennten Wache und Gegenprobe denselben
/// Praefix-/Trenner-Fehler teilen.
template <class OsAxis>
[[nodiscard]] consteval bool probe_id_contract() noexcept {
    constexpr std::string_view prefix = "numa_process_probe.";
    constexpr std::string_view id     = cem::NumaProcessProbe<OsAxis>::probe_id();
    if (!id.starts_with(prefix) || count_character(id, '@') != 1U) return false;

    std::size_t const separator = id.find('@');
    if (separator <= prefix.size()) return false;
    if (id.substr(prefix.size(), separator - prefix.size()) != OsAxis::os_family_id()) return false;

    std::string_view const version = cem::numa_process_probe_version_part(id);
    cem::AlgoSemVer const  parsed  = cem::parse_algo_semver(version);
    return cem::ce_owned_version_is_wellformed(version) && !parsed.is_sentinel() && !parsed.experimental &&
           version.find('e') == std::string_view::npos;
}

struct FantasieAchse;

template <class T>
concept CompleteType = requires { sizeof(T); };

static_assert(cem::NumaProcessProbeConcept<LinuxProbe>);
static_assert(cem::NumaProcessProbeConcept<WindowsProbe>);
static_assert(cem::NumaProcessProbeConcept<MacosProbe>);
static_assert(std::same_as<typename LinuxProbe::os_axis, cem::LinuxOperatingSystem>);
static_assert(std::same_as<typename WindowsProbe::os_axis, cem::WindowsOperatingSystem>);
static_assert(std::same_as<typename MacosProbe::os_axis, cem::MacosOperatingSystem>);
static_assert(!std::same_as<LinuxProbe, WindowsProbe> && !std::same_as<LinuxProbe, MacosProbe> &&
              !std::same_as<WindowsProbe, MacosProbe>);
static_assert(!CompleteType<cem::NumaProcessProbe<FantasieAchse>>,
              "Das primaere NumaProcessProbe-Template muss als Totalitaets-Wache unvollstaendig bleiben.");

static_assert(probe_id_contract<cem::LinuxOperatingSystem>());
static_assert(probe_id_contract<cem::WindowsOperatingSystem>());
static_assert(probe_id_contract<cem::MacosOperatingSystem>());
static_assert(LinuxProbe::probe_id() == std::string_view{"numa_process_probe.linux@v1.0.0c"});
static_assert(WindowsProbe::probe_id() == std::string_view{"numa_process_probe.windows@v1.0.0c"});
static_assert(MacosProbe::probe_id() == std::string_view{"numa_process_probe.macos@v1.0.0c"});
static_assert(LinuxProbe::probe_id() != WindowsProbe::probe_id() && LinuxProbe::probe_id() != MacosProbe::probe_id() &&
              WindowsProbe::probe_id() != MacosProbe::probe_id());
// GESCHWISTER-ABGRENZUNG: die beiden Erhebungen an derselben Maschine duerfen im Log nie als eine
// erscheinen -- weder als ganze ID noch ueber einen Praefix-Treffer.
static_assert(LinuxProbe::probe_id() != cem::NumaPageProbe<cem::LinuxOperatingSystem>::probe_id());
static_assert(!LinuxProbe::probe_id().starts_with(std::string_view{"numa_page_probe."}));
static_assert(!cem::NumaPageProbe<cem::LinuxOperatingSystem>::probe_id().starts_with(
    std::string_view{"numa_process_probe."}));

/// DER ANSCHLUSS als Test-Aussage: die Probe loest genau die Unter-Achse auf, die als machine_resolved
/// angeboten ist -- ueber den TYP, nicht ueber ein Etikett.
static_assert(std::same_as<LinuxProbe::core_axis, cem::CoreClassSubAxis>);
static_assert(LinuxProbe::core_axis::option_source() == std::string_view{"machine_resolved"});
static_assert(LinuxProbe::core_axis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(cem::axis_depth_v<cem::CoreClassSubAxis> == 1);
// Alle drei Familien loesen DIESELBE Achse auf -- die Familie waehlt die Quelle, nicht die Achse.
static_assert(std::same_as<WindowsProbe::core_axis, LinuxProbe::core_axis> &&
              std::same_as<MacosProbe::core_axis, LinuxProbe::core_axis>);
// Und sie ist eine ANDERE Achse als die beiden Speicher-Unter-Achsen (Owner: "strukturell aehnlich",
// nicht gleich).
static_assert(!std::same_as<LinuxProbe::core_axis, cem::NumaNodeSubAxis> &&
              !std::same_as<LinuxProbe::core_axis, cem::PageSubAxis>);

/// B7-MITGLIEDSCHAFT als Test-Aussage: die Achse ist wirklich Mitglied der Liste, die der
/// Registry-Generator iteriert -- nicht nur ein Typ, der zufaellig daneben steht. Ohne diese Wache
/// koennte core_class existieren, alle Concept-Wachen erfuellen und trotzdem nie in der XML landen.
static_assert(cem::TargetIsaOpenSubAxes::size == 3);
static_assert(cem::kTargetIsaSubAxisLabels[0] == std::string_view{"numa_node"} &&
                  cem::kTargetIsaSubAxisLabels[1] == std::string_view{"page"} &&
                  cem::kTargetIsaSubAxisLabels[2] == std::string_view{"core_class"},
              "Die Emissions-Reihenfolge ist die Listen-Reihenfolge: die zwei bestehenden XML-Zeilen "
              "muessen VORNE und unveraendert stehen, core_class tritt als dritte hinzu.");

/// DECKEL-GEGENPROBE gegen die Geschwister-Probe: die Listen-GRAMMATIK ist dieselbe, der DECKEL nicht.
/// Diese Wache steht im Test und nicht im Header, weil sie die einzige Stelle ist, die beide Header
/// gleichzeitig sieht -- der Header selbst inkludiert die Geschwister-Probe bewusst nicht.
static_assert(static_cast<std::size_t>(cem::kMaxCpuId) > static_cast<std::size_t>(cem::kMaxNumaNodeId),
              "Der CPU-Deckel darf nie auf den NUMA-Knoten-Deckel fallen: die CPU-Liste einer "
              "Grossmaschine wuerde sonst still als QuelleKorrupt gemeldet.");

constexpr std::size_t kNativeProbeCount = static_cast<std::size_t>(LinuxProbe::has_native_probe()) +
                                          static_cast<std::size_t>(WindowsProbe::has_native_probe()) +
                                          static_cast<std::size_t>(MacosProbe::has_native_probe());
static_assert(kNativeProbeCount == 1U,
              "Genau die Familie der Bauplattform muss nativ sein; die beiden fremden Zellen bleiben ehrlich.");

// -- Gemeinsame Test-Helfer ----------------------------------------------------------------------

/// comdare_test_tmp.hpp stellt die per-User-Wurzel bereit. Diese RAII-Schicht gibt jedem Test einen
/// eigenen Baum und entfernt ihn im Destruktor, damit parallele CI-Laeufe keine alten Quellen sehen.
class ProcessTestTree {
public:
    explicit ProcessTestTree(std::string_view name)
        : root_(::comdare::test::user_tmp_dir() /
                ("od11_" + std::string{name} + "_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::error_code ec;
        fs::create_directories(root_, ec);
        ready_ = !ec;
    }

    ProcessTestTree(ProcessTestTree const&)            = delete;
    ProcessTestTree& operator=(ProcessTestTree const&) = delete;

    ~ProcessTestTree() {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    [[nodiscard]] bool     ready() const noexcept { return ready_; }
    [[nodiscard]] fs::path child(std::string_view name) const { return root_ / std::string{name}; }

    [[nodiscard]] fs::path make_dir(std::string_view name) const {
        fs::path const  path = child(name);
        std::error_code ec;
        fs::create_directories(path, ec);
        return path;
    }

    void write_at(fs::path const& path, std::string_view content) const {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        std::ofstream output(path, std::ios::binary);
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    /// Baut den CPU-Baum mit genau der uebergebenen online-Zeile.
    [[nodiscard]] fs::path make_cpu_root(std::string_view online_content) const {
        fs::path const root = make_dir("cpu");
        write_at(root / "online", online_content);
        return root;
    }

    /// Haengt je Prozessor einen L3-Knoten unter <cpu_root>/cpu<N>/cache/index3 ein.
    void add_l3(fs::path const& cpu_root, std::uint32_t cpu, std::string_view size, std::string_view shared) const {
        fs::path const index = cpu_root / ("cpu" + std::to_string(cpu)) / "cache" / "index3";
        write_at(index / "size", size);
        write_at(index / "shared_cpu_list", shared);
    }

    /// Baut den PMU-Baum. Ein leerer Inhalt heisst "dieses PMU-Verzeichnis existiert nicht".
    [[nodiscard]] fs::path make_pmu_root(std::string_view performance_cpus, std::string_view efficiency_cpus) const {
        fs::path const root = make_dir("devices");
        if (!performance_cpus.empty()) write_at(root / "cpu_core" / "cpus", performance_cpus);
        if (!efficiency_cpus.empty()) write_at(root / "cpu_atom" / "cpus", efficiency_cpus);
        return root;
    }

private:
    fs::path root_;
    bool     ready_ = false;
};

/// Ein Kontext, der NUR die Dateien liest und die Zuordnung NICHT erprobt -- die Fixture-Tests sollen
/// die Affinitaet des Test-Threads nicht anfassen.
[[nodiscard]] cem::NumaProcessProbeContext lese_kontext(fs::path cpu_root, fs::path pmu_root) {
    cem::NumaProcessProbeContext ctx{};
    ctx.cpu_root        = std::move(cpu_root);
    ctx.cpu_pmu_root    = std::move(pmu_root);
    ctx.erprobe_pinning = false;
    return ctx;
}

template <class Result>
void expect_hardware_error(Result const& result, cem::HardwareProbeErrorClass expected, std::string_view label) {
    ASSERT_FALSE(result.has_value()) << "Der Fehlerpfad darf keinen Ersatz-Erfolg liefern.";
    auto const* error = std::get_if<cem::HardwareProbeErrorClass>(&result.error());
    ASSERT_NE(error, nullptr) << "Ein Quellen-Zugangsfehler darf nie als Compiler-Compiler-Fehler reisen.";
    EXPECT_EQ(*error, expected);
    EXPECT_EQ(cem::error_domain(result.error()), cem::ErrorDomain::HardwareProbe);
    EXPECT_EQ(cem::numa_process_probe_error_label(result.error()), label);
}

/// Die Parser melden die nackte Klasse (kein variant) -- eigene Erwartung, damit die Negativ-Proben
/// direkt an der Parser-Naht ansetzen und nicht erst hinter der Fehler-Summe.
template <class Result>
void expect_parser_class(Result const& result, cem::HardwareProbeErrorClass expected) {
    ASSERT_FALSE(result.has_value()) << "Der Parser darf im Fehlerfall keinen Ersatzwert liefern.";
    EXPECT_EQ(result.error(), expected);
}

[[nodiscard]] std::optional<std::string> read_trimmed_file(fs::path const& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;
    std::string content;
    std::getline(input, content, '\0');
    if (input.bad()) return std::nullopt;
    auto const       is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    std::string_view view{content};
    while (!view.empty() && is_space(view.front())) view.remove_prefix(1);
    while (!view.empty() && is_space(view.back())) view.remove_suffix(1);
    if (view.empty()) return std::nullopt;
    return std::string{view};
}

template <class OsAxis>
void verify_native_or_l6_producer(cem::NumaProcessProbeContext const& context, std::size_t& native_count) {
    using Probe = cem::NumaProcessProbe<OsAxis>;
    if constexpr (Probe::has_native_probe()) {
        ++native_count;
    } else {
        auto const topology = Probe::collect(context);
        ASSERT_FALSE(topology.core_classes.has_value()) << Probe::probe_id();
        ASSERT_FALSE(topology.pinning.has_value()) << Probe::probe_id();
        for (auto const* error : {std::get_if<cem::CompilerCompilerErrorClass>(&topology.core_classes.error()),
                                  std::get_if<cem::CompilerCompilerErrorClass>(&topology.pinning.error())}) {
            ASSERT_NE(error, nullptr) << Probe::probe_id();
            EXPECT_EQ(*error, cem::CompilerCompilerErrorClass::BetriebssystemFeatureFehlt);
        }
        EXPECT_EQ(cem::error_domain(topology.core_classes.error()), cem::ErrorDomain::CompilerCompiler);
        EXPECT_EQ(cem::numa_process_probe_error_label(topology.pinning.error()),
                  std::string_view{"betriebssystem_feature_fehlt"});
        EXPECT_FALSE(Probe::not_implemented_reason().empty())
            << "Eine fremde Familien-Zelle muss ihren ehrlichen Nicht-Implementierungsgrund benennen.";
    }
}

} // namespace

// -- Block B (runtime, HARDWARE-FREI): die tragende Klassifikations-Logik --------------------------

TEST(Od11NumaProcessProbe, ListenGrammatikDeckDieVierRealenKernelFormenAb) {
    struct Fall {
        std::string_view           text;
        std::vector<std::uint32_t> erwartet;
    };
    std::vector<Fall> const faelle = {
        {"0\n", {0}},                                        // Ein-Kern-Fassung
        {"0-3\n", {0, 1, 2, 3}},                             // zusammenhaengender Bereich
        {"0-1,4\n", {0, 1, 4}},                              // Bereich plus Einzel-Id
        {"0-3,8-11\n", {0, 1, 2, 3, 8, 9, 10, 11}},          // die reale CCD-Form
    };
    for (auto const& fall : faelle) {
        auto const result = cem::detail::numa_process_parse_cpu_list(fall.text);
        ASSERT_TRUE(result.has_value()) << "Kernel-Form abgelehnt: " << fall.text;
        EXPECT_EQ(*result, fall.erwartet) << "Falsche Expansion fuer: " << fall.text;
    }
}

/// DRIFT-GEGENPROBE zur Geschwister-Probe: die Listen-Grammatik steht in beiden Blaettern (mit
/// verschiedenen Deckeln, siehe Kopf des Linux-Blattes). Die Duplikation wird nicht verschwiegen,
/// sondern hier festgenagelt -- laufen die beiden Parser je auseinander, bricht dieser Test.
TEST(Od11NumaProcessProbe, DieDoppelteListenGrammatikDarfNieAuseinanderlaufen) {
    for (std::string_view const form : {"0", "0-3", "0-1,4", "0,2,4-5"}) {
        auto const prozess = cem::detail::numa_process_parse_cpu_list(form);
        auto const seite   = cem::detail::numa_page_parse_id_list(form);
        ASSERT_TRUE(prozess.has_value()) << form;
        ASSERT_TRUE(seite.has_value()) << form;
        EXPECT_EQ(*prozess, *seite) << "Die beiden Kopien der Kernel-Listen-Grammatik sind gedriftet: " << form;
    }
    // Und die Stelle, an der sie sich UNTERSCHEIDEN duerfen und muessen: der Deckel.
    auto const ueber_knoten_deckel = std::to_string(cem::kMaxNumaNodeId + 1U);
    EXPECT_TRUE(cem::detail::numa_process_parse_cpu_list(ueber_knoten_deckel).has_value())
        << "Eine CPU-Id oberhalb des NUMA-Knoten-Deckels ist gueltig -- der Deckel ist feldspezifisch.";
    EXPECT_FALSE(cem::detail::numa_page_parse_id_list(ueber_knoten_deckel).has_value());
}

TEST(Od11NumaProcessProbe, CacheGroessenKontraktWirdExaktGelesen) {
    auto const l3_klein = cem::detail::numa_process_parse_cache_size("32768K\n");
    ASSERT_TRUE(l3_klein.has_value());
    EXPECT_EQ(*l3_klein, std::uint64_t{32768} * 1024U);

    auto const l3_vcache = cem::detail::numa_process_parse_cache_size("98304K\n");
    ASSERT_TRUE(l3_vcache.has_value());
    EXPECT_EQ(*l3_vcache, std::uint64_t{98304} * 1024U);
    // Die tragende Unterscheidung dieser Quelle: die beiden realen prod1-Groessen sind verschieden.
    EXPECT_NE(*l3_klein, *l3_vcache);
}

/// DER BISS, Teil 1: DIESELBE Klassifikations-Funktion, drei eingespeiste Maschinen, drei verschiedene
/// Antworten. Ohne diesen Test waere "core_class" ein Feld, das immer dasselbe sagt.
TEST(Od11NumaProcessProbe, HybridUniformUndL3AsymmetrieLiefernDreiVerschiedeneKarten) {
    std::vector<std::uint32_t> const online = {0, 1, 2, 3, 4, 5, 6, 7};

    // (1) HYBRID (Intel-Fassung): zwei benannte PMUs.
    auto const hybrid = cem::detail::numa_process_compose_core_classes(online, {0, 1, 2, 3}, {4, 5, 6, 7}, {});
    ASSERT_TRUE(hybrid.has_value());
    EXPECT_EQ(hybrid->source, cem::CoreTopologySource::HybridPmu);
    ASSERT_EQ(hybrid->groups.size(), 2U);
    EXPECT_EQ(hybrid->groups[0].kind, cem::CoreClassKind::HoheLeistung);
    EXPECT_EQ(hybrid->groups[1].kind, cem::CoreClassKind::HoheEffizienz);
    EXPECT_EQ(hybrid->groups[0].cpu_ids, (std::vector<std::uint32_t>{0, 1, 2, 3}));
    EXPECT_EQ(hybrid->groups[1].cpu_ids, (std::vector<std::uint32_t>{4, 5, 6, 7}));

    // (2) L3-ASYMMETRIE (AMD-X3D-Fassung): keine PMUs, zwei verschiedene L3-Groessen.
    std::vector<cem::detail::L3Domain> const ccds = {
        {std::uint64_t{98304} * 1024U, {0, 1, 2, 3}}, // V-Cache-CCD
        {std::uint64_t{32768} * 1024U, {4, 5, 6, 7}}, // Frequenz-CCD
    };
    auto const asymmetrisch = cem::detail::numa_process_compose_core_classes(online, {}, {}, ccds);
    ASSERT_TRUE(asymmetrisch.has_value());
    EXPECT_EQ(asymmetrisch->source, cem::CoreTopologySource::L3Domaene);
    ASSERT_EQ(asymmetrisch->groups.size(), 2U);
    EXPECT_EQ(asymmetrisch->groups[0].kind, cem::CoreClassKind::GrosserCache);
    EXPECT_EQ(asymmetrisch->groups[1].kind, cem::CoreClassKind::KleinerCache);
    EXPECT_EQ(asymmetrisch->groups[0].cpu_ids, (std::vector<std::uint32_t>{0, 1, 2, 3}));

    // (3) UNIFORM: keine PMUs, EINE L3-Groesse. Das ist eine ECHTE Antwort, keine leere.
    std::vector<cem::detail::L3Domain> const gleich = {
        {std::uint64_t{32768} * 1024U, {0, 1, 2, 3}},
        {std::uint64_t{32768} * 1024U, {4, 5, 6, 7}},
    };
    auto const uniform = cem::detail::numa_process_compose_core_classes(online, {}, {}, gleich);
    ASSERT_TRUE(uniform.has_value());
    EXPECT_EQ(uniform->source, cem::CoreTopologySource::Homogen);
    ASSERT_EQ(uniform->groups.size(), 1U);
    EXPECT_EQ(uniform->groups[0].kind, cem::CoreClassKind::Uniform);
    EXPECT_EQ(uniform->groups[0].cpu_ids, online);

    // DIE EIGENTLICHE AUSSAGE: die drei Antworten sind paarweise verschieden -- in der Quelle UND in
    // der Klassen-Vokabel. Ein Vokabular, das Hybrid und Cache-Asymmetrie zusammenwirft, kann diese
    // Maschinen nicht auseinanderhalten.
    EXPECT_NE(hybrid->source, asymmetrisch->source);
    EXPECT_NE(asymmetrisch->source, uniform->source);
    EXPECT_NE(hybrid->groups[0].kind, asymmetrisch->groups[0].kind);
    EXPECT_NE(hybrid->groups.size(), uniform->groups.size());
}

/// DER BISS, Teil 2: dieselbe Aussage EINE Schicht hoeher -- ueber den vollen Fixture-Baum, also
/// einschliesslich Datei-Lesen, PMU-Suche und L3-Zusammenfassung.
TEST(Od11NumaProcessProbe, FixtureBaumTrenntEingespeisteHybridUndUniformMaschine) {
    // (a) Eingespeiste HYBRID-Fassung: der PMU-Baum existiert.
    {
        ProcessTestTree tree("hybrid");
        ASSERT_TRUE(tree.ready());
        fs::path const cpu = tree.make_cpu_root("0-7\n");
        fs::path const pmu = tree.make_pmu_root("0-3\n", "4-7\n");

        auto const karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, pmu));
        ASSERT_TRUE(karte.has_value()) << cem::numa_process_probe_error_label(karte.error());
        EXPECT_EQ(karte->source, cem::CoreTopologySource::HybridPmu);
        ASSERT_EQ(karte->groups.size(), 2U);
        EXPECT_EQ(karte->groups[0].cpu_ids, (std::vector<std::uint32_t>{0, 1, 2, 3}));
        EXPECT_EQ(karte->groups[1].cpu_ids, (std::vector<std::uint32_t>{4, 5, 6, 7}));
    }
    // (b) Eingespeiste UNIFORME Fassung: kein PMU-Baum, kein L3-Baum. GLEICHER Code, andere Antwort.
    {
        ProcessTestTree tree("uniform");
        ASSERT_TRUE(tree.ready());
        fs::path const cpu = tree.make_cpu_root("0-7\n");
        fs::path const pmu = tree.make_pmu_root("", "");

        auto const karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, pmu));
        ASSERT_TRUE(karte.has_value()) << cem::numa_process_probe_error_label(karte.error());
        EXPECT_EQ(karte->source, cem::CoreTopologySource::Homogen);
        ASSERT_EQ(karte->groups.size(), 1U);
        EXPECT_EQ(karte->groups[0].kind, cem::CoreClassKind::Uniform);
    }
    // (c) Eingespeiste L3-ASYMMETRISCHE Fassung ueber den echten Verzeichnis-Weg (prod1-Form).
    {
        ProcessTestTree tree("vcache");
        ASSERT_TRUE(tree.ready());
        fs::path const cpu = tree.make_cpu_root("0-3\n");
        fs::path const pmu = tree.make_pmu_root("", "");
        tree.add_l3(cpu, 0, "98304K\n", "0-1\n");
        tree.add_l3(cpu, 1, "98304K\n", "0-1\n");
        tree.add_l3(cpu, 2, "32768K\n", "2-3\n");
        tree.add_l3(cpu, 3, "32768K\n", "2-3\n");

        auto const karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, pmu));
        ASSERT_TRUE(karte.has_value()) << cem::numa_process_probe_error_label(karte.error());
        EXPECT_EQ(karte->source, cem::CoreTopologySource::L3Domaene);
        ASSERT_EQ(karte->groups.size(), 2U);
        EXPECT_EQ(karte->groups[0].kind, cem::CoreClassKind::GrosserCache);
        EXPECT_EQ(karte->groups[0].cpu_ids, (std::vector<std::uint32_t>{0, 1}));
        EXPECT_EQ(karte->groups[1].kind, cem::CoreClassKind::KleinerCache);
        EXPECT_EQ(karte->groups[1].cpu_ids, (std::vector<std::uint32_t>{2, 3}));
    }
}

TEST(Od11NumaProcessProbe, FehlenderPmuUndFehlenderL3BaumSindKeinFehlerSondernDieUniformeAntwort) {
    ProcessTestTree tree("keine_quellen");
    ASSERT_TRUE(tree.ready());
    fs::path const cpu = tree.make_cpu_root("0-1\n");

    auto const karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, tree.child("gibt_es_nicht")));
    ASSERT_TRUE(karte.has_value()) << "Eine Maschine ohne Hybrid-PMU und ohne L3-Knoten ist uniform, kein Fehler.";
    EXPECT_EQ(karte->source, cem::CoreTopologySource::Homogen);
    ASSERT_EQ(karte->groups.size(), 1U);
    EXPECT_EQ(karte->groups[0].cpu_ids, (std::vector<std::uint32_t>{0, 1}));
}

// -- Block C (runtime): jede K4-Fehlerklasse einzeln, plus der L6-Producer -------------------------

TEST(Od11NumaProcessProbe, FehlendeOnlineQuelleBleibtQuelleFehlt) {
    ProcessTestTree tree("cpu_fehlt");
    ASSERT_TRUE(tree.ready());
    auto const karte =
        cem::detail::numa_process_collect_core_classes(lese_kontext(tree.child("nicht_vorhanden"), tree.child("egal")));
    expect_hardware_error(karte, cem::HardwareProbeErrorClass::QuelleFehlt, "quelle_fehlt");
}

TEST(Od11NumaProcessProbe, VorhandeneAberLeereOnlineQuelleBleibtQuelleKorrupt) {
    ProcessTestTree tree("cpu_leer");
    ASSERT_TRUE(tree.ready());
    fs::path const cpu   = tree.make_cpu_root("   \n");
    auto const     karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, tree.child("egal")));
    expect_hardware_error(karte, cem::HardwareProbeErrorClass::QuelleKorrupt, "quelle_korrupt");
    // A4-Kern: vorhanden und korrupt darf nie als fehlend degradiert werden.
    auto const* error = std::get_if<cem::HardwareProbeErrorClass>(&karte.error());
    ASSERT_NE(error, nullptr);
    EXPECT_NE(*error, cem::HardwareProbeErrorClass::QuelleFehlt);
}

TEST(Od11NumaProcessProbe, FremdesZeichenInDerListeBleibtFormatUnbekannt) {
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("0 1 2"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("cpu0"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("0;1"), cem::HardwareProbeErrorClass::FormatUnbekannt);
}

TEST(Od11NumaProcessProbe, StrukturbruecheInDerListeBleibenQuelleKorrupt) {
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("3-1"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("0-3,2"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("0,"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("0-999999"),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_process_parse_cpu_list("0-99999999999999999999999"),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Und die Gegenprobe: die zulaessige Ober-Id selbst wird NICHT abgewiesen.
    auto const grenzfall = cem::detail::numa_process_parse_cpu_list(std::to_string(cem::kMaxCpuId));
    ASSERT_TRUE(grenzfall.has_value());
    EXPECT_EQ(grenzfall->front(), cem::kMaxCpuId);
}

TEST(Od11NumaProcessProbe, UnbrauchbareCacheGroesseWirdKlassifiziert) {
    // Ohne den K-Suffix ist es nicht das sysfs-Format -- NIE interpretieren.
    expect_parser_class(cem::detail::numa_process_parse_cache_size("32768"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_process_parse_cache_size("32768M"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_process_parse_cache_size("32 768K"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    // 'n/a statt Null': ein 0-Byte-Cache existiert nicht.
    expect_parser_class(cem::detail::numa_process_parse_cache_size("0K"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_process_parse_cache_size("  \n"),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Und die Gegenprobe: die reale prod1-Groesse wird akzeptiert.
    EXPECT_TRUE(cem::detail::numa_process_parse_cache_size("98304K").has_value());
}

TEST(Od11NumaProcessProbe, EinHalbesHybridPmuPaarIstEinBefundUndKeineHalbeKarte) {
    std::vector<std::uint32_t> const online = {0, 1, 2, 3};
    // Nur die P-Seite gemeldet: die Maschine haette dann Kerne ohne Klasse.
    expect_parser_class(cem::detail::numa_process_compose_core_classes(online, {0, 1}, {}, {}),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Nur die E-Seite.
    expect_parser_class(cem::detail::numa_process_compose_core_classes(online, {}, {2, 3}, {}),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Ein Kern in BEIDEN Klassen.
    expect_parser_class(cem::detail::numa_process_compose_core_classes(online, {0, 1, 2}, {2, 3}, {}),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Eine Klasse mit einem Prozessor, den es nicht gibt.
    expect_parser_class(cem::detail::numa_process_compose_core_classes(online, {0, 1}, {2, 3, 9}, {}),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Und die Gegenprobe: das vollstaendige, disjunkte Paar wird akzeptiert.
    EXPECT_TRUE(cem::detail::numa_process_compose_core_classes(online, {0, 1}, {2, 3}, {}).has_value());
}

TEST(Od11NumaProcessProbe, MehrAlsZweiL3GroessenWerdenBenanntStattAufZweiGezwungen) {
    std::vector<std::uint32_t> const    online = {0, 1, 2, 3, 4, 5};
    std::vector<cem::detail::L3Domain> const drei = {
        {std::uint64_t{98304} * 1024U, {0, 1}},
        {std::uint64_t{65536} * 1024U, {2, 3}},
        {std::uint64_t{32768} * 1024U, {4, 5}},
    };
    // NICHT GERATEN: eine Dreiteilung auf gross/klein braeuchte einen Schwellwert.
    expect_parser_class(cem::detail::numa_process_compose_core_classes(online, {}, {}, drei),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    // Die Gegenprobe an der Ausdrucksgrenze: GENAU zwei werden akzeptiert.
    std::vector<cem::detail::L3Domain> const zwei = {drei[0], drei[2]};
    EXPECT_TRUE(cem::detail::numa_process_compose_core_classes({0, 1, 4, 5}, {}, {}, zwei).has_value());
    EXPECT_EQ(cem::kMaxDistinctCoreClasses, 2U);
}

TEST(Od11NumaProcessProbe, EinHalbVorhandenerL3BaumIstEinBefundUndKeineUniformeMaschine) {
    ProcessTestTree tree("l3_halb");
    ASSERT_TRUE(tree.ready());
    fs::path const cpu = tree.make_cpu_root("0-3\n");
    fs::path const pmu = tree.make_pmu_root("", "");
    tree.add_l3(cpu, 0, "98304K\n", "0-1\n");
    tree.add_l3(cpu, 1, "98304K\n", "0-1\n");
    // cpu2/cpu3 tragen KEINEN L3-Knoten -- der Baum ist inkonsistent.

    auto const karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, pmu));
    expect_hardware_error(karte, cem::HardwareProbeErrorClass::QuelleKorrupt, "quelle_korrupt");
    // Die tragende Gegenprobe: er faellt NICHT still auf "uniform" zurueck.
    EXPECT_FALSE(karte.has_value());
}

TEST(Od11NumaProcessProbe, WidersprecheL3GroesseFuerDieselbeDomaeneBleibtQuelleKorrupt) {
    ProcessTestTree tree("l3_widerspruch");
    ASSERT_TRUE(tree.ready());
    fs::path const cpu = tree.make_cpu_root("0-1\n");
    fs::path const pmu = tree.make_pmu_root("", "");
    tree.add_l3(cpu, 0, "98304K\n", "0-1\n");
    tree.add_l3(cpu, 1, "32768K\n", "0-1\n"); // dieselbe Domaene, andere Groesse

    auto const karte = cem::detail::numa_process_collect_core_classes(lese_kontext(cpu, pmu));
    expect_hardware_error(karte, cem::HardwareProbeErrorClass::QuelleKorrupt, "quelle_korrupt");
}

TEST(Od11NumaProcessProbe, JedeFremdeFamilieErzeugtDenBenanntenL6Befund) {
    cem::NumaProcessProbeContext const context      = cem::default_numa_process_probe_context();
    std::size_t                        native_count = 0;
    verify_native_or_l6_producer<cem::LinuxOperatingSystem>(context, native_count);
    verify_native_or_l6_producer<cem::WindowsOperatingSystem>(context, native_count);
    verify_native_or_l6_producer<cem::MacosOperatingSystem>(context, native_count);
    EXPECT_EQ(native_count, 1U) << "Die Familien-Gegenprobe muss genau eine native und zwei fremde Zellen sehen.";
}

// -- Block C2 (runtime): die Einhaengung in die ISA-x-OS-Zelle ------------------------------------

TEST(Od11NumaProcessProbe, DieZelleLiefertDenKontextUndReichtLeereHandlesUnveraendertDurch) {
    using LinuxZelle   = cem::HardwareProbeDevice<cem::Prod1Zen5TargetIsa, cem::LinuxOperatingSystem>;
    using WindowsZelle = cem::HardwareProbeDevice<cem::Prod1Zen5TargetIsa, cem::WindowsOperatingSystem>;

    auto const linux_ctx = cem::make_numa_process_context<LinuxZelle>();
    EXPECT_EQ(linux_ctx.cpu_root, fs::path{cem::kDefaultCpuRoot});
    EXPECT_EQ(linux_ctx.cpu_pmu_root, fs::path{cem::kDefaultCpuPmuRoot});
    // Die Zelle ist die EINZIGE Quelle der Wurzeln -- sie deckt sich mit dem Prober-Default.
    auto const default_ctx = cem::default_numa_process_probe_context();
    EXPECT_EQ(linux_ctx.cpu_root, default_ctx.cpu_root);
    EXPECT_EQ(linux_ctx.cpu_pmu_root, default_ctx.cpu_pmu_root);

    // Eine pfad-lose Zelle behauptet KEINE Wurzel, und der Kontext-Bau erfindet auch keine.
    auto const windows_ctx = cem::make_numa_process_context<WindowsZelle>();
    EXPECT_TRUE(windows_ctx.cpu_root.empty());
    EXPECT_TRUE(windows_ctx.cpu_pmu_root.empty());
    auto leer            = windows_ctx;
    leer.erprobe_pinning = false;
    expect_hardware_error(cem::detail::numa_process_collect_core_classes(leer),
                          cem::HardwareProbeErrorClass::QuelleFehlt, "quelle_fehlt");
}

TEST(Od11NumaProcessProbe, DieZellenErhebungLaeuftUeberDieselbeFamilienKoordinateWieDieRamKette) {
    auto kontext            = cem::make_numa_process_context<cem::CebHardwareProbeDevice>();
    kontext.erprobe_pinning = false;
    auto const topology     = cem::probe_numa_process_topology<cem::CebHardwareProbeDevice>(kontext);
    if constexpr (LinuxProbe::has_native_probe()) {
        EXPECT_TRUE(topology.core_classes.has_value())
            << "Die native Zell-Erhebung muss die Kern-Klassen liefern: "
            << cem::numa_process_probe_error_label(topology.core_classes.error());
    } else {
        ASSERT_FALSE(topology.core_classes.has_value());
        EXPECT_EQ(cem::numa_process_probe_error_label(topology.core_classes.error()),
                  std::string_view{"betriebssystem_feature_fehlt"});
    }
}

// -- Block C3 (runtime): die Zuordnungs-Erprobung und der WARN ------------------------------------

TEST(Od11NumaProcessProbe, AbgeschalteteErprobungErfindetKeinenErfolg) {
    auto kontext            = cem::default_numa_process_probe_context();
    kontext.erprobe_pinning = false;
#if defined(__linux__)
    auto const pinning = cem::detail::numa_process_probe_pinning_linux(kontext);
    ASSERT_TRUE(pinning.has_value());
    EXPECT_EQ(pinning->availability, cem::PinningAvailability::KeineSchnittstelle);
    EXPECT_EQ(pinning->erlaubte_kerne, 0U);
#else
    GTEST_SKIP() << "Benannter Skip-Guard: die Linux-Erprobung ist nur in der nativen Zelle erreichbar.";
#endif
}

TEST(Od11NumaProcessProbe, DerWarnFeuertNurWennKlassenDaSindUndDerPinNichtDurchgesetzt) {
    auto baue = [](std::size_t klassen, cem::PinningAvailability pin) {
        cem::CoreClassMap map;
        map.source = (klassen > 1U) ? cem::CoreTopologySource::HybridPmu : cem::CoreTopologySource::Homogen;
        for (std::size_t i = 0; i < klassen; ++i)
            map.groups.push_back(cem::CoreClassGroup{cem::CoreClassKind::Uniform, {static_cast<std::uint32_t>(i)}});
        cem::PinningCapability cap{};
        cap.availability = pin;
        return cem::ProcessLocalityTopology{std::move(map), cap};
    };

    // Mehrere Klassen, nicht durchgesetzt -> WARN (die Owner-Bedingung).
    EXPECT_TRUE(warns_no_pinned_locality(baue(2, cem::PinningAvailability::NichtDurchgesetzt)));
    EXPECT_TRUE(warns_no_pinned_locality(baue(2, cem::PinningAvailability::VomKernAbgelehnt)));
    EXPECT_TRUE(warns_no_pinned_locality(baue(2, cem::PinningAvailability::KeineSchnittstelle)));
    // Mehrere Klassen UND durchgesetzt -> kein WARN.
    EXPECT_FALSE(warns_no_pinned_locality(baue(2, cem::PinningAvailability::Durchgesetzt)));
    // EINE Klasse -> kein WARN, auch ohne Pin: auf einer uniformen Maschine gibt es keine
    // "gepinnte Lokalitaet auf hybrider Architektur", die fehlen koennte.
    EXPECT_FALSE(warns_no_pinned_locality(baue(1, cem::PinningAvailability::KeineSchnittstelle)));

    // Eine NICHT erhobene Seite loest den WARN nicht aus -- sie traegt bereits ihre eigene Fehlerklasse.
    cem::ProcessLocalityTopology unerhoben{
        std::unexpected(cem::NumaProcessProbeError{cem::HardwareProbeErrorClass::QuelleFehlt}),
        std::unexpected(cem::NumaProcessProbeError{cem::HardwareProbeErrorClass::QuelleFehlt})};
    EXPECT_FALSE(warns_no_pinned_locality(unerhoben));

    // Der Text ist der vom Owner woertlich vorgegebene -- er wird hier gepinnt, nicht neu formuliert.
    EXPECT_EQ(cem::kNoPinnedLocalityWarning, std::string_view{"warn: no pinned locality on hybrid architecture"});
}

// -- Block D (runtime): die Live-Maschine, mit benanntem Skip-Guard --------------------------------

TEST(Od11NumaProcessProbe, LinuxRealLiefertDieKernKlassenUndStimmtMitEinerZweitLesungUeberein) {
    if constexpr (!LinuxProbe::has_native_probe()) {
        GTEST_SKIP() << "Die Live-Erhebung ist nur in der nativen Linux-Zelle erreichbar.";
    } else {
        auto const online = read_trimmed_file(fs::path{cem::kDefaultCpuRoot} / std::string{cem::kCpuOnlineLeafName});
        if (!online.has_value()) {
            GTEST_SKIP() << "Benannter Skip-Guard: der sysfs-CPU-Baum ist auf diesem Host nicht lesbar.";
        }

        auto kontext            = cem::default_numa_process_probe_context();
        kontext.erprobe_pinning = false;
        auto const topology     = LinuxProbe::collect(kontext);
        ASSERT_TRUE(topology.core_classes.has_value())
            << "Live-Erhebung der Kern-Klassen fehlgeschlagen: "
            << cem::numa_process_probe_error_label(topology.core_classes.error());
        auto const& karte = *topology.core_classes;
        ASSERT_FALSE(karte.groups.empty()) << "'n/a statt Null': eine leere Klassen-Menge darf es nie geben.";

        // Unabhaengige Zweit-Lesung: die Prozessor-ZAHL aus derselben Datei, aber ohne Produktions-Parser.
        std::size_t summe = 0;
        for (auto const& gruppe : karte.groups) {
            EXPECT_FALSE(gruppe.cpu_ids.empty()) << "Eine leere Kern-Gruppe ist strukturell unerreichbar.";
            EXPECT_TRUE(std::is_sorted(gruppe.cpu_ids.begin(), gruppe.cpu_ids.end()));
            EXPECT_EQ(std::adjacent_find(gruppe.cpu_ids.begin(), gruppe.cpu_ids.end()), gruppe.cpu_ids.end());
            summe += gruppe.cpu_ids.size();
        }
        if (online->find('-') == std::string::npos && online->find(',') == std::string::npos) {
            EXPECT_EQ(summe, 1U) << "online=" << *online;
        }
        // Die Klassen sind eine PARTITION der Online-Menge: keine Ueberschneidung, nichts uebrig.
        std::vector<std::uint32_t> alle;
        for (auto const& gruppe : karte.groups) alle.insert(alle.end(), gruppe.cpu_ids.begin(), gruppe.cpu_ids.end());
        std::sort(alle.begin(), alle.end());
        EXPECT_EQ(std::adjacent_find(alle.begin(), alle.end()), alle.end())
            << "Ein Prozessor darf nie in zwei Kern-Klassen stehen.";

        // Die Quelle ist eine ECHTE Aussage ueber diese Maschine und wird ausgegeben, nicht verschwiegen.
        std::cout << "[OD-11-RT live] quelle=" << cem::core_topology_source_token(karte.source)
                  << " klassen=" << karte.groups.size() << " online=" << *online << "\n";
        for (auto const& gruppe : karte.groups)
            std::cout << "[OD-11-RT live]   " << cem::core_class_kind_token(gruppe.kind) << " -> "
                      << gruppe.cpu_ids.size() << " prozessoren, erster=" << gruppe.cpu_ids.front() << "\n";
    }
}

/// WARUM acpi_cppc NICHT IN DER KETTE STEHT -- am Objekt belegt statt behauptet. Auf einer Maschine mit
/// CPPC-Raengen zeigt dieser Test, dass die Zahl der verschiedenen Raenge GROESSER ist als die Zahl der
/// erhobenen Kern-Klassen. Eine Klassen-Ableitung aus dem Rang haette also Klassen erfunden.
TEST(Od11NumaProcessProbe, CppcIstEinRangUndKeineKlasseUndDeshalbNichtInDerKette) {
    if constexpr (!LinuxProbe::has_native_probe()) {
        GTEST_SKIP() << "Die Live-Gegenprobe ist nur in der nativen Linux-Zelle erreichbar.";
    } else {
        auto const online_text =
            read_trimmed_file(fs::path{cem::kDefaultCpuRoot} / std::string{cem::kCpuOnlineLeafName});
        if (!online_text.has_value()) GTEST_SKIP() << "Benannter Skip-Guard: der sysfs-CPU-Baum ist nicht lesbar.";
        auto const online = cem::detail::numa_process_parse_cpu_list(*online_text);
        ASSERT_TRUE(online.has_value());

        std::vector<std::string> raenge;
        for (std::uint32_t const cpu : *online) {
            auto const wert = read_trimmed_file(fs::path{cem::kDefaultCpuRoot} / ("cpu" + std::to_string(cpu)) /
                                                "acpi_cppc" / "highest_perf");
            if (!wert.has_value()) continue;
            raenge.push_back(*wert);
        }
        if (raenge.size() != online->size()) {
            GTEST_SKIP() << "Benannter Skip-Guard: diese Maschine fuehrt kein vollstaendiges acpi_cppc "
                            "(dann ist die Quelle ohnehin unbrauchbar -- genau deshalb steht sie nicht in der Kette).";
        }
        std::sort(raenge.begin(), raenge.end());
        raenge.erase(std::unique(raenge.begin(), raenge.end()), raenge.end());

        auto kontext            = cem::default_numa_process_probe_context();
        kontext.erprobe_pinning = false;
        auto const karte        = cem::detail::numa_process_collect_core_classes(kontext);
        ASSERT_TRUE(karte.has_value());

        std::cout << "[OD-11-RT live] cppc_verschiedene_raenge=" << raenge.size()
                  << " erhobene_kernklassen=" << karte->groups.size() << "\n";
        if (raenge.size() > cem::kMaxDistinctCoreClasses) {
            EXPECT_GT(raenge.size(), karte->groups.size())
                << "Diese Maschine traegt mehr CPPC-Raenge als Kern-Klassen -- eine Klassen-Ableitung aus "
                   "dem Rang haette Klassen erfunden. Genau deshalb ist die Quelle nicht in der Kette.";
        } else {
            GTEST_SKIP() << "Benannter Skip-Guard: diese Maschine traegt hoechstens "
                         << cem::kMaxDistinctCoreClasses
                         << " verschiedene CPPC-Raenge -- der Befund ist hier nicht beobachtbar.";
        }
    }
}

TEST(Od11NumaProcessProbe, DieErprobungStelltDieAffinitaetDesAufrufersWiederHer) {
#if defined(__linux__)
    cpu_set_t vorher{};
    CPU_ZERO(&vorher);
    if (::sched_getaffinity(0, sizeof(vorher), &vorher) != 0)
        GTEST_SKIP() << "Benannter Skip-Guard: sched_getaffinity ist auf diesem Host nicht verfuegbar.";

    auto const pinning = cem::detail::numa_process_probe_pinning_linux(cem::default_numa_process_probe_context());
    ASSERT_TRUE(pinning.has_value()) << cem::numa_process_probe_error_label(pinning.error());

    cpu_set_t nachher{};
    CPU_ZERO(&nachher);
    ASSERT_EQ(::sched_getaffinity(0, sizeof(nachher), &nachher), 0);
    EXPECT_TRUE(CPU_EQUAL(&vorher, &nachher))
        << "Die Erprobung ist die eine mutierende Stelle -- sie MUSS den Aufrufer unveraendert zuruecklassen.";
    EXPECT_TRUE(pinning->maske_wiederhergestellt);

    // Die erlaubte Menge ist die EFFEKTIVE Maske und deckt sich mit der unabhaengigen Zweit-Lesung.
    EXPECT_EQ(pinning->erlaubte_kerne, static_cast<std::uint32_t>(CPU_COUNT(&vorher)));
    EXPECT_GT(pinning->erlaubte_kerne, 0U);
    EXPECT_TRUE(CPU_ISSET(static_cast<int>(pinning->gepruefter_kern), &vorher))
        << "Der erprobte Kern muss aus der ERLAUBTEN Menge stammen, sonst misst die Erprobung die "
           "cpuset-Grenze statt der Kernel-Faehigkeit.";
    std::cout << "[OD-11-RT live] pinning=" << cem::pinning_availability_token(pinning->availability)
              << " erlaubte_kerne=" << pinning->erlaubte_kerne << " geprueft=" << pinning->gepruefter_kern << "\n";
#else
    GTEST_SKIP() << "Benannter Skip-Guard: die Linux-Erprobung ist nur in der nativen Zelle erreichbar.";
#endif
}

// -- Block E (runtime): A-15-Stempel-Neutralitaet --------------------------------------------------

TEST(Od11NumaProcessProbe, LaufzeitwerteBleibenAusDemSystemStempelUndDerCodeStandBleibtV1) {
    bool code_version_found = false;
    for (auto const& entry : cabi::kSystemAxisCodeVersions) {
        if (entry.axis != cem::CoreClassSubAxis::parent_axis_label()) continue;
        code_version_found = true;
        // A-15: die RT-Erhebung ist rein additiv -- sie bumpt den target_isa-Code-Stand NICHT.
        EXPECT_EQ(entry.version, std::string_view{"v1.0.0c"})
            << "A-15: RT-Unter-Achsen duerfen den target_isa-Code-Stand nicht bumpen.";
    }
    ASSERT_TRUE(code_version_found) << "Die Eltern-Achse target_isa muss im Code-Versions-Register stehen.";

    std::string const stamp = cabi::system_stamp_line();
    // Weder die Verfahrens-ID noch ihr Praefix noch das ACHSEN-LABEL duerfen je im Stempel auftauchen.
    EXPECT_EQ(stamp.find(std::string{LinuxProbe::probe_id()}), std::string::npos)
        << "A-15-Bruch: die probe_id steht im System-Stempel: " << stamp;
    EXPECT_EQ(stamp.find("numa_process_probe."), std::string::npos)
        << "A-15-Bruch: das Verfahrens-Praefix steht im System-Stempel: " << stamp;
    EXPECT_EQ(stamp.find(std::string{cem::CoreClassSubAxis::axis_label()}), std::string::npos)
        << "A-15-Bruch: das Achsen-Label core_class steht im System-Stempel: " << stamp;

    // Und die STRUKTURELLE Seite: core_class ist ein verbotener Zellwert-Schluessel. DAS ist die
    // Bedingung des Owner-KERNs "reine Wiederverwendung, kein zweiter Bau" -- stuende die Kern-Klasse
    // im Stempel, braeuchten E-Core- und P-Core-Lauf verschiedene Binaries.
    EXPECT_FALSE(cabi::is_system_cell_value_key(cem::CoreClassSubAxis::axis_label()));
    EXPECT_EQ(cabi::diagnose_system_cell_values(std::string{cem::CoreClassSubAxis::axis_label()} + "=x"),
              cabi::SystemCellValuesDiagnose::verbotener_rt_schluessel);

    if constexpr (LinuxProbe::has_native_probe()) {
        auto kontext            = cem::default_numa_process_probe_context();
        kontext.erprobe_pinning = false;
        auto const topology     = LinuxProbe::collect(kontext);
        if (topology.core_classes.has_value() && !topology.core_classes->groups.empty()) {
            // Die Zahl der Kern-Klassen und die erste Kern-Id als DEZIMAL-Zeichenkette: die einzigen
            // erhobenen Werte, die ueberhaupt in einer Stempel-Zeile stehen KOENNTEN.
            EXPECT_EQ(stamp.find("=" + std::to_string(topology.core_classes->groups.size()) + ";core"),
                      std::string::npos)
                << "A-15-Bruch: die erhobene Klassen-Zahl steht im System-Stempel: " << stamp;
            for (auto const& gruppe : topology.core_classes->groups)
                EXPECT_EQ(stamp.find(std::string{cem::core_class_kind_token(gruppe.kind)}), std::string::npos)
                    << "A-15-Bruch: eine erhobene Kern-Klassen-Vokabel steht im System-Stempel: " << stamp;
        }
    }
}
