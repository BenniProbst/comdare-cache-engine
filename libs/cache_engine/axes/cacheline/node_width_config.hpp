#pragma once
// C2 (GO4/#8 F-C, 2026-07-12) — node_width_config.hpp: die FF2-Unterachse "Knoten-Breite in Cache-Lines".
//
// Thesis-Grundlage (FF2, kapitel/de/01_einleitung.tex:94-99): "widerspruechliche Literaturbefunde zur optimalen
// Knoten-/Cache-Line-Groesse (EINE Cache-Line bei CSS/CSB+ gegenueber bis zu SECHZEHN bei Hankins/Patel)
// bias-frei nachmessen". Das ist eine ANDERE Dimension als die Line-GROESSE der cacheline-Unterachse
// (cacheline_config.hpp, 32/64/128/256 Byte): hier wird die BREITE eines Knotens IN Cache-Lines (1..16)
// permutiert. Dossier GO4 F-C §1/C2: die zwei Dimensionen sind als ZWEI getrennte Unterachsen ausgewiesen.
//
// Muster = die bestehende cacheline-Unterachse (KF-3/KF-5), exakt gespiegelt:
//   - NTTP-faehige Config (strukturell) + CRTP-Mixin NodeWidthAware<Cfg> + Concept NodeWidthConfigurable.
//   - Compile-time only (kein Runtime-Switch im Hot-Path); Default = Native (0) -> Verhalten unveraendert.
//   - Binary-id-relevant NUR profil-aktiviert (profile_to_tree.hpp: ref=="node_width" -> statische Sub-Ebene
//     node_width.width_in_lines); ohne Profil-Aktivierung aendert sich KEINE binary_id.
//   - Realer Konsum: das Node-Organ deklariert die Breite (NodeTypeStrategyBase, defaulted NTTP); der
//     LayoutAwareChunkedStore (Node-Layout-Pfad) macht das Chunk-/Knoten-Backing W Cache-Lines breit
//     (Kapazitaet folgt aus dem physischen Record-Layout, axis_04_node_type_layout_aware_store.hpp).

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::cacheline {

// ── Wertraum (5 Messwerte {1,2,4,8,16} + neutrales Native=0 als Aus-Default) ─────────────────────────────
// Native ist KEIN Messwert: es bedeutet "keine Breiten-Vorgabe" (Node behaelt seine intrinsische Kapazitaet)
// und ist der Default der Unterachse — dadurch bleiben alle bestehenden Organe/Binaries byte-identisch.
enum class NodeWidthInLines : std::uint8_t { Native = 0, W1 = 1, W2 = 2, W4 = 4, W8 = 8, W16 = 16 };

/// Compile-Time-Konfiguration der Knoten-Breiten-Unterachse EINES Node-Organs. Strukturell → als NTTP nutzbar.
struct NodeWidthConfig {
    NodeWidthInLines width_in_lines = NodeWidthInLines::Native;

    [[nodiscard]] constexpr bool operator==(NodeWidthConfig const&) const noexcept = default;
};

// ── Concept: ein Organ traegt die Knoten-Breiten-Unterachse ──────────────────────────────────────────────
template <typename T>
concept NodeWidthConfigurable = requires {
    { T::node_width_config() } -> std::same_as<NodeWidthConfig>;
};

// ── CRTP-Mixin: Node-Organe erben dies (via NodeTypeStrategyBase), um die Unterachse zu tragen ───────────
/// Cfg ist die per-Organ-Konfiguration (vom Codegen/Profil je Organ gesetzt). Native (0) = keine Vorgabe.
template <NodeWidthConfig Cfg>
struct NodeWidthAware {
    [[nodiscard]] static constexpr NodeWidthConfig node_width_config() noexcept { return Cfg; }
    /// Breite in Cache-Lines (0 = Native, keine Vorgabe).
    [[nodiscard]] static constexpr std::size_t node_width_in_lines() noexcept {
        return static_cast<std::size_t>(Cfg.width_in_lines);
    }
    /// Knoten-Backing-Breite in Bytes bei gegebener Line-Groesse (Default 64 = CLU-Bezugsgroesse). Native → 0.
    [[nodiscard]] static constexpr std::size_t node_width_bytes(std::size_t line_bytes = 64) noexcept {
        return node_width_in_lines() * line_bytes;
    }
};

// ── Enumeration: die 5 Messwerte der FF2-Unterachse (fuer Codegen/Registry; Native gehoert NICHT dazu) ────
[[nodiscard]] constexpr std::array<NodeWidthConfig, 5> all_node_widths() noexcept {
    return {NodeWidthConfig{NodeWidthInLines::W1}, NodeWidthConfig{NodeWidthInLines::W2},
            NodeWidthConfig{NodeWidthInLines::W4}, NodeWidthConfig{NodeWidthInLines::W8},
            NodeWidthConfig{NodeWidthInLines::W16}};
}

/// Compile-Time-Factory aus dem Integer-Wert (Bruecke fuer Codegen/Tests, Muster make_config).
/// lines∈{1,2,4,8,16}; alles andere → Native (konservativer Aus-Default, kein Raten).
[[nodiscard]] constexpr NodeWidthConfig make_node_width(unsigned lines) noexcept {
    NodeWidthConfig c;
    c.width_in_lines = (lines == 1)    ? NodeWidthInLines::W1
                       : (lines == 2)  ? NodeWidthInLines::W2
                       : (lines == 4)  ? NodeWidthInLines::W4
                       : (lines == 8)  ? NodeWidthInLines::W8
                       : (lines == 16) ? NodeWidthInLines::W16
                                       : NodeWidthInLines::Native;
    return c;
}

inline constexpr std::uint32_t kNodeWidthSubaxisVersion = 1;

} // namespace comdare::cache_engine::cacheline
