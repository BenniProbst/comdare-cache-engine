// tests/unit/test_fk6_algo_error_classes.cpp -- A15 / FK-6: das ORAKEL der ALGORITHMEN-Fehlerraeume.
//
// FK-5 (E-24 C9) hat den Fehlerraum an den 18 Organ-Haupt-Achsen-CRTP-Basen verankert und die
// per-Varianten-Verfeinerung ausdruecklich als "deklarierte Luecke" stehen lassen
// (organ_axis_error_classes.hpp). FK-6 schliesst sie -- aber GEZIELT, nicht flaechig, und der Grund
// dafuer ist gemessen worden, bevor eine Zeile Wache entstand:
//
//   126  registrierte Organ-Varianten (mp_size ueber AllRegisteredOrganVariantsFlat)
//    26  davon tragen requires_specialized_hardware()  (die gesamte Allokator-Achse)
//     1  davon meldet true: pim_malloc
//     0  von 18 Achsen fuehren 'quelle_nicht_verfuegbar' -- das Etikett hatte keinen Traeger
//
// Fuer 125 der 126 deckt die ACHSE den Fehlerraum vollstaendig ab; 126 Deklarationen waeren 125-mal
// Abschrift gewesen. Fuer pim_malloc deckt sie ihn NICHT ab: der Algorithmus misst gegen PIM-DPU-
// Hardware, die auf einer CPU-Flotte strukturell fehlt (D2 SourceUnavailable), waehrend die
// Allokator-Achse nur den Boden (mess_fehler) fuehrt. Ohne die eigene Klasse laese ein solcher Lauf
// als "mess_fehler" -- als DEFEKT DES ALGORITHMUS statt als fehlende Quelle.
//
// DIESE TU BEWEIST VIER DINGE, die man sonst nur behaupten kann:
//
//   (A) DIE MIGRATION. pim_malloc fuehrt 'quelle_nicht_verfuegbar' UND behaelt den Boden. Dieser
//       Block war vor der Migration ROT -- er ist der T-1-Kern und nicht nachtraeglich angepasst.
//   (B) DIE WACHE BEISST. Frisch gewuerfelte Koeder-Typen, die genau EINE Invariante verletzen, und
//       der Nachweis, dass das Praedikat sie ABLEHNT. Ohne diesen Block waere die Wache gruen, weil
//       sie nichts pruefen kann.
//   (C) DIE UEBERGANGSLISTE TRAEGT WIRKLICH. Derselbe Koeder wird einmal gegen die leere und einmal
//       gegen eine Liste MIT seinem Namen gefahren -- nur so ist der Durchlass-Zweig ueberhaupt
//       beobachtbar. Bei fest verdrahteter (heute leerer) Liste waere er eine tote Haelfte.
//   (D) KEINE TAUTOLOGIE IN DIE ANDERE RICHTUNG. Ein nicht-HW-gateter Geschwister-Algorithmus
//       (std_malloc) erfuellt die Wache OHNE eigene Deklaration. Wuerde die Wache flaechig fordern,
//       schluege sie hier an -- und genau das soll sie nicht.
//
// SELBSTCHECK: prueft die DEKLARATION und die WACHE (compile-time-Fehlerraum je Algorithmus). Es
// prueft NICHT, dass der Harness zur Laufzeit wirklich SourceUnavailable setzt, wenn PIM-Hardware
// fehlt -- diese Verdrahtung liegt am Mess-Pfad und ist hier ausdruecklich NICHT behauptet. Es
// beruehrt keine Messung: permutation_axes.xml, binary_id, algo_sig und golden bleiben unangetastet
// (die Wache emittiert nichts).
//
// ASCII-only.

#include <cache_engine/measurement/axis_error.hpp> // die EINE D2-Taxonomie-Quelle (measurement-Schicht)
#include <topics/organ_axis_error_classes.hpp>     // FK-5-Etiketten + FK-6-Praedikat/Wache

#include <organ_axes/alloc/axis_06_allocator_pim_malloc.hpp> // der EINE HW-gatete Algorithmus
#include <organ_axes/alloc/axis_06_allocator_std_malloc.hpp> // ein nicht-gatetes Geschwister (D)

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

namespace tp = ::comdare::cache_engine::topics;
namespace ms = ::comdare::cache_engine::measurement;
namespace al = ::comdare::cache_engine::alloc;

int  g_fail = 0;
void check(char const* was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

// -- (B) DIE KOEDER, frisch gewuerfelt (K13: nicht aus einer Doku abgeschrieben) ------------------
// Jeder verletzt GENAU EINE Bedingung; der Name ist zufaellig gewuerfelt, damit er mit keinem
// Bestandsnamen kollidiert und die Listen-Probe in (C) eine echte Aussage macht.
constexpr std::string_view kKoederName = "qz7f_hw_koeder_4b19";

/// KOEDER 1 -- HW-gated, aber der Fehlerraum fuehrt NUR den Boden. Das ist der Zustand, in dem
/// pim_malloc VOR der Migration war: die Wache MUSS ihn ablehnen.
struct KoederHwGatedOhneKlasse {
    [[nodiscard]] static constexpr bool             requires_specialized_hardware() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return kKoederName; }
    [[nodiscard]] static constexpr auto             error_classes() noexcept { return tp::kOrganAxisErrorFloor; }
};

/// KOEDER 2 -- HW-gated und MIT der Klasse. Die Gegenrichtung: die Wache darf ihn NICHT ablehnen,
/// sonst waere sie kein Praedikat, sondern ein Verbot.
struct KoederHwGatedMitKlasse {
    [[nodiscard]] static constexpr bool             requires_specialized_hardware() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "qz7f_hw_ok_4b19"; }
    [[nodiscard]] static constexpr auto             error_classes() noexcept {
        return std::array<std::string_view, 2>{tp::kOrganErrMessFehler, tp::kOrganErrQuelleNichtVerfuegbar};
    }
};

/// KOEDER 3 -- NICHT gated, nur der Boden. Er belegt, dass die Wache nicht flaechig fordert.
struct KoederOhneGate {
    [[nodiscard]] static constexpr bool             requires_specialized_hardware() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "qz7f_kein_gate_4b19"; }
    [[nodiscard]] static constexpr auto             error_classes() noexcept { return tp::kOrganAxisErrorFloor; }
};

/// Die Test-eigene Uebergangsliste fuer (C). Sie steht NUR hier -- die Produktions-Liste bleibt leer.
constexpr auto kTestListeMitKoeder = std::array<std::string_view, 1>{kKoederName};
constexpr auto kLeereListe         = std::array<std::string_view, 0>{};

} // namespace

int main() {
    std::cout << "== (A) T-1-KERN: pim_malloc fuehrt 'quelle_nicht_verfuegbar' ==\n";
    {
        constexpr auto satz = al::PIMMallocAllocator::error_classes();
        std::cout << "  [INFO] pim_malloc::error_classes().size() = " << satz.size() << "\n";
        for (std::string_view const e : satz) std::cout << "  [INFO]   fuehrt: '" << e << "'\n";
        check("pim_malloc fuehrt 'quelle_nicht_verfuegbar' (PIM-DPU-HW kann strukturell fehlen)",
              tp::fehlerraum_enthaelt(satz, tp::kOrganErrQuelleNichtVerfuegbar));
        // Der Boden bleibt: eine PIM-Allokation kann AUCH real scheitern (OOM). Die neue Klasse
        // ERGAENZT den Achsen-Boden, sie ERSETZT ihn nicht -- sonst waere ein echter Algo-Fehler
        // ploetzlich unbenennbar.
        check("pim_malloc behaelt den Achsen-Boden 'mess_fehler' (ergaenzt, ersetzt nicht)",
              tp::fehlerraum_enthaelt(satz, tp::kOrganErrMessFehler));
        check("pim_malloc meldet requires_specialized_hardware() == true (Vorbedingung des Befunds)",
              al::PIMMallocAllocator::requires_specialized_hardware());
        // Cross-Layer-Naht wie in FK-5: das Etikett ist zeichenfolgen-gleich zur measurement-Quelle.
        check("'quelle_nicht_verfuegbar' == sample_status_label(SourceUnavailable)",
              tp::kOrganErrQuelleNichtVerfuegbar == ms::sample_status_label(ms::SampleStatus::SourceUnavailable));
        check("die FK-6-Wache laesst pim_malloc durch (gegen die LEERE Produktions-Liste)",
              tp::algo_fehlerraum_erfuellt<al::PIMMallocAllocator>(tp::kAlgoFehlerraumUebergangsliste));
    }

    std::cout << "== (B) DIE WACHE BEISST: Koeder, die genau eine Invariante verletzen ==\n";
    {
        static_assert(!tp::algo_fehlerraum_erfuellt<KoederHwGatedOhneKlasse>(kLeereListe));
        static_assert(tp::algo_fehlerraum_erfuellt<KoederHwGatedMitKlasse>(kLeereListe));
        static_assert(tp::algo_fehlerraum_erfuellt<KoederOhneGate>(kLeereListe));
        check("HW-gated OHNE 'quelle_nicht_verfuegbar' wird ABGELEHNT",
              !tp::algo_fehlerraum_erfuellt<KoederHwGatedOhneKlasse>(kLeereListe));
        check("HW-gated MIT der Klasse wird durchgelassen",
              tp::algo_fehlerraum_erfuellt<KoederHwGatedMitKlasse>(kLeereListe));
        check("NICHT-gated ohne eigene Deklaration wird durchgelassen",
              tp::algo_fehlerraum_erfuellt<KoederOhneGate>(kLeereListe));
    }

    std::cout << "== (C) DIE UEBERGANGSLISTE TRAEGT (und ist heute leer) ==\n";
    {
        // DERSELBE Koeder, zwei Listen: ohne Eintrag abgelehnt, mit Eintrag geduldet. Nur diese
        // Gegenueberstellung macht den Durchlass-Zweig beobachtbar -- bei fest verdrahteter leerer
        // Liste koennte ihn niemand ausloesen, und er waere eine tote Wachen-Haelfte.
        static_assert(!tp::algo_fehlerraum_erfuellt<KoederHwGatedOhneKlasse>(kLeereListe));
        static_assert(tp::algo_fehlerraum_erfuellt<KoederHwGatedOhneKlasse>(kTestListeMitKoeder));
        check("derselbe Koeder: OHNE Listen-Eintrag abgelehnt",
              !tp::algo_fehlerraum_erfuellt<KoederHwGatedOhneKlasse>(kLeereListe));
        check("derselbe Koeder: MIT Listen-Eintrag geduldet (Durchlass-Zweig beobachtet)",
              tp::algo_fehlerraum_erfuellt<KoederHwGatedOhneKlasse>(kTestListeMitKoeder));
        // DIE KENNZAHL. Sie darf nur schrumpfen. Steht hier je eine Zahl > 0, gehoert zu jedem
        // Eintrag eine Begruendung in organ_axis_error_classes.hpp (Abnahme-Formel ##06).
        std::cout << "  [INFO] Uebergangsliste Laenge = " << tp::kAlgoFehlerraumUebergangsliste.size() << "\n";
        check("Uebergangsliste ist leer (0 offene Migrationen)", tp::kAlgoFehlerraumUebergangsliste.size() == 0u);
    }

    std::cout << "== (D) KEINE TAUTOLOGIE: das nicht-gatete Geschwister bleibt unberuehrt ==\n";
    {
        // std_malloc liegt auf DERSELBEN Achse wie pim_malloc und deklariert NICHTS eigenes. Es
        // erbt den Achsen-Boden -- und das ist richtig so: seine Quelle ist die CPU, und die ist da.
        check("std_malloc meldet requires_specialized_hardware() == false",
              !al::StdMalloc::requires_specialized_hardware());
        check("std_malloc erfuellt die Wache OHNE eigene Deklaration",
              tp::algo_fehlerraum_erfuellt<al::StdMalloc>(tp::kAlgoFehlerraumUebergangsliste));
        check("std_malloc fuehrt 'quelle_nicht_verfuegbar' NICHT (es braucht sie nicht)",
              !tp::fehlerraum_enthaelt(al::StdMalloc::error_classes(), tp::kOrganErrQuelleNichtVerfuegbar));
    }

    std::cout << (g_fail == 0 ? "\nALLE PROBEN GRUEN\n" : "\nFEHLER\n");
    return g_fail == 0 ? 0 : 1;
}
