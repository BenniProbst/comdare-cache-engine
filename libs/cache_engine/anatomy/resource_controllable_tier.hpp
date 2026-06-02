#pragma once
// KF-4 (2026-06-02) — IResourceControllableTier: ABI-stabiles Laufzeit-STEUER-Sub-Interface
// (die Tier-Binary-Seite von "Algorithm_Resource_Control", Prüf-Dock-Kontroll-Ebene).
//
// User-Direktive 2026-06-02: einige Achsen bieten Parameter, die ZUR LAUFZEIT steuerbar sein müssen.
// Die Steuerschnittstelle bleibt AKTIV AUCH BEI ABGESCHALTETER MESSUNG — sie steuert algorithmus-INTERNE
// Properties (Thread-Anzahl, Prefetch-Distanz, Pool-Budget, Batch-Größe, Inline-Schwelle), NICHT die Messung.
//
// ABI-SICHER nach EXAKT demselben Designprinzip wie IScannableTier/IObservableTier (scannable_tier.hpp):
//   - eigenständiges Sub-Interface; der Host fragt es via dynamic_cast<IResourceControllableTier*> ab;
//     alt-gebaute DLLs ohne Fähigkeit → nullptr → Host degradiert sauber (kein ABI-Bruch).
//   - quert die Grenze als reine vtable + flacher POD (nur uint64) → ABI-stabil, memcpy-fähig.
//   - NIEMALS in-place an bestehenden Interfaces ändern (SEH-0xc0000005-Lektion).
// UNTERSCHIED zu Observer/Scan: NICHT an COMDARE_MEASUREMENT_ON gebunden — die Steuer-Ebene ist IMMER
// einkompiliert, weil sie unabhängig von der Messung wirken muss.
//
// "Sub-Structs je Achse" (User): EIN flacher POD mit per-Achsen-Feldern; jeder Organ-Algorithmus einer
// betroffenen Achse implementiert apply/query (Achsen-CRTP-Base liefert No-op-Defaults für nicht-betroffene
// Organe — KF-5). Sentinel 0 = "nicht setzen / Default beibehalten".
//
// @related [[feedback_compile_time_only_no_runtime]] (Ausnahme: OS-seitige Laufzeit-Properties, Faustregel §7)
// @doku   docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md §7-A

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// ComdareResourceControlV1 — flacher, ABI-stabiler POD der laufzeit-einstellbaren,
// algorithmus-internen Achsen-Properties (Algorithm_Resource_Control).
// Sentinel 0 = "nicht setzen / Default beibehalten".
// ─────────────────────────────────────────────────────────────────────────────
struct ComdareResourceControlV1 {
    std::uint64_t thread_count           = 0;  // Achse concurrency (axis_08) — ursprüngl. Laufzeit-Variable
    std::uint64_t prefetch_distance      = 0;  // Achse prefetch    (axis_07) — Distanz/Tiefe in Cache-Lines
    std::uint64_t pool_budget_bytes      = 0;  // Achse allocator   (axis_06) — Arena-/Pool-Budget (Bytes)
    std::uint64_t batch_size             = 0;  // Achse traversal   (axis_03a) — Batch-/Working-Set-Größe
    std::uint64_t inline_threshold_bytes = 0;  // Achse value_handle(axis_14) — Inline/External-Schwelle (Bytes)
    std::uint64_t controllable_axis_count = 0; // Meta/Diagnose: real reagierende Achsen (query: max möglich)

    [[nodiscard]] constexpr bool operator==(ComdareResourceControlV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareResourceControlV1>,
              "ABI-Pflicht: Resource-Control-POD muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareResourceControlV1>,
              "ABI-Pflicht: Resource-Control-POD muss memcpy-fähig (trivially_copyable) sein");

/// ABI-Version des Resource-Control-Formats. Major-Bump bei Layout-Änderung (neue Achse = neuer POD V2).
inline constexpr std::uint32_t kResourceControlVersion = 1;

// ─────────────────────────────────────────────────────────────────────────────
// IResourceControllableTier — optionales Steuer-Sub-Interface (IMMER verfügbar, auch Messung-aus).
// ─────────────────────────────────────────────────────────────────────────────
class IResourceControllableTier {
public:
    virtual ~IResourceControllableTier() = default;

    /// Liefert die UNTERSTÜTZTEN Obergrenzen + Defaults je Feld (0 = Achse nicht steuerbar). Der Host
    /// (CacheEngineBuilder/Prüf-Dock) durchläuft die Werte NUR innerhalb dieser Grenzen + der System-
    /// Umgebungs-/Ressourcenlimits (Faustregel §7). `controllable_axis_count` = Zahl steuerbarer Achsen.
    virtual void tier_query_resource_caps(ComdareResourceControlV1* out_caps) const noexcept = 0;

    /// Wendet die Werte aus *in an (Feld == 0 → unverändert). Gibt die Zahl der Achsen zurück, die den
    /// Wert real angenommen haben (≤ controllable_axis_count). noexcept (Steuer-Op darf den Lauf nicht werfen).
    virtual std::uint64_t tier_apply_resource_control(ComdareResourceControlV1 const* in) noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
