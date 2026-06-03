#pragma once
// L-76b Goldstandard-Erweiterung — axis_growth: die Sequence-Gattungs-Achse für die Kapazitäts-Wachstums-Policy
// einer wachsenden V-Sequenz. Bisher nur EIN inline-DoublingGrowth-Default in anatomy/sequence_composition.hpp;
// hier die echte MEHR-Policy-Achse (GoldenRatio/FixedChunk/Exact) als reale Achsen-Varianten. Alle erfüllen das
// GrowthPolicy-Concept (next_capacity/growth_factor) aus sequence_composition.hpp (Wiederverwendung, kein Duplikat).
//
// Jede Policy bestimmt die nächste Kapazität bei Überlauf — ein realer Algorithmus-Unterschied (Speicher vs.
// Realloc-Häufigkeit), KEIN Stub: DoublingGrowth (×2, std::vector-Standard) / GoldenRatioGrowth (×1.5, folly/MSVC,
// erlaubt Realloc-Adress-Reuse) / FixedChunkGrowth (+Chunk additiv, page-aligned) / ExactGrowth (1:1, minimaler Speicher).
// C++23, header-only.

#include "anatomy/sequence_composition.hpp"   // GrowthPolicy-Concept + DoublingGrowth (Wiederverwendung)

#include <cstddef>

namespace comdare::cache_engine::sequence::axis_growth {

/// GoldenRatioGrowth — Kapazität ×1.5 (current + current/2). Speicherschonender als Verdopplung; der ~1.5-Faktor
/// erlaubt (anders als ×2) dass frühere Allokations-Blöcke beim Realloc wiederverwendet werden (folly/MSVC-Stil).
struct GoldenRatioGrowth {
    [[nodiscard]] std::size_t next_capacity(std::size_t current, std::size_t requested) const noexcept {
        std::size_t next = current + (current >> 1);     // ×1.5
        if (next < requested) next = requested;
        if (next <= current)  next = current + 1;         // mind. +1 Fortschritt (current==0/1-Randfall)
        return next;
    }
    [[nodiscard]] double growth_factor() const noexcept { return 1.5; }
};

/// FixedChunkGrowth<Chunk> — additives Wachstum um Chunk-Slots (konstante Realloc-Größe, z.B. Page-aligned). Der
/// Wachstumsfaktor ist nicht multiplikativ-konstant → growth_factor() liefert 0.0 als „additiv"-Sentinel.
template <std::size_t Chunk = 64>
struct FixedChunkGrowth {
    static constexpr std::size_t chunk = Chunk;
    [[nodiscard]] std::size_t next_capacity(std::size_t current, std::size_t requested) const noexcept {
        std::size_t next = current + Chunk;
        return next < requested ? requested : next;
    }
    [[nodiscard]] double growth_factor() const noexcept { return 0.0; }   // additiv, kein multiplikativer Faktor
};

/// ExactGrowth — keine Über-Allokation: Kapazität == angefordert. Minimaler Speicher, maximale Realloc-Häufigkeit.
struct ExactGrowth {
    [[nodiscard]] std::size_t next_capacity(std::size_t /*current*/, std::size_t requested) const noexcept {
        return requested;
    }
    [[nodiscard]] double growth_factor() const noexcept { return 1.0; }   // exakt 1:1
};

}  // namespace comdare::cache_engine::sequence::axis_growth
