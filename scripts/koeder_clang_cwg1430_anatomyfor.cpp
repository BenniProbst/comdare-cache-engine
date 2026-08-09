// =============================================================================
//  PROBE zur Koeder-Batterie koeder_clang_cwg1430.sh
//  scripts/koeder_clang_cwg1430_anatomyfor.cpp
// =============================================================================
//
// WAS SIE NACHWEIST
// Sie belegt, WARUM bei der CWG-1430-Heilung in anatomy/container_framework.hpp
// (ce development 2cb0ea34, Bau-Commit f7535751) nur CompositionFor auf eine
// Klassen-Template-Indirektion umgestellt wurde und AnatomyFor NICHT -- obwohl
// beide Zeilen direkt nebeneinander stehen und dieselbe Form haben.
// Die Begruendung steht im Header; DIESE Datei ist ihr Beleg (Doku-Commit
// e2a4cb45 / 9817a101).
//
//     A_PACK_DIREKT     traits::AnatomyFor als PACK, direkt auf die Bindung
//     B_PACK_INDIREKT   dasselbe ueber die Klassen-Indirektion
//     C_MOCK_DIREKT     der Mock-Fall HEUTE: AnatomyFor = void (haengt NICHT von Comp ab)
//     D_MOCK_INDIREKT   derselbe Mock, aber ueber die Indirektion
//
// DIE AUSSAGE, in zwei Haelften:
//   A vs B  -- ja, AnatomyFor traegt denselben Fehler, SOBALD es ein Pack wird,
//              und die Indirektion wuerde ihn beheben.
//   C vs D  -- ABER die Indirektion wuerde etwas KAPUTT machen: sie verzoegert
//              die Aufloesung, und damit faellt die fruehe Fehlererkennung weg,
//              auf der der Mock-Block in container_framework.hpp beruht.
//              C bricht (gewollt, das ist die Wache), D nicht mehr.
// Deshalb: AnatomyFor bleibt direkt. Ein Pack-Umbau kann dort nicht still
// durchrutschen -- er bricht auf BEIDEN Compilern, was koeder_clang_cwg1430.sh
// am echten Header nachweist (Koeder KA1).
//
// WIE MAN SIE FAEHRT
//     sh scripts/koeder_clang_cwg1430.sh          (faehrt sie mit)
// oder einzeln:
//     for V in A_PACK_DIREKT B_PACK_INDIREKT C_MOCK_DIREKT D_MOCK_INDIREKT; do
//         g++     -std=c++23 -fsyntax-only -D$V scripts/koeder_clang_cwg1430_anatomyfor.cpp
//         clang++ -std=c++23 -fsyntax-only -D$V scripts/koeder_clang_cwg1430_anatomyfor.cpp
//     done
//
// ERWARTETE AUSGABE (Stand 08.08.2026, g++ 15.3 / clang 22.1)
//     A_PACK_DIREKT      g++ OK      clang FEHLER    <- CWG 1430 trifft auch AnatomyFor
//     B_PACK_INDIREKT    g++ OK      clang OK        <- die Indirektion wuerde helfen
//     C_MOCK_DIREKT      g++ FEHLER  clang FEHLER    <- die fruehe Erkennung WIRKT
//     D_MOCK_INDIREKT    g++ OK      clang OK        <- ... und waere mit Indirektion weg
// Kippt C von FEHLER auf OK, ist die Mock-Eigenschaft verloren gegangen: dann
// traegt der Verzicht auf die Indirektion nicht mehr und AnatomyFor gehoert
// nachgezogen.
//
// ASCII-only.
// =============================================================================
template <class Comp>
struct Anatomie {
    using element_type = int;
};

struct EchteBindung { // wie GenusBindingTraits: AnatomyFor mit EINEM Parameter
    template <class Comp>
    using AnatomyFor = Anatomie<Comp>;
};

namespace d {
template <class B, class... C>
struct AnatomieFuer {
    using type = typename B::template AnatomyFor<C...>;
};
} // namespace d

#if defined(A_PACK_DIREKT) // A: traits::AnatomyFor als PACK, direkt auf die Bindung
template <class B>
struct traits {
    template <class... C>
    using AnatomyFor = typename B::template AnatomyFor<C...>;
    template <class Comp>
    using ElementTypeFor = typename AnatomyFor<Comp>::element_type;
};
using X = traits<EchteBindung>::AnatomyFor<Anatomie<int>>;
#elif defined(B_PACK_INDIREKT) // B: dasselbe, aber ueber die Klassen-Indirektion
template <class B>
struct traits {
    template <class... C>
    using AnatomyFor = typename d::AnatomieFuer<B, C...>::type;
    template <class Comp>
    using ElementTypeFor = typename AnatomyFor<Comp>::element_type;
};
using X = traits<EchteBindung>::AnatomyFor<Anatomie<int>>;
#elif defined(C_MOCK_DIREKT)   // C: der MOCK-Fall heute -- AnatomyFor = void, haengt NICHT von Comp ab
struct MockVoid {
    template <class Comp>
    using AnatomyFor = void;
};
template <class B>
struct traits {
    template <class Comp>
    using AnatomyFor = typename B::template AnatomyFor<Comp>;
    template <class Comp>
    using ElementTypeFor = typename AnatomyFor<Comp>::element_type;
};
using X = traits<MockVoid>; // NUR die Klasse instanziieren, ElementTypeFor NIE aufrufen
static_assert(sizeof(X) >= 0);
#elif defined(D_MOCK_INDIREKT) // D: derselbe Mock, aber AnatomyFor ueber die Indirektion
struct MockVoid {
    template <class Comp>
    using AnatomyFor = void;
};
template <class B>
struct traits {
    template <class Comp>
    using AnatomyFor = typename d::AnatomieFuer<B, Comp>::type;
    template <class Comp>
    using ElementTypeFor = typename AnatomyFor<Comp>::element_type;
};
using X = traits<MockVoid>;
static_assert(sizeof(X) >= 0);
#else
#error "Genau einer der Faelle A_PACK_DIREKT/B_PACK_INDIREKT/C_MOCK_DIREKT/D_MOCK_INDIREKT muss gewaehlt werden."
#endif
int main() { return 0; }
