// M-CE-16 (G3 Batch-2) Regressionstest: comdare::simd::avx2::add_u64 las im Carry-Pass das noch
// UNINITIALISIERTE result[j] der Tail-Limbs (UB) und verlor den Übertrag des vektorisierten Präfix in die
// Tail-Limbs → empirisch r[4]==12 statt 13. avx512-/neon-Fassung waren korrekt; nur AVX2 war kaputt.
//
// Der Test kompiliert je ISA-Stufe (Baseline / -mavx2 / -mavx512f, s. CMakeLists.txt) und ruft SOWOHL die
// namespace-spezifische Backend-Fassung (avx2::/avx512::/neon::) ALS AUCH die unified ops::-Dispatch-Fassung
// direkt auf. scalar::add_u64 ist die vertrauenswürdige Referenz (Standard-Multi-Precision-Add).

#include <comdare/simd/SIMDOps.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace cs = ::comdare::simd;

namespace {

constexpr std::uint64_t kHigh = std::uint64_t{1} << 63; // 2^63 — zwei davon addiert überlaufen ein Limb exakt.

// Der kanonische 5-Limb-Bug-Fall: Limb 3 (letztes vektorisiertes AVX2-Limb, 4-er-Blöcke) überläuft und muss
// seinen Übertrag in Limb 4 (das Tail-Limb) tragen → korrekt: {0,0,0,0,13}, carry-out 0. Der Bug lieferte r[4]=12.
struct CarryCase {
    std::array<std::uint64_t, 5> a{0, 0, 0, kHigh, 5};
    std::array<std::uint64_t, 5> b{0, 0, 0, kHigh, 7};
};

// Prüft eine Backend-Funktion gegen den handgerechneten Erwartungswert des 5-Limb-Carry-Falls.
template <class AddFn>
void expect_carry_case(AddFn add, char const* backend) {
    CarryCase                    c{};
    std::array<std::uint64_t, 5> r{};
    std::uint64_t const          carry = add(c.a.data(), c.b.data(), r.data(), r.size());
    EXPECT_EQ(r[0], 0u) << backend;
    EXPECT_EQ(r[1], 0u) << backend;
    EXPECT_EQ(r[2], 0u) << backend;
    EXPECT_EQ(r[3], 0u) << backend << " (Limb 3 = 2^63 + 2^63 mod 2^64)";
    EXPECT_EQ(r[4], 13u) << backend << " — Übertrag von Limb 3 muss in Limb 4 (5+7+1=13), NICHT verloren gehen";
    EXPECT_EQ(carry, 0u) << backend << " final carry-out";
}

// Vergleicht eine Backend-Funktion gegen die skalare Referenz über viele zufällige Multi-Limb-Eingaben und
// Längen (inkl. der 4-/8-er Vektor-/Tail-Grenzen). Deckt die korrekte Übertrags-Fortpflanzung breit ab.
template <class AddFn>
void expect_matches_scalar_reference(AddFn add, char const* backend) {
    std::mt19937_64                              rng{0xA11CE5EEDull};
    std::uniform_int_distribution<std::uint64_t> dist;
    for (std::size_t count :
         {std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{4}, std::size_t{5}, std::size_t{7},
          std::size_t{8}, std::size_t{9}, std::size_t{16}, std::size_t{17}, std::size_t{33}, std::size_t{64}}) {
        for (int rep = 0; rep < 32; ++rep) {
            std::vector<std::uint64_t> a(count), b(count), r_ref(count), r_test(count);
            for (std::size_t i = 0; i < count; ++i) {
                // Mit Wahrscheinlichkeit ~1/3 nahe der Limb-Obergrenze wählen, um Überläufe/Ketten zu erzwingen.
                a[i] = (rng() % 3u == 0u) ? (~std::uint64_t{0} - (rng() % 4u)) : dist(rng);
                b[i] = (rng() % 3u == 0u) ? (~std::uint64_t{0} - (rng() % 4u)) : dist(rng);
            }
            std::uint64_t const c_ref  = cs::scalar::add_u64(a.data(), b.data(), r_ref.data(), count);
            std::uint64_t const c_test = add(a.data(), b.data(), r_test.data(), count);
            EXPECT_EQ(c_test, c_ref) << backend << " carry count=" << count << " rep=" << rep;
            EXPECT_EQ(r_test, r_ref) << backend << " result count=" << count << " rep=" << rep;
        }
    }
}

} // namespace

// scalar ist immer verfügbar und ist die Referenz — der 5-Limb-Fall muss auch dort 13 liefern.
TEST(SimdAddU64Carry, ScalarCarryCase) { expect_carry_case(&cs::scalar::add_u64, "scalar"); }

// Die unified Dispatch-Fassung (aktives Backend der Kompilat-ISA) muss den Carry-Fall korrekt lösen.
TEST(SimdAddU64Carry, UnifiedOpsCarryCase) { expect_carry_case(&cs::ops::add_u64, "ops(active-backend)"); }

#ifdef __AVX2__
// KERN-REGRESSION M-CE-16: mit -mavx2 ist genau die zuvor kaputte AVX2-Fassung instanziiert.
TEST(SimdAddU64Carry, Avx2CarryCaseRegression) { expect_carry_case(&cs::avx2::add_u64, "avx2"); }
TEST(SimdAddU64Carry, Avx2MatchesScalarReference) { expect_matches_scalar_reference(&cs::avx2::add_u64, "avx2"); }
#endif

#ifdef __AVX512F__
TEST(SimdAddU64Carry, Avx512CarryCase) { expect_carry_case(&cs::avx512::add_u64, "avx512"); }
TEST(SimdAddU64Carry, Avx512MatchesScalarReference) { expect_matches_scalar_reference(&cs::avx512::add_u64, "avx512"); }
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
TEST(SimdAddU64Carry, NeonCarryCase) { expect_carry_case(&cs::neon::add_u64, "neon"); }
TEST(SimdAddU64Carry, NeonMatchesScalarReference) { expect_matches_scalar_reference(&cs::neon::add_u64, "neon"); }
#endif
