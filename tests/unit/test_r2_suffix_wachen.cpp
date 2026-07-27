// tests/unit/test_r2_suffix_wachen.cpp -- Lane F R2 (O-8 Schritt 11): die Wachen T-a / T-b / T-c auf
// der Suffix-Single-Source profile_facade/system_version_suffix.hpp.
//
// WARUM ES DIESE WACHEN VORHER NICHT GEBEN KONNTE: bis R3 wurde der build_version-Suffix an fuenf
// Orten imperativ zusammengeklebt, in drei verschiedenen Reihenfolgen. Eine Wache haette gegen
// nichts vergleichen koennen -- es gab keine Ordnung, nur Konkatenations-Ketten. Seit R3 ist die
// Ordnung ein Datum, und erst dadurch werden diese drei Zusicherungen ueberhaupt formulierbar.
//
//   T-a  EINE QUELLE      -- gleiche Glieder ergeben denselben String, egal welcher Beitragsort ihn
//                            anfordert. Das ist die Zusicherung, die die Divergenz W-6/W-13
//                            unmoeglich macht: es gibt keinen zweiten Weg mehr, ein Suffix zu bauen.
//   T-b  GENAU EIN +ceb=  -- das CEB-Contract-Segment darf exakt einmal vorkommen. Doppelte
//                            Provenienz war ein realer Fehlermodus (Basis-build_version UND
//                            Perm-Suffix trugen es), und er ist im Ergebnis-String nachweisbar.
//   T-c  ORDNUNG          -- die Segment-Folge ist die bindende (+cxx+opt+ext ...) und das
//                            gate-Segment steht am ENDE (OP-7). Das Gegenstueck compile-time sitzt
//                            als NETZ 4 im Registry-Generator (dort: Abdeckung je Haupt-Achse).
//
// Alle drei laufen gegen die REALE Funktion, nicht gegen eine Nachbildung.

#include <profile_facade/system_version_suffix.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace pf = ::comdare::cache_engine::profile_facade;

int  g_fail = 0;
void check(char const* what, bool ok, std::string const& detail = {}) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) {
        if (!detail.empty()) std::cout << "        " << detail << "\n";
        ++g_fail;
    }
}
void check_eq(char const* what, std::string const& got, std::string const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n        got  = '" << got << "'\n";
    if (!ok) std::cout << "        want = '" << want << "'\n";
    if (!ok) ++g_fail;
}

/// Zaehlt, wie oft `needle` in `hay` vorkommt (nicht ueberlappend).
[[nodiscard]] std::size_t count_occurrences(std::string const& hay, std::string_view needle) {
    std::size_t n = 0;
    for (std::size_t pos = hay.find(needle); pos != std::string::npos; pos = hay.find(needle, pos + needle.size())) ++n;
    return n;
}

/// Ein repraesentativ vollstaendig belegtes Glieder-Set (jedes Segment einmal sichtbar).
[[nodiscard]] pf::SystemVersionSuffixParts full_parts() {
    pf::SystemVersionSuffixParts p;
    p.cxx               = "g++-16";
    p.opt               = "O3";
    p.simd              = "avx2";
    p.ceb               = "7.0";
    p.target_isa        = "aarch64";
    p.telemetry         = "silent";
    p.build_type        = "Debug";
    p.gate_contribution = "[-mavx512f]";
    return p;
}

} // namespace

int main() {
    std::cout << "== T-a: EINE Quelle ==\n";
    {
        // Der Kern der Single-Source-Zusicherung: die Funktion ist rein. Vor R3 ergab dieselbe
        // Belegung je nach Aufrufort "+ext=avx2+cxx=..." ODER "+cxx=...+ext=avx2".
        auto const p = full_parts();
        check("Zwei Aufrufe mit denselben Gliedern sind identisch",
              pf::compose_system_version_suffix(p) == pf::compose_system_version_suffix(p));

        pf::SystemVersionSuffixParts perm_like; // wie die Perm-Schleife fuellt
        perm_like.cxx  = "g++-16";
        perm_like.opt  = "O3";
        perm_like.simd = "avx2";
        pf::SystemVersionSuffixParts facade_like; // wie die Facade-Naht fuellt (andere SETZ-Reihenfolge)
        facade_like.simd = "avx2";
        facade_like.cxx  = "g++-16";
        facade_like.opt  = "O3";
        check_eq("Perm-Suffix == Facade-Suffix", pf::compose_system_version_suffix(perm_like),
                 pf::compose_system_version_suffix(facade_like));
    }
    {
        // D2.8(ii): ein leeres Glied erzeugt NIE ein leeres Segment -- fuer jedes Glied, nicht nur simd.
        pf::SystemVersionSuffixParts p;
        p.cxx               = "g++-16";
        p.opt               = "O3";
        std::string const s = pf::compose_system_version_suffix(p);
        check_eq("Nur cxx+opt belegt", s, "+cxx=g++-16+opt=O3");
        for (auto const& seg : pf::kSuffixSegmentOrder) {
            if (seg == "+cxx=" || seg == "+opt=") continue;
            check("Leeres Glied erzeugt kein Segment", s.find(seg) == std::string::npos, std::string{seg});
        }
    }

    std::cout << "== T-b: genau EIN +ceb= ==\n";
    {
        check("Vollbelegung traegt +ceb= genau einmal",
              count_occurrences(pf::compose_system_version_suffix(full_parts()), "+ceb=") == 1u);
        pf::SystemVersionSuffixParts p = full_parts();
        p.ceb                          = {};
        check("Ohne CEB-Glied gar kein +ceb=", count_occurrences(pf::compose_system_version_suffix(p), "+ceb=") == 0u);
    }

    std::cout << "== T-c: Segment-Ordnung ==\n";
    {
        std::string const s   = pf::compose_system_version_suffix(full_parts());
        auto const        pos = [&s](std::string_view seg) { return s.find(seg); };
        std::cout << "  [INFO] Vollbelegung = '" << s << "'\n";
        check("bindende Form beginnt +cxx +opt +ext",
              pos("+cxx=") < pos("+opt=") && pos("+opt=") < pos("+ext=") && pos("+ext=") < pos("+ceb="));
        // OP-7: das gate-Segment steht GANZ am Ende.
        bool gate_last = true;
        for (auto const& seg : pf::kSuffixSegmentOrder)
            if (seg != "+gate=" && !(pos(seg) < pos("+gate="))) gate_last = false;
        check("gate-Segment steht am ENDE (OP-7)", gate_last);
        // Der emittierte String folgt der DEKLARIERTEN Ordnung, nicht einer zufaellig gleichen.
        std::size_t last     = 0;
        bool        in_order = true;
        for (auto const& seg : pf::kSuffixSegmentOrder) {
            auto const q = s.find(seg);
            if (q == std::string::npos || q < last) in_order = false;
            if (q != std::string::npos) last = q;
        }
        check("Emission folgt kSuffixSegmentOrder Segment fuer Segment", in_order);
    }
    {
        // Die Ordnung selbst muss wohlgeformt sein -- sonst prueft T-c gegen eine kaputte Referenz.
        auto order = pf::kSuffixSegmentOrder;
        std::sort(order.begin(), order.end());
        check("Ordnung doublettenfrei", std::adjacent_find(order.begin(), order.end()) == order.end());
        bool shape_ok = true;
        for (auto const& seg : pf::kSuffixSegmentOrder)
            if (seg.empty() || seg.front() != '+' || seg.back() != '=') shape_ok = false;
        check("Jedes Segment hat die Form +<name>=", shape_ok);
    }

    std::cout << (g_fail == 0 ? "ALLE R2-WACHEN GRUEN\n" : "R2-WACHEN ROT\n");
    return g_fail == 0 ? 0 : 1;
}
