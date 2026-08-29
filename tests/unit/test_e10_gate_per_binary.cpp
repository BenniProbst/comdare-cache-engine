// tests/unit/test_e10_gate_per_binary.cpp -- E-10/ORG-19 SCHRITT 3, Koeder k8 (26.08.2026,
// Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19 (b) Schritt 3 + LEAD-AUFLAGE F-V1 Variante a).
//
// GEGENSTAND: gate_for_binary = DER EINE Helfer des per-Binary-Gate-Wegs (simd_build_gate.hpp,
// C-3a-UMSCHLAG). Aus den (achse,wert)-Paaren einer Binary (job.binary_id -> ceb_parse_path) werden
// required/meaningful per Binary aggregiert (18+1-Register, organ_meta_meta_requirement.hpp) und durch
// das Pruef-Dock zu {flags, identity_text} verdichtet -- Orchestrator-Admission, CompileFn-Naht und
// Glied-[5]-/Suffix-Kanal rechnen denselben Weg.
//
// VIER BEWEISE (je mit eigener Bruchstelle):
//   (A) PRODUKTION LEER: fuer memory_only-Achsen und JEDE Route sind flags UND identity_text leer
//       (Produktions-Register deklariert nichts; N-5/B-3 des Designplans: Rechenweg per Binary,
//       Wert unveraendert).
//   (B) PROBE-LITERALE (K13, das Register beisst): mit dem 2c-Probe-Register + expliziter
//       Prod1-Signatur liefert das disk-Traeger-Paar auf Route Avx2 EXAKT {"-mavx2"} und
//       "gate=[-mavx2]"; das memory_only-Paar liefert NICHTS. Host-unabhaengig (Signatur injiziert).
//   (C) RSP-BYTE-PROBE (F-V1a, ERSATZ der lazy==Katalog-Abnahme im S1-S4-Korridor): fuer ALLE 320
//       golden-ids und alle 3 Routen ist die neue per-Job-Fassaden-Zeile (opt + march +
//       gate_for_binary(axes, route).flags) byte-identisch zur Referenz OHNE Gate-Beitrag.
//   (D) D1-KLASSEN je einmal literal am per-Binary-Aggregat: HardwareErweiterungFehlt (avx512_fp16
//       fehlt in Prod1Zen5Signature), CompileKombination (Route Avx2 sperrt AVX-512-Tier),
//       ToolchainFehlt (Dialekt ohne Schreibweise).
//
// T-1 rot-zuerst: VOR Schritt 3a uebersetzt diese TU nicht (Symbol gate_for_binary fehlt) -- der
// Compile-Rot-Beleg liegt im Beweisort (bau/t1-rot/R5-s3-k8-compile.log).

#include "generated_source_catalog.hpp" // generated_catalog_static_levels (320er-golden-Baum)

#include <builder/experiment_tree/ceb_generator.hpp>   // ex::ceb_parse_path (binary_id -> (achse,wert)-Paare)
#include <builder/experiment_tree/experiment_tree.hpp> // ExperimentTree / StaticBinaryView / ExperimentNodeFactory

#include <cache_engine/measurement/machine_simd_signature.hpp>
#include <cache_engine/measurement/organ_meta_meta_requirement.hpp>
#include <cache_engine/measurement/simd_build_gate.hpp>
#include <cache_engine/measurement/simd_organ_requirement.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace tlz  = ::comdare::cache_engine::thesis_lazy;
namespace meas = ::comdare::cache_engine::measurement;

namespace {

int g_fail = 0;

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

// Die binary_ids des 320er-golden-Baums in kanonischer StaticBinaryView-Reihenfolge
// (Muster test_lazy_adhoc_source_gen.cpp binary_ids()).
std::vector<std::string> golden_320_ids() {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(tlz::generated_catalog_static_levels());
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::vector<std::string>   ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}

// SPIEGEL der per-Job-Fassaden-Naht SEIT E-10 Schritt 3c (profile_run_facade.cpp compile_for_perm,
// per-Job-CompileFn): EIN flags-String, opt zuerst, dann march, dann die per-Binary-Gate-Extras aus
// gate_for_binary(ceb_parse_path(binary_id), route_of_march_flag(march)).
[[nodiscard]] std::string rsp_zeile_per_binary(std::string const& opt, std::string const& march,
                                               std::vector<std::pair<std::string, std::string>> const& axes) {
    std::string flags = opt;
    if (!march.empty()) {
        flags += ' ';
        flags += march;
    }
    auto const gate = meas::gate_for_binary(axes, meas::route_of_march_flag(march));
    for (auto const& mf : gate.flags) {
        flags += ' ';
        flags += mf;
    }
    return flags;
}

// Referenz: die Zeile OHNE jeden Gate-Beitrag (der Vor-E-10-Stand fuer jede Flotten-Binary).
[[nodiscard]] std::string rsp_zeile_referenz(std::string const& opt, std::string const& march) {
    std::string flags = opt;
    if (!march.empty()) {
        flags += ' ';
        flags += march;
    }
    return flags;
}

struct RouteFall {
    char const* name;
    char const* march;
};
constexpr RouteFall kRouten[]{
    {"no_extension", ""},
    {"avx2", "-mavx2"},
    {"avx512", "-mavx512f"},
};

// (achse,wert)-Paare wie aus ceb_parse_path: einmal MIT disk-Traeger, einmal memory_only.
std::vector<std::pair<std::string, std::string>> const kDiskAxes{{"search_algo", "k_ary"},
                                                                 {"persistence_target", "persistence_disk_writeback"}};
std::vector<std::pair<std::string, std::string>> const kMemoryOnlyAxes{
    {"search_algo", "k_ary"}, {"persistence_target", "persistence_memory_only"}};

} // namespace

int main() {
    std::cout << "test_e10_gate_per_binary -- E-10 Schritt 3 / k8 (gate_for_binary = DER EINE Helfer)\n";

    // ---- (A) PRODUKTION LEER: memory_only + disk auf allen Routen ohne Beitrag (Register leer) ----
    std::cout << "\n---- (A) Produktions-Register: flags/identity_text leer fuer jede Route ----\n";
    for (auto const& r : kRouten) {
        auto const memory_only = meas::gate_for_binary(kMemoryOnlyAxes, meas::route_of_march_flag(r.march));
        auto const disk        = meas::gate_for_binary(kDiskAxes, meas::route_of_march_flag(r.march));
        check_true((std::string{"(A) memory_only flags leer, route="} + r.name).c_str(), memory_only.flags.empty());
        check_true((std::string{"(A) memory_only identity leer, route="} + r.name).c_str(),
                   memory_only.identity_text.empty());
        check_true((std::string{"(A) disk flags leer (required LEER, KN-1), route="} + r.name).c_str(),
                   disk.flags.empty());
        check_true((std::string{"(A) disk identity leer, route="} + r.name).c_str(), disk.identity_text.empty());
    }
    // Die per-Binary-Hooks (3a) sind am Produktions-Register ebenso leer:
    check_true("(A) organ_required_for_axes(disk) leer", meas::organ_required_for_axes(kDiskAxes).empty());
    check_true("(A) organ_meaningful_for_axes(disk) NICHT leer (search_algo traegt 4 Katalog-Flags)",
               !meas::organ_meaningful_for_axes(kDiskAxes).empty());
    check_true("(A) gate_contributes_for(required(disk), meaningful(disk)) == false",
               !meas::gate_contributes_for(meas::organ_required_for_axes(kDiskAxes),
                                           meas::organ_meaningful_for_axes(kDiskAxes)));

    // ---- (B) PROBE-LITERALE: das 2c-Probe-Register beisst PER BINARY (K13) ----
    std::cout << "\n---- (B) Probe-Register + Prod1-Signatur: disk-Paar -> {-mavx2}, memory_only -> {} ----\n";
    {
        auto const disk =
            meas::gate_for_binary_in(meas::detail::kProbeMetaMetaRequirement, meas::Prod1Zen5Signature::signature(),
                                     kDiskAxes, meas::SimdRoute::Avx2);
        check_eq("(B) disk flags.size", disk.flags.size(), std::size_t{1});
        check_eq("(B) disk flags[0]", disk.flags.empty() ? std::string{} : disk.flags.front(), std::string{"-mavx2"});
        check_eq("(B) disk identity_text", disk.identity_text, std::string{"gate=[-mavx2]"});
        auto const memory_only =
            meas::gate_for_binary_in(meas::detail::kProbeMetaMetaRequirement, meas::Prod1Zen5Signature::signature(),
                                     kMemoryOnlyAxes, meas::SimdRoute::Avx2);
        check_true("(B) memory_only flags leer (Traeger-Selektion je Binary)", memory_only.flags.empty());
        check_true("(B) memory_only identity leer", memory_only.identity_text.empty());
        // Konsistenz DESSELBEN Aufrufs: identity_text ist die Formatierung der flags (ein Aufruf, beide Felder).
        check_eq("(B) identity == format(flags)", meas::format_gate_contribution(disk.flags), disk.identity_text);
    }

    // ---- (C) RSP-BYTE-PROBE: 320 golden-ids x 3 Routen byte-identisch zur Referenz ----
    std::cout << "\n---- (C) rsp-Diff je Route: 320 golden-ids, per-Job-Zeile == Referenz ohne Gate ----\n";
    {
        auto const ids = golden_320_ids();
        check_eq("(C) golden id-Zahl", ids.size(), std::size_t{320});
        std::size_t vergleiche = 0;
        std::size_t abweichend = 0;
        for (auto const& id : ids) {
            auto const axes = ex::ceb_parse_path(id);
            for (auto const& r : kRouten) {
                ++vergleiche;
                if (rsp_zeile_per_binary("-O2", r.march, axes) != rsp_zeile_referenz("-O2", r.march)) ++abweichend;
            }
        }
        check_eq("(C) rsp-Vergleiche (320 x 3)", vergleiche, std::size_t{960});
        check_eq("(C) abweichende rsp-Zeilen", abweichend, std::size_t{0});
    }

    // ---- (D) D1-KLASSEN je einmal literal am per-Binary-Aggregat ----
    std::cout << "\n---- (D) D1-Klassen am per-Binary-Weg (Aggregat -> pruef_dock) ----\n";
    {
        // HardwareErweiterungFehlt: die Meta-Meta verlangte avx512_fp16 -- Prod1 (Zen 5) fuehrt KEIN fp16.
        static constexpr std::array<meas::SimdFeatureFlag, 1>          kFp16Required{meas::kAvx512Fp16};
        static constexpr std::array<meas::OrganMetaMetaRequirement, 1> kFp16Register{
            {{"disk_io", "persistence_target", "persistence_disk_writeback", kFp16Required, kFp16Required}}};
        std::vector<std::pair<std::string_view, std::string_view>> disk_sichten;
        for (auto const& [a, v] : kDiskAxes) disk_sichten.emplace_back(a, v);
        auto const fp16_req = meas::aggregate_required_for_axes_kern(disk_sichten, kFp16Register);
        check_eq("(D) fp16-Aggregat count (per Binary am Traeger-Paar)", fp16_req.count, std::size_t{1});
        auto const hw = meas::pruef_dock(fp16_req.als_span(), fp16_req.als_span(),
                                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx512);
        check_true("(D) HardwareErweiterungFehlt: state Abgelehnt", hw.state == meas::SimdGateState::Abgelehnt);
        check_true("(D) HardwareErweiterungFehlt: Klasse",
                   hw.error.has_value() && *hw.error == meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt);
        check_eq("(D) HardwareErweiterungFehlt: Label",
                 std::string{hw.error.has_value() ? meas::error_class_label(*hw.error) : std::string_view{}},
                 std::string{"hardware_erweiterung_fehlt"});

        // CompileKombination: avx512_vpopcntdq ist auf Prod1 vorhanden UND sinnvoll deklariert,
        // aber die Grob-Route Avx2 sperrt den AVX-512-Tier (route_allows).
        static constexpr std::array<meas::SimdFeatureFlag, 1>          kVpopRequired{meas::kAvx512Vpopcntdq};
        static constexpr std::array<meas::OrganMetaMetaRequirement, 1> kVpopRegister{
            {{"disk_io", "persistence_target", "persistence_disk_writeback", kVpopRequired, kVpopRequired}}};
        auto const vpop_req = meas::aggregate_required_for_axes_kern(disk_sichten, kVpopRegister);
        check_eq("(D) vpop-Aggregat count", vpop_req.count, std::size_t{1});
        auto const ck = meas::pruef_dock(vpop_req.als_span(), vpop_req.als_span(),
                                         meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx2);
        check_true("(D) CompileKombination: state Abgelehnt", ck.state == meas::SimdGateState::Abgelehnt);
        check_true("(D) CompileKombination: Klasse",
                   ck.error.has_value() && *ck.error == meas::CompilerCompilerErrorClass::CompileKombination);
        check_eq("(D) CompileKombination: Label",
                 std::string{ck.error.has_value() ? meas::error_class_label(*ck.error) : std::string_view{}},
                 std::string{"compile_kombination"});

        // ToolchainFehlt: der aktive Dialekt kennt keine Schreibweise des Flags (leere clang-Spalte).
        static constexpr meas::SimdFeatureFlag                kDemoOhneClang{"demo_ohne_clang", "-mdemo", "",
                                                                             meas::SimdFlagTier::Avx256};
        static constexpr std::array<meas::SimdFeatureFlag, 1> kDemoRequired{kDemoOhneClang};
        auto const tf = meas::pruef_dock(kDemoRequired, kDemoRequired, kDemoRequired, meas::SimdRoute::Avx2,
                                         meas::SimdDialect::Clang);
        check_true("(D) ToolchainFehlt: state Abgelehnt", tf.state == meas::SimdGateState::Abgelehnt);
        check_true("(D) ToolchainFehlt: Klasse",
                   tf.error.has_value() && *tf.error == meas::CompilerCompilerErrorClass::ToolchainFehlt);
        check_eq("(D) ToolchainFehlt: Label",
                 std::string{tf.error.has_value() ? meas::error_class_label(*tf.error) : std::string_view{}},
                 std::string{"toolchain_fehlt"});
    }

    std::cout << "\n"
              << (g_fail == 0 ? "GRUEN" : "ROT") << " test_e10_gate_per_binary -- Fehlschlaege: " << g_fail << "\n";
    return g_fail == 0 ? 0 : 1;
}
