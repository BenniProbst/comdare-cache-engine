#pragma once
// bestandslog_index.hpp -- G3 / #46b Lagerhaltung, Scheibe B3 (Ledger Section 62-B, B18).
//
// Der LAGER-INDEX: LagerKey -> BestandEintrag ueber std::map-Lookup. Der LagerKey ist ein TUPEL aus
// zwei getrennten Teilen (Section 62-NACHTRAG-4):
//   (1) Sha512Key       -- der 64-Byte-Anatomie-Digest ueber die Stempel-Zeilen.
//   (2) ZellKoordinaten -- die drei Runtime-Strings combo/opt/simd, die der Digest NICHT traegt.
// Die Key-Ableitung von (1) ruft ctsha512 ueber die Stempel-Zeilen in DIREKTER Konkatenation (kein
// Separator, keine Whitespace) -- exakt dieselbe Digest wie abi::anatomy_fingerprint_hex (append organ;
// system; measurement; merge). Das bleibt die EINE SHA512-Wahrheit: der Digest-Teil des Lager-Keys ==
// der Binary-Stempel-SHA, kein zweiter Preimage. Verifiziert gegen den eingefrorenen A1-Testvektor
// (Section 66 Lager-Gate).
//
// Teil (2) wird NICHT in den Digest eingerechnet und auch NICHT an ihn angehaengt: waere die Zelle Teil
// des Preimage, driftete der Lager-Key vom einkompilierten Fingerprint weg und der .fingerprint-Sidecar
// waere als Anker unbrauchbar. Waere sie ein an den Hex-String angeklebtes Suffix, gaebe es ein
// Trennzeichen-Problem und eine zweite, konkurrierende Schluessel-Form. Stattdessen: EIN Schluessel-
// OBJEKT mit zwei getrennt geklammerten Feldern, lexikographisch verglichen (defaulted <=>).
//
// Kein Hot-Path (Ledger-Anm. zu B18): der Lookup laeuft vor dem Bau, nicht im gemessenen Pfad.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib + ctsha512.

#include "bestandslog_document.hpp" // BestandEintrag + ZellKoordinaten

#include <sha512/ctsha512.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::bestandslog {

// 64-Byte SHA512-Digest als map-Schluessel. operator<=> vergleicht das Byte-Array lexikographisch.
struct Sha512Key {
    std::array<std::uint8_t, 64> b{};

    friend auto operator<=>(Sha512Key const&, Sha512Key const&) = default;
    friend bool operator==(Sha512Key const&, Sha512Key const&)  = default;
};

// Key aus den Stempel-Zeilen: sha512(concat(lines...)) OHNE Separator. Fuer die Binary-Genus sind das
// die vier Anatomy-Zeilen in Reihenfolge organ + system + measurement + merge -> identisch zu
// abi::anatomy_fingerprint_hex. Kein zweiter Delimiter, sonst driftet der Lager-Key vom Stempel weg.
[[nodiscard]] inline Sha512Key derive_key_from_lines(std::span<std::string_view const> lines) {
    std::string pre;
    for (std::string_view l : lines) pre.append(l);
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(pre.data()), pre.size()});
    Sha512Key k;
    for (std::size_t i = 0; i < 64; ++i) k.b[i] = digest[i];
    return k;
}

// Sha512Key -> 128-hex (Kleinbuchstaben) == abi::to_hex-Form.
[[nodiscard]] inline std::string to_hex(Sha512Key const& k) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string           out(128, '0');
    for (std::size_t i = 0; i < 64; ++i) {
        out[2 * i]     = kHex[(k.b[i] >> 4) & 0xf];
        out[2 * i + 1] = kHex[k.b[i] & 0xf];
    }
    return out;
}

// 128-hex -> Sha512Key (LAUFZEIT; nullopt bei falscher Laenge / Nicht-Hex-Zeichen). Runtime-Pendant
// zum consteval sha512::from_hex -- fuer den Index-Aufbau aus einem geladenen Dokument.
[[nodiscard]] inline std::optional<Sha512Key> key_from_hex(std::string_view hex) {
    if (hex.size() != 128) return std::nullopt;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    Sha512Key k;
    for (std::size_t i = 0; i < 64; ++i) {
        int const hi = nib(hex[2 * i]);
        int const lo = nib(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return std::nullopt;
        k.b[i] = static_cast<std::uint8_t>((hi << 4) | lo);
    }
    return k;
}

// ---------------------------------------------------------------------------
// LagerKey -- der zusammengesetzte Lager-Schluessel (Section 62-NACHTRAG-4). Zwei getrennt geklammerte
// Felder; die Ordnung ist lexikographisch erst ueber den Digest, dann ueber die Zelle (defaulted <=>).
// ---------------------------------------------------------------------------
struct LagerKey {
    Sha512Key       sha;   // Anatomie-Digest (der Fingerprint-Teil)
    ZellKoordinaten zelle; // [d,e,f] combo/opt/simd (der Zell-Teil; leer = nicht gemeldet)

    friend auto operator<=>(LagerKey const&, LagerKey const&) = default;
    friend bool operator==(LagerKey const&, LagerKey const&)  = default;
};

// Lager-Schluessel aus einem geladenen Eintrag: 128-hex -> Digest, Zell-Felder unveraendert uebernommen.
// nullopt, wenn key_sha512 kein gueltiges 128-hex ist (ein solcher Eintrag gehoert nicht in den Index).
[[nodiscard]] inline std::optional<LagerKey> lager_key_from_entry(BestandEintrag const& e) {
    auto k = key_from_hex(e.key_sha512);
    if (!k) return std::nullopt;
    return LagerKey{*k, e.zelle};
}

// Lager-Schluessel aus 128-hex + Zelle (der Weg der Laufzeit-Naht: der Host reicht den Fingerprint als
// Hex-String und die drei Zell-Strings). nullopt bei ungueltigem Hex -> der Aufrufer wertet das als Miss.
[[nodiscard]] inline std::optional<LagerKey> lager_key_from_hex(std::string_view hex, ZellKoordinaten const& zelle) {
    auto k = key_from_hex(hex);
    if (!k) return std::nullopt;
    return LagerKey{*k, zelle};
}

// Der Lager-Index: (SHA512, Zelle) -> Eintrag.
using LagerIndex = std::map<LagerKey, BestandEintrag>;

} // namespace comdare::cache_engine::builder::bestandslog
