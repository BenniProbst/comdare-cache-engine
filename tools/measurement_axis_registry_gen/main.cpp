// PAKET W2-B (2026-07-19) - measurement_axis_registry_gen: erzeugt die Mess-Achsen-Registry-XML
// `measurement_axis_registry.xml` per COMPILE-TIME-REFLEKTION der realen Mess-Achsen-Typen aus dem
// Mess-Modul (measurement/measurement_axis_registry.hpp + system_axis.hpp).
//
// WAS (Ledger §28/§30): die Mess-Achsen-ART (system_measurement, "Blut") bekommt ihre EIGENE Registry-
// Bibliothek in IHREM Modul -- das Angebot des Compiles fuer die PLANER-Stufe (Mess->Planer). Reflektiert
// werden: (1) die 16 MeasurementCategory-Achsen (kMeasurementAxisRegistry), (2) die 3 Kollektoren
// (WallClock/ObserverSnapshot/Pmc SystemAxis) mit ihren zugeordneten Kategorien + Regime, (3) die
// dynamischen Mess-Dimensionen (RC-POD-gekoppelt + workload). Kein zweiter Parser: die Wurzel bleibt
// <comdare_axis_registry> mit <axis id=...>/<baustein name=...> (read_axis_registry-kompatibel).
//
// PFLICHT-LEITPLANKE (Kapseln-Regel K1..K3, axis_binding-Anti-Lehre): NUR ueber die realen CT-Klassen
// reflektieren, NIE handschreiben. Kategorie-Namen kommen aus kMeasurementAxisRegistry (E4-CSV-Vokabular-
// Single-Source); Kollektor-Kategorien aus <Collector>::do_categories() (auf axis_info(cat).name
// abgebildet); Regime aus regime()/regime_of (Ordinal). Die RC-POD-Dim-id-Strings sind an den
// Feldzugriff auf ComdareResourceControlV1 compile-gekoppelt (ein Feld-Rename bricht DIESEN Generator-
// Build -> kein stiller XML-Phantom). Ein Laufzeit-GUARD verwirft unsaubere Namen (kein '(' / ' ' / leer).
// Konsument+Contract-Test IM SELBEN Increment = der registry_roundtrip-ctest.
//
// EHRLICHE LUECKEN (NUR reflektieren was im Code existiert, sonst TODO -- §32-F1/F7):
//   - repetition: eine reine setting_label-DynDim OHNE CT-Anker (kein Feld/keine Methode) -> TODO, nicht
//     emittiert (waere ein handgeschriebenes Phantom).
//   - 3 Mess-Modi Debug/Mess/Release: existieren NICHT als Typen -> TODO, nicht emittiert.
//   - Regime-/Kategorie-Ordinal statt Label: MeasurementRegime hat keine string-Label-Single-Source ->
//     Ordinal reflektiert (0=TimeObserver, 1=PmcCounter laut system_axis.hpp).
//
// binary_id-DOKTRIN: Mess-Achsen ("Blut") stehen NIE in kCompositionAxisNames -> binary_id="never"
// (golden unberuehrt; Mess-Werte sind CSV-Spalten/setting_label, nie binary_id).
//
// C++23. Ausfuehren: `measurement_axis_registry_gen --out <pfad>`.

#include <builder/codegen/type_name.hpp> // type_name<W>() (FQ-Typ der Kollektoren)

#include <anatomy/resource_controllable_tier.hpp>            // ComdareResourceControlV1 + kResourceControlVersion
#include <cache_engine/measurement/load_framework_system_axis.hpp> // workload-Unter-Achsen-Label (Single-Source)
#include <cache_engine/measurement/measurement_axis_registry.hpp>  // kMeasurementAxisRegistry + for_each + axis_info
#include <cache_engine/measurement/system_axis.hpp>                // 3 Kollektoren + MeasurementRegime + regime_of

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace cg   = ::comdare::cache_engine::builder::codegen;
namespace meas = ::comdare::cache_engine::measurement;
namespace anat = ::comdare::cache_engine::anatomy;

namespace {

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

template <class W>
[[nodiscard]] std::string fq_type() {
    return "::" + std::string{cg::type_name<W>()};
}

[[nodiscard]] std::string short_name(std::string const& fq) {
    std::size_t const lt   = fq.find('<');
    std::string const head = (lt == std::string::npos) ? fq : fq.substr(0, lt);
    std::size_t const cc   = head.rfind("::");
    return (cc == std::string::npos) ? head : head.substr(cc + 2);
}

[[nodiscard]] bool name_is_clean(std::string_view n) {
    return !n.empty() && n.find('(') == std::string_view::npos && n.find(' ') == std::string_view::npos;
}

std::vector<std::string> g_names;
void                     note_name(std::string_view n) { g_names.emplace_back(n); }

// Emittiert einen Kollektor-Baustein (WallClock/ObserverSnapshot/Pmc): FQ-Typ + Regime-Ordinal + die je
// Kollektor zugeordneten Kategorien (aus do_categories(), auf axis_info(cat).name abgebildet).
template <class Collector>
void emit_collector(std::ofstream& f) {
    std::string const type = fq_type<Collector>();
    std::string const name = short_name(type);
    note_name(name);
    f << "    <baustein name=\"" << xml_escape(name) << "\" wrapper=\"" << xml_escape(name) << "\" type=\""
      << xml_escape(type) << "\" enabled=\"true\" regime_ordinal=\""
      << static_cast<std::uint32_t>(Collector::regime()) << "\">\n";
    for (meas::MeasurementCategory const cat : Collector::do_categories()) {
        std::string_view const cname = meas::axis_info(cat).name;
        f << "      <category ref=\"" << xml_escape(cname) << "\"/>\n";
    }
    f << "    </baustein>\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string out_path = "measurement_axis_registry.xml";
    for (int i = 1; i < argc; ++i) {
        std::string_view const a = argv[i];
        if (a == "--out" && i + 1 < argc) {
            out_path = argv[++i];
        } else if (a == "--help" || a == "-h") {
            std::cout << "measurement_axis_registry_gen --out <file>\n";
            return 0;
        } else {
            std::cerr << "measurement_axis_registry_gen: unbekanntes Argument '" << a << "'\n";
            return 2;
        }
    }

    std::ofstream f{out_path, std::ios::binary};
    if (!f) {
        std::cerr << "measurement_axis_registry_gen: kann Ausgabedatei nicht oeffnen: " << out_path << "\n";
        return 4;
    }

    f << "<comdare_axis_registry engine=\"cache_engine_measurement\" schema=\"1\" "
         "generator=\"measurement_axis_registry_gen\">\n";
    f << "  <!-- GENERIERT von tools/measurement_axis_registry_gen (PAKET W2-B) per compile-time-Reflektion\n";
    f << "       der realen Mess-Achsen-Typen (measurement/measurement_axis_registry.hpp + system_axis.hpp).\n";
    f << "       NICHT von Hand editieren. Ledger §28/§30: Mess-Art-Registry im Mess-Modul (Angebot Mess->Planer). -->\n";
    f << "  <!-- Mess-Achsen (\"Blut\") stehen NIE in der binary_id (binary_id=never); Werte = CSV-Spalten/\n";
    f << "       setting_label. Regime-Ordinal: 0=TimeObserver, 1=PmcCounter (system_axis.hpp regime_of). -->\n";

    // ── 1) Die 16 MeasurementCategory-Achsen (kMeasurementAxisRegistry, reflektiert via for_each_measurement_axis). ──
    f << "  <axis id=\"measurement_category\" category=\"system_measurement\" axis_kind=\"system_measurement\""
      << " binary_id=\"never\" stage=\"ct\" baustein_count=\"" << meas::kMeasurementAxisCount << "\">\n";
    meas::for_each_measurement_axis([&](meas::MeasurementAxisInfo const& info) {
        note_name(info.name);
        f << "    <baustein name=\"" << xml_escape(info.name) << "\" category_ordinal=\""
          << static_cast<std::uint32_t>(info.category) << "\" regime_ordinal=\""
          << static_cast<std::uint32_t>(info.regime) << "\" enabled=\"true\"/>\n";
    });
    f << "  </axis>\n";

    // ── 2) Die 3 Kollektoren (Mess-Quellen) mit ihren Kategorien + Regime. ──
    f << "  <axis id=\"collector\" category=\"system_measurement\" axis_kind=\"system_measurement\""
      << " binary_id=\"never\" stage=\"ct\" baustein_count=\"3\">\n";
    emit_collector<meas::WallClockSystemAxis>(f);
    emit_collector<meas::ObserverSnapshotSystemAxis>(f);
    emit_collector<meas::PmcSystemAxis>(f);
    f << "  </axis>\n";

    // ── 3) Die dynamischen Mess-Dimensionen (RuntimeVariableLoop). RC-POD-Dims sind an den Feldzugriff auf
    //    ComdareResourceControlV1 compile-gekoppelt; workload aus dem realen Unter-Achsen-Label reflektiert. ──
    // Compile-Kopplung: ein Feld-Rename am RC-POD bricht DIESE Zugriffe (kein stiller XML-Phantom). Die
    // id-Strings bleiben Literale (C++23 hat keine Feld-Namen-Reflexion), sind aber an die Felder gebunden.
    constexpr anat::ComdareResourceControlV1 kRcProbe{};
    static_cast<void>(kRcProbe.thread_count);
    static_cast<void>(kRcProbe.prefetch_distance);
    static_cast<void>(kRcProbe.pool_budget_bytes);
    static_cast<void>(kRcProbe.batch_size);
    static_cast<void>(kRcProbe.inline_threshold_bytes);

    f << "  <dynamic_dims resource_control_version=\"" << anat::kResourceControlVersion << "\">\n";
    f << "    <!-- workload: Single-Source LoadFrameworkSystemAxis::sub_axis_label() (Angebots-Eigner =\n";
    f << "         System-Registry load_framework); hier als Mess-Sweep-Dimension referenziert. -->\n";
    {
        std::string_view const wl = meas::YcsbLoadFrameworkAxis::sub_axis_label();
        note_name(wl);
        f << "    <dim id=\"" << xml_escape(wl)
          << "\" stage=\"runtime\" source=\"system:load_framework\" value_type=\"token\"/>\n";
    }
    f << "    <!-- RC-POD-gekoppelte Dimensionen (ComdareResourceControlV1-Felder; setting_label axis.var=value). -->\n";
    for (std::string_view const dim :
         {std::string_view{"thread_count"}, std::string_view{"prefetch_distance"},
          std::string_view{"pool_budget_bytes"}, std::string_view{"batch_size"},
          std::string_view{"inline_threshold_bytes"}}) {
        note_name(dim);
        f << "    <dim id=\"" << xml_escape(dim) << "\" stage=\"runtime\" source=\"resource_control_pod\""
          << " value_type=\"uint\"/>\n";
    }
    f << "    <!-- TODO(W2-B): repetition ist eine reine setting_label-DynDim OHNE CT-Anker -> nicht emittiert\n";
    f << "         (kein handgeschriebenes Phantom). Erst mit einer constexpr-Namens-Single-Source reflektierbar. -->\n";
    f << "    <!-- TODO(W2-B, §32-F1/F7): die 3 Mess-Modi Debug/Mess/Release existieren NICHT als Typen ->\n";
    f << "         nicht emittiert; erst nach ihrer Typisierung als Mess-Unter-Achse reflektierbar. -->\n";
    f << "  </dynamic_dims>\n";

    f << "</comdare_axis_registry>\n";
    f.flush();

    for (auto const& n : g_names) {
        if (!name_is_clean(n)) {
            std::cerr << "measurement_axis_registry_gen: GUARD-BRUCH - unsauberer Name/Id '" << n << "'.\n";
            return 3;
        }
    }
    if (!f) {
        std::cerr << "measurement_axis_registry_gen: Schreibfehler an " << out_path << "\n";
        return 4;
    }

    std::cout << "measurement_axis_registry_gen: 16 Kategorien + 3 Kollektoren + 6 dyn. Dimensionen -> " << out_path
              << "\n";
    return 0;
}
