#pragma once
// P-CACHELINE-LITERAL (2026-08-04) -- cacheline_line_bytes.hpp: die KONSUM-BRUECKE der Line-Groessen-
// Dimension der cacheline-Unterachse (KF-3/KF-5, cacheline_config.hpp).
//
// WARUM eine eigene Datei und KEINE neue Dimension: cacheline_config.hpp bleibt die alleinige Quelle des
// WERTRAUMS (CacheLineSize {B32,B64,B128,B256} x alignment x sw_hint = 60 Configs). Diese Datei fuegt
// NICHTS zum Wertraum hinzu; sie beantwortet genau EINE Frage, die bisher jeder Konsument fuer sich
// beantwortet hat -- und zwar mit einem Literal: "welche Line-Groesse in Bytes gilt fuer Organ T?".
// Muster = node_width_config.hpp: eine kleine, eigene Header-Einheit im Achsen-Verzeichnis axes/cacheline/,
// selbe Namensraum-Wurzel comdare::cache_engine::cacheline.
//
// OWNER-KERN 04.08.2026 (generalisierte Schnitt-Regel): ALLE Achsen-Eigenschaften duerfen nur ueber die
// Achse abgebildet werden. Die Cache-Line-Groesse ist Eigentum DIESER Unterachse; fremder Code (Stores,
// Genus-Implementierungen) darf sie weder hartkodieren noch nachrechnen, sondern bezieht sie hier.
//
// KF-6-NAHT (Posten 62, NICHT hier): heute instanziieren alle Strategie-Basen die Default-CacheLineConfig{}
// (line_size = B64), weil der Codegen die per-Organ-Config noch nicht als NTTP emittiert. Sobald KF-6 das
// tut, liefert line_bytes_of<T>() OHNE weitere Aenderung den permutierten Wert -- die Konsumenten folgen
// automatisch. Das ist der Zweck dieses Schnitts: die Durchbindung bewegt die NTTP-Belegung an der
// Strategie-Basis, nicht mehr die Konsumenten.
//
// FALLE (bewusst dokumentiert, axis_05_memory_layout_cache_line_aligned.hpp:54 "Duplikat-Bug"):
// NICHT mit L::cache_line_size() verwechseln. Das ist der INTRINSISCHE Design-Deskriptor einer
// Layout-Strategie (aos_strict = 1, packed_bitmap = 8, cache_line_aligned = 64) und beschreibt deren
// eigenes Entwurfs-Merkmal -- NICHT die permutierbare Hardware-Line-Groesse dieser Unterachse.

#include "cacheline_config.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::cacheline {

/// Die Line-Groesse des ACHSEN-DEFAULTS in Bytes (CacheLineConfig{}.line_size == B64 == 64).
/// Einziger Bezugspunkt fuer "am Default verhaltensneutral"-Beweise der Konsumenten.
inline constexpr std::size_t kDefaultLineBytes = static_cast<std::size_t>(CacheLineConfig{}.line_size);
static_assert(kDefaultLineBytes == 64, "Achsen-Default der cacheline-Unterachse ist nicht mehr B64 = 64 Byte");

/// Concept-Zwilling zu CacheLineConfigurable fuer den benannten Linien-Zugriff, den die Organ-Achsen-Basen
/// anbieten (z.B. MemoryLayoutStrategyBase::cacheline_subaxis_line_bytes()).
template <typename T>
concept CacheLineLineBytesAware = requires {
    { T::cacheline_subaxis_line_bytes() } noexcept -> std::convertible_to<std::size_t>;
};

/// Compile-Time-Aufloesung der wirksamen Line-Groesse eines Organs/einer Strategie T, in dieser Ordnung:
///   1. der benannte Unterachsen-Zugriff T::cacheline_subaxis_line_bytes() (Organ-Achsen-Basen),
///   2. die getragene Config T::cacheline_config().line_size (CacheLineAware-Mixin ohne benannten Zugriff),
///   3. der Achsen-Default kDefaultLineBytes (Organ traegt die Unterachse nicht -- Konsument bleibt am
///      Default-Verhalten, statt ein eigenes Literal zu erfinden).
/// Rein compile-time (constexpr, kein Runtime-Wert, kein Runtime-Switch).
template <typename T>
[[nodiscard]] constexpr std::size_t line_bytes_of() noexcept {
    if constexpr (CacheLineLineBytesAware<T>)
        return static_cast<std::size_t>(T::cacheline_subaxis_line_bytes());
    else if constexpr (CacheLineConfigurable<T>)
        return static_cast<std::size_t>(T::cacheline_config().line_size);
    else
        return kDefaultLineBytes;
}

// Selbstbeweis der Fallback-Stufe 3 (Typ ohne jede Unterachsen-Naht -> Achsen-Default, kein Literal).
namespace detail {
struct cacheline_unaware_probe_t {};
} // namespace detail
static_assert(line_bytes_of<detail::cacheline_unaware_probe_t>() == kDefaultLineBytes);
// Selbstbeweis der Stufe 2 (blosses Mixin, kein benannter Zugriff) -- B128 muss durchschlagen.
static_assert(line_bytes_of<CacheLineAware<CacheLineConfig{CacheLineSize::B128}>>() == 128);
static_assert(line_bytes_of<CacheLineAware<CacheLineConfig{}>>() == kDefaultLineBytes);

inline constexpr std::uint32_t kCacheLineBytesBridgeVersion = 1;

} // namespace comdare::cache_engine::cacheline
