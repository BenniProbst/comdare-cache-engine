// =============================================================================
//  PROBE zur Koeder-Batterie koeder_clang_constexpr_grammatik.sh
//  scripts/koeder_clang_constexpr_op_matrix.cpp
// =============================================================================
//
// WAS SIE NACHWEIST
// Sie grenzt den Defekt ein, den die Heilung in abi/mess_gates_glied.hpp behebt
// (ce development 8d5ba807, Bau-Commit b83e872c). Der Befund lautete zunaechst
// "constexpr std::string geht mit clang nicht" -- das ist FALSCH und zu breit.
// Diese Matrix zeigt: es ist GENAU EINE Operation.
//
//     P1  std::string s{sv};            aus einem string_view      <- NUR DIESE bricht
//     P2  std::string s{"abc"};         aus einem const char*
//     P3  s += "abc";                   operator+=(const char*)
//     P4  s.append(sv);                 append(string_view)
//     P5  30x s += 'x';                 Wachsen ueber die SSO-Grenze
//     P6  10x s += "abcd";              dito, mit const char*
//     P7  s == sv;                      Vergleich string == string_view
//
// WARUM DAS WICHTIG IST: die Unterscheidung entscheidet ueber die BAUART der
// Heilung. Waere die ganze Klasse betroffen, muesste jede constexpr-Bildung im
// Haus auf heap-freie Puffer -- so ist es eine benannte, enge Ausnahme, und
// lager_pfad_grammatik.hpp konnte mit einer str_von()-Hilfe geheilt werden statt
// mit einem Vollumbau (s. die Begruendung im Kopf jener Datei).
//
// WIE MAN SIE FAEHRT
//     sh scripts/koeder_clang_constexpr_grammatik.sh      (faehrt sie mit)
// oder einzeln:
//     for P in P1 P2 P3 P4 P5 P6 P7; do
//         g++     -std=c++23 -fsyntax-only -D$P scripts/koeder_clang_constexpr_op_matrix.cpp
//         clang++ -std=c++23 -fsyntax-only -D$P scripts/koeder_clang_constexpr_op_matrix.cpp
//     done
//
// ERWARTETE AUSGABE (Stand 08.08.2026, g++ 15.3 / clang 22.1 gegen libstdc++)
//     P1   g++: OK   clang: FEHL      <-- die EINE brechende Operation
//     P2   g++: OK   clang: OK
//     P3   g++: OK   clang: OK
//     P4   g++: OK   clang: OK
//     P5   g++: OK   clang: OK
//     P6   g++: OK   clang: OK
//     P7   g++: OK   clang: OK
// Wenn P1 mit clang OK meldet, ist der Bibliotheks-Defekt behoben und die
// heap-freie Bildung in mess_gates_glied.hpp waere nicht mehr ZWINGEND (sie
// bleibt trotzdem der bessere Bau -- s. dort). Wenn eine der anderen Zeilen
// FEHL meldet, ist die Fehlerklasse GROESSER geworden: dann greift die
// str_von()-Begruendung in lager_pfad_grammatik.hpp nicht mehr und jene Datei
// gehoert neu bewertet.
//
// GEMESSEN gegen libstdc++ 13, 15.3 UND 16 -- alle drei brechen bei P1. Es ist
// KEIN Toolchain-Mismatch, den man mit --gcc-install-dir wegkonfigurieren kann.
// ASCII-only.
// =============================================================================
#include <string>
#include <string_view>
constexpr std::string_view SV = "abc";
#if defined(P1) // Konstruktor aus string_view
constexpr bool f() {
    std::string s{SV};
    return s.size() == 3;
}
#elif defined(P2) // Konstruktor aus const char*
constexpr bool f() {
    std::string s{"abc"};
    return s.size() == 3;
}
#elif defined(P3) // leer + operator+=(const char*)
constexpr bool f() {
    std::string s;
    s += "abc";
    return s.size() == 3;
}
#elif defined(P4) // leer + append(string_view)
constexpr bool f() {
    std::string s;
    s.append(SV);
    return s.size() == 3;
}
#elif defined(P5) // leer + operator+=(char), 30x (ueber SSO hinaus)
constexpr bool f() {
    std::string s;
    for (int i = 0; i < 30; ++i) s += 'x';
    return s.size() == 30;
}
#elif defined(P6) // leer + += const char*, ueber SSO hinaus (40 Zeichen)
constexpr bool f() {
    std::string s;
    for (int i = 0; i < 10; ++i) s += "abcd";
    return s.size() == 40;
}
#elif defined(P7) // Vergleich string == string_view
constexpr bool f() {
    std::string s;
    s += "abc";
    return s == SV;
}
#else
#error "Genau eine der Proben P1..P7 muss per -D gewaehlt werden."
#endif
static_assert(f());
int main() { return 0; }
