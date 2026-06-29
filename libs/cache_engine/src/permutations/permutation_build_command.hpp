#pragma once
// V41.F.6.1.G CacheEngineBuilder CLI-Flag-Builder (2026-05-26)
//
// @stand V41.F.6.1.G ERGAENZUNG
//
// **User-Direktive 2026-05-26 (Doku §15.10):**
//   "Die CacheEngineBuilder Anwendung wird die Kompilation mit Build-Flags fuer
//    die Permutation-Binary aufrufen und daher direkt auf der Kommandozeile setzen."
//
//   cmake -B build/perm_<hash> -DCOMDARE_AXIS_<NN>_ENABLE_<VENDOR>=ON|OFF ...
//
// Pragmatisch fuer jetzt: liefert nur den String-Builder. Tatsaechliches
// std::system(cmd.c_str()) bleibt der CacheEngineBuilder-App ueberlassen —
// hier nur die wiederverwendbare Logik um CMake-CLI-Commands zu konstruieren.
//
// **Pflicht-Wrapper-API (siehe StdMalloc::flag_suffix() etc.):**
//   static constexpr std::string_view flag_suffix() noexcept;
//   z.B. "STD" / "MIMALLOC" / "SNMALLOC" / "PMR"

#include "permutation_engine.hpp" // PermTuple + HasIterableAspect

#include <boost/mp11.hpp>

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::permutations {

namespace mp = boost::mp11;

// ───────────────────────────────────────────────────────────────────────────
// (1) Hash → Hex-String (16 Hex-Zeichen)
// ───────────────────────────────────────────────────────────────────────────

[[nodiscard]] inline std::string hash_to_hex(std::uint64_t h) {
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(h));
    return std::string{buf, 16};
}

// ───────────────────────────────────────────────────────────────────────────
// (2) cmake-Invocation-Prefix pro Permutation
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief "cmake -B <build_root>/perm_<hex_hash> -S <source_dir>"
 *
 * Wird vom CacheEngineBuilder pro Permutation gerufen, dann mit Achs-Flags
 * (siehe emit_axis_flags) zu einem vollstaendigen CMake-Befehl konkateniert.
 */
template <class P>
[[nodiscard]] std::string build_cmake_invocation_prefix(std::string_view source_dir, std::string_view build_root) {
    std::ostringstream os;
    os << "cmake -B " << build_root << "/perm_" << hash_to_hex(P::hash()) << " -S " << source_dir;
    return os.str();
}

// ───────────────────────────────────────────────────────────────────────────
// (3) Pro-Achs-Flags: SelectedVendor=ON, alle anderen=OFF
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief Generiere CMake-Flags fuer eine Topic-Achse
 *
 * @tparam SelectedVendor  Der in dieser Permutation aktive Vendor (ON)
 * @tparam AllVendorsList  mp_list aller bekannten Vendor der Achse (rest OFF)
 *
 * @param flag_prefix      z.B. "COMDARE_AXIS_06_ENABLE"
 *
 * Beispiel-Output (4 Vendor, StdMalloc selected):
 *   " -DCOMDARE_AXIS_06_ENABLE_STD=ON"
 *   " -DCOMDARE_AXIS_06_ENABLE_MIMALLOC=OFF"
 *   " -DCOMDARE_AXIS_06_ENABLE_SNMALLOC=OFF"
 *   " -DCOMDARE_AXIS_06_ENABLE_PMR=OFF"
 */
template <class SelectedVendor, class AllVendorsList>
[[nodiscard]] std::string emit_axis_flags(std::string_view flag_prefix) {
    std::string out;
    mp::mp_for_each<AllVendorsList>([&]<class V>(V) {
        constexpr bool selected = std::is_same_v<V, SelectedVendor>;
        out += " -D";
        out += flag_prefix;
        out += "_";
        out += V::flag_suffix();
        out += (selected ? "=ON" : "=OFF");
    });
    return out;
}

// ───────────────────────────────────────────────────────────────────────────
// (4) Komplett-Build-Command fuer Single-Topic-Permutation (heute LIVE)
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief Komplettes CMake-Command fuer 1-Topic-Permutation
 *
 * @tparam P                Permutation (PermTuple<V>)
 * @tparam AllVendorsList   mp_list aller Vendor dieser Achse
 *
 * Liefert: "cmake -B <build_root>/perm_<hash> -S <src> -DCOMDARE_AXIS_<NN>_ENABLE_<X>=ON|OFF ..."
 *
 * Heute nur fuer 1 Topic implementiert. Multi-Topic-Erweiterung in F.6.1.G+
 * (parallel zu Batch 2-8 Vendor-Vollausbau).
 */
template <class P, class AllVendorsList>
[[nodiscard]] std::string build_cmake_command_for_single_topic(std::string_view source_dir, std::string_view build_root,
                                                               std::string_view axis_flag_prefix) {
    static_assert(mp::mp_size<typename P::variants>::value == 1,
                  "build_cmake_command_for_single_topic: P muss genau 1 Vendor haben "
                  "(1 Topic-Achse). Multi-Topic-Variante folgt in F.6.1.G+.");

    using SelectedVendor = mp::mp_at_c<typename P::variants, 0>;
    std::string cmd      = build_cmake_invocation_prefix<P>(source_dir, build_root);
    cmd += emit_axis_flags<SelectedVendor, AllVendorsList>(axis_flag_prefix);
    return cmd;
}

} // namespace comdare::cache_engine::permutations
