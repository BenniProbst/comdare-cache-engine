// test_ms2_pre_touch_seitenfehler.cpp -- MS-2: WIRKT DIE VORAB-BERUEHRUNG, ODER IST SIE VORSORGE
// OHNE BELEG?
//
// SELBSTCHECK: dieser Test zaehlt KEINE Allokationen -- das tut test_ms1_arenen_kein_alloc_im_fenster
// mit einer voellig anderen Observablen (operator-new-Zaehler). Hier wird ausschliesslich
// ru_minflt gemessen, der Minor-Fault-Zaehler des Prozesses. Zwei Tests, zwei Observablen, keiner
// verdeckt den anderen.
//
// == DIE FRAGE ======================================================================================
// Ein vor-reservierter Bereich ist erst dann wirklich reserviert, wenn die SEITEN BELEGT sind. Ein
// anonymer Block liefert virtuellen Adressraum; der erste Schreibzugriff je Seite kostet einen Minor
// Fault. Ohne Gegenmassnahme laegen diese Fehler MITTEN IM MESSFENSTER -- ausgerechnet in dem
// Apparat, der Latenz messen soll.
//
// Die Vorab-Beruehrung soll die Kosten in den Aufbau verschieben. SOLL -- gemessen wird, ob sie es
// tut. Ist der Unterschied nicht messbar, sagt dieser Test das ausdruecklich, statt eine Vorsorge
// ohne Beleg als Erfolg zu verbuchen.
//
// == DIE LAGE DER MASCHINE, am Objekt erhoben (prod1, 2026-08-09) ===================================
//   getconf PAGESIZE                                    -> 4096
//   cat /sys/kernel/mm/transparent_hugepage/enabled      -> always [madvise] never
// Der zweite Wert ist fuer dieses Mass entscheidend: THP steht auf "madvise", NICHT auf "always".
// Damit bekommt eine anonyme Reservierung ohne ausdrueckliches madvise KEINE Riesenseiten, und ein
// Fault deckt genau 4 KiB ab. Stuende THP auf "always", deckte ein Fault bis zu 2 MiB ab und die
// erwartete Fehlerzahl faellt um den Faktor 512 -- die gemessene Zahl waere dann nicht falsch, aber
// anders zu lesen. Deshalb steht der Systemzustand hier und wird nicht vorausgesetzt.
//
// Der Zustand bestaetigt zugleich die Entwurfs-Festlegung "keine pauschalen Huge Pages": systemweites
// THP "always" kann synchrone Compaction ausloesen -- Latenzspitzen genau im Messmoment.
//
// Build: Standalone int main() (kein gtest). Linux-Pfad (getrusage); auf anderen Plattformen meldet
// der Test ehrlich, dass er nicht messen kann, statt still gruen zu laufen.

#include <builder/measure_storage/mess_arena.hpp>
#include <builder/measure_storage/mess_speicher_kanon.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>

#if defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

int g_fail = 0;

void pruefe(bool ok, char const* was) {
    if (!ok) {
        ++g_fail;
        std::cout << "  [ERR] " << was << "\n";
    }
}

namespace ms = ::comdare::cache_engine::builder::measure_storage;

// 64 MiB = 2 097 152 Zeilen a 32 Byte = 16 384 Seiten a 4 KiB. Die Groesse ist so gewaehlt, dass die
// erwartete Fehlerzahl vierstellig ist -- bei einer kleinen Arena verschwaende sie im Rauschen der
// uebrigen Prozess-Aktivitaet und der Test koennte MIT und OHNE nicht unterscheiden.
inline constexpr std::uint64_t kZeilen          = (64ull * 1024ull * 1024ull) / sizeof(ms::MessCheckpointZeile);
inline constexpr std::uint64_t kErwarteteSeiten = (64ull * 1024ull * 1024ull) / 4096ull;

struct Lauf {
    std::uint64_t minflt_delta = 0;
    std::uint64_t majflt_delta = 0;
    std::uint64_t schreiben_ns = 0;
    std::uint64_t aufbau_ns    = 0;
    bool          steht        = false;
};

[[nodiscard]] std::uint64_t minor_faults() noexcept {
#if defined(__linux__)
    rusage r{};
    if (getrusage(RUSAGE_SELF, &r) != 0) return 0;
    return static_cast<std::uint64_t>(r.ru_minflt);
#else
    return 0;
#endif
}

[[nodiscard]] std::uint64_t major_faults() noexcept {
#if defined(__linux__)
    rusage r{};
    if (getrusage(RUSAGE_SELF, &r) != 0) return 0;
    return static_cast<std::uint64_t>(r.ru_majflt);
#else
    return 0;
#endif
}

/// Ein Durchgang: reservieren (mit oder ohne Vorab-Beruehrung), dann die Arena voll schreiben und
/// dabei die Seitenfehler und die Zeit messen.
///
/// SELBSTCHECK: die Zeitnahme umklammert AUSSCHLIESSLICH die Schreib-Schleife. Der Aufbau wird
/// getrennt gemessen -- sonst waere die Behauptung "die Kosten wandern in den Aufbau" nicht
/// nachpruefbar, sondern nur verschoben.
[[nodiscard]] Lauf durchgang(ms::VorabBeruehrung vorab) {
    using uhr = std::chrono::steady_clock;
    Lauf          l{};
    ms::MessArena arena;

    auto const t_a0 = uhr::now();
    l.steht         = arena.reservieren(kZeilen, vorab);
    auto const t_a1 = uhr::now();
    l.aufbau_ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t_a1 - t_a0).count());
    if (!l.steht) return l;

    ms::MessCheckpointZeile z{};
    z.tag = ms::tag_bauen(ms::MessEbene::Micro, ms::Richtung::Ein);

    std::uint64_t const min_vor = minor_faults();
    std::uint64_t const maj_vor = major_faults();
    auto const          t0      = uhr::now();
    for (std::uint64_t i = 0; i < kZeilen; ++i) {
        z.zeit_ticks = i;
        (void)arena.anhaengen(z);
    }
    auto const          t1       = uhr::now();
    std::uint64_t const min_nach = minor_faults();
    std::uint64_t const maj_nach = major_faults();

    l.minflt_delta = min_nach - min_vor;
    l.majflt_delta = maj_nach - maj_vor;
    l.schreiben_ns = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    return l;
}

} // namespace

int main() {
    std::cout << "== MS-2: wirkt die Vorab-Beruehrung? ==\n";
    std::cout << "   Arena = 64 MiB = " << kZeilen << " Zeilen a 32 Byte = " << kErwarteteSeiten << " Seiten a 4 KiB\n";

#if !defined(__linux__)
    std::cout << "   getrusage nicht verfuegbar -- auf dieser Plattform NICHT GEMESSEN, nicht gruen.\n";
    return 1;
#else
    // Reihenfolge bewusst: erst OHNE, dann MIT. Beide bekommen eine eigene, frische Reservierung --
    // es gibt keinen Aufwaerm-Effekt der einen auf die andere, weil verschiedene Seiten belegt werden.
    Lauf const ohne = durchgang(ms::VorabBeruehrung::Nein);
    Lauf const mit  = durchgang(ms::VorabBeruehrung::Ja);

    pruefe(ohne.steht && mit.steht, "beide Reservierungen stehen");

    std::cout << "-- (1) OHNE Vorab-Beruehrung --\n";
    std::cout << "     Aufbau = " << ohne.aufbau_ns << " ns, Schreiben = " << ohne.schreiben_ns
              << " ns, minor faults = " << ohne.minflt_delta << ", major faults = " << ohne.majflt_delta << "\n";
    std::cout << "-- (2) MIT Vorab-Beruehrung --\n";
    std::cout << "     Aufbau = " << mit.aufbau_ns << " ns, Schreiben = " << mit.schreiben_ns
              << " ns, minor faults = " << mit.minflt_delta << ", major faults = " << mit.majflt_delta << "\n";

    // Die eigentliche Gegenueberstellung, in beiden Groessen.
    std::cout << "-- (3) Gegenueberstellung --\n";
    std::cout << "     Seitenfehler im Fenster: ohne = " << ohne.minflt_delta << ", mit = " << mit.minflt_delta
              << " (erwartete Seiten = " << kErwarteteSeiten << ")\n";
    if (mit.schreiben_ns > 0) {
        std::cout << "     Schreibzeit: ohne = " << ohne.schreiben_ns << " ns, mit = " << mit.schreiben_ns
                  << " ns, Verhaeltnis ohne/mit = "
                  << (static_cast<double>(ohne.schreiben_ns) / static_cast<double>(mit.schreiben_ns)) << "\n";
    }
    std::cout << "     Aufbau: ohne = " << ohne.aufbau_ns << " ns, mit = " << mit.aufbau_ns
              << " ns -- die Vorab-Beruehrung kostet " << (mit.aufbau_ns - ohne.aufbau_ns)
              << " ns, und zwar VOR dem Messfenster\n";

    // (a) DIE POSITIVKONTROLLE: ohne Vorab-Beruehrung MUSS es Fehler geben. Ohne diese Zusicherung
    //     waere ein Messaufbau, der grundsaetzlich 0 liefert (falscher Zaehler, falsches Fenster),
    //     nicht von einer wirksamen Vorsorge zu unterscheiden.
    pruefe(ohne.minflt_delta > 0, "(a) ohne Vorab-Beruehrung treten Seitenfehler im Fenster auf");

    // (b) Die Groessenordnung muss zur Rechnung passen. Nicht auf die Zahl genau -- der Prozess hat
    //     nebenher eigene Fehler -- aber in der richtigen Dimension. Eine Abweichung um Faktor 2
    //     hiesse, dass hier etwas anderes gemessen wird als angenommen (z.B. doch Riesenseiten).
    pruefe(ohne.minflt_delta >= kErwarteteSeiten / 2u && ohne.minflt_delta <= kErwarteteSeiten * 2u,
           "(b) die Fehlerzahl ohne Vorsorge liegt in der gerechneten Groessenordnung");

    // (c) DIE ZUSAGE: mit Vorab-Beruehrung bleibt das Fenster praktisch frei. Nicht "== 0"
    //     festgeschrieben, weil ein fremder Fehler des Prozesses (Ausgabepuffer, Signal) in dasselbe
    //     Fenster fallen kann; die Schranke ist zwei Zehnerpotenzen unter dem Vergleichslauf, und die
    //     tatsaechliche Zahl steht oben.
    pruefe(mit.minflt_delta * 100u < ohne.minflt_delta,
           "(c) mit Vorab-Beruehrung liegt das Fenster zwei Zehnerpotenzen tiefer");

    // (d) Keine Major Faults -- sonst waere Reclaim im Spiel und die Zusage haette ein Loch, das
    //     ru_minflt allein nicht zeigt.
    pruefe(ohne.majflt_delta == 0 && mit.majflt_delta == 0, "(d) keine major faults in beiden Laeufen");

    // (e) EHRLICHKEIT ueber die ZEIT. Die Seitenfehler-Zahl ist die harte Aussage; ob sie sich auch
    //     in der Schreibzeit niederschlaegt, ist eine ZWEITE, schwaechere Frage. Sie wird gemessen
    //     und ausgesprochen -- aber NICHT zur Abnahme gemacht: eine Zeitmessung auf einer Maschine
    //     mit 32 Kernen und fremder Last ist kein Beweis, und sie zur Bedingung zu machen hiesse,
    //     einen Test zu bauen, der bei Fremdlast rot wird, ohne dass etwas kaputt ist.
    if (ohne.schreiben_ns > mit.schreiben_ns) {
        std::cout << "     BEFUND Zeit: die Vorab-Beruehrung ist auch in der Schreibzeit messbar ("
                  << (ohne.schreiben_ns - mit.schreiben_ns) << " ns Unterschied auf " << kZeilen << " Zeilen)\n";
    } else {
        std::cout << "     BEFUND Zeit: in der Schreibzeit NICHT messbar -- die Aussage dieses Tests ruht "
                     "allein auf der Seitenfehler-Zahl\n";
    }

    if (g_fail == 0) {
        std::cout << "OK: MS-2\n";
        return 0;
    }
    std::cout << "FEHLGESCHLAGEN: " << g_fail << " Pruefung(en)\n";
    return 1;
#endif
}
