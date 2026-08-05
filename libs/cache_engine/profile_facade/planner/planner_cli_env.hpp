#pragma once
// profile_facade/planner/planner_cli_env.hpp -- die geteilten Env-/String-Primitiven der Planer-CLI (W1).
//
// W1 (F1-HART, Owner-KERN 05.08.2026): der Experiment-Planer wird eine EIGENE Binary
// (apps/experiment_planner). Beim Schnitt an der Director-Rollen-Naht brauchen BEIDE Hosts -- die neue
// Planer-App (fingerprint-Fenster, Profil-Aufloesung) und der verbleibende Treiber/CEB
// (super Code/02_messung_driver/main.cpp, run-Pfad) -- dieselben vier Bausteine. Sie standen bis W1
// ausschliesslich treiber-lokal (main.cpp trim_copy/env_trimmed + GoldenRange/parse_golden_range_env);
// eine Kopie in der neuen App waere die Sorte Doppel-Ableitung, die der Modulschnitt gerade verbietet
// (ZWEI MODULE, KEINE VERERBUNG, aber auch KEIN Substanz-Duplikat). Deshalb: HOIST an die eine Stelle,
// die beide Hosts ohnehin konsumieren -- die ce-Planer-Welt.
//
// ANSPRUCHSLOS (Section 62-A): header-only, NUR stdlib, keine Umbrella-/Katalog-Kette, keine neuen
// Pflicht-Envs. Semantik BYTE-IDENTISCH zum Treiber-Stand vor W1 (die Funktionsrumpfe sind wortgleich
// uebernommen, inklusive der Fail-loud-Meldungstexte -- ein geaenderter Text waere ein stiller
// Verhaltens-Bruch der bestehenden CI-Diagnose).
//
// FEHLERKLASSEN-DOKTRIN: parse_golden_range_env wirft bei Fehlform, statt still auf die Voll-View zu
// fallen (ein Tippfehler in COMDARE_GOLDEN_N_RANGE wuerde sonst den ganzen 2^17-Bau statt eines Chunks
// starten). Der Aufrufer faengt und meldet die Klasse (rc 2 in beiden Hosts).

#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace comdare::cache_engine::planner {

/// Randstaendigen Whitespace abschneiden (Env-Werte aus CI-YAML tragen haeufig Zeilenumbrueche).
[[nodiscard]] inline std::string trim_copy(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0) s.remove_suffix(1);
    return std::string{s};
}

/// Eine Umgebungsvariable getrimmt lesen; ungesetzt => leerer String (kein optional -- die Hosts
/// behandeln "ungesetzt" und "leer" ueberall gleich).
[[nodiscard]] inline std::string env_trimmed(char const* name) {
    if (char const* e = std::getenv(name); e != nullptr) return trim_copy(e);
    return {};
}

// INC-G6 (Ledger 33/34, 2026-07-19): das golden-N Chunk-Fenster. COMDARE_GOLDEN_N_RANGE="start:count" ->
// {start, count}. Leer/ungesetzt = nullopt (kein Fenster, Ist-Verhalten). Fail-loud bei Fehlform.
// count==0 ist syntaktisch gueltig (deaktiviert das Fenster).
struct GoldenRange {
    std::size_t start = 0;
    std::size_t count = 0;
};

[[nodiscard]] inline std::optional<GoldenRange> parse_golden_range_env() {
    std::string const s = env_trimmed("COMDARE_GOLDEN_N_RANGE");
    if (s.empty()) return std::nullopt;
    std::size_t const colon = s.find(':');
    if (colon == std::string::npos)
        throw std::runtime_error("COMDARE_GOLDEN_N_RANGE erwartet 'start:count': '" + s + "'");
    auto const parse_u = [&s](std::string_view part) -> std::size_t {
        std::uint64_t v      = 0;
        auto const [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), v, 10);
        if (part.empty() || ec != std::errc{} || ptr != part.data() + part.size() ||
            v > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)()))
            throw std::runtime_error("COMDARE_GOLDEN_N_RANGE ungueltig: '" + s + "'");
        return static_cast<std::size_t>(v);
    };
    std::string_view const sv{s};
    return GoldenRange{parse_u(sv.substr(0, colon)), parse_u(sv.substr(colon + 1))};
}

/// Eine nicht-negative Groesse strikt aus der Env lesen (leer => nullopt, Fehlform => Wurf).
/// Muster/Meldungstext wortgleich zum Treiber-Stand (parse_size_env_strict); der planer_block-Kontext
/// der Planer-App liest damit COMDARE_BUILD_PARALLEL.
[[nodiscard]] inline std::optional<std::size_t> parse_size_env_strict(char const* name) {
    std::string const s = env_trimmed(name);
    if (s.empty()) return std::nullopt;

    std::uint64_t value  = 0;
    auto const [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value, 10);
    if (ec != std::errc{} || ptr != s.data() + s.size() ||
        value > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        throw std::runtime_error(std::string{name} + " ungueltig: '" + s + "'");
    }
    return static_cast<std::size_t>(value);
}

} // namespace comdare::cache_engine::planner
