// Goal-V4 G3 Batch-1 (M-CE-01): ScallocAllocator::reallocate spiegelt die alignment-Weiche.
//
// Pinnt, dass reallocate denselben alignment-Pfad wie allocate/deallocate nimmt: bei alignment >
// max_align_t stammt der Pointer aus portable_aligned_alloc und darf NICHT an ::scalloc_realloc
// gehen (Fremd-Heap → Korruption). Verifiziert: korrektes Alignment + Datenerhalt ueber realloc.
//
// Hinweis: In diesem Build ist scalloc_enabled==0 (Vendor nicht eingebunden) → der aktive Pfad ist
// der portable Zweig. Der Test verifiziert reallocate-Korrektheit auf dem aktiven Pfad (Alignment +
// Daten); der gespiegelte scalloc-enabled-Zweig ist per Code-Review/Struktur-Angleichung abgesichert
// und wuerde bei Vendor-Aktivierung denselben Pfad nehmen. Unter ASan (out-of-tree) korruptionsfrei.
//
// Standalone (plain int main, KEIN gtest).

#include <axes/alloc/axis_06_allocator_scalloc.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace al = ::comdare::cache_engine::alloc;
using SA     = al::ScallocAllocator;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

static bool is_aligned(void* p, std::size_t a) { return (reinterpret_cast<std::uintptr_t>(p) & (a - 1)) == 0; }

static unsigned char pat(std::size_t i) { return static_cast<unsigned char>((i * 37u + 11u) & 0xFFu); }

int main() {
    std::cout << "==== Scalloc reallocate Alignment-Korrektheit (M-CE-01) ====\n";
    std::cout << "  (scalloc_enabled=" << (SA::enabled ? 1 : 0) << ")\n";
    SA a;

    // Grosses alignment (> max_align_t): allocate nimmt portable_aligned_alloc; reallocate MUSS
    // denselben Zweig spiegeln. Verifiziert Alignment + Datenerhalt (kein Fremd-Heap-realloc).
    for (std::size_t align : {std::size_t{64}, std::size_t{128}, std::size_t{256}, std::size_t{4096}}) {
        std::string const tag = " (align=" + std::to_string(align) + ")";
        std::size_t const n0  = 200;
        auto*             p   = static_cast<unsigned char*>(a.allocate(n0, align));
        tr("allocate != null" + tag, p != nullptr);
        tr("allocate-Pointer ausgerichtet" + tag, p != nullptr && is_aligned(p, align));
        if (p == nullptr) continue;
        for (std::size_t i = 0; i < n0; ++i) p[i] = pat(i);

        std::size_t const n1 = 4000; // vergroessern
        auto*             q  = static_cast<unsigned char*>(a.reallocate(p, n0, n1, align));
        tr("reallocate != null" + tag, q != nullptr);
        tr("reallocate-Pointer ausgerichtet" + tag, q != nullptr && is_aligned(q, align));
        bool preserved = (q != nullptr);
        for (std::size_t i = 0; i < n0 && q != nullptr; ++i)
            if (q[i] != pat(i)) {
                preserved = false;
                break;
            }
        tr("reallocate erhaelt die alten Daten (kein Fremd-Heap-Pfad)" + tag, preserved);
        a.deallocate(q, n1, align);
    }

    // Kleines alignment (<= max_align_t): normaler realloc-Pfad, Datenerhalt.
    {
        std::size_t const align = alignof(std::max_align_t);
        auto*             p     = static_cast<unsigned char*>(a.allocate(100, align));
        for (std::size_t i = 0; i < 100 && p; ++i) p[i] = pat(i);
        auto* q         = static_cast<unsigned char*>(a.reallocate(p, 100, 500, align));
        bool  preserved = (q != nullptr);
        for (std::size_t i = 0; i < 100 && q; ++i)
            if (q[i] != pat(i)) {
                preserved = false;
                break;
            }
        tr("kleines alignment: realloc != null + Daten erhalten", q != nullptr && preserved);
        a.deallocate(q, 500, align);
    }

    std::cout << "==== Scalloc reallocate Alignment-Korrektheit: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
