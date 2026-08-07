// Paket OS-U3 -- Laufzeit-Erhebung der drei operating_system-Unter-Achsen je OS-Familie.
//
// ZWECK: Block A beweist CT-Dispatch, Totalitaets-Wache und K5-Versionierung. Block B liest die
// Linux-Werte real und unabhaengig gegen uname(2), procfs und einen zweiten os-release-Parser. Block C
// belegt die K4-Fehlerdomaenen einschliesslich des ersten L6-Producers fuer
// BetriebssystemFeatureFehlt. Block D prueft A-15: kein erhobener RT-Wert gelangt in den Binary-Stempel.
//
// WARUM EIN EIGENER TEST: Die Familien-Wahl ist ein Template-Typ und die Werte sind RT-Daten. Nur diese
// Kombination kann zugleich beweisen, dass keine Runtime-Familienwahl eingefuehrt wurde und dass die
// prozessfreien Quellen auf dem laufenden Host echte, nicht statisch gepinnte Werte liefern.
//
// K2/A-15: Der Test startet ebenfalls keinen Prozess. Er nutzt nur uname(2) und Datei-Reads. Die
// Stempel-Gegenprobe liest den bestehenden Stempel, schreibt aber weder Registry noch ABI-Daten.

#include "comdare_test_tmp.hpp"

#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/system_axis_code_versions.hpp>
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/operating_system_probe.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#if defined(__linux__)
#include <sys/utsname.h>
#endif

namespace cem  = ::comdare::cache_engine::measurement;
namespace cabi = ::comdare::cache_engine::abi;
namespace fs   = std::filesystem;

using LinuxProbe   = cem::OperatingSystemProbe<cem::LinuxOperatingSystem>;
using WindowsProbe = cem::OperatingSystemProbe<cem::WindowsOperatingSystem>;
using MacosProbe   = cem::OperatingSystemProbe<cem::MacosOperatingSystem>;

namespace {

// -- Block A (compile-time): CT-Dispatch, Totalitaets-Wache und K5-Versionierung -------------------

[[nodiscard]] constexpr std::size_t count_character(std::string_view text, char needle) noexcept {
    std::size_t count = 0;
    for (char const c : text)
        if (c == needle) ++count;
    return count;
}

template <class OsAxis>
[[nodiscard]] consteval bool probe_id_contract() noexcept {
    constexpr std::string_view prefix = "os_probe.";
    constexpr std::string_view id     = cem::OperatingSystemProbe<OsAxis>::probe_id();
    if (!id.starts_with(prefix) || count_character(id, '@') != 1U) return false;

    std::size_t const separator = id.find('@');
    if (separator <= prefix.size()) return false;
    if (id.substr(prefix.size(), separator - prefix.size()) != OsAxis::os_family_id()) return false;

    std::string_view const version = cem::os_probe_version_part(id);
    cem::AlgoSemVer const  parsed  = cem::parse_algo_semver(version);
    // FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026): hier standen bis zum Umbau zwei weitere Terme --
    // `!parsed.experimental` und `version.find('e') == npos`. BEIDE SIND ERSATZLOS ENTFALLEN: 'e' bedeutet
    // ab jetzt EFFICIENCY CORE und ist ein legitimes Flag. Der zweite Term war ausserdem schon vorher zu
    // grob -- er haette jedes Flag-Token mit einem 'e' darin verworfen.
    return cem::ce_owned_version_is_wellformed(version) && !parsed.is_sentinel();
}

struct FantasieAchse;

template <class T>
concept CompleteType = requires { sizeof(T); };

static_assert(cem::OperatingSystemProbeConcept<LinuxProbe>);
static_assert(cem::OperatingSystemProbeConcept<WindowsProbe>);
static_assert(cem::OperatingSystemProbeConcept<MacosProbe>);
static_assert(std::same_as<typename LinuxProbe::os_axis, cem::LinuxOperatingSystem>);
static_assert(std::same_as<typename WindowsProbe::os_axis, cem::WindowsOperatingSystem>);
static_assert(std::same_as<typename MacosProbe::os_axis, cem::MacosOperatingSystem>);
static_assert(!std::same_as<LinuxProbe, WindowsProbe> && !std::same_as<LinuxProbe, MacosProbe> &&
              !std::same_as<WindowsProbe, MacosProbe>);
static_assert(!CompleteType<cem::OperatingSystemProbe<FantasieAchse>>,
              "Das primaere OperatingSystemProbe-Template muss als Totalitaets-Wache unvollstaendig bleiben.");

static_assert(probe_id_contract<cem::LinuxOperatingSystem>());
static_assert(probe_id_contract<cem::WindowsOperatingSystem>());
static_assert(probe_id_contract<cem::MacosOperatingSystem>());
static_assert(LinuxProbe::probe_id() == std::string_view{"os_probe.linux@1.0.0.c"});
static_assert(WindowsProbe::probe_id() == std::string_view{"os_probe.windows@1.0.0.c"});
static_assert(MacosProbe::probe_id() == std::string_view{"os_probe.macos@1.0.0.c"});
static_assert(LinuxProbe::probe_id() != WindowsProbe::probe_id() && LinuxProbe::probe_id() != MacosProbe::probe_id() &&
              WindowsProbe::probe_id() != MacosProbe::probe_id());

constexpr std::size_t kNativeProbeCount = static_cast<std::size_t>(LinuxProbe::has_native_probe()) +
                                          static_cast<std::size_t>(WindowsProbe::has_native_probe()) +
                                          static_cast<std::size_t>(MacosProbe::has_native_probe());
static_assert(kNativeProbeCount == 1U,
              "Genau die Familie der Bauplattform muss nativ sein; die beiden fremden Zellen bleiben ehrlich.");

// -- Gemeinsame Test-Helfer ----------------------------------------------------------------------

[[nodiscard]] std::string trim_copy(std::string_view text) {
    auto const is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
    while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
    return std::string{text};
}

/// Zweiter, absichtlich unabhaengiger Minimal-Parser fuer ID aus /etc/os-release. Er ruft keinen
/// Produktions-Helfer auf: sonst koennten Erhebung und Gegenprobe denselben Quote-/Key-Fehler teilen.
[[nodiscard]] std::optional<std::string> independently_read_os_release_id(fs::path const& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;

    std::string line;
    while (std::getline(input, line)) {
        std::string const clean = trim_copy(line);
        if (clean.empty() || clean.front() == '#') continue;
        std::size_t const separator = clean.find('=');
        if (separator == std::string::npos || trim_copy(std::string_view{clean}.substr(0, separator)) != "ID") continue;

        std::string value = trim_copy(std::string_view{clean}.substr(separator + 1));
        if (value.size() >= 2U &&
            ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2U);
        }
        if (!value.empty()) return value;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> independently_read_trimmed_file(fs::path const& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;
    std::string content;
    std::getline(input, content, '\0');
    if (input.bad()) return std::nullopt;
    content = trim_copy(content);
    if (content.empty()) return std::nullopt;
    return content;
}

/// comdare_test_tmp.hpp stellt die per-User-Wurzel bereit. Diese RAII-Schicht gibt jedem Test einen
/// eigenen Baum und entfernt ihn im Destruktor, damit parallele CI-Laeufe keine alten Quellen sehen.
class OsProbeTestTree {
public:
    explicit OsProbeTestTree(std::string_view name)
        : root_(::comdare::test::user_tmp_dir() /
                ("os_u3_" + std::string{name} + "_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::error_code ec;
        fs::create_directories(root_, ec);
        ready_ = !ec;
    }

    OsProbeTestTree(OsProbeTestTree const&)            = delete;
    OsProbeTestTree& operator=(OsProbeTestTree const&) = delete;

    ~OsProbeTestTree() {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    [[nodiscard]] bool     ready() const noexcept { return ready_; }
    [[nodiscard]] fs::path child(std::string_view name) const { return root_ / std::string{name}; }

    [[nodiscard]] fs::path write(std::string_view name, std::string_view content) const {
        fs::path const path = child(name);
        std::ofstream  output(path, std::ios::binary);
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        return path;
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
    EXPECT_EQ(cem::os_probe_error_label(result.error()), label);
}

template <class OsAxis>
void verify_native_or_l6_producer(cem::OperatingSystemProbeContext const& context, std::size_t& native_count) {
    using Probe = cem::OperatingSystemProbe<OsAxis>;
    if constexpr (Probe::has_native_probe()) {
        ++native_count;
    } else {
        auto const result = Probe::collect(context);
        ASSERT_FALSE(result.has_value()) << Probe::probe_id();
        auto const* error = std::get_if<cem::CompilerCompilerErrorClass>(&result.error());
        ASSERT_NE(error, nullptr) << Probe::probe_id();
        EXPECT_EQ(*error, cem::CompilerCompilerErrorClass::BetriebssystemFeatureFehlt);
        EXPECT_EQ(cem::error_domain(result.error()), cem::ErrorDomain::CompilerCompiler);
        EXPECT_EQ(cem::os_probe_error_label(result.error()), std::string_view{"betriebssystem_feature_fehlt"});
        EXPECT_FALSE(Probe::has_native_probe());
        EXPECT_FALSE(Probe::not_implemented_reason().empty())
            << "Eine fremde Familien-Zelle muss ihren ehrlichen Nicht-Implementierungsgrund benennen.";
    }
}

template <class OsAxis>
void verify_native_values_are_not_stamped(std::size_t& native_count) {
    using Probe = cem::OperatingSystemProbe<OsAxis>;
    if constexpr (Probe::has_native_probe()) {
        ++native_count;
        auto const result = Probe::collect(cem::default_operating_system_probe_context());
        ASSERT_TRUE(result.has_value()) << "Die native Familie muss fuer die A-15-Gegenprobe erhebbar sein.";
        ASSERT_FALSE(result->kernel.empty());
        ASSERT_FALSE(result->os_version.empty());
        std::string const stamp = cabi::system_stamp_line();
        EXPECT_EQ(stamp.find(result->kernel), std::string::npos)
            << "A-15-Bruch: der erhobene Kernel steht im System-Stempel: " << stamp;
        EXPECT_EQ(stamp.find(result->os_version), std::string::npos)
            << "A-15-Bruch: die erhobene OS-Version steht im System-Stempel: " << stamp;
    }
}

} // namespace

// -- Block B (runtime): Linux real, Werte nie statisch gepinnt ------------------------------------

TEST(OsU3Probe, LinuxRealLiefertDreiWerteUndStimmtMitUnameUndOsReleaseUeberein) {
#if defined(__linux__)
    auto const result = LinuxProbe::collect(cem::default_operating_system_probe_context());
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->os_version.empty());
    EXPECT_FALSE(result->kernel.empty());
    EXPECT_FALSE(result->build.empty());
    EXPECT_EQ(result->os_version.find('"'), std::string::npos);
    EXPECT_EQ(result->os_version.find('\''), std::string::npos);

    struct utsname live_uts{};
    ASSERT_EQ(::uname(&live_uts), 0) << "Der unabhaengige uname(2)-Read fuer die Gegenprobe ist fehlgeschlagen.";
    EXPECT_EQ(result->kernel, std::string{live_uts.release});

    auto const id = independently_read_os_release_id(fs::path{"/etc/os-release"});
    ASSERT_TRUE(id.has_value())
        << "Der unabhaengige Test-Parser konnte ID aus /etc/os-release nicht lesen; die Gegenprobe waere leer.";
    EXPECT_TRUE(result->os_version == *id || result->os_version.starts_with(*id + "-"))
        << "os_version muss mit dem unabhaengig gelesenen ID beginnen: ID=" << *id
        << ", os_version=" << result->os_version;
#else
    GTEST_SKIP() << "Linux-Realtest benoetigt __linux__; die fremde Zelle wird in Block C geprueft.";
#endif
}

TEST(OsU3Probe, LinuxKernelStimmtMitProcfsUebereinWennDieQuelleLesbarIst) {
#if defined(__linux__)
    auto const proc_release = independently_read_trimmed_file(fs::path{"/proc/sys/kernel/osrelease"});
    if (!proc_release.has_value()) {
        GTEST_SKIP() << "Benannter Skip-Guard: /proc/sys/kernel/osrelease ist auf diesem Linux nicht lesbar.";
    }

    auto const result = LinuxProbe::collect(cem::default_operating_system_probe_context());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->kernel, *proc_release);
#else
    GTEST_SKIP() << "Procfs-Gegenprobe benoetigt __linux__; keine Linux-Live-Quelle auf dieser Plattform.";
#endif
}

// -- Block C (runtime): K4-Fehlerdomaenen und der L6-Producer -------------------------------------

TEST(OsU3Probe, FehlendeOsReleaseKetteBleibtQuelleFehlt) {
    if constexpr (!LinuxProbe::has_native_probe()) {
        GTEST_SKIP() << "Die os-release-Zugangsprobe ist nur in der nativen Linux-Zelle erreichbar.";
    } else {
        OsProbeTestTree tree("missing");
        ASSERT_TRUE(tree.ready());
        cem::OperatingSystemProbeContext context = cem::default_operating_system_probe_context();
        context.os_release_path                  = tree.child("nicht_vorhanden_primary");
        context.os_release_fallback_path         = tree.child("nicht_vorhanden_fallback");

        auto const result = LinuxProbe::collect(context);
        expect_hardware_error(result, cem::HardwareProbeErrorClass::QuelleFehlt, "quelle_fehlt");
    }
}

TEST(OsU3Probe, VorhandeneAberInhaltsloseOsReleaseQuelleBleibtQuelleKorrupt) {
    if constexpr (!LinuxProbe::has_native_probe()) {
        GTEST_SKIP() << "Die os-release-Zugangsprobe ist nur in der nativen Linux-Zelle erreichbar.";
    } else {
        OsProbeTestTree tree("corrupt");
        ASSERT_TRUE(tree.ready());
        fs::path const corrupt = tree.write("os-release", "\n# nur ein Kommentar\n   # noch ein Kommentar\n\n");
        ASSERT_TRUE(fs::exists(corrupt));

        cem::OperatingSystemProbeContext context = cem::default_operating_system_probe_context();
        context.os_release_path                  = corrupt;
        context.os_release_fallback_path         = tree.child("nicht_vorhanden_fallback");

        auto const result = LinuxProbe::collect(context);
        expect_hardware_error(result, cem::HardwareProbeErrorClass::QuelleKorrupt, "quelle_korrupt");
        if (result.has_value()) return;
        auto const* error = std::get_if<cem::HardwareProbeErrorClass>(&result.error());
        ASSERT_NE(error, nullptr);
        EXPECT_NE(*error, cem::HardwareProbeErrorClass::QuelleFehlt)
            << "A4-Kern: vorhanden und korrupt darf nie als fehlend degradiert werden.";
    }
}

TEST(OsU3Probe, JedeFremdeFamilieErzeugtDenBenanntenL6Befund) {
    cem::OperatingSystemProbeContext const context      = cem::default_operating_system_probe_context();
    std::size_t                            native_count = 0;
    verify_native_or_l6_producer<cem::LinuxOperatingSystem>(context, native_count);
    verify_native_or_l6_producer<cem::WindowsOperatingSystem>(context, native_count);
    verify_native_or_l6_producer<cem::MacosOperatingSystem>(context, native_count);
    EXPECT_EQ(native_count, 1U) << "Die Familien-Gegenprobe muss genau eine native und zwei fremde Zellen sehen.";
}

// -- Block D (runtime): A-15-Stempel-Neutralitaet ------------------------------------------------

TEST(OsU3Probe, LaufzeitwerteBleibenAusDemSystemStempelUndDerCodeStandBleibtV1) {
    bool code_version_found = false;
    for (auto const& entry : cabi::kSystemAxisCodeVersions) {
        if (entry.axis != cem::LinuxOperatingSystem::axis_label()) continue;
        code_version_found = true;
        // A13-M3/C4: die Migration zieht das HW-Flag an, sie BUMPT den Stand nicht (1.0.0 bleibt 1.0.0).
        EXPECT_EQ(entry.version, std::string_view{"1.0.0.c"})
            << "A-15: RT-Unter-Achsen duerfen den operating_system-Code-Stand nicht bumpen.";
    }
    ASSERT_TRUE(code_version_found);

    std::size_t native_count = 0;
    verify_native_values_are_not_stamped<cem::LinuxOperatingSystem>(native_count);
    verify_native_values_are_not_stamped<cem::WindowsOperatingSystem>(native_count);
    verify_native_values_are_not_stamped<cem::MacosOperatingSystem>(native_count);
    EXPECT_EQ(native_count, 1U);
}
