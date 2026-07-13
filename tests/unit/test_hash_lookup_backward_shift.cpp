// Goal-V4 G3 Batch-1 (M-CE-03): HashLookup unregister == Backward-Shift-Deletion (Knuth Algo R).
//
// Pinnt, dass unregister() die Linear-Probing-Probe-Kette NICHT zerreisst. Der frueher fehlerhafte
// Pfad (blosses buckets_[pos].reset()) haette Keys HINTER der Luecke als Miss gemeldet und beim
// Re-Insert Duplikate erzeugt. Keys, die sich um Vielfache der Kapazitaet unterscheiden, kollidieren
// im Home-Slot (Fibonacci-Multiplikator ungerade → bijektiv mod 2^n) → erzwingt echte Ketten.
//
// Standalone (plain int main, KEIN gtest).

#include <axes/cache_traversal/axis_03b_cache_traversal_hash_lookup.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace ct = ::comdare::cache_engine::cache_traversal;
using HL     = ct::HashLookup;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "==== HashLookup Backward-Shift-Deletion (M-CE-03) ====\n";

    // Kapazitaet 8 (Power-of-2); Keys 0/8/16 → gleicher Home-Slot 0 → Kette in Slots 0,1,2.
    // 3 Keys < Rehash-Schwelle (size>=6 bei cap 8) → Kapazitaet bleibt 8, Struktur stabil.

    // ── Test 1: Loeschung am Ketten-Anfang zieht Nachfolger nach ─────────────────────────────────
    {
        HL h(8);
        h.register_entry(0, 100);
        h.register_entry(8, 108);
        h.register_entry(16, 116);
        tr("1: alle drei vor Delete aufloesbar", h.resolve(0) == 100 && h.resolve(8) == 108 && h.resolve(16) == 116);
        tr("1: unregister(0) meldet true", h.unregister(0));
        // Bug (nur reset): resolve(8)/resolve(16) waeren MISS (Kette zerbrochen).
        tr("1: nach unregister(0) findet resolve(8) den Nachbarn (kein False Miss)", h.resolve(8) == 108);
        tr("1: nach unregister(0) findet resolve(16) den Nachbarn (kein False Miss)", h.resolve(16) == 116);
        tr("1: resolve(0) ist jetzt MISS", !h.resolve(0).has_value());
        tr("1: tracked_count == 2", h.tracked_count() == 2);
    }

    // ── Test 2: Loeschung in der Ketten-Mitte ────────────────────────────────────────────────────
    {
        HL h(8);
        h.register_entry(0, 100);
        h.register_entry(8, 108);
        h.register_entry(16, 116);
        tr("2: unregister(8) (Mitte) true", h.unregister(8));
        tr("2: resolve(0) weiterhin ok", h.resolve(0) == 100);
        tr("2: resolve(16) weiterhin ok (kein False Miss)", h.resolve(16) == 116);
        tr("2: resolve(8) MISS", !h.resolve(8).has_value());
        tr("2: tracked_count == 2", h.tracked_count() == 2);
    }

    // ── Test 3: kein Duplikat / kein Verlust nach Re-Insert ──────────────────────────────────────
    {
        HL h(8);
        h.register_entry(0, 100);
        h.register_entry(8, 108);
        h.register_entry(16, 116);
        h.unregister(0);
        h.register_entry(0, 999); // Re-Insert des geloeschten Keys
        tr("3: Re-Insert liefert neuen Wert", h.resolve(0) == 999);
        tr("3: Nachbarn weiter korrekt", h.resolve(8) == 108 && h.resolve(16) == 116);
        tr("3: tracked_count == 3 (kein Verlust, kein Duplikat)", h.tracked_count() == 3);
    }

    // ── Test 4: Wrap-around-Kette (Home-Slot nahe mask) ──────────────────────────────────────────
    // Keys 3/11/19 (k ≡ 3 mod 8) → Home-Slot 7 → belegen Slots 7,0,1 (Wrap ueber die Maske).
    {
        HL h(8);
        h.register_entry(3, 203);
        h.register_entry(11, 211);
        h.register_entry(19, 219);
        tr("4: Home-Slot-7 Wrap-Kette aufloesbar", h.resolve(3) == 203 && h.resolve(11) == 211 && h.resolve(19) == 219);
        tr("4: unregister(3) (Slot 7) true", h.unregister(3));
        tr("4: Wrap-Nachbarn weiter ok (kein False Miss)", h.resolve(11) == 211 && h.resolve(19) == 219);
        tr("4: resolve(3) MISS", !h.resolve(3).has_value());
    }

    std::cout << "==== HashLookup Backward-Shift-Deletion: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
