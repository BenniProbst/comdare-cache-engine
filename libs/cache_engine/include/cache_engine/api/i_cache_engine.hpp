// SPDX-License-Identifier: Apache-2.0
// V41.E11 (Skelett 2026-05-25) → F5.R1 (#35, 2026-07-16) — ICacheEngine: Master-Framework-Gesamt-Fassade.
//
// ZWECK: cache-engine ist Master-Framework. Konsumenten (Diplomarbeit / ExperimentDriver) linken NUR gegen
// cache-engine und benutzen EINE Tür — get_cache_engine() — statt die einzelnen Subsystem-Header direkt
// einzubinden. Die Fassade ist reine Vermittlung ÜBER den HEUTE existierenden in-repo-Subsystemen; sie ist
// keine Parallelstruktur und hält weder Baseline-Zellen noch ABI-/golden-Artefakte.
//
// F5.R1-HISTORIE: Das 25.05.-Skelett deklarierte get_cache_engine() (Zeile 47), definierte es aber NIE →
// latente Linker-Falle; die 6 Provider waren nur forward-declared und zielten auf leere modules/-Tombstones.
// R1 (non-gated, gegen die HEUTIGEN Subsysteme) schließt die Falle nach dem F.4-Blaupausen-Muster
// (api/i_cache_engine_tools.hpp, get_cache_engine_tools() mit header-only Default-Impl):
//   • VERDRAHTET (real, in-repo):
//       measurement   → cache_engine/measurement (MeasurementCategory-System-Achsen-Registry)
//       isa_dispatch  → cache_engine/platform_probe (CPUID-Auto-Discovery)
//       build_tools   → builder/codegen (vermittelt über die schon funktionierende F.4-Tools-Fassade)
//       test_system   → test_infra/workload_generator (ebenfalls über F.4 — DRY, keine Duplikate)
//   • DEFERRED-#274 (V42): search_engine + cache_engine_core zielen auf die via #274 noch NICHT
//       geschnittenen Auslagerungs-Module. EHRLICH deferred — kein Fake-Verhalten, keine Fassaden-Accessoren.
//       Der Modul-Schnitt ist R2 (user-gated Fork-4), NICHT Teil von R1. deferred_providers() macht den
//       Zustand testbar sichtbar, statt ihn zu stubben.
//
// R3-ABGRENZUNG (Doppelbelegung "ICacheEngine"): Es gibt ZWEI Typen dieses Namens in DISTINKTEN Namespaces —
// KEINE Sprach-Kollision, nur potentielle Leser-Verwechslung, hier autoritativ per Rollen-Doku aufgelöst:
//   • comdare::cache_engine::api::ICacheEngine (DIESE Datei) = MODUL-VERMITTLUNGS-Fassade (Facade-Pattern,
//     virtuelle Host-/Builder-seitige Vermittlung der Subsysteme; NICHT im Hot-Path).
//   • comdare::cache_engine::ICacheEngine (concepts/i_cache_engine.hpp) = U09-PIPELINE-Wurzel (Visitor +
//     Mediator + State-Holder über die 12 ISubEngine-Stufen; die eigentliche Empfehlungs-Pipeline).
// Rollen-Doku wurde der Umbenennung vorgezogen, weil sie die KLEINERE, ripple-freie Variante ist: die beiden
// Typen liegen bereits in getrennten Namespaces (api:: vs. cache_engine::), sodass der Compiler sie ohnehin
// unterscheidet; eine Umbenennung würde den load-bearing Symbolnamen get_cache_engine()/ICacheEngine, den die
// gesamte F5-Kette + README referenzieren, ohne technischen Gewinn brechen.

#pragma once

#include <cache_engine/api/i_cache_engine_tools.hpp> // ICodegenTool / IWorkloadTool / get_cache_engine_tools()
#include <cache_engine/measurement/measurement_axis_registry.hpp> // kMeasurementCategoryCount / axis_info()
#include <cache_engine/measurement/measurement_category.hpp>      // MeasurementCategory
#include <cache_engine/platform_probe/cpuid_probe.hpp>            // CpuidProbeResults / probe_cpuid()

#include <cstddef>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::api {

// ─────────────────────────────────────────────────────────────────────────────
// Provider-Interfaces (pure virtual) — je in-repo-Subsystem eine schmale, representative Vermittlung.
// ─────────────────────────────────────────────────────────────────────────────

/// Measurement-Subsystem: die 16 MeasurementCategory-System-Mess-Achsen ("Blut", Dossier 19 Querschnitt M)
/// aus cache_engine/measurement/measurement_axis_registry.hpp (constexpr-Single-Source, kein Runtime-Switch).
class IMeasurementProvider {
public:
    virtual ~IMeasurementProvider() = default;
    /// Anzahl der System-Mess-Achsen (== kMeasurementCategoryCount, static_assert-gesichert).
    [[nodiscard]] virtual std::size_t system_axis_count() const = 0;
    /// Enum-Name einer Mess-Kategorie (E4-Reporting/CSV-Spalten-Vokabular).
    [[nodiscard]] virtual std::string_view axis_name(measurement::MeasurementCategory c) const = 0;
};

/// ISA-Dispatch-/Plattform-Subsystem: CPUID-Auto-Discovery aus cache_engine/platform_probe/cpuid_probe.hpp
/// (Multi-OS x86_64/ARM64/RISC-V, header-only, OS-Enablement-gated für AVX/AVX-512).
class IIsaDispatchProvider {
public:
    virtual ~IIsaDispatchProvider() = default;
    /// Frischer Plattform-Probe (Vendor, Brand, ISA-Flags, Cache-Line, Cores).
    [[nodiscard]] virtual platform_probe::CpuidProbeResults detect() const = 0;
};

/// Build-Tools-Subsystem: Modul-Codegen aus builder/codegen. Vermittelt die BEREITS funktionierende
/// F.4-Tools-Fassade (ICodegenTool) statt den Emitter zu duplizieren (DRY, Single-Source render_adhoc_*).
class IBuildToolsProvider {
public:
    virtual ~IBuildToolsProvider()                = default;
    [[nodiscard]] virtual ICodegenTool& codegen() = 0;
};

/// Test-System-Subsystem: YCSB-Workload-Generierung aus test_infra/workload_generator. Ebenfalls über die
/// F.4-Tools-Fassade (IWorkloadTool) vermittelt — keine Duplikate.
class ITestSystemProvider {
public:
    virtual ~ITestSystemProvider()                   = default;
    [[nodiscard]] virtual IWorkloadTool& workloads() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// DEFERRED-#274 (V42): Provider für die via #274 noch NICHT geschnittenen Auslagerungs-Module.
// Interface-Deklarationen halten die Taxonomie sichtbar; sie werden NICHT gestubbt und NICHT an die Fassade
// angeschlossen, bis Fork-4/#274 (R2) die Module schneidet. "Unverdrahtet ≠ tot" (Doktrin).
// ─────────────────────────────────────────────────────────────────────────────
class ISearchEngineProvider { // DEFERRED-#274 (V42) — Ziel: comdare-search-engine
public:
    virtual ~ISearchEngineProvider() = default;
};
class ICacheEngineCoreProvider { // DEFERRED-#274 (V42) — Ziel: comdare-cache-engine-core
public:
    virtual ~ICacheEngineCoreProvider() = default;
};

/// Ehrliche Auskunft über einen noch nicht verdrahteten (deferred) Provider — kein Fake-Verhalten.
struct DeferredProvider {
    std::string_view name;      ///< z.B. "search_engine"
    std::string_view issue;     ///< z.B. "#274"
    std::string_view milestone; ///< z.B. "V42 / Fork-4"
    std::string_view reason;    ///< warum noch nicht verdrahtet
};

// ─────────────────────────────────────────────────────────────────────────────
// Master-Framework-Fassade. Konsumenten linken NUR gegen cache-engine + benutzen ICacheEngine.
// ─────────────────────────────────────────────────────────────────────────────
class ICacheEngine {
public:
    virtual ~ICacheEngine() = default;

    // Vermittlungs-Accessoren — NUR die heute existierenden in-repo-Subsysteme. Owner bleibt die Fassade.
    [[nodiscard]] virtual IMeasurementProvider& measurement()  = 0;
    [[nodiscard]] virtual IIsaDispatchProvider& isa_dispatch() = 0;
    [[nodiscard]] virtual IBuildToolsProvider&  build_tools()  = 0;
    [[nodiscard]] virtual ITestSystemProvider&  test_system()  = 0;

    /// Ehrliche Liste der #274-deferred Provider (search-engine / core) — statt Fassaden-Accessoren mit Fake.
    [[nodiscard]] virtual std::vector<DeferredProvider> deferred_providers() const = 0;

    [[nodiscard]] virtual std::string_view framework_version() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Header-only Default-Implementation — adaptiert die ECHTEN in-repo-Subsysteme (F.4-Blaupausen-Muster).
// ─────────────────────────────────────────────────────────────────────────────

class DefaultMeasurementProvider final : public IMeasurementProvider {
public:
    [[nodiscard]] std::size_t      system_axis_count() const override { return measurement::kMeasurementCategoryCount; }
    [[nodiscard]] std::string_view axis_name(measurement::MeasurementCategory c) const override {
        return measurement::axis_info(c).name;
    }
};

class DefaultIsaDispatchProvider final : public IIsaDispatchProvider {
public:
    [[nodiscard]] platform_probe::CpuidProbeResults detect() const override { return platform_probe::probe_cpuid(); }
};

class DefaultBuildToolsProvider final : public IBuildToolsProvider {
public:
    // DRY: builder/codegen ist in der F.4-Tools-Fassade schon adaptiert — hier vermittelt, nicht dupliziert.
    [[nodiscard]] ICodegenTool& codegen() override { return get_cache_engine_tools().codegen(); }
};

class DefaultTestSystemProvider final : public ITestSystemProvider {
public:
    [[nodiscard]] IWorkloadTool& workloads() override { return get_cache_engine_tools().workload_generator(); }
};

class DefaultCacheEngine final : public ICacheEngine {
public:
    [[nodiscard]] IMeasurementProvider& measurement() override { return measurement_; }
    [[nodiscard]] IIsaDispatchProvider& isa_dispatch() override { return isa_; }
    [[nodiscard]] IBuildToolsProvider&  build_tools() override { return build_tools_; }
    [[nodiscard]] ITestSystemProvider&  test_system() override { return test_system_; }

    [[nodiscard]] std::vector<DeferredProvider> deferred_providers() const override {
        return {
            DeferredProvider{"search_engine", "#274", "V42 / Fork-4",
                             "search-engine-Auslagerungsmodul noch nicht geschnitten"},
            DeferredProvider{"cache_engine_core", "#274", "V42 / Fork-4",
                             "cache-engine-core-Auslagerungsmodul noch nicht geschnitten"},
        };
    }

    [[nodiscard]] std::string_view framework_version() const override { return "comdare-cache-engine-v1"; }

private:
    DefaultMeasurementProvider measurement_{};
    DefaultIsaDispatchProvider isa_{};
    DefaultBuildToolsProvider  build_tools_{};
    DefaultTestSystemProvider  test_system_{};
};

/// Factory-Function — eine prozessweite Default-Instanz (inline ⇒ EIN static-Local über alle TUs, ODR-sicher).
/// Schließt die F5-Linker-Falle: get_cache_engine() ist jetzt header-only DEFINIERT (vorher nur deklariert).
/// Konsumenten linken NUR gegen cache-engine und nutzen ICacheEngine.
[[nodiscard]] inline ICacheEngine& get_cache_engine() {
    static DefaultCacheEngine instance;
    return instance;
}

} // namespace comdare::cache_engine::api
