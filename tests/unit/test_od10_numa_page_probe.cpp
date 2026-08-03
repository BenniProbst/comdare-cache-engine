// Paket OD-10-RT -- Laufzeit-Erhebung der beiden target_isa-Unter-Achsen numa_node und page.
//
// ZWECK: Block A beweist CT-Dispatch, Totalitaets-Wache, K5-Versionierung und den ANSCHLUSS an die
// statische Einhaengung aus f6a5dc02 (option_source == "machine_resolved"). Block B faehrt die
// tragende Parser-/Topologie-Logik HARDWARE-FREI gegen einen Fixture-sysfs-Baum im Test-Tempdir.
// Block C belegt jede K4-Fehlerklasse einzeln, einschliesslich des L6-Producers fuer
// BetriebssystemFeatureFehlt. Block D liest die Live-Maschine mit benanntem Skip-Guard gegen eine
// unabhaengige Zweit-Lesung. Block E prueft A-15: kein erhobener RT-Wert gelangt in den Binary-Stempel.
//
// WARUM DIE TESTS HARDWARE-FREI TRAGEN: der gesamte sysfs-Teil der Linux-Zelle ist portables
// std::filesystem plus String-Auswertung und steht ausserhalb jedes Plattform-Guards. Genau deshalb
// laufen Listen-Grammatik, Namens-Kontrakt, Pool-Zahl, Komposition und ALLE Negativ-Proben auf jeder
// Bau-Plattform -- nicht nur dort, wo zufaellig ein NUMA-Baum liegt.
//
// K2/A-15: Der Test startet ebenfalls keinen Prozess. Er nutzt nur Datei-Reads und sysconf. Die
// Stempel-Gegenprobe liest den bestehenden Stempel, schreibt aber weder Registry noch ABI-Daten.

#include "comdare_test_tmp.hpp"

#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/system_axis_code_versions.hpp>
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/numa_page_probe.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/target_isa_sub_axes.hpp>

#include <gtest/gtest.h>

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
#include <unistd.h>
#endif

namespace cem  = ::comdare::cache_engine::measurement;
namespace cabi = ::comdare::cache_engine::abi;
namespace fs   = std::filesystem;

using LinuxProbe   = cem::NumaPageProbe<cem::LinuxOperatingSystem>;
using WindowsProbe = cem::NumaPageProbe<cem::WindowsOperatingSystem>;
using MacosProbe   = cem::NumaPageProbe<cem::MacosOperatingSystem>;

namespace {

// -- Block A (compile-time): CT-Dispatch, Totalitaet, K5 und der ANSCHLUSS -------------------------

[[nodiscard]] constexpr std::size_t count_character(std::string_view text, char needle) noexcept {
    std::size_t count = 0;
    for (char const c : text)
        if (c == needle) ++count;
    return count;
}

/// Zweite, ABSICHTLICH unabhaengige Formulierung des K5-Vertrags. Sie ruft die Produktions-Helfer
/// numa_page_probe_family_part/at_count NICHT auf: sonst koennten Wache und Gegenprobe denselben
/// Praefix-/Trenner-Fehler teilen.
template <class OsAxis>
[[nodiscard]] consteval bool probe_id_contract() noexcept {
    constexpr std::string_view prefix = "numa_page_probe.";
    constexpr std::string_view id     = cem::NumaPageProbe<OsAxis>::probe_id();
    if (!id.starts_with(prefix) || count_character(id, '@') != 1U) return false;

    std::size_t const separator = id.find('@');
    if (separator <= prefix.size()) return false;
    if (id.substr(prefix.size(), separator - prefix.size()) != OsAxis::os_family_id()) return false;

    std::string_view const version = cem::numa_page_probe_version_part(id);
    cem::AlgoSemVer const  parsed  = cem::parse_algo_semver(version);
    return cem::ce_owned_version_is_wellformed(version) && !parsed.is_sentinel() && !parsed.experimental &&
           version.find('e') == std::string_view::npos;
}

struct FantasieAchse;

template <class T>
concept CompleteType = requires { sizeof(T); };

static_assert(cem::NumaPageProbeConcept<LinuxProbe>);
static_assert(cem::NumaPageProbeConcept<WindowsProbe>);
static_assert(cem::NumaPageProbeConcept<MacosProbe>);
static_assert(std::same_as<typename LinuxProbe::os_axis, cem::LinuxOperatingSystem>);
static_assert(std::same_as<typename WindowsProbe::os_axis, cem::WindowsOperatingSystem>);
static_assert(std::same_as<typename MacosProbe::os_axis, cem::MacosOperatingSystem>);
static_assert(!std::same_as<LinuxProbe, WindowsProbe> && !std::same_as<LinuxProbe, MacosProbe> &&
              !std::same_as<WindowsProbe, MacosProbe>);
static_assert(!CompleteType<cem::NumaPageProbe<FantasieAchse>>,
              "Das primaere NumaPageProbe-Template muss als Totalitaets-Wache unvollstaendig bleiben.");

static_assert(probe_id_contract<cem::LinuxOperatingSystem>());
static_assert(probe_id_contract<cem::WindowsOperatingSystem>());
static_assert(probe_id_contract<cem::MacosOperatingSystem>());
static_assert(LinuxProbe::probe_id() == std::string_view{"numa_page_probe.linux@v1.0.0c"});
static_assert(WindowsProbe::probe_id() == std::string_view{"numa_page_probe.windows@v1.0.0c"});
static_assert(MacosProbe::probe_id() == std::string_view{"numa_page_probe.macos@v1.0.0c"});
static_assert(LinuxProbe::probe_id() != WindowsProbe::probe_id() && LinuxProbe::probe_id() != MacosProbe::probe_id() &&
              WindowsProbe::probe_id() != MacosProbe::probe_id());

/// DER ANSCHLUSS als Test-Aussage: die Probe loest genau die beiden Unter-Achsen auf, die f6a5dc02 als
/// machine_resolved angeboten hat -- ueber den TYP, nicht ueber ein Etikett.
static_assert(std::same_as<LinuxProbe::numa_axis, cem::NumaNodeSubAxis>);
static_assert(std::same_as<LinuxProbe::page_axis, cem::PageSubAxis>);
static_assert(LinuxProbe::numa_axis::option_source() == std::string_view{"machine_resolved"});
static_assert(LinuxProbe::page_axis::option_source() == std::string_view{"machine_resolved"});
static_assert(LinuxProbe::numa_axis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(LinuxProbe::page_axis::parent_axis_label() == std::string_view{"target_isa"});
// Alle drei Familien loesen DIESELBEN beiden Achsen auf -- die Familie waehlt die Quelle, nicht die Achse.
static_assert(std::same_as<WindowsProbe::numa_axis, LinuxProbe::numa_axis> &&
              std::same_as<MacosProbe::page_axis, LinuxProbe::page_axis>);

constexpr std::size_t kNativeProbeCount = static_cast<std::size_t>(LinuxProbe::has_native_probe()) +
                                          static_cast<std::size_t>(WindowsProbe::has_native_probe()) +
                                          static_cast<std::size_t>(MacosProbe::has_native_probe());
static_assert(kNativeProbeCount == 1U,
              "Genau die Familie der Bauplattform muss nativ sein; die beiden fremden Zellen bleiben ehrlich.");

// -- Gemeinsame Test-Helfer ----------------------------------------------------------------------

/// comdare_test_tmp.hpp stellt die per-User-Wurzel bereit. Diese RAII-Schicht gibt jedem Test einen
/// eigenen Baum und entfernt ihn im Destruktor, damit parallele CI-Laeufe keine alten Quellen sehen.
class NumaPageTestTree {
public:
    explicit NumaPageTestTree(std::string_view name)
        : root_(::comdare::test::user_tmp_dir() /
                ("od10_" + std::string{name} + "_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::error_code ec;
        fs::create_directories(root_, ec);
        ready_ = !ec;
    }

    NumaPageTestTree(NumaPageTestTree const&)            = delete;
    NumaPageTestTree& operator=(NumaPageTestTree const&) = delete;

    ~NumaPageTestTree() {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    [[nodiscard]] bool     ready() const noexcept { return ready_; }
    [[nodiscard]] fs::path child(std::string_view name) const { return root_ / std::string{name}; }

    /// Legt <root>/<dir> an und gibt den Pfad zurueck.
    [[nodiscard]] fs::path make_dir(std::string_view name) const {
        fs::path const  path = child(name);
        std::error_code ec;
        fs::create_directories(path, ec);
        return path;
    }

    void write_at(fs::path const& path, std::string_view content) const {
        std::ofstream output(path, std::ios::binary);
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    /// Baut einen NUMA-Baum mit genau der uebergebenen online-Zeile.
    [[nodiscard]] fs::path make_numa_root(std::string_view online_content) const {
        fs::path const root = make_dir("node");
        write_at(root / "online", online_content);
        return root;
    }

    /// Baut einen Hugepage-Baum: je Paar (Verzeichnisname, nr_hugepages-Inhalt) ein Knoten.
    [[nodiscard]] fs::path make_hugepage_root(std::vector<std::pair<std::string, std::string>> const& entries) const {
        fs::path const  root = make_dir("hugepages");
        std::error_code ec;
        for (auto const& [name, count] : entries) {
            fs::create_directories(root / name, ec);
            write_at(root / name / "nr_hugepages", count);
        }
        return root;
    }

private:
    fs::path root_;
    bool     ready_ = false;
};

template <class Result>
void expect_hardware_error(Result const& result, cem::HardwareProbeErrorClass expected, std::string_view label) {
    ASSERT_FALSE(result.has_value()) << "Der Fehlerpfad darf keinen Ersatz-Erfolg liefern.";
    auto const* error = std::get_if<cem::HardwareProbeErrorClass>(&result.error());
    ASSERT_NE(error, nullptr) << "Ein Quellen-Zugangsfehler darf nie als Compiler-Compiler-Fehler reisen.";
    EXPECT_EQ(*error, expected);
    EXPECT_EQ(cem::error_domain(result.error()), cem::ErrorDomain::HardwareProbe);
    EXPECT_EQ(cem::numa_page_probe_error_label(result.error()), label);
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
void verify_native_or_l6_producer(cem::NumaPageProbeContext const& context, std::size_t& native_count) {
    using Probe = cem::NumaPageProbe<OsAxis>;
    if constexpr (Probe::has_native_probe()) {
        ++native_count;
    } else {
        auto const topology = Probe::collect(context);
        ASSERT_FALSE(topology.numa_nodes.has_value()) << Probe::probe_id();
        ASSERT_FALSE(topology.page_sizes.has_value()) << Probe::probe_id();
        for (auto const* error : {std::get_if<cem::CompilerCompilerErrorClass>(&topology.numa_nodes.error()),
                                  std::get_if<cem::CompilerCompilerErrorClass>(&topology.page_sizes.error())}) {
            ASSERT_NE(error, nullptr) << Probe::probe_id();
            EXPECT_EQ(*error, cem::CompilerCompilerErrorClass::BetriebssystemFeatureFehlt);
        }
        EXPECT_EQ(cem::error_domain(topology.numa_nodes.error()), cem::ErrorDomain::CompilerCompiler);
        EXPECT_EQ(cem::numa_page_probe_error_label(topology.page_sizes.error()),
                  std::string_view{"betriebssystem_feature_fehlt"});
        EXPECT_FALSE(Probe::not_implemented_reason().empty())
            << "Eine fremde Familien-Zelle muss ihren ehrlichen Nicht-Implementierungsgrund benennen.";
    }
}

} // namespace

// -- Block B (runtime, HARDWARE-FREI): die tragende Parser-/Topologie-Logik ------------------------

TEST(Od10NumaPageProbe, ListenGrammatikDeckDieVierRealenKernelFormenAb) {
    struct Fall {
        std::string_view           text;
        std::vector<std::uint32_t> erwartet;
    };
    std::vector<Fall> const faelle = {
        {"0\n", {0}},                // Ein-Knoten-Maschine (prod1-Form)
        {"0-3\n", {0, 1, 2, 3}},     // zusammenhaengender Bereich
        {"0-1,4\n", {0, 1, 4}},      // Bereich plus Einzel-Id
        {"0,2,4-5\n", {0, 2, 4, 5}}, // gemischt, aufsteigend
    };
    for (auto const& fall : faelle) {
        auto const result = cem::detail::numa_page_parse_id_list(fall.text);
        ASSERT_TRUE(result.has_value()) << "Kernel-Form abgelehnt: " << fall.text;
        EXPECT_EQ(*result, fall.erwartet) << "Falsche Expansion fuer: " << fall.text;
    }
}

TEST(Od10NumaPageProbe, HugepageNamensKontraktWirdExaktGelesen) {
    auto const zwei_mib = cem::detail::numa_page_parse_hugepage_dir_name("hugepages-2048kB");
    ASSERT_TRUE(zwei_mib.has_value());
    EXPECT_EQ(*zwei_mib, std::uint64_t{2} * 1024U * 1024U);

    auto const ein_gib = cem::detail::numa_page_parse_hugepage_dir_name("hugepages-1048576kB");
    ASSERT_TRUE(ein_gib.has_value());
    EXPECT_EQ(*ein_gib, std::uint64_t{1} << 30);
}

TEST(Od10NumaPageProbe, KompositionSortiertUndTrenntPoolBelegtVonNurUnterstuetzt) {
    std::vector<cem::detail::HugepageNode> const nodes = {
        {std::uint64_t{1} << 30, 0}, // 1 GiB, kein Pool -> NurUnterstuetzt
        {std::uint64_t{2} << 20, 7}, // 2 MiB, Pool belegt -> PoolBelegt
    };
    auto const result = cem::detail::numa_page_compose_capabilities(4096, nodes);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3U);
    EXPECT_EQ((*result)[0], (cem::PageSizeCapability{4096, cem::PageSizeAvailability::Basis}));
    EXPECT_EQ((*result)[1], (cem::PageSizeCapability{std::uint64_t{2} << 20, cem::PageSizeAvailability::PoolBelegt}));
    EXPECT_EQ((*result)[2],
              (cem::PageSizeCapability{std::uint64_t{1} << 30, cem::PageSizeAvailability::NurUnterstuetzt}));
    // Die tragende Unterscheidung: derselbe Knoten mit und ohne Pool ergibt VERSCHIEDENE Freigaben.
    EXPECT_NE((*result)[1].availability, (*result)[2].availability);
}

TEST(Od10NumaPageProbe, FixtureBaumLiefertBeideAchsenOhneJedeHardware) {
    NumaPageTestTree tree("fixture");
    ASSERT_TRUE(tree.ready());
    cem::NumaPageProbeContext context{};
    context.numa_node_root = tree.make_numa_root("0-1\n");
    context.hugepage_root  = tree.make_hugepage_root({{"hugepages-2048kB", "3\n"}, {"hugepages-1048576kB", "0\n"}});

    auto const nodes = cem::detail::numa_page_collect_nodes(context);
    ASSERT_TRUE(nodes.has_value());
    EXPECT_EQ(*nodes, (std::vector<std::uint32_t>{0, 1}));

    auto const pages = cem::detail::numa_page_collect_pages(context, 4096);
    ASSERT_TRUE(pages.has_value());
    ASSERT_EQ(pages->size(), 3U);
    EXPECT_EQ((*pages)[0].availability, cem::PageSizeAvailability::Basis);
    EXPECT_EQ((*pages)[1].availability, cem::PageSizeAvailability::PoolBelegt);
    EXPECT_EQ((*pages)[2].availability, cem::PageSizeAvailability::NurUnterstuetzt);
}

TEST(Od10NumaPageProbe, FehlendeHugepageWurzelIstDieVollstaendigeAntwortNurBasisSeite) {
    NumaPageTestTree tree("keine_hugepages");
    ASSERT_TRUE(tree.ready());
    cem::NumaPageProbeContext context{};
    context.numa_node_root = tree.make_numa_root("0\n");
    context.hugepage_root  = tree.child("gibt_es_nicht");

    auto const pages = cem::detail::numa_page_collect_pages(context, 4096);
    ASSERT_TRUE(pages.has_value()) << "Ein Kernel ohne Hugepage-Baum ist kein Zugangsfehler.";
    ASSERT_EQ(pages->size(), 1U);
    EXPECT_EQ((*pages)[0].size_bytes, 4096U);
    EXPECT_EQ((*pages)[0].availability, cem::PageSizeAvailability::Basis);
}

// -- Block C (runtime): jede K4-Fehlerklasse einzeln, plus der L6-Producer -------------------------

TEST(Od10NumaPageProbe, FehlendeOnlineQuelleBleibtQuelleFehlt) {
    NumaPageTestTree tree("numa_fehlt");
    ASSERT_TRUE(tree.ready());
    cem::NumaPageProbeContext context{};
    context.numa_node_root = tree.child("nicht_vorhanden");
    context.hugepage_root  = tree.child("egal");

    auto const nodes = cem::detail::numa_page_collect_nodes(context);
    expect_hardware_error(nodes, cem::HardwareProbeErrorClass::QuelleFehlt, "quelle_fehlt");
}

TEST(Od10NumaPageProbe, VorhandeneAberLeereOnlineQuelleBleibtQuelleKorrupt) {
    NumaPageTestTree tree("numa_leer");
    ASSERT_TRUE(tree.ready());
    cem::NumaPageProbeContext context{};
    context.numa_node_root = tree.make_numa_root("   \n");
    context.hugepage_root  = tree.child("egal");

    auto const nodes = cem::detail::numa_page_collect_nodes(context);
    expect_hardware_error(nodes, cem::HardwareProbeErrorClass::QuelleKorrupt, "quelle_korrupt");
    // A4-Kern: vorhanden und korrupt darf nie als fehlend degradiert werden.
    auto const* error = std::get_if<cem::HardwareProbeErrorClass>(&nodes.error());
    ASSERT_NE(error, nullptr);
    EXPECT_NE(*error, cem::HardwareProbeErrorClass::QuelleFehlt);
}

TEST(Od10NumaPageProbe, FremdesZeichenInDerListeBleibtFormatUnbekannt) {
    expect_parser_class(cem::detail::numa_page_parse_id_list("0 1 2"), cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_page_parse_id_list("node0"), cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_page_parse_id_list("0;1"), cem::HardwareProbeErrorClass::FormatUnbekannt);
}

TEST(Od10NumaPageProbe, StrukturbruecheInDerListeBleibenQuelleKorrupt) {
    // Absteigender Bereich, Ueberlappung, Komma am Ende, Deckel-Ueberschreitung, Ueberlauf.
    expect_parser_class(cem::detail::numa_page_parse_id_list("3-1"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_parse_id_list("0-3,2"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_parse_id_list("0,"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_parse_id_list("0-99999"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_parse_id_list("0-99999999999999999999999"),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Und die Gegenprobe: die zulaessige Ober-Id selbst wird NICHT abgewiesen.
    auto const grenzfall = cem::detail::numa_page_parse_id_list(std::to_string(cem::kMaxNumaNodeId));
    ASSERT_TRUE(grenzfall.has_value());
    EXPECT_EQ(grenzfall->front(), cem::kMaxNumaNodeId);
}

TEST(Od10NumaPageProbe, FremderHugepageVerzeichnisnameBleibtFormatUnbekannt) {
    expect_parser_class(cem::detail::numa_page_parse_hugepage_dir_name("free_hugepages"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_page_parse_hugepage_dir_name("hugepages-2048MB"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_page_parse_hugepage_dir_name("hugepages-2048kBx"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    // Und die STRENGE-Gegenprobe an der Wurzel: ein fremder Eintrag wird NICHT still uebersprungen.
    NumaPageTestTree tree("fremder_knoten");
    ASSERT_TRUE(tree.ready());
    cem::NumaPageProbeContext context{};
    context.hugepage_root = tree.make_hugepage_root({{"hugepages-2048kB", "0\n"}, {"etwas_anderes", "0\n"}});
    auto const pages      = cem::detail::numa_page_collect_pages(context, 4096);
    expect_hardware_error(pages, cem::HardwareProbeErrorClass::FormatUnbekannt, "format_unbekannt");
}

TEST(Od10NumaPageProbe, UnbrauchbarerHugepageInhaltBleibtQuelleKorrupt) {
    // Nicht-Zweierpotenz und Null-Groesse am Namens-Parser.
    expect_parser_class(cem::detail::numa_page_parse_hugepage_dir_name("hugepages-3000kB"),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_parse_hugepage_dir_name("hugepages-0kB"),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Huge-Seite nicht groesser als die Basis-Seite -- keine Huge-Seite, sondern ein kaputter Baum.
    std::vector<cem::detail::HugepageNode> const zu_klein = {{4096, 0}};
    expect_parser_class(cem::detail::numa_page_compose_capabilities(4096, zu_klein),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Zwei Knoten fuer dieselbe Groesse.
    std::vector<cem::detail::HugepageNode> const doppelt = {{std::uint64_t{2} << 20, 0}, {std::uint64_t{2} << 20, 1}};
    expect_parser_class(cem::detail::numa_page_compose_capabilities(4096, doppelt),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    // Basis-Seite 0 bzw. keine Zweierpotenz ('n/a statt Null': 0 ist nie 'die kleinste Seite').
    expect_parser_class(cem::detail::numa_page_compose_capabilities(0, {}),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_compose_capabilities(3000, {}),
                        cem::HardwareProbeErrorClass::QuelleKorrupt);
}

TEST(Od10NumaPageProbe, KaputterPoolZaehlerWirdKlassifiziertUndNullIstEinGueltigerWert) {
    auto const null = cem::detail::numa_page_parse_pool_count("0\n");
    ASSERT_TRUE(null.has_value()) << "nr_hugepages=0 ist die Stufe NurUnterstuetzt, kein Fehler.";
    EXPECT_EQ(*null, 0U);
    expect_parser_class(cem::detail::numa_page_parse_pool_count("  \n"), cem::HardwareProbeErrorClass::QuelleKorrupt);
    expect_parser_class(cem::detail::numa_page_parse_pool_count("viele"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
    expect_parser_class(cem::detail::numa_page_parse_pool_count("3 seiten"),
                        cem::HardwareProbeErrorClass::FormatUnbekannt);
}

TEST(Od10NumaPageProbe, JedeFremdeFamilieErzeugtDenBenanntenL6Befund) {
    cem::NumaPageProbeContext const context      = cem::default_numa_page_probe_context();
    std::size_t                     native_count = 0;
    verify_native_or_l6_producer<cem::LinuxOperatingSystem>(context, native_count);
    verify_native_or_l6_producer<cem::WindowsOperatingSystem>(context, native_count);
    verify_native_or_l6_producer<cem::MacosOperatingSystem>(context, native_count);
    EXPECT_EQ(native_count, 1U) << "Die Familien-Gegenprobe muss genau eine native und zwei fremde Zellen sehen.";
}

// -- Block D (runtime): die Live-Maschine, mit benanntem Skip-Guard --------------------------------

TEST(Od10NumaPageProbe, LinuxRealLiefertBeideAchsenUndStimmtMitEinerZweitLesungUeberein) {
    if constexpr (!LinuxProbe::has_native_probe()) {
        GTEST_SKIP() << "Die Live-Erhebung ist nur in der nativen Linux-Zelle erreichbar.";
    } else {
        auto const online =
            read_trimmed_file(fs::path{cem::kDefaultNumaNodeRoot} / std::string{cem::kNumaOnlineLeafName});
        if (!online.has_value()) {
            GTEST_SKIP() << "Benannter Skip-Guard: der sysfs-Knoten-Baum ist auf diesem Host nicht lesbar.";
        }

        auto const topology = LinuxProbe::collect(cem::default_numa_page_probe_context());
        ASSERT_TRUE(topology.numa_nodes.has_value()) << "Live-Erhebung der Knoten fehlgeschlagen: "
                                                     << cem::numa_page_probe_error_label(topology.numa_nodes.error());
        EXPECT_FALSE(topology.numa_nodes->empty()) << "'n/a statt Null': eine leere Erfolgs-Menge darf es nie geben.";

        // Unabhaengige Zweit-Lesung: die Knoten-ZAHL aus derselben Datei, aber ohne den Produktions-Parser.
        std::size_t erwartete_knoten = 1;
        for (char const c : *online)
            if (c == ',') ++erwartete_knoten;
        if (online->find('-') == std::string::npos)
            EXPECT_EQ(topology.numa_nodes->size(), erwartete_knoten)
                << "online=" << *online << " ergibt eine andere Knoten-Zahl als die Erhebung.";

        ASSERT_TRUE(topology.page_sizes.has_value()) << "Live-Erhebung der Seiten fehlgeschlagen: "
                                                     << cem::numa_page_probe_error_label(topology.page_sizes.error());
        ASSERT_FALSE(topology.page_sizes->empty());
        EXPECT_EQ(topology.page_sizes->front().availability, cem::PageSizeAvailability::Basis);
#if defined(__linux__)
        EXPECT_EQ(topology.page_sizes->front().size_bytes, static_cast<std::uint64_t>(::sysconf(_SC_PAGESIZE)))
            << "Die Basis-Seite muss der unabhaengig gelesenen sysconf-Groesse entsprechen.";
#endif
        // Aufsteigend und duplikatfrei -- die Werte-Menge einer Achse ist geordnet, nicht irgendwie.
        for (std::size_t i = 1; i < topology.page_sizes->size(); ++i)
            EXPECT_LT((*topology.page_sizes)[i - 1].size_bytes, (*topology.page_sizes)[i].size_bytes);
    }
}

// -- Block E (runtime): A-15-Stempel-Neutralitaet --------------------------------------------------

TEST(Od10NumaPageProbe, LaufzeitwerteBleibenAusDemSystemStempelUndDerCodeStandBleibtV1) {
    bool code_version_found = false;
    for (auto const& entry : cabi::kSystemAxisCodeVersions) {
        if (entry.axis != cem::NumaNodeSubAxis::parent_axis_label()) continue;
        code_version_found = true;
        // A-15: die RT-Erhebung ist rein additiv -- sie bumpt den target_isa-Code-Stand NICHT.
        EXPECT_EQ(entry.version, std::string_view{"v1.0.0c"})
            << "A-15: RT-Unter-Achsen duerfen den target_isa-Code-Stand nicht bumpen.";
    }
    ASSERT_TRUE(code_version_found) << "Die Eltern-Achse target_isa muss im Code-Versions-Register stehen.";

    std::string const stamp = cabi::system_stamp_line();
    // Weder die Verfahrens-ID noch ihr Praefix duerfen je im Stempel auftauchen.
    EXPECT_EQ(stamp.find(std::string{LinuxProbe::probe_id()}), std::string::npos)
        << "A-15-Bruch: die probe_id steht im System-Stempel: " << stamp;
    EXPECT_EQ(stamp.find("numa_page_probe."), std::string::npos)
        << "A-15-Bruch: das Verfahrens-Praefix steht im System-Stempel: " << stamp;

    if constexpr (LinuxProbe::has_native_probe()) {
        auto const topology = LinuxProbe::collect(cem::default_numa_page_probe_context());
        if (topology.page_sizes.has_value() && !topology.page_sizes->empty()) {
            // Die Basis-Seitengroesse als DEZIMAL-Zeichenkette: sie ist der einzige erhobene Wert, der
            // ueberhaupt in einer Stempel-Zeile stehen KOENNTE, und sie steht dort nicht.
            EXPECT_EQ(stamp.find("=" + std::to_string(topology.page_sizes->front().size_bytes)), std::string::npos)
                << "A-15-Bruch: die erhobene Basis-Seitengroesse steht im System-Stempel: " << stamp;
        }
    }
}
