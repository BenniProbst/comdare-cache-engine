#pragma once
// FixedLengthFingerprint - Fixed-Length Hash fuer komplexe Keys (REV 7 §4.2(c))
//
// Wenn Key (bei 2+ Variadic-Template-Params) ein komplexes Objekt ist,
// wird er implicit via FixedLengthFingerprint<Bytes>::hash() auf einen
// std::array<std::byte, Bytes> reduziert.
//
// Default-Bytes = 16 (128 bit), ausreichend fuer kollisionsarme Hashing
// in typischen Workloads. Konfigurierbar pro Permutations-Build.

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace comdare::fingerprint {

// FNV-1a Hash als Basis (deterministisch, 64-bit) — Skelett, fuer hochwertige
// Workloads spaeter durch xxhash3/blake3 ersetzt
inline constexpr std::uint64_t kFnvOffsetBasis = 0xCBF29CE484222325ULL;
inline constexpr std::uint64_t kFnvPrime       = 0x00000100000001B3ULL;

[[nodiscard]] inline constexpr std::uint64_t fnv1a_64(std::span<std::byte const> bytes) noexcept {
    std::uint64_t hash = kFnvOffsetBasis;
    for (auto b : bytes) {
        hash ^= static_cast<std::uint8_t>(b);
        hash *= kFnvPrime;
    }
    return hash;
}

template <std::size_t Bytes = 16>
class FixedLengthFingerprint {
public:
    static constexpr std::size_t kFingerprintBytes = Bytes;
    using fingerprint_t                            = std::array<std::byte, Bytes>;

    template <typename T>
    [[nodiscard]] static fingerprint_t hash(T const& obj) noexcept {
        // Schritt 1: serialisiere obj zu byte-span (Skelett: bit-cast fuer trivially-copyable)
        fingerprint_t              result{};
        std::span<std::byte const> bytes;
        if constexpr (std::is_trivially_copyable_v<T>) {
            bytes = std::span<std::byte const>{reinterpret_cast<std::byte const*>(&obj), sizeof(T)};
        } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
            bytes = std::span<std::byte const>{reinterpret_cast<std::byte const*>(obj.data()), obj.size()};
        } else {
            // Fallback: zero-fill — User-Overload Pflicht!
            return result;
        }

        // Schritt 2: FNV-1a 64-bit hash, extended to Bytes via re-hashing
        for (std::size_t i = 0; i < Bytes; i += 8) {
            std::uint64_t const h          = fnv1a_64(bytes);
            std::size_t const   copy_bytes = std::min<std::size_t>(8, Bytes - i);
            std::memcpy(result.data() + i, &h, copy_bytes);
            // Mix with original bytes for next chunk
            bytes = bytes.subspan(0, std::min(bytes.size(), bytes.size() - 1));
            if (bytes.empty()) break;
        }
        return result;
    }
};

// to_binary_string — Funktions-Ueberladungen fuer Einfach-Typen vs Komplexe
// (REV 7 §4.2(c) + §4.3)

inline constexpr std::size_t kFixedKeyBytes = 16;
using BinaryKey                             = std::array<std::byte, kFixedKeyBytes>;

// Einfach-Typen: direkter binary-cast
template <typename Key>
    requires std::is_arithmetic_v<Key> || std::is_pointer_v<Key>
[[nodiscard]] inline BinaryKey to_binary_string(Key const& k) noexcept {
    BinaryKey result{};
    std::memcpy(result.data(), &k, std::min(sizeof(k), kFixedKeyBytes));
    return result;
}

// std::string / std::string_view: cap auf kFixedKeyBytes (mit Null-Padding)
[[nodiscard]] inline BinaryKey to_binary_string(std::string const& k) noexcept {
    BinaryKey result{};
    std::memcpy(result.data(), k.data(), std::min(k.size(), kFixedKeyBytes));
    return result;
}

[[nodiscard]] inline BinaryKey to_binary_string(std::string_view k) noexcept {
    BinaryKey result{};
    std::memcpy(result.data(), k.data(), std::min(k.size(), kFixedKeyBytes));
    return result;
}

// Komplexe Typen: via FixedLengthFingerprint
template <typename Key>
    requires(!std::is_arithmetic_v<Key> && !std::is_pointer_v<Key> && !std::is_same_v<Key, std::string> &&
             !std::is_same_v<Key, std::string_view>)
[[nodiscard]] inline BinaryKey to_binary_string(Key const& k) noexcept {
    return FixedLengthFingerprint<kFixedKeyBytes>::hash(k);
}

} // namespace comdare::fingerprint
