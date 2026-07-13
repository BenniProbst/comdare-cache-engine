// Goal-V4 G3 Batch-1 (M-CE-04): CuckooFilter-Verdraengungs-Korrektheit.
//
// Pinnt, dass insert_key bei zwei vollen Kandidaten-Buckets die ECHTE Kuckucks-Verdraengung
// (Fan CoNEXT 2014 §3) ausfuehrt statt einen residenten Fingerprint still zu ueberschreiben.
// Die Filter-Gattungs-Garantie (may-contain, KEINE False Negatives) MUSS fuer ALLE bereits
// eingefuegten Keys halten. Der frueher fehlerhafte Pfad (`table_[i1*kSlots] = fp`) haette
// hier echte False Negatives erzeugt.
//
// Standalone (plain int main, KEIN gtest) analog test_filter_real_from_keys.cpp.

#include <axes/filter_axis/axis_filter_cuckoo.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace flt = ::comdare::cache_engine::filter_axis;
using CF      = flt::CuckooFilter;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "==== Cuckoo Verdraengungs-Korrektheit (M-CE-04) ====\n";

    // ── Test A: gezielte Verdraengung — ein alter Key ueberlebt, KEIN False Negative ─────────────
    // Konstruiere ueber die public-static-Helfer bucket1_/bucket2_/fingerprint_ eine Situation, in der
    // beide Kandidaten-Buckets des Trigger-Keys voll sind → insert_key MUSS verdraengen, nie ueberschreiben.
    {
        std::size_t const          B0 = CF::bucket1_(0);
        std::vector<std::uint64_t> b0keys;
        std::unordered_set<int>    fps0;
        for (std::uint64_t k = 1; b0keys.size() < 8 && k < 5'000'000ull; ++k) {
            if (CF::bucket1_(k) != B0) continue;
            int const fp = CF::fingerprint_(k);
            if (fps0.count(fp)) continue; // paarweise verschiedene Fingerprints (kein idempotentes Dedup)
            fps0.insert(fp);
            b0keys.push_back(k);
        }
        tr("A: >=5 Keys mit gleichem bucket1_ + distinct fp gefunden", b0keys.size() >= 5);

        // Trigger = erster Key nach den 4 Fillern, dessen Alternativ-Bucket != B0 ist.
        std::uint64_t trigger = 0;
        std::size_t   B1      = B0;
        for (std::size_t i = 4; i < b0keys.size(); ++i) {
            std::size_t const cand = CF::bucket2_(B0, CF::fingerprint_(b0keys[i]));
            if (cand != B0) {
                trigger = b0keys[i];
                B1      = cand;
                break;
            }
        }
        tr("A: Trigger-Key mit B1 != B0 gefunden", trigger != 0 && B1 != B0);

        // 4 Keys mit bucket1_ == B1, distinct fp, disjunkt zu den B0-Keys.
        std::vector<std::uint64_t>        b1keys;
        std::unordered_set<int>           fps1;
        std::unordered_set<std::uint64_t> used(b0keys.begin(), b0keys.end());
        for (std::uint64_t k = 1; b1keys.size() < 4 && k < 5'000'000ull; ++k) {
            if (CF::bucket1_(k) != B1) continue;
            if (used.count(k)) continue;
            int const fp = CF::fingerprint_(k);
            if (fps1.count(fp)) continue;
            fps1.insert(fp);
            b1keys.push_back(k);
            used.insert(k);
        }
        tr("A: 4 Keys fuer B1 mit distinct fp gefunden", b1keys.size() == 4);

        CF cf;
        for (int i = 0; i < 4; ++i) cf.insert_key(b0keys[i]); // fuellt B0 (4 Slots)
        for (int i = 0; i < 4; ++i) cf.insert_key(b1keys[i]); // fuellt B1 (4 Slots)

        bool base_pos = true;
        for (int i = 0; i < 4; ++i) {
            if (!cf.probe_key(b0keys[i])) base_pos = false;
            if (!cf.probe_key(b1keys[i])) base_pos = false;
        }
        tr("A: 8 Filler proben positiv (vor Trigger)", base_pos);

        cf.insert_key(trigger); // B0 voll + i2==B1 voll → Verdraengung

        std::size_t pos = 0;
        for (int i = 0; i < 4; ++i) {
            pos += cf.probe_key(b0keys[i]) ? 1u : 0u;
            pos += cf.probe_key(b1keys[i]) ? 1u : 0u;
        }
        pos += cf.probe_key(trigger) ? 1u : 0u;
        std::cout << "     positiv nach Verdraengung: " << pos << "/9\n";
        tr("A: nach Verdraengung proben ALLE 9 Keys positiv (keine False Negatives)", pos == 9);
    }

    // ── Test B: Hochlast — viele Inserts mit vielfacher Verdraengung, keine False Negatives ───────
    {
        CF                      cf;
        constexpr std::uint64_t N = 9800; // ~60% von kBuckets*kSlots (16384); b=4-Cuckoo fuegt alle ein
        for (std::uint64_t i = 0; i < N; ++i) cf.insert_key(i);
        std::uint64_t pos = 0;
        for (std::uint64_t i = 0; i < N; ++i) pos += cf.probe_key(i) ? 1u : 0u;
        std::cout << "     Hochlast positiv: " << pos << "/" << N << "\n";
        tr("B: alle " + std::to_string(N) + " eingefuegten Keys proben positiv (Verdraengung erhaelt Membership)",
           pos == N);
    }

    std::cout << "==== Cuckoo Verdraengungs-Korrektheit: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
