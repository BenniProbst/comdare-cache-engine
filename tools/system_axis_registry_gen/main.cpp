// PAKET W2-B (2026-07-19) - system_axis_registry_gen: erzeugt die CEB-System-Achsen-Registry-XML
// `system_axis_registry.xml` per COMPILE-TIME-REFLEKTION der realen CebSystemAxis-Typen aus dem
// measurement-Modul (measurement/*_system_axis.hpp + *_sub_axis.hpp + *_family_axis.hpp).
//
// WAS (Ledger §28/§30): die System-Achsen-ART (system_config, AxisKind::system_config) bekommt ihre
// EIGENE Registry-Bibliothek in IHREM Modul -- das Angebot des Compiles fuer die CEB-Stufe (System->CEB).
// Reflektiert werden die BAU-TREIBENDEN CebSystemAxis-Haupt-Achsen (seit A3: DREI) samt ihrer Unter-Achsen/
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
// external_utils_family_axis + simd_sub_axis; nicht reaktiviert) und hardware_isa_system_axis.hpp =
// HOST-Deskriptor/Mess-Gate (treibt NICHT den Bau) -> beide sind NICHT Teil des Bau-treibenden Angebots.
//
// binary_id-DOKTRIN: System-Achsen stehen NIE in kCompositionAxisNames -> binary_id="never" je Achse
// (golden==320 / golden-N=2^17 unberuehrt; die -O/-march/-mcx16-WERTE fliessen in die CompileFn/den
// build_version-Suffix, NIE in die binary_id -- Q2-Ruling: Flags->CompileFn).
//
// C++23. Ausfuehren: `system_axis_registry_gen --out <pfad>`. TABU (permutation_axes.xml, golden,
// ABI) wird NICHT beruehrt - der Generator LIEST nur Typen.

#include <builder/codegen/type_name.hpp> // type_name<W>() (FQ-Typ, compile-time)

#include <cache_engine/abi/system_axis_order.hpp>   // P5/A7': kSystemAxisOrder (Ordnungs-Single-Source)
#include <profile_facade/system_version_suffix.hpp> // T-c/NETZ 4: kSuffixSegmentOrder (Suffix-Single-Source)
#include <cache_engine/measurement/ceb_complex_system_axis.hpp> // O-8 Schritt 6: aeussere Komplex-Achse + Gruppe
#include <cache_engine/measurement/compiler_atomic_sub_axis.hpp>
#include <cache_engine/measurement/compiler_system_axis.hpp>
#include <cache_engine/measurement/external_utils_family_axis.hpp>
#include <cache_engine/measurement/load_framework_measurement_axis.hpp>
#include <cache_engine/measurement/machine_simd_signature.hpp> // Section 40.a: deklarierte Maschinen-Signaturen
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/optimization_level_sub_axis.hpp>
#include <cache_engine/measurement/scheduling_system_axis.hpp>
#include <cache_engine/measurement/simd_feature_flag.hpp> // Section 40.a: feingranularer Flag-Katalog (code=Wahrheit)
#include <cache_engine/measurement/simd_sub_axis.hpp>
#include <cache_engine/measurement/target_isa_complex_axis.hpp> // O-8 Schritt 6: innerer Komplex-Wrapper
#include <cache_engine/measurement/target_isa_sub_axes.hpp>     // O-8 Schritt 6: numa_node + page
#include <cache_engine/measurement/target_isa_system_axis.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace cg   = ::comdare::cache_engine::builder::codegen;
namespace meas = ::comdare::cache_engine::measurement;
namespace cabi = ::comdare::cache_engine::abi;
namespace cpf  = ::comdare::cache_engine::profile_facade;

// =====================================================================================================
// P5/A7' -- ORDNUNGS-WACHE (Lane A, 26.07.2026). Byte-neutral: prueft nur, aendert nichts.
//
// PROBLEM (Riss 2): die kanonische System-Achsen-Ordnung existiert mehrfach - als kSystemAxisOrder
// (abi/system_axis_order.hpp, Single-Source seit A1), als Reihenfolge der emit_axis_open-Aufrufe hier
// unten, als Kopf-Reihenfolge der generierten XML und als Segment-Folge im build_version-Suffix.
// Bis A1 pruefte NICHTS die Reihenfolge; A1 hat Ordnung <-> Code-Versions-Tabelle gekoppelt. Dieses
// Paket schliesst die naechsten zwei Kopien: Generator-Aufruffolge und XML-Kopf (inkl. Anzahl-Literal).
//
// DREI NETZE, absichtlich unterschiedlich, weil sie unterschiedliche Drift fangen:
//   NETZ 1 (compile-time, unten): die REALEN Achsen-TYPEN, deren axis_label() der Generator
//           emittiert, gegen kSystemAxisOrder. Faengt Typ-/Label-Drift und falsche Deklaration.
//   NETZ 2 (Laufzeit, in main): die TATSAECHLICHE emit_axis_open-Aufruffolge gegen kSystemAxisOrder.
//           Faengt Aufruf-Umsortierung - das kann NETZ 1 grundsaetzlich nicht sehen, weil eine
//           Reihenfolge, die nur in der Anweisungs-FOLGE steckt, compile-time nicht lesbar ist.
//   NETZ 3 (Laufzeit): das Anzahl-Literal der stdout-Zeile kommt aus kSystemAxisOrderCount, nie als Zahl.
//
//   NETZ 4 (T-c, compile-time, O-8 Schritt 11): die vierte Kopie -- die SUFFIX-Segment-Ordnung.
//           Sie war bis R3 nicht pruefbar, weil das Suffix an fuenf Orten imperativ zusammengeklebt
//           wurde; seit profile_facade/system_version_suffix.hpp ist die Ordnung ein DATUM
//           (kSuffixSegmentOrder), und damit existiert erstmals etwas, wogegen verglichen werden kann.
//
//   WAS NETZ 4 PRUEFT -- und was ausdruecklich NICHT: es prueft die ABDECKUNG, nicht die relative
//   Reihenfolge gegen kSystemAxisOrder. Der Grund ist eine echte Eigenschaft, keine Bequemlichkeit:
//   die bindende Suffix-Form ist "+cxx+opt+ext" (Perm-Schleife), waehrend die Achsen-Ordnung
//   target_isa, operating_system, external_utils lautet. Das Suffix folgt also der BAU-Reihenfolge,
//   die Achsen-Ordnung der Achsen-Systematik -- zwei verschiedene Ordnungen mit je eigener
//   Berechtigung. Eine Wache, die sie gleichsetzt, wuerde eine Uebereinstimmung behaupten, die weder
//   der Owner noch der Plan verlangt. Die INNERE Ordnung des Suffix ist stattdessen dort verankert,
//   wo sie hingehoert: als static_assert an kSuffixSegmentOrder selbst.
//   Netz 4 faengt die Drift, die dort NICHT sichtbar waere: eine System-HAUPT-Achse, die in die
//   Ordnung eintritt oder sie verlaesst, ohne dass jemand entschieden hat, ob und wie sie im Suffix
//   auftaucht. Genau das ist gerade zweimal passiert (A3: 5 -> 3 Achsen; Schritt 6: operating_system
//   neu) -- und niemand haette es gemerkt.
//
// A3 IST VOLLZOGEN (O-8 Schritt 4): die Wache ist mit der Ordnung umgezogen (5 -> 3 Haupt-Achsen),
// ohne dass hier eine Zahl stand -- sie war gegen kSystemAxisOrder formuliert, also genuegte die
// Aenderung der Single-Source. Genau dafuer wurde sie so gebaut.
// =====================================================================================================

namespace {

/// NETZ 1: die Achsen-TYPEN in genau der Folge, in der main() sie unten per emit_axis_open emittiert.
/// Diese Liste ist die einzige Stelle, an der die Emissions-Folge deklarativ steht; NETZ 2 belegt zur
/// Laufzeit, dass die Aufrufe ihr wirklich folgen.
/// A3 (O-8 Schritt 4): DREI Haupt-Achsen. compiler und scheduling sind Unter-Achsen geworden
/// (Schritt 6), load_framework hat den Realm gewechselt (Mess-Seite, Ledger 69.1) -- ihre
/// Emissions-Koerper bleiben geparkt, aber sie stehen nicht mehr in dieser Folge.
using SystemAxisEmissionTypes =
    std::tuple<meas::X86_64TargetIsa, meas::LinuxOperatingSystem, meas::SimdExternalUtilsFamily>;

inline constexpr std::array<std::string_view, std::tuple_size_v<SystemAxisEmissionTypes>> kEmissionOrderFromTypes{{
    meas::X86_64TargetIsa::axis_label(),
    meas::LinuxOperatingSystem::axis_label(),
    meas::SimdExternalUtilsFamily::axis_label(),
}};

[[nodiscard]] consteval bool emission_types_match_order() {
    if (kEmissionOrderFromTypes.size() != cabi::kSystemAxisOrderCount) return false;
    for (std::size_t i = 0; i < cabi::kSystemAxisOrderCount; ++i)
        if (kEmissionOrderFromTypes[i] != cabi::kSystemAxisOrder[i]) return false;
    return true;
}
static_assert(emission_types_match_order(),
              "P5/A7' NETZ 1: die Achsen-TYPEN, die dieser Generator emittiert, stimmen in Zahl oder "
              "Reihenfolge nicht mit abi::kSystemAxisOrder ueberein. kSystemAxisOrder ist die "
              "Single-Source (A1) - entweder die Ordnung dort anpassen oder die Typ-Liste hier, NIE "
              "nur eine von beiden.");

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

/// P5/A7' NETZ 2: sammelt AUSSCHLIESSLICH die Haupt-Achsen-Ids in ihrer echten Emissions-Folge.
/// Getrennt von g_names, weil dort auch Bausteine und Optionen landen - fuer die Ordnungs-Wache zaehlt
/// nur die Achsen-Ebene. Gefuellt in emit_axis_open, geprueft am Ende von main.
std::vector<std::string> g_system_axis_order;

void note_name(std::string_view n) { g_names.emplace_back(n); }

// -- Emit-Bausteine (jeder Wert aus einem realen Typ-Member reflektiert). --

// Ein <baustein> mit FQ-Typ + Kurz-Typ; zusaetzliche Attribute schreibt der Aufrufer via extra.
// O-8 Schritt 6: `indent` ist neu und hat den bisherigen Wert als DEFAULT -- alle Bestands-Aufrufe
// emittieren damit byte-identisch weiter. Gebraucht wird er von der aeusseren Komplex-Achse, deren
// Unter-Achsen-Gruppe eine Ebene tiefer sitzt als eine Haupt-Achse.
template <class W>
void emit_baustein(std::ofstream& f, std::string_view name, std::string const& extra,
                   std::string_view indent = "    ") {
    std::string const type = fq_type<W>();
    note_name(name);
    ++g_baustein_total;
    f << indent << "<baustein name=\"" << xml_escape(name) << "\" wrapper=\"" << xml_escape(short_name(type))
      << "\" type=\"" << xml_escape(type) << "\" enabled=\"true\"" << extra << "/>\n";
}

// Eine <option> einer Unter-Achse mit den 3 Dialekt-Flags (gpp/clang/msvc) + optionalem extra-Attribut.
void emit_option(std::ofstream& f, std::string_view id, std::string_view gpp, std::string_view clang,
                 std::string_view msvc, std::string const& extra, std::string_view indent = "      ") {
    note_name(id);
    f << indent << "<option id=\"" << xml_escape(id) << "\"" << extra << ">\n";
    f << indent << "  <flag dialect=\"gpp\" value=\"" << xml_escape(gpp) << "\"/>\n";
    f << indent << "  <flag dialect=\"clang\" value=\"" << xml_escape(clang) << "\"/>\n";
    f << indent << "  <flag dialect=\"msvc\" value=\"" << xml_escape(msvc) << "\"/>\n";
    f << indent << "</option>\n";
}

void emit_axis_open(std::ofstream& f, std::string_view id, std::string_view stage, std::size_t baustein_count) {
    note_name(id);
    g_system_axis_order.emplace_back(id); // P5/A7' NETZ 2: echte Emissions-Folge der Haupt-Achsen
    f << "  <axis id=\"" << xml_escape(id) << "\" category=\"system_config\" axis_kind=\"system_config\""
      << " binary_id=\"never\" stage=\"" << xml_escape(stage) << "\" baustein_count=\"" << baustein_count << "\">\n";
}

/// P5/A9a war dies der Emissions-Koerper der HAUPT-Achse "compiler".
/// A3 (O-8 Schritt 4) nahm compiler aus kSystemAxisEmitters; O-8 Schritt 6 haengt es hier als Glied
/// der Unter-Achsen-GRUPPE an die AEUSSERE Komplex-Achse (O-1r, Owner-GO OP-5) -- ausdruecklich NICHT
/// an target_isa: die Gruppe beschreibt, WIE aus der Rekombination aller drei Achsen gebaut wird.
/// Inhalt (Bausteine, opt_level, atomic128) UNVERAENDERT; nur die Klammer darum ist neu.
void emit_compound_sub_axis_group_compiler(std::ofstream& f) {
    // -- compiler (CompilerSystemAxis): 2 Bausteine gcc/clang + Unter-Achsen opt_level/atomic128 --
    note_name(meas::GccCompilerAxis::axis_label());
    f << "      <sub_axis id=\"" << xml_escape(meas::GccCompilerAxis::axis_label()) << "\" parent=\""
      << xml_escape(meas::CebCompoundSystemAxis::axis_label()) << "\" stage=\"ct\" baustein_count=\"2\">\n";
    {
        std::string extra;
        extra = std::string{" driver=\""} + std::string{meas::GccCompilerAxis::driver_default()} +
                "\" supports_fno_gnu_unique=\"" +
                (meas::GccCompilerAxis::supports_fno_gnu_unique() ? "true" : "false") + "\"";
        emit_baustein<meas::GccCompilerAxis>(f, meas::GccCompilerAxis::compiler_id(), extra, "        ");
        extra = std::string{" driver=\""} + std::string{meas::ClangCompilerAxis::driver_default()} +
                "\" supports_fno_gnu_unique=\"" +
                (meas::ClangCompilerAxis::supports_fno_gnu_unique() ? "true" : "false") + "\"";
        emit_baustein<meas::ClangCompilerAxis>(f, meas::ClangCompilerAxis::compiler_id(), extra, "        ");
    }
    // opt_level (Unter-Achse; parent aus parent_axis_label(); Optionen O0..Ofast; is_ieee754_deterministic).
    f << "        <sub_axis id=\"" << xml_escape(meas::OptO0Option::axis_label()) << "\" parent=\""
      << xml_escape(meas::OptO0Option::parent_axis_label())
      << "\" stage=\"runtime\" value_type=\"token\" option_count=\"5\">\n";
    {
        auto emit_opt = [&](std::string_view id, std::string_view gpp, std::string_view clang, std::string_view msvc,
                            bool det) {
            std::string const extra = std::string{" ieee754_deterministic=\""} + (det ? "true" : "false") + "\"";
            emit_option(f, id, gpp, clang, msvc, extra, "          ");
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
    f << "        </sub_axis>\n";
    // atomic128 (Unter-Achse unter compiler; Optionen no_cx16/cx16; -mcx16 = Codegen-Flag).
    f << "        <sub_axis id=\"" << xml_escape(meas::NoCx16Option::axis_label()) << "\" parent=\""
      << xml_escape(meas::NoCx16Option::parent_axis_label())
      << "\" stage=\"runtime\" value_type=\"token\" option_count=\"2\">\n";
    emit_option(f, meas::NoCx16Option::atomic128_id(), meas::NoCx16Option::gcc_flag(), meas::NoCx16Option::clang_flag(),
                meas::NoCx16Option::msvc_flag(), std::string{}, "          ");
    emit_option(f, meas::Cx16Option::atomic128_id(), meas::Cx16Option::gcc_flag(), meas::Cx16Option::clang_flag(),
                meas::Cx16Option::msvc_flag(), std::string{}, "          ");
    f << "        </sub_axis>\n";
    f << "      </sub_axis>\n";
}

/// P5/A9a: Emissions-Koerper der Haupt-Achse "external_utils" (1:1 aus main() herausgezogen, Inhalt
/// UNVERAENDERT -- die XML muss byte-identisch bleiben). Aufgerufen ausschliesslich ueber die
/// kSystemAxisEmitters-Tabelle, die kSystemAxisOrder folgt.
void emit_system_axis_external_utils(std::ofstream& f) {
    // -- 2) external_utils (ExternalUtilsFamilyAxis): Familie simd + Unter-Achse simd (march je Dialekt) --
    emit_axis_open(f, meas::SimdExternalUtilsFamily::axis_label(), "ct", 1);
    {
        std::string const extra =
            std::string{" family_id=\""} + std::string{meas::SimdExternalUtilsFamily::family_id()} + "\"";
        emit_baustein<meas::SimdExternalUtilsFamily>(f, meas::SimdExternalUtilsFamily::family_id(), extra);
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
    // Section 40.a: feingranulares SIMD-Flag-Vokabular (code=Wahrheit aus simd_feature_flag.hpp). Die grobe
    // simd-Sub-Achse oben bleibt Runner-Routing-Vorstufe; DIESE Einzel-Flags entscheiden den Bau (E4-Gate).
    // cpuinfo-Id (Signatur-Match) und g++/clang-Flag (Compile) sind GETRENNT gefuehrt (Unterstrich-Falle).
    f << "    <simd_feature_catalog count=\"" << meas::kSimdFeatureFlagCatalog.size() << "\">\n";
    for (auto const& fl : meas::kSimdFeatureFlagCatalog) {
        note_name(fl.cpuinfo);
        f << "      <flag cpuinfo=\"" << xml_escape(fl.cpuinfo) << "\" gpp=\"" << xml_escape(fl.gpp) << "\" clang=\""
          << xml_escape(fl.clang) << "\" tier=\"" << xml_escape(meas::tier_label(fl.tier)) << "\"/>\n";
    }
    f << "    </simd_feature_catalog>\n";
    f << "  </axis>\n";
}

// O-8 Schritt 6: die Unter-Achsen des target_isa-Komplex-Wrappers. Vorwaerts deklariert, weil ihre
// Koerper weiter unten stehen (scheduling war bis A3 eine Haupt-Achse und behaelt seinen Platz in
// der Datei; numa_node/page sind neu).
void emit_target_isa_sub_axis_scheduling(std::ofstream& f);

/// O-8 Schritt 6 (Bauplan IV.2.2 / Owner OD-2): die drei FESTEN GLIEDER des Komplexes, je benannter
/// Rekombination. Muster fixed_enum_tuple (wie scheduling): die Glieder sind sub_dims EINER Achse,
/// NICHT drei Achsen -- RF-7, "EIN Feld im Haupt-Achsen-Array".
/// Ein nicht deklariertes Glied wird als declared="false" OHNE Wert emittiert: die Registry sagt dann
/// ehrlich "nicht erhoben" statt einer erfundenen Zahl.
/// P3 (Auflage A2, Owner-Entscheid E-3 "additiv"): ein deklariertes RAM-Glied traegt ZUSAETZLICH
/// declaration_source -- die konservierte source_id des eingefrorenen Ursprungs (z.B.
/// "dmidecode 2026-07"). Das ist eine NOTIZ, keine Stufe: Laufzeit-Stufen (configured_measured/
/// spd_jedec_base) erscheinen ausschliesslich in CSV+Log, NIE in dieser XML. Das declared-Attribut
/// BLEIBT unveraendert (A6-Konsumenten-Schutz); der Generator probt weiterhin NIE -- der Wert kommt
/// aus der constexpr-Deklarations-Tabelle und ist damit maschinen-unabhaengig byte-stabil.
void emit_target_isa_complex_members(std::ofstream& f) {
    auto emit_complex = [&f]<class Cx>(std::type_identity<Cx>) {
        note_name(Cx::complex_id());
        f << "      <complex id=\"" << xml_escape(Cx::complex_id()) << "\" isa=\"" << xml_escape(Cx::target_isa_id())
          << "\">\n";
        auto const emit_num = [&f](std::string_view id, std::uint32_t v, std::string_view source = {}) {
            f << "        <sub_dim id=\"" << xml_escape(id) << "\" declared=\"" << (v != 0 ? "true" : "false") << "\"";
            if (v != 0) f << " value=\"" << v << "\"";
            // Die A2-Notiz reist nur mit einem deklarierten Wert (Kohaerenz-Wache: machine_identity.hpp).
            if (v != 0 && !source.empty()) f << " declaration_source=\"" << xml_escape(source) << "\"";
            f << "/>\n";
        };
        emit_num("ram_frequency_mhz", Cx::ram_frequency_mhz(), Cx::declared_machine()->ram_frequency_source);
        emit_num("cas_latency_cl", Cx::cas_latency_cl());
        // Glied 3 = das O-4a-Kern-Tupel vendor/family/model/stepping (OP-4-Format), NICHT das
        // symbolische XML-Etikett gleichen Namens -- jenes ist der Aufloesungs-Schluessel.
        auto const core = Cx::cpu_fabrication();
        f << "        <sub_dim id=\"cpu_fabrication\" declared=\"" << (core.declared ? "true" : "false") << "\"";
        if (core.declared) {
            f << " value=\"" << xml_escape(core.vendor) << "/" << core.family << "/" << core.model << "/"
              << core.stepping << "\"";
        }
        f << "/>\n";
        f << "      </complex>\n";
    };
    f << "    <sub_axis id=\"target_isa_complex\" parent=\"" << xml_escape(meas::TargetIsaAxisTag::axis_label())
      << "\" stage=\"ct\" kind=\"fixed_enum_tuple\""
      << " complex_count=\"" << meas::kAllTargetIsaComplexIds.size() << "\">\n";
    f << "      <!-- Die drei festen Glieder je benannter Rekombination (Owner OD-2). Werte kommen aus\n";
    f << "           kDeclaredMachines, das seinerseits <machines> der Anwender-XML spiegelt (O-4b).\n";
    f << "           declaration_source konserviert den eingefrorenen Ursprung einer deklarierten Zahl\n";
    f << "           (A2-Notiz, keine Stufe; Laufzeit-Stufen stehen NIE in dieser XML; P3). -->\n";
    emit_complex(std::type_identity<meas::Prod1Zen5TargetIsa>{});
    emit_complex(std::type_identity<meas::Prod2RaptorLakeTargetIsa>{});
    f << "    </sub_axis>\n";
}

/// O-8 Schritt 6: die zwei Unter-Achsen OHNE compile-statischen Options-Katalog (numa_node, page).
/// Ihre zulaessige Werte-Menge haengt an der Maschine und wird zur Laufzeit aufgeloest (OD-10,
/// Folge-Paket) -- hier steht das ANGEBOT, nicht die Aufloesung. Muster: die workload-Unter-Achse.
void emit_target_isa_open_sub_axes(std::ofstream& f) {
    auto emit_open = [&f]<class Sub>(std::type_identity<Sub>) {
        note_name(Sub::axis_label());
        f << "    <sub_axis id=\"" << xml_escape(Sub::axis_label()) << "\" parent=\""
          << xml_escape(Sub::parent_axis_label()) << "\" stage=\"runtime\" value_type=\"token\" option_source=\""
          << xml_escape(Sub::option_source()) << "\"/>\n";
    };
    emit_open(std::type_identity<meas::NumaNodeSubAxis>{});
    emit_open(std::type_identity<meas::PageSubAxis>{});
}

/// P5/A9a: Emissions-Koerper der Haupt-Achse "target_isa".
/// O-8 Schritt 6: target_isa ist jetzt der KOMPLEX-WRAPPER (Owner OD-2) -- zu den zwei ISA-Bausteinen
/// treten die drei festen Glieder als fixed_enum_tuple und die drei Unter-Achsen scheduling,
/// numa_node und page, die "allein der Komplex-Wrapper erhaelt".
void emit_system_axis_target_isa(std::ofstream& f) {
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
    emit_target_isa_complex_members(f);
    emit_target_isa_sub_axis_scheduling(f);
    emit_target_isa_open_sub_axes(f);
    f << "  </axis>\n";
}

/// A3 (O-8 Schritt 4): Emissions-Koerper der NEUEN Haupt-Achse "operating_system". Aufbau nach dem
/// target_isa-Muster (fixed_enum_tuple, OP-10): drei feste Klassen-Identitaeten als Bausteine.
/// Distribution/Version sind AUSDRUECKLICH keine Bausteine, sondern Stempel-Variablen aus <machines>.
void emit_system_axis_operating_system(std::ofstream& f) {
    emit_axis_open(f, meas::LinuxOperatingSystem::axis_label(), "ct", 3);
    {
        // Rein statische Reflexion wie beim target_isa-Emitter -- KEINE Instanzen: die CRTP-Basis
        // haelt ihren Konstruktor protected (leeres Dach, Anti-Runtime-Switch), eine Aggregat-
        // Initialisierung Os{} kaeme daran nicht vorbei.
        std::string extra =
            std::string{" posix=\""} + (meas::LinuxOperatingSystem::is_posix_family() ? "true" : "false") + "\"";
        emit_baustein<meas::LinuxOperatingSystem>(f, meas::LinuxOperatingSystem::os_family_id(), extra);
        extra = std::string{" posix=\""} + (meas::WindowsOperatingSystem::is_posix_family() ? "true" : "false") + "\"";
        emit_baustein<meas::WindowsOperatingSystem>(f, meas::WindowsOperatingSystem::os_family_id(), extra);
        extra = std::string{" posix=\""} + (meas::MacosOperatingSystem::is_posix_family() ? "true" : "false") + "\"";
        emit_baustein<meas::MacosOperatingSystem>(f, meas::MacosOperatingSystem::os_family_id(), extra);
    }
    f << "  </axis>\n";
}

/// P5/A9a war dies der Emissions-Koerper der HAUPT-Achse "scheduling".
/// A3 (O-8 Schritt 4) nahm scheduling aus kSystemAxisEmitters; O-8 Schritt 6 haengt es hier als
/// UNTER-Achse am target_isa-Komplex-Wrapper wieder ein (Owner OD-2). Damit ist es weder Haupt-Achse
/// noch geparkt, sondern an seinem endgueltigen Platz.
/// UNVERAENDERT bleiben die 5 Sub-Dimensionen und ihre Werte: der Umzug ist eine Struktur-Aenderung,
/// keine Inhalts-Aenderung. Das `parent` kommt jetzt aus dem TYP (CebSubAxis), nicht mehr aus dem
/// eigenen Label -- deshalb steht hier parent_axis_label() und nicht axis_label().
void emit_target_isa_sub_axis_scheduling(std::ofstream& f) {
    // Die id-Etiketten der Sub-Dims haben KEINE string-Single-Source (nur getypte Accessoren) -> sie sind
    // an den Accessor-AUFRUF compile-gekoppelt (Rename bricht diesen Build). Enum-WERTE als Ordinal reflektiert.
    note_name(meas::DefaultSchedulingSystemAxis::axis_label());
    f << "    <sub_axis id=\"" << xml_escape(meas::DefaultSchedulingSystemAxis::axis_label()) << "\" parent=\""
      << xml_escape(meas::DefaultSchedulingSystemAxis::parent_axis_label())
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

/// P5/A9a: Emissions-Koerper der Haupt-Achse "load_framework" (1:1 aus main() herausgezogen, Inhalt
/// UNVERAENDERT -- die XML muss byte-identisch bleiben). Aufgerufen ausschliesslich ueber die
/// kSystemAxisEmitters-Tabelle, die kSystemAxisOrder folgt.
[[maybe_unused]] void emit_system_axis_load_framework(std::ofstream& f) {
    // -- 5) load_framework (LoadFrameworkMeasurementAxis): 1 Baustein ycsb + dyn. Unter-Achse "workload". --
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
}

// =====================================================================================================
// P5/A9a -- EMITTER-TABELLE: der Generator ITERIERT die Ordnung, statt sie von Hand aufzurufen.
//
// VORHER standen hier fuenf handgeschriebene emit_axis_open-Aufrufe hintereinander in main(). Die
// Reihenfolge war damit eine EIGENSCHAFT DES KONTROLLFLUSSES - unlesbar fuer jede Wache und nur durch
// Lesen der Datei pruefbar. JETZT ist sie ein Datum: diese Tabelle. main() laeuft sie ab.
//
// WARUM Funktions-Zeiger und kein mp11-Fold: mp11 ist in dieser TU nicht verfuegbar (nur type_name.hpp,
// kein boost/mp11-Include), und dieser Generator ist ein BAU-ZEIT-WERKZEUG, kein Mess-Pfad -- die
// Zero-Cost-/Kein-Runtime-Switch-Direktive schuetzt die gemessene Bibliothek, nicht dieses Tool. Es ist
// ausserdem KEIN Switch: die Tabelle wird positions-getreu abgelaufen, es gibt keine Verzweigung auf
// Laufzeit-Zustand und keine String-Dispatch.
//
// Die Tabelle traegt das Label aus dem REALEN Typ (nicht als Literal) -- NETZ 1 unten prueft, dass ihre
// Folge kSystemAxisOrder ist, NETZ 2 in main prueft, dass die Emission ihr wirklich folgt.
// =====================================================================================================
struct SystemAxisEmitter {
    std::string_view label;       ///< aus <Typ>::axis_label(), nie handgeschrieben
    void (*emit)(std::ofstream&); ///< der herausgezogene Emissions-Koerper
};

inline constexpr std::array<SystemAxisEmitter, cabi::kSystemAxisOrderCount> kSystemAxisEmitters{{
    {meas::X86_64TargetIsa::axis_label(), &emit_system_axis_target_isa},
    {meas::LinuxOperatingSystem::axis_label(), &emit_system_axis_operating_system},
    {meas::SimdExternalUtilsFamily::axis_label(), &emit_system_axis_external_utils},
}};

[[nodiscard]] consteval bool emitter_table_matches_order() {
    if (kSystemAxisEmitters.size() != cabi::kSystemAxisOrderCount) return false;
    for (std::size_t i = 0; i < cabi::kSystemAxisOrderCount; ++i) {
        if (kSystemAxisEmitters[i].label != cabi::kSystemAxisOrder[i]) return false;
        if (kSystemAxisEmitters[i].emit == nullptr) return false;
    }
    return true;
}
// =====================================================================================================
// NETZ 4 (T-c): je System-HAUPT-Achse eine bewusste Suffix-Entscheidung.
//
// Die Tabelle ordnet jeder Achse aus kSystemAxisOrder ihr Suffix-Segment zu -- oder ausdruecklich
// KEINES. "Kein Segment" ist eine gueltige, aber BENANNTE Antwort: operating_system erscheint heute
// nicht im build_version, weil die Flotte je Runner ohnehin genau ein OS faehrt und die Distro eine
// Stempel-Variable ist (OP-10). Wer das aendert, aendert es hier sichtbar.
// =====================================================================================================
struct SuffixSegmentBinding {
    std::string_view axis;    ///< aus kSystemAxisOrder, nie handgeschrieben
    std::string_view segment; ///< aus kSuffixSegmentOrder, oder leer = bewusst kein Segment
};

inline constexpr std::array<SuffixSegmentBinding, cabi::kSystemAxisOrderCount> kSuffixSegmentBindings{{
    {meas::X86_64TargetIsa::axis_label(), cpf::kSuffixSegmentOrder[4]},         // "+target=" (nur bei Cross)
    {meas::LinuxOperatingSystem::axis_label(), std::string_view{}},             // bewusst KEIN Segment (OP-10)
    {meas::SimdExternalUtilsFamily::axis_label(), cpf::kSuffixSegmentOrder[2]}, // "+ext=" (Hub-Auspraegung)
}};

[[nodiscard]] consteval bool suffix_bindings_cover_axis_order() {
    if (kSuffixSegmentBindings.size() != cabi::kSystemAxisOrderCount) return false;
    for (std::size_t i = 0; i < cabi::kSystemAxisOrderCount; ++i)
        if (kSuffixSegmentBindings[i].axis != cabi::kSystemAxisOrder[i]) return false;
    return true;
}
[[nodiscard]] consteval bool suffix_bindings_are_known_segments() {
    for (auto const& b : kSuffixSegmentBindings) {
        if (b.segment.empty()) continue; // benanntes "kein Segment"
        bool found = false;
        for (auto const& seg : cpf::kSuffixSegmentOrder)
            if (seg == b.segment) found = true;
        if (!found) return false;
    }
    return true;
}
static_assert(suffix_bindings_cover_axis_order(),
              "T-c/NETZ 4: fuer JEDE System-Haupt-Achse aus kSystemAxisOrder muss hier eine "
              "Suffix-Entscheidung stehen -- ein Segment ODER ausdruecklich keines. Eine Achse ist "
              "eingetreten, ausgetreten oder umsortiert worden, ohne dass jemand entschieden hat, wie "
              "sie im build_version erscheint.");
static_assert(suffix_bindings_are_known_segments(),
              "T-c/NETZ 4: ein hier genanntes Suffix-Segment steht nicht in kSuffixSegmentOrder "
              "(profile_facade/system_version_suffix.hpp) -- die Segment-Namen haben EINE Quelle.");

static_assert(emitter_table_matches_order(),
              "P5/A9a: die Emitter-Tabelle folgt nicht abi::kSystemAxisOrder (Reihenfolge, Zahl oder ein "
              "nullptr-Emitter). kSystemAxisOrder ist die Single-Source (A1) - dort aendern, nicht hier.");

/// O-8 Schritt 6 (Bauplan IV.2.7 / O-1r, Owner-GO OP-5): die AEUSSERE System-Komplex-Haupt-Achse.
///
/// WARUM SIE NICHT IN kSystemAxisEmitters STEHT: sie ist keine vierte Haupt-Achse, sondern die
/// KLAMMER um die drei ("alle 3 sind Haupt-Achsen, verhalten sich aber wie EINE"). Sie hier in die
/// Tabelle zu haengen wuerde kSystemAxisOrderCount auf 4 treiben und damit den Owner-KERN "GENAU DREI
/// Haupt-Achsen" brechen -- die drei Ordnungs-Netze wuerden das auch sofort melden. Sie wird deshalb
/// NACH der Ordnungs-Schleife emittiert, wie machine_signatures auch.
///
/// Die Glieder-Liste kommt aus member_labels() des Typs, nie aus Literalen: driftet die Rekombination,
/// driftet die XML mit, und die compile-time-Wachen im Header schlagen zuerst an.
void emit_system_complex_axis(std::ofstream& f) {
    note_name(meas::CebCompoundSystemAxis::axis_label());
    auto const members = meas::CebCompoundSystemAxis::member_labels();
    f << "  <system_complex_axis id=\"" << xml_escape(meas::CebCompoundSystemAxis::axis_label())
      << "\" category=\"system_config\" binary_id=\"never\" stage=\"ct\" member_count=\"" << members.size() << "\">\n";
    f << "    <!-- Command-Pattern-Wrapper der Rekombination der DREI Haupt-Achsen (O-1r). Er ist KEINE\n";
    f << "         vierte Haupt-Achse: kSystemAxisOrder bleibt bei dreien. load_framework ist KEIN Glied\n";
    f << "         (es lebt seit A3 im Mess-Realm), die System-Meta-Metas fahren unter external_utils. -->\n";
    for (auto const& m : members) f << "    <member axis=\"" << xml_escape(m) << "\"/>\n";
    f << "    <sub_axis_group id=\"" << xml_escape(meas::BuildToolchainSubAxes::group_label()) << "\" sub_axis_count=\""
      << meas::BuildToolchainSubAxes::size() << "\">\n";
    f << "      <!-- compiler + opt_level + atomic128 haengen HIER und nicht an target_isa: die Gruppe\n";
    f << "           beschreibt den BAU aus der Rekombination aller drei Achsen (O-1r). Die Liste nennt\n";
    f << "           die drei Glieder flach; die Verschachtelung darunter zeigt die tatsaechliche\n";
    f << "           Eltern-Kette (opt_level und atomic128 haengen unveraendert an compiler). -->\n";
    for (auto const& g : meas::BuildToolchainSubAxes::labels())
        f << "      <group_member axis=\"" << xml_escape(g) << "\"/>\n";
    emit_compound_sub_axis_group_compiler(f);
    f << "    </sub_axis_group>\n";
    f << "  </system_complex_axis>\n";
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
    f << "       editieren. Ledger §28/§30: System-Art-Registry im Mess-/System-Modul (Angebot fuer System->CEB). "
         "-->\n";
    f << "  <!-- Haupt-Achse=CT-statisch (stage=ct, in die CEB/Tier-Binary einkompiliert); Unter-Achse=dynamisch\n";
    f << "       (stage=runtime, vom Planer permutiert). Q2/K2: opt_level/simd/atomic128 materialisieren als\n";
    f << "       CompileFn-Flags (build_version-Suffix), NIE als Laufzeit-Typ-Switch und NIE in der binary_id. -->\n";
    f << "  <!-- AUSSCHLUSS: extension_hardware_system_axis.hpp = DEPRECATED-Insel; hardware_isa_system_axis.hpp =\n";
    f << "       HOST-Deskriptor (treibt NICHT den Bau) -- beide NICHT Teil dieses Bau-treibenden Angebots. -->\n";

    // -- P5/A9a: die Ordnung wird ABGELAUFEN, nicht wiederholt. Fuenf Handaufrufe sind hier entfallen. --
    for (auto const& e : kSystemAxisEmitters) e.emit(f);

    // == O-8 Schritt 6 (IV.2.7/O-1r): die AEUSSERE Komplex-Achse. NACH der Ordnungs-Schleife, weil sie
    //    die Klammer um die drei Haupt-Achsen ist und keine vierte von ihnen (siehe Funktions-Kommentar). ==
    emit_system_complex_axis(f);

    // == Section 40.a: deklarierte per-Maschine SIMD-Flag-Signaturen (reflektiert aus machine_simd_signature.hpp,
    //    Host-Capability-Domaene). DIESE Signatur entscheidet den Bau (Gate Organ <= Signatur geschnitten
    //    Sinnhaftigkeit); die grobe simd-Sub-Achse bleibt Runner-Routing-Vorstufe. NIE binary_id. ==
    {
        f << "  <machine_signatures count=\"3\">\n";
        // static-only Zugriff (die Signatur-Typen werden NIE instanziiert -- wie alle Achsen-Structs);
        // std::type_identity traegt den Typ ohne Konstruktion (der Basis-ctor ist bewusst protected).
        auto emit_machine = [&f]<class Sig>(std::type_identity<Sig>) {
            note_name(Sig::machine_id());
            f << "    <machine id=\"" << xml_escape(Sig::machine_id()) << "\" host_isa=\""
              << xml_escape(Sig::host_isa()) << "\" flag_count=\"" << Sig::signature().size() << "\">\n";
            for (auto const& fl : Sig::signature()) f << "      <flag cpuinfo=\"" << xml_escape(fl.cpuinfo) << "\"/>\n";
            f << "    </machine>\n";
        };
        emit_machine(std::type_identity<meas::Prod1Zen5Signature>{});
        emit_machine(std::type_identity<meas::Prod2RaptorLakeSignature>{});
        emit_machine(std::type_identity<meas::OdroidGracemontSignature>{});
        f << "  </machine_signatures>\n";
    }

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

    // -- P5/A7' NETZ 2: ORDNUNGS-WACHE ueber die TATSAECHLICHE Emissions-Folge der Haupt-Achsen. --
    //    NETZ 1 (static_assert oben) prueft die DEKLARIERTE Typ-Folge; nur dieses Netz belegt, dass die
    //    emit_axis_open-AUFRUFE ihr wirklich folgen. Bricht mit rc!=0, damit Roundtrip-/Build-Schritt es sieht.
    //    Der XML-Kopf ist damit mit-abgedeckt: die <axis>-Elemente stehen in genau dieser Aufruf-Folge in
    //    der Datei, also ist "XML-Kopf == kSystemAxisOrder" die direkte Folge dieser Pruefung.
    if (g_system_axis_order.size() != cabi::kSystemAxisOrderCount) {
        std::cerr << "system_axis_registry_gen: ORDNUNGS-WACHE - " << g_system_axis_order.size()
                  << " Haupt-Achsen emittiert, kSystemAxisOrder erwartet " << cabi::kSystemAxisOrderCount
                  << " (Single-Source abi/system_axis_order.hpp).\n";
        return 5;
    }
    for (std::size_t i = 0; i < cabi::kSystemAxisOrderCount; ++i) {
        if (g_system_axis_order[i] != cabi::kSystemAxisOrder[i]) {
            std::cerr << "system_axis_registry_gen: ORDNUNGS-WACHE - Position " << i << " ist '"
                      << g_system_axis_order[i] << "', kSystemAxisOrder verlangt '" << cabi::kSystemAxisOrder[i]
                      << "'. Generator-Aufruffolge und Single-Source sind auseinandergelaufen.\n";
            return 5;
        }
    }

    // NETZ 3: die Anzahl kommt aus der Single-Source, nicht aus einem Literal. Byte-neutral - der Text
    // kommt aus kSystemAxisOrderCount; mit A3 (O-8 Schritt 4) meldet sie automatisch 3 statt frueher 5.
    std::cout << "system_axis_registry_gen: " << cabi::kSystemAxisOrderCount
              << " System-Achsen-Elemente (opt_level/atomic128/simd als sub_axis), " << g_baustein_total
              << " Bausteine, simd_feature_catalog=" << meas::kSimdFeatureFlagCatalog.size()
              << " Flags, machine_signatures=3 -> " << out_path << "\n";
    return 0;
}
