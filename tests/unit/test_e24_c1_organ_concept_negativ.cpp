// test_e24_c1_organ_concept_negativ -- E-24 / S12.1, Stufe C1: die NEGATIV-COMPILE-FIXTURE (Luecke L6).
//
// DIESE UEBERSETZUNGSEINHEIT MUSS BRECHEN. Das ist kein Versehen, sondern ihr einziger Zweck:
// Bauplan-Dossier Paragraf 4.1 (Orakel-Klasse I, Commit C1) verlangt "Nicht-Organ-Typ am OrganConcept =
// Compile-Fehler-Fixture". Ein Concept, dessen Verletzung NICHT bricht, ist wertlos -- und ein
// erfuellbarer Negativ-Assert (`static_assert(Concept<X> == false)`) beweist nur die Concept-Auswertung,
// NICHT die Wirkung an der Verwendungsstelle. Genau diese Wirkung wird hier gezeigt.
//
// SIE WIRD DESHALB NIE VOM SAMMEL-BAU GEBAUT: das CMake-Ziel ist EXCLUDE_FROM_ALL und steht bewusst
// NICHT in COMDARE_TEST_TARGETS. Gebaut wird sie ausschliesslich VOM ctest-Test gleichen Namens, der
// den Bau aufruft und die erwartete Diagnose per PASS_REGULAR_EXPRESSION auf den Marker
// "E24-C1-NEGATIV-PROBE" prueft. Baut sie versehentlich DURCH, fehlt der Marker -> Test ROT.
//
// ERWARTETE DIAGNOSEN (drei, unabhaengig voneinander):
//   E24-C1-NEGATIV-PROBE-1  ein Typ ohne Identitaets-Flaeche ist kein Organ.
//   E24-C1-NEGATIV-PROBE-2  ein Typ ohne const-noexcept observe_all() hat keine Beobachtungs-Flaeche.
//   (3) die CRTP-Wache OrganGuard beisst bei der KONSTRUKTION eines nicht-konformen Traegers -- mit
//       ihrer EIGENEN Meldung aus organ_concept.hpp. Das ist der Beweis, dass die in C1 (b/3) an alle
//       fuenf Anatomien angeheftete Wache kein toter Buchstabe ist.

#include "anatomy/organ_concept.hpp"

#include <cstddef>
#include <string_view>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der
// gleichnamigen Fixture-Typen ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;

/// (1) Ein beliebiger Typ -- keine Identitaet, keine Beobachtung.
struct NichtOrgan {
    int x = 0;
};

/// (2) Identitaet vollstaendig, Beobachtung fehlt: der haeufigste reale Fehlerfall (jemand baut eine
///     neue Gattungs-Anatomie und vergisst observe_all -- sie waere dann messtechnisch stumm).
struct OrganOhneBeobachtung {
    using composition_t = int;

    static constexpr std::string_view  composition_name() noexcept { return "OrganOhneBeobachtung"; }
    static constexpr std::string_view  paper_id() noexcept { return "P00 E24-C1-Negativ-Probe"; }
    static constexpr cea::AnatomyGenus genus() noexcept { return cea::AnatomyGenus::Set; }
    static constexpr std::size_t       organ_count() noexcept { return 0; }
    // observe_all() FEHLT ABSICHTLICH.
};

/// (3) Ein Traeger, der die Wache erbt, ohne den Vertrag zu erfuellen.
struct FalscherTraeger : OrganOhneBeobachtung, cea::OrganGuard<FalscherTraeger> {
    FalscherTraeger() noexcept = default;
};

static_assert(cea::OrganConcept<NichtOrgan>,
              "E24-C1-NEGATIV-PROBE-1: ein Nicht-Organ erfuellt OrganConcept NICHT -- erwarteter Bruch");

static_assert(cea::OrganObservable<OrganOhneBeobachtung>,
              "E24-C1-NEGATIV-PROBE-2: ohne const-noexcept observe_all() gibt es keine "
              "Beobachtungs-Flaeche -- erwarteter Bruch");

} // namespace

int main() {
    // Erst die KONSTRUKTION instanziiert den Ctor-Rumpf der CRTP-Wache und damit ihre static_asserts.
    FalscherTraeger traeger{};
    (void)traeger;
    return 0;
}
