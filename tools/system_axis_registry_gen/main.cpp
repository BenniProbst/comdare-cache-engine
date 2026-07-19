// PAKET W2-B (2026-07-19) - system_axis_registry_gen: erzeugt die CEB-System-Achsen-Registry-XML
// `system_axis_registry.xml` per COMPILE-TIME-REFLEKTION der realen CebSystemAxis-Typen aus dem
// measurement-Modul (measurement/*_system_axis.hpp + *_sub_axis.hpp + *_family_axis.hpp).
//
// WAS (Ledger §28/§30): die System-Achsen-ART (system_config, AxisKind::system_config) bekommt ihre
// EIGENE Registry-Bibliothek in IHREM Modul -- das Angebot des Compiles fuer die CEB-Stufe (System->CEB).
// Reflektiert werden die 6 aktiven, BAU-TREIBENDEN CebSystemAxis-Haupt-Achsen samt ihrer Unter-Achsen/
// Optionen/Flags je Compiler-Dialekt. Kein zweiter Parser: die Wurzel bleibt <comdare_axis_registry>
// mit <axis id=...>/<baustein name=...>, sodass read_axis_registry (validate_profile.hpp) das
// Achsen-/Baustein-Skelett weiterhin lesen kann; die neuen Unter-Achsen-/Flag-Kind-Elemente sind
// vorwaerts-kompatibel (heutiger v1-Leser ignoriert sie, der Resolver liest sie spaeter).
//
// PFLICHT-LEITPLANKE (Kapseln-Regel K1..K3, axis_binding-Anti-Lehre): NUR ueber die realen CT-Klassen
// reflektieren, NIE handschreiben. JEDER emittierte Wert kommt aus einem constexpr-Member/-Aufruf des
// realen Typs (axis_label()/compiler_id()/opt_level_id()/gcc_opt_flag()/...). Wo es keine string-id-
// Single-Source gibt (Scheduling-Sub-Dimensionen: getypte Accessoren OHNE id-String), werden die WERTE
// reflektiert und die id-Etiketten sind an den Accessor-AUFRUF compile-gekoppelt (ein Rename bricht
// DIESEN Generator-Build -> kein stiller XML-Phantom). Ein Laufzeit-GUARD verwirft unsaubere Namen
// (kein '(' / ' ' / leer). Konsument+Contract-Test IM SELBEN Increment = der registry_roundtrip-ctest.
//
// AUSSCHLUSS (dokumentiert): extension_hardware_system_axis.hpp = DEPRECATED-Insel (abgeloest durch
// extension_hardware_family_axis + simd_sub_axis; nicht reaktiviert) und hardware_isa_system_axis.hpp =
// HOST-Deskriptor/Mess-Gate (treibt NICHT den Bau) -> beide sind NICHT Teil des Bau-treibenden Angebots.
//
// binary_id-DOKTRIN: System-Achsen stehen NIE in kCompositionAxisNames -> binary_id="never" je Achse
// (golden==320 / golden-N=2^17 unberuehrt; die -O/-march/-mcx16-WERTE fliessen in die CompileFn/den
// build_version-Suffix, NIE in die binary_id -- Q2-Ruling: Flags->CompileFn).
//
// C++23. Ausfuehren: `system_axis_registry_gen --out <pfad>`. TABU (permutation_axes.xml, golden,
// ABI) wird NICHT beruehrt - der Generator LIEST nur Typen.

#include <builder/codegen/type_name.hpp> // type_name<W>() (FQ-Typ, compile-time)

#include <cache_engine/measurement/compiler_atomic_sub_axis.hpp>
#include <cache_engine/measurement/compiler_system_axis.hpp>
#include <cache_engine/measurement/extension_hardware_family_axis.hpp>
#include <cache_engine/measurement/load_framework_system_axis.hpp>
#include <cache_engine/measurement/optimization_level_sub_axis.hpp>
#include <cache_engine/measurement/scheduling_system_axis.hpp>
#include <cache_engine/measurement/simd_sub_axis.hpp>
#include <cache_engine/measurement/target_isa_system_axis.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace cg   = ::comdare::cache_engine::builder::codegen;
namespace meas = ::comdare::cache_engine::measurement;

namespace {

// XML-Escape (identisch zur axis_registry_gen-Konvention).
[[nodiscard]] std::string xml_escape(std::string_view s) {
    std::string o;
    o.reserve(s.size());
    for (char const c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            case '"': o += "&quot;"; break;
            case '\'': o += "&apos;"; break;
            default: o += c;
        }
    }
    return o;
}

// FQ-Typ ('::'-praefixiert) via type_name<W>() (compile-time, GCC/Clang liefern die reine Form).
template <class W>
[[nodiscard]] std::string fq_type() {
    return "::" + std::string{cg::type_name<W>()};
}

// Kurz-Typ: alles vor einem eventuellen '<', dann hinter dem letzten "::".
[[nodiscard]] std::string short_name(std::string const& fq) {
    std::size_t const lt   = fq.find('<');
    std::string const head = (lt == std::string::npos) ? fq : fq.substr(0, lt);
    std::size_t const cc   = head.rfind("::");
    return (cc == std::string::npos) ? head : head.substr(cc + 2);
}

// GUARD: kein reflektierter Name/Id darf '(' / ' ' / leer sein (faengt konditionale/gelekte Formen).
[[nodiscard]] bool name_is_clean(std::string_view n) {
    return !n.empty() && n.find('(') == std::string_view::npos && n.find(' ') == std::string_view::npos;
}

// Sammelt alle emittierten Namen/Ids fuer den Sauberkeits-GUARD (vor dem Schreiben geprueft).
std::vector<std::string> g_names;
std::size_t              g_baustein_total = 0;

void note_name(std::string_view n) { g_names.emplace_back(n); }

// -- Emit-Bausteine (jeder Wert aus einem realen Typ-Member reflektiert). --

// Ein <baustein> mit FQ-Typ + Kurz-Typ; zusaetzliche Attribute schreibt der Aufrufer via extra.
template <class W>
void emit_baustein(std::ofstream& f, std::string_view name, std::string const& extra) {
    std::string const type = fq_type<W>();
    note_name(name);
    ++g_baustein_total;
    f << "    <baustein name=\"" << xml_escape(name) << "\" wrapper=\"" << xml_escape(short_name(type)) << "\" type=\""
      << xml_escape(type) << "\" enabled=\"true\"" << extra << "/>\n";
}

// Eine <option> einer Unter-Achse mit den 3 Dialekt-Flags (gpp/clang/msvc) + optionalem extra-Attribut.
void emit_option(std::ofstream& f, std::string_view id, std::string_view gpp, std::string_view clang,
                 std::string_view msvc, std::string const& extra) {
    note_name(id);
    f << "      <option id=\"" << xml_escape(id) << "\"" << extra << ">\n";
    f << "        <flag dialect=\"gpp\" value=\"" << xml_escape(gpp) << "\"/>\n";
    f << "        <flag dialect=\"clang\" value=\"" << xml_escape(clang) << "\"/>\n";
    f << "        <flag dialect=\"msvc\" value=\"" << xml_escape(msvc) << "\"/>\n";
    f << "      </option>\n";
}

void emit_axis_open(std::ofstream& f, std::string_view id, std::string_view stage, std::size_t baustein_count) {
    note_name(id);
    f << "  <axis id=\"" << xml_escape(id) << "\" category=\"system_config\" axis_kind=\"system_config\""
      << " binary_id=\"never\" stage=\"" << xml_escape(stage) << "\" baustein_count=\"" << baustein_count << "\">\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string out_path = "system_axis_registry.xml";
    for (int i = 1; i < argc; ++i) {
        std::string_view const a = argv[i];
        if (a == "--out" && i + 1 < argc) {
            out_path = argv[++i];
        } else if (a == "--help" || a == "-h") {
            std::cout << "system_axis_registry_gen --out <file>\n";
            return 0;
        } else {
            std::cerr << "system_axis_registry_gen: unbekanntes Argument '" << a << "'\n";
            return 2;
        }
    }

    std::ofstream f{out_path, std::ios::binary};
    if (!f) {
        std::cerr << "system_axis_registry_gen: kann Ausgabedatei nicht oeffnen: " << out_path << "\n";
        return 4;
    }

    f << "<comdare_axis_registry engine=\"cache_engine_system\" schema=\"1\" generator=\"system_axis_registry_gen\">\n";
    f << "  <!-- GENERIERT von tools/system_axis_registry_gen (PAKET W2-B) per compile-time-Reflektion der\n";
    f << "       realen CebSystemAxis-Typen (measurement/*_system_axis.hpp/*_sub_axis.hpp). NICHT von Hand\n";
    f << "       editieren. Ledger §28/§30: System-Art-Registry im Mess-/System-Modul (Angebot fuer System->CEB). -->\n";
    f << "  <!-- Haupt-Achse=CT-statisch (stage=ct, in die CEB/Tier-Binary einkompiliert); Unter-Achse=dynamisch\n";
    f << "       (stage=runtime, vom Planer permutiert). Q2/K2: opt_level/simd/atomic128 materialisieren als\n";
    f << "       CompileFn-Flags (build_version-Suffix), NIE als Laufzeit-Typ-Switch und NIE in der binary_id. -->\n";
    f << "  <!-- AUSSCHLUSS: extension_hardware_system_axis.hpp = DEPRECATED-Insel; hardware_isa_system_axis.hpp =\n";
    f << "       HOST-Deskriptor (treibt NICHT den Bau) -- beide NICHT Teil dieses Bau-treibenden Angebots. -->\n";

    // ── 1) compiler (CompilerSystemAxis): 2 Bausteine gcc/clang + Unter-Achsen opt_level/atomic128 ──
    emit_axis_open(f, meas::GccCompilerAxis::axis_label(), "ct", 2);
    {
        std::string extra;
        extra = std::string{" driver=\""} + std::string{meas::GccCompilerAxis::driver_default()} +
                "\" supports_fno_gnu_unique=\"" + (meas::GccCompilerAxis::supports_fno_gnu_unique() ? "true" : "false") +
                "\"";
        emit_baustein<meas::GccCompilerAxis>(f, meas::GccCompilerAxis::compiler_id(), extra);
        extra = std::string{" driver=\""} + std::string{meas::ClangCompilerAxis::driver_default()} +
                "\" supports_fno_gnu_unique=\"" +
                (meas::ClangCompilerAxis::supports_fno_gnu_unique() ? "true" : "false") + "\"";
        emit_baustein<meas::ClangCompilerAxis>(f, meas::ClangCompilerAxis::compiler_id(), extra);
    }
    // opt_level (Unter-Achse; parent aus parent_axis_label(); Optionen O0..Ofast; is_ieee754_deterministic).
    f << "    <sub_axis id=\"" << xml_escape(meas::OptO0Option::axis_label()) << "\" parent=\""
      << xml_escape(meas::OptO0Option::parent_axis_label())
      << "\" stage=\"runtime\" value_type=\"token\" option_count=\"5\">\n";
    {
        auto emit_opt = [&](std::string_view id, std::string_view gpp, std::string_view clang, std::string_view msvc,
                            bool det) {
            std::string const extra = std::string{" ieee754_deterministic=\""} + (det ? "true" : "false") + "\"";
            emit_option(f, id, gpp, clang, msvc, extra);
        };
        emit_opt(meas::OptO0Option::opt_level_id(), meas::OptO0Option::gcc_opt_flag(),
                 meas::OptO0Option::clang_opt_flag(), meas::OptO0Option::msvc_opt_flag(),
                 meas::OptO0Option::is_ieee754_deterministic());
        emit_opt(meas::OptO1Option::opt_level_id(), meas::OptO1Option::gcc_opt_flag(),
                 meas::OptO1Option::clang_opt_flag(), meas::OptO1Option::msvc_opt_flag(),
                 meas::OptO1Option::is_ieee754_deterministic());
        emit_opt(meas::OptO2Option::opt_level_id(), meas::OptO2Option::gcc_opt_flag(),
                 meas::OptO2Option::clang_opt_flag(), meas::OptO2Option::msvc_opt_flag(),
                 meas::OptO2Option::is_ieee754_deterministic());
        emit_opt(meas::OptO3Option::opt_level_id(), meas::OptO3Option::gcc_opt_flag(),
                 meas::OptO3Option::clang_opt_flag(), meas::OptO3Option::msvc_opt_flag(),
                 meas::OptO3Option::is_ieee754_deterministic());
        emit_opt(meas::OptOfastOption::opt_level_id(), meas::OptOfastOption::gcc_opt_flag(),
                 meas::OptOfastOption::clang_opt_flag(), meas::OptOfastOption::msvc_opt_flag(),
                 meas::OptOfastOption::is_ieee754_deterministic());
    }
    f << "    </sub_axis>\n";
    // atomic128 (Unter-Achse unter compiler; Optionen no_cx16/cx16; -mcx16 = Codegen-Flag).
    f << "    <sub_axis id=\"" << xml_escape(meas::NoCx16Option::axis_label()) << "\" parent=\""
      << xml_escape(meas::NoCx16Option::parent_axis_label())
      << "\" stage=\"runtime\" value_type=\"token\" option_count=\"2\">\n";
    emit_option(f, meas::NoCx16Option::atomic128_id(), meas::NoCx16Option::gcc_flag(), meas::NoCx16Option::clang_flag(),
                meas::NoCx16Option::msvc_flag(), std::string{});
    emit_option(f, meas::Cx16Option::atomic128_id(), meas::Cx16Option::gcc_flag(), meas::Cx16Option::clang_flag(),
                meas::Cx16Option::msvc_flag(), std::string{});
    f << "    </sub_axis>\n";
    f << "  </axis>\n";

    // ── 2) extension_hardware (ExtensionHardwareFamilyAxis): Familie simd + Unter-Achse simd (march je Dialekt) ──
    emit_axis_open(f, meas::SimdExtensionHardwareFamily::axis_label(), "ct", 1);
    {
        std::string const extra =
            std::string{" family_id=\""} + std::string{meas::SimdExtensionHardwareFamily::family_id()} + "\"";
        emit_baustein<meas::SimdExtensionHardwareFamily>(f, meas::SimdExtensionHardwareFamily::family_id(), extra);
    }
    f << "    <sub_axis id=\"" << xml_escape(meas::SimdNoExtOption::axis_label()) << "\" parent=\""
      << xml_escape(meas::SimdNoExtOption::parent_axis_label())
      << "\" stage=\"runtime\" value_type=\"token\" option_count=\"3\">\n";
    emit_option(f, meas::SimdNoExtOption::simd_id(), meas::SimdNoExtOption::gcc_march_flag(),
                meas::SimdNoExtOption::clang_march_flag(), meas::SimdNoExtOption::msvc_march_flag(), std::string{});
    emit_option(f, meas::SimdAvx2Option::simd_id(), meas::SimdAvx2Option::gcc_march_flag(),
                meas::SimdAvx2Option::clang_march_flag(), meas::SimdAvx2Option::msvc_march_flag(), std::string{});
    emit_option(f, meas::SimdAvx512Option::simd_id(), meas::SimdAvx512Option::gcc_march_flag(),
                meas::SimdAvx512Option::clang_march_flag(), meas::SimdAvx512Option::msvc_march_flag(), std::string{});
    f << "    </sub_axis>\n";
    f << "  </axis>\n";

    // ── 3) target_isa (TargetIsaSystemAxis): 2 Bausteine x86_64/aarch64 (native + Cross-Triple/-march) ──
    emit_axis_open(f, meas::X86_64TargetIsa::axis_label(), "ct", 2);
    {
        std::string extra = std::string{" native=\""} + (meas::X86_64TargetIsa::is_native() ? "true" : "false") +
                            "\" triple=\"" + std::string{meas::X86_64TargetIsa::target_triple()} + "\" march=\"" +
                            std::string{meas::X86_64TargetIsa::target_march()} + "\"";
        emit_baustein<meas::X86_64TargetIsa>(f, meas::X86_64TargetIsa::target_isa_id(), extra);
        extra = std::string{" native=\""} + (meas::Aarch64TargetIsa::is_native() ? "true" : "false") + "\" triple=\"" +
                std::string{meas::Aarch64TargetIsa::target_triple()} + "\" march=\"" +
                std::string{meas::Aarch64TargetIsa::target_march()} + "\"";
        emit_baustein<meas::Aarch64TargetIsa>(f, meas::Aarch64TargetIsa::target_isa_id(), extra);
    }
    f << "  </axis>\n";

    // ── 4) scheduling (SchedulingSystemAxis): 1 Baustein; 5 fixe Sub-Dimensionen (getypte Accessoren). ──
    // Die id-Etiketten der Sub-Dims haben KEINE string-Single-Source (nur getypte Accessoren) -> sie sind
    // an den Accessor-AUFRUF compile-gekoppelt (Rename bricht diesen Build). Enum-WERTE als Ordinal reflektiert.
    emit_axis_open(f, meas::DefaultSchedulingSystemAxis::axis_label(), "ct", 1);
    {
        std::string const type = fq_type<meas::DefaultSchedulingSystemAxis>();
        emit_baustein<meas::DefaultSchedulingSystemAxis>(f, short_name(type), std::string{});
        f << "    <sub_axis id=\"scheduling_dims\" parent=\""
          << xml_escape(meas::DefaultSchedulingSystemAxis::axis_label())
          << "\" stage=\"ct\" kind=\"fixed_enum_tuple\">\n";
        f << "      <!-- Ordinals = rohe Enum-Werte (kein CT-Label-Vorbild); id-Strings sind an die Accessor-\n";
        f << "           Aufrufe compile-gekoppelt (concepts::scheduling_strategy.hpp). -->\n";
        f << "      <sub_dim id=\"worker_pool_layout\" value_ordinal=\""
          << static_cast<std::uint32_t>(meas::DefaultSchedulingSystemAxis::worker_pool_layout()) << "\"/>\n";
        f << "      <sub_dim id=\"simd_worker_count_limit\" value=\""
          << meas::DefaultSchedulingSystemAxis::simd_worker_count_limit() << "\"/>\n";
        f << "      <sub_dim id=\"hetero_core_dispatch\" value_ordinal=\""
          << static_cast<std::uint32_t>(meas::DefaultSchedulingSystemAxis::hetero_core_dispatch()) << "\"/>\n";
        f << "      <sub_dim id=\"co_routine_strategy\" value_ordinal=\""
          << static_cast<std::uint32_t>(meas::DefaultSchedulingSystemAxis::co_routine_strategy()) << "\"/>\n";
        f << "      <sub_dim id=\"batch_granularity\" value_ordinal=\""
          << static_cast<std::uint32_t>(meas::DefaultSchedulingSystemAxis::batch_granularity()) << "\"/>\n";
        f << "    </sub_axis>\n";
    }
    f << "  </axis>\n";

    // ── 5) load_framework (LoadFrameworkSystemAxis): 1 Baustein ycsb + dynamische Unter-Achse "workload". ──
    // Die workload-Optionen (ycsb_a..f/Ranges) sind Anwender-XML-Selektion (Lastprofil-Akten), kein CT-Angebot;
    // hier wird NUR das Unter-Achsen-Label reflektiert (Single-Source der setting_label-Konvention).
    emit_axis_open(f, meas::YcsbLoadFrameworkAxis::axis_label(), "ct", 1);
    {
        std::string const extra =
            std::string{" framework_id=\""} + std::string{meas::YcsbLoadFrameworkAxis::framework_id()} + "\"";
        emit_baustein<meas::YcsbLoadFrameworkAxis>(f, meas::YcsbLoadFrameworkAxis::framework_id(), extra);
        f << "    <sub_axis id=\"" << xml_escape(meas::YcsbLoadFrameworkAxis::sub_axis_label()) << "\" parent=\""
          << xml_escape(meas::YcsbLoadFrameworkAxis::axis_label())
          << "\" stage=\"runtime\" value_type=\"token\" option_source=\"user_load_profile_akten\"/>\n";
    }
    f << "  </axis>\n";

    f << "</comdare_axis_registry>\n";
    f.flush();

    // -- GUARD: kein emittierter Name/Id darf unsauber sein (nachtraeglich, Datei bereits geschrieben; bei
    //    Bruch mit rc!=0 abbrechen, damit der Roundtrip-/Build-Schritt es sieht). --
    for (auto const& n : g_names) {
        if (!name_is_clean(n)) {
            std::cerr << "system_axis_registry_gen: GUARD-BRUCH - unsauberer Name/Id '" << n << "'.\n";
            return 3;
        }
    }
    if (!f) {
        std::cerr << "system_axis_registry_gen: Schreibfehler an " << out_path << "\n";
        return 4;
    }

    std::cout << "system_axis_registry_gen: 5 System-Achsen-Elemente (opt_level/atomic128/simd als sub_axis), " << g_baustein_total << " Bausteine -> " << out_path
              << "\n";
    return 0;
}
