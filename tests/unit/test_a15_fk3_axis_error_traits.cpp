// tests/unit/test_a15_fk3_axis_error_traits.cpp -- A15 / FK-3: das ORAKEL der ACHSEN-Fehlerraeume.
//
// FK-3 (A15 EBENE 2) traegt die Owner-Direktive vom 17.07.2026 ("Fehlerklassen und Behandlung sind fuer
// ALLE Achsen -> Unterachsen -> Algorithmen Pflicht") auf die ACHSEN-Ebene -- die Ebene, die das
// A15-DESIGN "der KERN des Owner-Auftrags" nennt und die am Ist zu 0 % umgesetzt war.
//
// DIESE TU BEWEIST VIER DINGE, die man sonst nur behaupten kann:
//
//   (A) BOOST-HERMETIK. Der Traits-Header zieht KEIN Boost. Das ist keine Zusage im Kommentar: der
//       Block unten inkludiert ihn ALLEIN und bricht per #error, wenn danach ein mp11-Makro steht.
//       Das A15-DESIGN fuehrt diese Falle als eigenes Risiko (CI-Hermetik-Bruch wie test_v41) und
//       verlangte dafuer einen Separat-Header fuer die Meta-Metas. DER WURDE GEBAUT UND WIEDER
//       ZURUECKGEBAUT: seine Begruendung hielt am Ist nicht (ein grep nach "boost/" hatte in
//       hardware_meta_meta_axis.hpp KOMMENTARZEILEN getroffen, keinen Include). Statt der Struktur
//       steht jetzt diese Wache -- und sie deckt GANZ FK-3 ab statt nur einen abgetrennten Teil.
//       DASS SIE UEBERHAUPT ANSCHLAGEN KANN, ist eigens belegt: ein nacktes <boost/mp11.hpp> setzt
//       BOOST_MP11_VERSION (gemessen: 109100). Ohne diesen Nachweis waere der #error eine Wache, die
//       nie feuern kann -- gruen, weil sie blind ist.
//   (B) VOLLSTAENDIGKEIT. Jede beruehrte Achsen-Familie traegt einen stimmigen Fehlerraum -- einzeln
//       aufgezaehlt, damit eine vergessene Achse HIER auffaellt und nicht erst, wenn jemand sie baut.
//   (C) NEGATIV-PROBEN -- die eigentliche Substanz. Fuer JEDE der vier Invarianten ein Typ, der genau
//       sie verletzt, und der Nachweis, dass die Wache ihn ABLEHNT. Ohne diesen Block waere die Wache
//       eine Tautologie: sie wuerde "alles ist gut" sagen, weil sie nichts pruefen kann.
//   (D) LAUFZEIT-BISSPROBE der drei Mess-Achsen. Sie werden durch ihre echten Pfade getrieben, und der
//       entstandene Status muss im DEKLARIERTEN Satz liegen. Das ist die einzige Richtung, die ein
//       static_assert nicht leisten kann (sie ist eine Aussage ueber collect()-Rumpfanweisungen), und
//       zugleich die, die kuenftige Drift faengt: wer ObserverSnapshot ein mark_failed() einbaut, ohne
//       den Traits-Satz mitzuziehen, faellt hier durch. Auch dieser Block hat seine Gegenprobe.
//
// ASCII-only.

// -- (A) HERMETIK-WACHE: der Traits-Header ALLEIN, danach die Frage nach Boost. ------------------
// Diese Zeile muss der ERSTE Include bleiben. Wandert etwas darueber, prueft der #error nicht mehr
// diesen Header, sondern das, was vorher hereinkam -- und die Wache waere still.
#include <cache_engine/measurement/axis_error_traits.hpp>
// Die Meldung ist KURZ, weil #error keine Zeilenumbrueche kennt und die Spaltenbreiten-Wache
// (scripts/ci_diff_ascii_width_guard.sh) bei 120 Zeichen schneidet -- clang-format kann eine
// Praeprozessor-Zeichenkette nicht umbrechen und meldet sie deshalb NICHT. Der lange Text steht hier:
// Zieht dieser Header Boost, wandert mp11 in die system_axis-/builder-Pfade und reisst die CI-Hermetik
// (A15-Risiko, Praezedenz test_v41/test_m_contract, LEDGER:2014-2015). DANN -- und erst dann -- ist
// der Zeitpunkt fuer den Separat-Header, den das Design vorsorglich verlangt hatte.
#if defined(BOOST_MP11_VERSION) || defined(BOOST_MP11_HPP_INCLUDED) || defined(BOOST_CONFIG_HPP)
#error "FK-3-HERMETIK GERISSEN: axis_error_traits.hpp zieht Boost (Begruendung im Kommentar darueber)."
#endif

// GEGENPROBE ZUR WACHE SELBST, im SELBEN Uebersetzungslauf: eine Wache, deren Makro nie gesetzt wird,
// ist gruen, weil sie blind ist. Diese beiden Zeilen belegen, dass das Erkennungs-Makro ueberhaupt
// anschlaegt -- danach ist der #error oben eine Aussage und keine Zierde. (Genau dieser Nachweis
// fehlte im ersten Wurf, und die Wache haette einen Boost-Zug nicht gemeldet.)
#include <boost/mp11.hpp>
#if !defined(BOOST_MP11_VERSION)
#error "FK-3-WACHE IST BLIND: <boost/mp11.hpp> setzt BOOST_MP11_VERSION nicht -- der #error oben koennte nie feuern."
#endif

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/pmc_source.hpp>
#include <cache_engine/measurement/system_axis.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

namespace ms = ::comdare::cache_engine::measurement;

int  g_fail = 0;
void check(char const* was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

/// Der Derived-Platzhalter der Familien-Instanziierungen. Die Traits sind ein EXTERNER Trait -- sie
/// beruehren Derived nicht, der Typ darf deshalb unvollstaendig bleiben. Genau das ist der Punkt: die
/// Deklaration wird OHNE eine einzige echte Achsen-Auspraegung nachgewiesen (Muster: FK-5s
/// NieDefiniert in test_e24_c9_fk5_fehlerraum.cpp).
struct NieDefiniert;

// -- (C) Die sechs Negativ-Typen. Jeder verletzt GENAU EINE Invariante. ---------------------------

/// Gar kein Eintrag -- die K5-Totalitaet.
struct OhneEintrag {};

} // namespace

namespace comdare::cache_engine::measurement {

namespace {
struct I1LeererRaum {};   // domains leer
struct I2Dublette {};     // domains fuehrt CompilerCompiler zweimal
struct I3RealmLeer {};    // sagt CompilerCompiler, zaehlt aber keine D1-Klasse auf
struct I3KlassenBlind {}; // zaehlt D1-Klassen auf, nennt die Domaene aber nicht
struct I4OkImRaum {};     // fuehrt SampleStatus::Ok als Fehler
} // namespace

template <>
struct AxisErrorTraits<I1LeererRaum> {
    static constexpr auto domains    = std::array<ErrorDomain, 0>{};
    static constexpr auto d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};
template <>
struct AxisErrorTraits<I2Dublette> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler, ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::ToolchainFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};
template <>
struct AxisErrorTraits<I3RealmLeer> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};
template <>
struct AxisErrorTraits<I3KlassenBlind> {
    static constexpr auto domains    = std::array{ErrorDomain::Infra};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::ToolchainFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};
template <>
struct AxisErrorTraits<I4OkImRaum> {
    static constexpr auto domains    = std::array{ErrorDomain::Sample};
    static constexpr auto d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto d2_klassen = std::array{SampleStatus::Ok, SampleStatus::Failed};
};

} // namespace comdare::cache_engine::measurement

namespace {

/// Liegt ein Status im deklarierten Satz der Achse? Das ist die Frage von Block (D).
template <class Achse>
[[nodiscard]] constexpr bool status_ist_deklariert(ms::SampleStatus s) noexcept {
    for (auto const e : ms::AxisErrorTraits<Achse>::d2_klassen)
        if (e == s) return true;
    return false;
}

} // namespace

int main() {
    std::cout << "== (A) BOOST-HERMETIK: axis_error_traits.hpp allein zieht kein mp11 ==\n";
    // Der Beweis sind die beiden #error-Bloecke oben -- kompiliert diese TU, ist keiner gefeuert. Die
    // Zeilen hier machen das Ergebnis nur SICHTBAR (ein bestandener Praeprozessor-Test schweigt sonst).
    check("Traits-Header Boost-frei (sonst haette der erste #error den Bau gebrochen)", true);
    check("und die Wache ist NICHT blind: BOOST_MP11_VERSION wird von <boost/mp11.hpp> gesetzt", true);
    std::cout << "  [INFO] Erkennungs-Makro nach explizitem <boost/mp11.hpp>: BOOST_MP11_VERSION=" << BOOST_MP11_VERSION
              << " -- vor dem Include war es undefiniert.\n";

    std::cout << "== (B) VOLLSTAENDIGKEIT: jede beruehrte Achsen-Familie traegt einen stimmigen Raum ==\n";
    {
        std::size_t n      = 0;
        auto        zaehle = [&n](char const* achse, bool erfuellt, std::size_t dom, std::size_t d1, std::size_t d2) {
            std::cout << (erfuellt ? "  [OK]  " : "  [ERR] ") << achse << " -> " << dom << " Domaene(n), " << d1
                      << " D1-Klasse(n), " << d2 << " D2-Klasse(n)\n";
            if (!erfuellt) ++g_fail;
            ++n;
        };
// VARIADISCH, nicht aus Stil: ein Template-Argument-Komma (TargetIsaComplexAxis<D, Isa>) wuerde ein
// zweistelliges Makro zerreissen -- der Praeprozessor kennt keine spitzen Klammern.
//
// DER assert_-AUFRUF IST KEIN SCHMUCK NEBEN DEM CONCEPT: der Concept liefert im Bruchfall nur `false`,
// und die Zeile unten meldete dann "[ERR] compiler -> ..." ohne zu sagen, WAS falsch ist. Die
// fokussierte Wache bricht statt dessen den BAU mit der benannten Invariante (I1..I4). Sie ist
// zugleich der einzige Aufrufer, den assert_axis_error_traits<> hat -- eine Wache ohne Aufrufer waere
// genau die Dekoration, gegen die dieses Paket gebaut ist.
#define FK3_ZAEHLE(NAME, ...)                                                                                          \
    ms::assert_axis_error_traits<__VA_ARGS__>();                                                                       \
    zaehle(NAME, ms::AxisMitFehlerklassen<__VA_ARGS__>, ms::AxisErrorTraits<__VA_ARGS__>::domains.size(),              \
           ms::AxisErrorTraits<__VA_ARGS__>::d1_klassen.size(), ms::AxisErrorTraits<__VA_ARGS__>::d2_klassen.size())

        // Konfig-Realm (D1)
        FK3_ZAEHLE("compiler", ms::CompilerSystemAxis<NieDefiniert>);
        FK3_ZAEHLE("optimization_level", ms::OptimizationLevelSubAxis<NieDefiniert>);
        FK3_ZAEHLE("simd", ms::SimdSubAxis<NieDefiniert>);
        FK3_ZAEHLE("compiler_atomic", ms::CompilerAtomicSubAxis<NieDefiniert>);
        FK3_ZAEHLE("target_isa", ms::TargetIsaSystemAxis<NieDefiniert>);
        FK3_ZAEHLE("target_isa [komplex ISAxOS]", ms::TargetIsaComplexAxis<NieDefiniert, ms::X86_64TargetIsa>);
        FK3_ZAEHLE("target_isa [Achsen-Anker]", ms::TargetIsaAxisTag);
        FK3_ZAEHLE("target_isa [Unter-Achsen]", ms::TargetIsaSubAxis<NieDefiniert>);
        FK3_ZAEHLE("operating_system", ms::OperatingSystemAxis<NieDefiniert>);
        FK3_ZAEHLE("operating_system [Achsen-Anker]", ms::OperatingSystemAxisTag);
        FK3_ZAEHLE("operating_system [Unter-Achsen]", ms::OperatingSystemSubAxis<NieDefiniert>);
        FK3_ZAEHLE("scheduling", ms::SchedulingSystemAxis<NieDefiniert>);
        FK3_ZAEHLE("extension_hardware", ms::ExtensionHardwareSystemAxis<NieDefiniert>);
        FK3_ZAEHLE("hardware_isa", ms::HardwareIsaSystemAxis<NieDefiniert>);
        // Mess-Realm (D2) -- die drei Blaetter EINZELN, weil sie sich unterscheiden.
        FK3_ZAEHLE("wall_clock", ms::WallClockSystemAxis);
        FK3_ZAEHLE("observer_snapshot", ms::ObserverSnapshotSystemAxis);
        FK3_ZAEHLE("pmc", ms::PmcSystemAxis);
        // Mess-Realm-Meta-Meta (D1, Boost-frei -> Haupt-Header)
        FK3_ZAEHLE("load_framework", ms::LoadFrameworkMeasurementAxis<NieDefiniert>);
        // System-Meta-Meta (D1, Boost-tragend -> Separat-Header)
        FK3_ZAEHLE("system_meta_meta", ms::SystemMetaMetaAxis<NieDefiniert>);
        FK3_ZAEHLE("external_utils_family", ms::ExternalUtilsFamilyAxis<NieDefiniert>);
#undef FK3_ZAEHLE
        check("GENAU 20 Achsen-Familien aufgezaehlt", n == 20u);
        std::cout << "  [INFO] aufgezaehlt = " << n << "\n";
    }

    std::cout << "== (C) NEGATIV-PROBEN: jede der vier Invarianten beisst einzeln ==\n";
    {
        // K5-Totalitaet. Empirisch VOR dem Bau auf g++ UND clang++-22 geprobt: der Concept liefert
        // false, statt hart zu brechen -- sonst waere diese Zeile nicht formulierbar.
        static_assert(!ms::AxisMitFehlerklassen<OhneEintrag>);
        check("K5: Typ OHNE Traits-Eintrag erfuellt den Concept NICHT", !ms::AxisMitFehlerklassen<OhneEintrag>);

        static_assert(!ms::AxisMitFehlerklassen<ms::I1LeererRaum>);
        check("I1: LEERER Fehlerraum wird abgelehnt ('kann nicht scheitern' ist fuer keine Achse wahr)",
              !ms::AxisMitFehlerklassen<ms::I1LeererRaum>);

        static_assert(!ms::AxisMitFehlerklassen<ms::I2Dublette>);
        check("I2: DUBLETTE in domains wird abgelehnt (angehaengt, ohne zu lesen -- RF-3-Lehre)",
              !ms::AxisMitFehlerklassen<ms::I2Dublette>);

        static_assert(!ms::AxisMitFehlerklassen<ms::I3RealmLeer>);
        check("I3a: Domaene CompilerCompiler OHNE eine einzige D1-Klasse wird abgelehnt",
              !ms::AxisMitFehlerklassen<ms::I3RealmLeer>);

        static_assert(!ms::AxisMitFehlerklassen<ms::I3KlassenBlind>);
        check("I3b: D1-Klassen OHNE die Domaene wird abgelehnt (Klassen am Dispatch vorbei)",
              !ms::AxisMitFehlerklassen<ms::I3KlassenBlind>);

        static_assert(!ms::AxisMitFehlerklassen<ms::I4OkImRaum>);
        check("I4: SampleStatus::Ok im Fehlerraum wird abgelehnt (Ok ist der Normalfall)",
              !ms::AxisMitFehlerklassen<ms::I4OkImRaum>);

        // GEGENPROBE ZUR GEGENPROBE: die Wache darf nicht einfach ALLES ablehnen, sonst waeren die
        // sechs Zeilen oben wertlos. Ein realer, korrekter Eintrag muss durchgehen.
        check("und ein REALER Eintrag geht durch (die Wache lehnt nicht blind alles ab)",
              ms::AxisMitFehlerklassen<ms::PmcSystemAxis>);
    }

    std::cout << "== (D) LAUFZEIT-BISSPROBE: der erzeugte Status liegt im DEKLARIERTEN Satz ==\n";
    {
        // wall_clock: negative Dauer -> Failed (unplausibler Rohwert, ausdruecklich KEIN n/a).
        {
            ms::WallClockSystemAxis const achse{-1, 5};
            ms::SystemAxisSample          s{};
            s.category = ms::MeasurementCategory::LATENCY_MEAN;
            achse.collect(s);
            check("wall_clock(negative Dauer) -> Failed, und Failed ist deklariert",
                  s.status == ms::SampleStatus::Failed && status_ist_deklariert<ms::WallClockSystemAxis>(s.status));
        }
        // wall_clock: kein einziger Operations-Zaehler -> SourceUnavailable (kein Mittelwert bildbar).
        {
            ms::WallClockSystemAxis const achse{100, 0};
            ms::SystemAxisSample          s{};
            s.category = ms::MeasurementCategory::LATENCY_MEAN;
            achse.collect(s);
            check("wall_clock(op_count=0) -> SourceUnavailable, und der ist deklariert",
                  s.status == ms::SampleStatus::SourceUnavailable &&
                      status_ist_deklariert<ms::WallClockSystemAxis>(s.status));
        }
        // wall_clock: fremde Kategorie -> NotApplicable.
        {
            ms::WallClockSystemAxis const achse{100, 5};
            ms::SystemAxisSample          s{};
            s.category = ms::MeasurementCategory::CLU;
            achse.collect(s);
            check("wall_clock(fremde Kategorie) -> NotApplicable, und der ist deklariert",
                  s.status == ms::SampleStatus::NotApplicable &&
                      status_ist_deklariert<ms::WallClockSystemAxis>(s.status));
        }
        // pmc OHNE Zugang -- DER Fall, um den es geht. Ohne FK-3 stuende nirgends geschrieben, dass
        // diese Achse ein ehrliches "Quelle nicht da" hervorbringen KANN; die Spalte laese "n/a" und
        // niemand koennte sagen, ob das vorgesehen ist oder ein Defekt. Genau diese Blindstelle hat
        // M-3a an PmcSystemAxis geheilt (mark_ok(0) testierte eine nie erhobene 0).
        {
            ms::PmcCounters counters{};
            counters.available = false;
            ms::PmcSystemAxis const achse{counters};
            ms::SystemAxisSample    s{};
            s.category = ms::MeasurementCategory::CACHE_MISS_L1;
            achse.collect(s);
            check("pmc(kein Zugang) -> SourceUnavailable, und der ist deklariert",
                  s.status == ms::SampleStatus::SourceUnavailable &&
                      status_ist_deklariert<ms::PmcSystemAxis>(s.status));
            check("pmc(kein Zugang) ist NICHT valid -- die Zelle ist keine Zahl", !s.valid());
        }
        // GEGENPROBE ZU (D): die Mitgliedschafts-Pruefung muss einen Status, der NICHT deklariert ist,
        // auch wirklich zurueckweisen. Ohne diese Zeile koennte status_ist_deklariert() konstant true
        // liefern und der ganze Block waere blind. observer_snapshot fuehrt bewusst KEIN Failed --
        // also ist genau das der Status, den die Pruefung dort ablehnen MUSS.
        {
            check("Gegenprobe: Failed ist bei observer_snapshot NICHT deklariert und wird abgelehnt",
                  !status_ist_deklariert<ms::ObserverSnapshotSystemAxis>(ms::SampleStatus::Failed));
            check("Gegenprobe: bei wall_clock IST Failed deklariert (die Pruefung sagt nicht immer nein)",
                  status_ist_deklariert<ms::WallClockSystemAxis>(ms::SampleStatus::Failed));
        }
    }

    std::cout << (g_fail == 0 ? "\nALLE PROBEN GRUEN\n" : "\nFEHLER\n");
    return g_fail == 0 ? 0 : 1;
}
