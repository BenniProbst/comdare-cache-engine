#pragma once
// abi/subaxis_valueset_segment.hpp -- A13-M3/C3: das Sub-Achsen-WERTESET-Segment als CONSTEVAL-Single-Source.
//
// WARUM DIESER HEADER EXISTIERT (F7-Konsolidierung 01.08., "schwerster Befund" der VERIFY-Sektion in
// super docs/sessions/20260801-KONSOLIDIERT-gesamtarchitektur-lager-batch-eta-sha512.md): das Werteset der
// globalen Sub-Achsen (cacheline / node_width / alloc_hw) aendert bei einer ERWEITERUNG das serialisierte
// Bit-Layout, OHNE dass irgendeine Varianten-algo_version bumpt. Die Organ-STEMPEL-Zeile traegt es
// ausdruecklich NICHT ("NUR Haupt-Achsen, kein Sub-Achsen-Schwanz", compose_organ_stamp_line). Solange das
// Skip-Kriterium der Dreifach-Sidecar-Vergleich war, fing die .algos-Signatur den Bump ab -- sie haengt das
// Segment als festen Schwanz an jede Signatur. Unter dem kommenden SHA512-ONLY-Skip-Gate (F7) faellt dieser
// Faenger weg: ein Werteset-Bump wuerde eine layout-geaenderte Binary STILL wiederverwenden. Deshalb ist das
// Segment ab A13-M3 ein EXPLIZITES Preimage-Glied des Anatomie-Fingerprints (anatomy_fingerprint.hpp).
//
// KEINE ZWEITE ERHEBUNG: die drei Versions-Konstanten bleiben, wo sie sind (ihre jeweiligen Achsen-Header);
// dieser Header rendert sie EINMAL consteval. Die bisherige Laufzeit-Quelle
// ex::sub_axis_valueset_segment() (axis_variant_version_table.hpp) reicht ab jetzt GENAU diesen String
// durch -- die .algos-Sidecar-Bytes bleiben damit unveraendert, und CT- und Laufzeit-Weg koennen nicht
// auseinanderlaufen (O-8-Schritt-12-Lehre: kein uebersehener dritter Ableitungsweg).
//
// Die drei Achsen-Header sind bewusst RELATIV inkludiert (Muster anatomy_module_abi_v1_decl.hpp): dieser
// Header wird auch aus Uebersetzungseinheiten gezogen, die libs/cache_engine NICHT als Suchpfad tragen.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, consteval (compile-time only, nie zur Laufzeit).

#include "../../../axes/alloc/alloc_hw_config.hpp"       // kAllocHwSubaxisVersion
#include "../../../axes/cacheline/cacheline_config.hpp"  // kCacheLineSubaxisVersion
#include "../../../axes/cacheline/node_width_config.hpp" // kNodeWidthSubaxisVersion

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Puffer-Obergrenze der gerenderten Segment-Zeile. Drei feste Namen + drei Dezimalzahlen -- 128 Byte sind
/// mehr als der doppelte Kopfraum. Wird sie je ueberschritten, bricht die consteval-Auswertung sichtbar
/// (der Puffer-Zugriff ist dann kein konstanter Ausdruck mehr).
inline constexpr std::size_t kSubAxisValuesetSegmentMax = 128;

/// Rendert "sub=cacheline@v<N>,node_width@v<N>,alloc_hw@v<N>" -- BYTE-GLEICH zur bisherigen Laufzeit-Form
/// (Bauplan Section 2). Die Reihenfolge ist fix und darf nur GEMEINSAM mit den Konsumenten geaendert werden
/// (.algos-Sidecar-Vergleich UND Fingerprint-Preimage haengen daran).
[[nodiscard]] consteval std::array<char, kSubAxisValuesetSegmentMax> sub_axis_valueset_segment_array() noexcept {
    std::array<char, kSubAxisValuesetSegmentMax> out{};
    std::size_t                                  p   = 0;
    auto                                         put = [&](std::string_view s) {
        for (char c : s) out[p++] = c;
    };
    auto put_num = [&](std::uint32_t v) {
        char        buf[10]{};
        std::size_t n = 0;
        do {
            buf[n++] = static_cast<char>('0' + (v % 10u));
            v /= 10u;
        } while (v != 0u);
        while (n != 0u) out[p++] = buf[--n];
    };
    put("sub=cacheline@v");
    put_num(::comdare::cache_engine::cacheline::kCacheLineSubaxisVersion);
    put(",node_width@v");
    put_num(::comdare::cache_engine::cacheline::kNodeWidthSubaxisVersion);
    put(",alloc_hw@v");
    put_num(::comdare::cache_engine::alloc::kAllocHwSubaxisVersion);
    out[p] = '\0';
    return out;
}

/// Der gerenderte Werteset-Schwanz (constexpr storage) + seine string_view (ohne '\0').
inline constexpr auto             kSubAxisValuesetSegmentArray = sub_axis_valueset_segment_array();
inline constexpr std::string_view kSubAxisValuesetSegment{kSubAxisValuesetSegmentArray.data()};

static_assert(!kSubAxisValuesetSegment.empty(), "Das Werteset-Segment darf nie leer sein (Preimage-Glied).");
static_assert(kSubAxisValuesetSegment.find('\n') == std::string_view::npos,
              "A13-M3/OF-M3-1: das Werteset-Segment darf den Domain-Separator '\\n' nicht enthalten, sonst "
              "waere die Preimage-Zerlegung wieder mehrdeutig.");

} // namespace comdare::cache_engine::abi
