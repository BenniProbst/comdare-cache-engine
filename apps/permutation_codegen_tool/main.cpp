// SPDX-License-Identifier: Apache-2.0
// #25 Teil B — comdare-permutation-codegen CLI-Wrapper (C++23-Port von codegen.cmake)
//
// NEUES opt-in-Backend `cpp` fuer den Permutations-Codegen. Erzeugt fuer dieselben Eingaben
// (COMDARE_TARGET_ISA × COMDARE_PROFILE × COMDARE_MODE × COMDARE_OUTPUT) eine BYTE-IDENTISCHE
// permutations.cmake wie tools/permutation_codegen/codegen.cmake (Default-Backend `cmake`).
//
// Das Default-Backend bleibt UNVERAENDERT `cmake`; dieses Tool wird nur aktiv, wenn der User
// COMDARE_PERMUTATION_CODEGEN_BACKEND=cpp setzt (cmake/permutations.cmake, elseif "cpp"-Zweig).
//
// Aufruf (spiegelt codegen.cmake / codegen.sh):
//   comdare-permutation-codegen --target-isa auto --profile smoke
//       --mode on_build_on_demand --output <path/to/permutations.cmake>
// Beide Argument-Formen werden akzeptiert: "--key value" UND "--key=value".

#include <builder/permutation_codegen_tool/permutation_codegen_tool.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace pc = ::comdare::cache_engine::builder::permutation_codegen;

namespace {

void print_usage() {
    std::cerr << "Usage: comdare-permutation-codegen [options]\n"
                 "\n"
                 "C++23-Port von tools/permutation_codegen/codegen.cmake (opt-in Backend 'cpp').\n"
                 "Erzeugt eine byte-identische permutations.cmake + permutations_manifest.txt.\n"
                 "\n"
                 "Options (jeweils '--key value' oder '--key=value'):\n"
                 "  --target-isa ISA     Ziel-ISA (auto|scalar|sse4|avx2|avx512|neon). Default: auto\n"
                 "  --profile P          Profile-Filter (smoke|medium|full). Default: smoke\n"
                 "  --mode M             on_build_on_demand|on_rebuild|off_pause_build. Default: on_build_on_demand\n"
                 "  --output FILE        Pfad der zu erzeugenden permutations.cmake (erforderlich)\n"
                 "  --help               Diese Hilfe anzeigen und beenden\n";
}

// Liefert den Wert fuer eine Option: entweder aus "--key=value" (im selben Token) oder aus dem
// naechsten argv-Token ("--key value"). Erhoeht i im Zwei-Token-Fall.
[[nodiscard]] std::optional<std::string> take_value(std::string_view token, std::string_view key, int& i, int argc,
                                                    char** argv) {
    if (token == key) {
        if (i + 1 >= argc) return std::nullopt;
        return std::string{argv[++i]};
    }
    // "--key=value"
    std::string const prefix = std::string{key} + "=";
    if (token.size() > prefix.size() && token.substr(0, prefix.size()) == prefix) {
        return std::string{token.substr(prefix.size())};
    }
    return std::nullopt;
}

} // namespace

int main(int argc, char** argv) {
    pc::Inputs  inputs;
    std::string profile_raw = "smoke";
    bool        have_output = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        if (a == "--help") {
            print_usage();
            return 0;
        }
        if (auto v = take_value(a, "--target-isa", i, argc, argv)) {
            inputs.target_isa = *v;
            continue;
        }
        if (auto v = take_value(a, "--profile", i, argc, argv)) {
            profile_raw = *v;
            continue;
        }
        if (auto v = take_value(a, "--mode", i, argc, argv)) {
            inputs.mode = *v;
            continue;
        }
        if (auto v = take_value(a, "--output", i, argc, argv)) {
            inputs.output = *v;
            have_output   = true;
            continue;
        }
        std::cerr << "comdare-permutation-codegen: unbekanntes oder unvollstaendiges Argument: " << a << "\n\n";
        print_usage();
        return 2;
    }

    if (!have_output) {
        std::cerr << "comdare-permutation-codegen: FEHLER: --output erforderlich (Pfad zur permutations.cmake).\n\n";
        print_usage();
        return 2;
    }

    std::optional<pc::Profile> const profile = pc::parse_profile(profile_raw);
    if (!profile) {
        std::cerr << "comdare-permutation-codegen: FEHLER: unbekanntes --profile: " << profile_raw
                  << " (smoke|medium|full).\n";
        return 2;
    }
    inputs.profile = *profile;

    std::string error;
    if (!pc::generate(inputs, &error)) {
        std::cerr << "comdare-permutation-codegen: FEHLER: " << error << "\n";
        return 1;
    }

    auto const perms = pc::enumerate_permutations(inputs.profile, inputs.target_isa);
    std::cout << "comdare-permutation-codegen: " << perms.size()
              << " Permutationen (Profile=" << pc::profile_name(inputs.profile) << ", ISA=" << inputs.target_isa
              << ") -> " << inputs.output.string() << "\n";
    return 0;
}
