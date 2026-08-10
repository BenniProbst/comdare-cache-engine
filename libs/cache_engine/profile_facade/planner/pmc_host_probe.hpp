#pragma once
// pmc_host_probe.hpp -- DIE ERKENNUNG. Der Planer fragt zur LAUFZEIT den Host, auf dem er gerade laeuft.
//
// OWNER-WORTLAUT (10.08.2026, verbatim): "Dabei erkennt jeder Planer auf jeder Maschine fuer sich, ob PMC
// existiert und ob daher das PMC in der CEB verbaut wird."
// OWNER-PRAEZISIERUNG (10.08.2026, verbatim): "Moment mal JEDER Planer? Es gibt ja nur einen Planer oder
// wie hast du das jetzt verstanden?" -- am Objekt gezaehlt gibt es GENAU EINEN Planer
// (apps/experiment_planner, ein einziges add_executable). "Jeder Planer auf jeder Maschine" meint deshalb
// jeden LAUF dieses EINEN Programms auf einem Host. DARAUS DIE BAU-ANFORDERUNG, die diese Datei traegt:
// DIESELBE Binary muss auf prod1 und prod2 zu VERSCHIEDENEN Ergebnissen kommen. Eine Erkennung, die zur
// BAUZEIT des Planers festgelegt wird (ein #ifdef, eine CMake-Option), erfuellt das NICHT.
//
// DER SCHEINBARE WIDERSPRUCH ZUM ARCHITEKTUR-KANON, aufgeloest: "was zur Uebersetzungszeit entschieden
// werden kann, wird dort entschieden" gilt weiter -- die PMC-Erkennung KANN dort nicht entschieden werden,
// weil ihr Gegenstand (diese Maschine, jetzt, mit diesen Rechten) zur Uebersetzungszeit des Planers nicht
// existiert. Der Planer entscheidet zur LAUFZEIT, was die CEB zur COMPILE-Zeit einbaut; die dritte Stufe
// (Tier) sieht davon nichts. Kein Runtime-Switch wandert in ein Tier-Binary.
//
// ------------------------------------------------------------------------------------------------
// DAS KRITERIUM -- UND WARUM ES NICHT "ALLE EVENTS MUESSEN BEISSEN" LAUTET
// ------------------------------------------------------------------------------------------------
// Der Entwurf zu dieser Arbeit verlangte: alle Events muessen oeffnen UND zaehlen, Teilverfuegbarkeit =>
// unbrauchbar. AM OBJEKT GEMESSEN IST DAS FALSCH, und zwar in der gefaehrlichen Richtung:
//   * Die reale Quelle definiert ihre eigene Ehrlichkeit als ODER, nicht als UND:
//     `ready_ = l1d_offen_() || ll_offen_() || dtlb_offen_() || branch_offen_()`
//     (builder/linux_perf_pmc_source.hpp, Konstruktor-Ende).
//   * prod1 ist ein AMD Ryzen 9 9950X3D (Zen 5). Dort kennt der GENERISCHE Last-Level-Zaehler das Event
//     nicht (ENOENT) -- der Quell-Kopf beschreibt genau diesen Fall und haelt dafuer den amd_l3-Zweitpfad
//     vor, der bewusst KEINEN Wert fuellt. prod1 hat also strukturell Teilverfuegbarkeit.
//   * prod1 misst nachweislich PMC (Job 189916: pmc_available=1, cache_misses_l1=4190096).
// Ein UND-Kriterium erklaerte damit ausgerechnet die Maschine fuer PMC-los, auf der PMC beweisbar laeuft --
// und baute dort ab sofort ohne Messfuehler. Das ist kein konservativer Fehler, sondern ein Datenverlust.
//
// DIE ZWEITE HAELFTE DER ENTWURFS-BEGRUENDUNG ("sonst entstuende eine stille vierte Permutation") traegt
// ebenfalls nicht: die Permutation, die der Owner benennt, kommt aus dem BINARY OVERHEAD EINGEBAUTER
// MESSFUEHLER -- also aus der COMPILE-ZEIT-Aufnahme. Ob zur LAUFZEIT drei oder vier Zaehler Werte liefern,
// aendert am Kompilat kein Byte. Runtime-Verfuegbarkeit und Compile-Zeit-Einbau sind zwei Sachen; sie zu
// vermengen erzeugt die Phantom-Permutation, die man vermeiden wollte.
//
// GEBAUT IST DAHER: brauchbar :<=> (a) der cpuid-Vendor ist eine BEKANNTE Hardwareform (amd|intel) UND
// (b) MINDESTENS EIN Event der EINEN Liste (measurement::kPmcEvents) oeffnet UND beisst. Das spiegelt die
// Quelle exakt. Der volle Biss-Vektor steht im Befund und in der emittierten Zeile -- die Ehrlichkeit
// sitzt in der AUSGABE, nicht in einer strengeren Schwelle (V-1).
//
// KOEDER-PFLICHT (K13): "oeffnet" genuegt NICHT. Ein Zaehler kann oeffnen und im ganzen Fenster keine
// Hardware-Zeit bekommen (t_running==0; die Quelle kennt diesen Fall und verwirft dort den Wert). Deshalb
// laeuft zwischen ENABLE und READ ein cache-feindlicher Pointer-Chase ueber einen Puffer weit jenseits L1:
// liefert der Zaehler danach 0, hat der KOEDER NICHT GEBISSEN und das Event zaehlt als nicht brauchbar.
//
// FAIL-CLOSED, ausdruecklich: unbekannter Vendor => unbrauchbar, auch wenn Zaehler oeffnen. Nicht-Linux =>
// unbrauchbar (leere Event-Liste). Probe nicht gefahren => unbrauchbar. In KEINEM dieser Faelle wird PMC
// behauptet.
//
// WAS DIESE PROBE NICHT BEWEIST -- und wo das aufgefangen wird:
//   (a) Benutzbarkeit auf dem RUNNER-Host, der die emittierten Jobs spaeter faehrt. Der Planer laeuft
//       nicht zwingend dort. Aufgefangen: die emittierte Zeile sagt "beweist-nicht=runner-host", und der
//       CEB-Gegeneingang (builder/pmc_startup_pruefung.hpp) prueft die Lage am LAUF-Host hart nach.
//   (b) Bestand der Rechte-/paranoid-Lage bis zur Messzeit. Ebenfalls (a)-Gegeneingang.
//   (c) Nicht-Linux: strukturell nicht pruefbar (s.o.).
//
// LEHRBUCH-STRATEGY, statisch: die Syscall-Schicht ist ein Template-Policy-Parameter. Default ist die
// REALE Strategie; die Tests setzen eine Fake-Strategie ein und koennen damit alle Lagen erzeugen, auch
// die, die auf dieser Maschine gar nicht herstellbar sind (intel auf einem AMD-Host). Kein Runtime-Switch,
// keine vtable, kein std::variant.
//
// header-only, C++23. ASCII-only.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <cache_engine/measurement/pmc_event_set.hpp>       // DIE EINE Event-Liste
#include <cache_engine/measurement/pmc_vendor_registry.hpp> // die ZWEI Hardware-Komponenten
#include <cache_engine/platform_probe/cpuid_probe.hpp>      // probe_cpuid().vendor

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>
#include <vector>
#endif

namespace comdare::cache_engine::planner {

/// Die drei moeglichen Lagen. "Unbrauchbar" ist der Default -- wer nicht gemessen hat, behauptet nichts.
enum class PmcLage : std::uint8_t {
    Unbrauchbar, ///< keine benutzbare PMU ODER unbekannte Hardwareform => CEB wird OHNE PMC gebaut
    Amd,         ///< AuthenticAMD mit beissender PMU
    Intel,       ///< GenuineIntel mit beissender PMU
};

/// Der Befund EINES Probe-Laufs. Er traegt seinen eigenen NENNER mit (V-1): wer ihn liest, sieht nicht nur
/// das Ergebnis, sondern woraus es entstanden ist -- eine nackte 0 waere kein Befund.
struct PmcHostBefund {
    PmcLage     lage           = PmcLage::Unbrauchbar;
    bool        probe_gefahren = false; ///< false == niemand hat gefragt (NICHT dasselbe wie "kein PMC")
    std::string cpuid_vendor;           ///< der GEMESSENE Vendor-String, roh
    std::size_t events_geprueft = 0;    ///< NENNER: |kPmcEvents|
    std::size_t events_gebissen = 0;    ///< ZAEHLER: davon geoeffnet UND mit Wert > 0 nach dem Koeder
    std::string biss_vektor;            ///< je Event "<name>=<0|1>", ';'-getrennt -- die Aussage, nicht die Zahl
    std::string fehlgrund;              ///< bei Unbrauchbar IMMER nicht-leer (fail-loud)

    /// Die Vendor-id fuer Stempel und CMake-Argument. Bei Unbrauchbar LEER -- es gibt keinen Default-Vendor.
    [[nodiscard]] std::string_view vendor_id() const noexcept {
        switch (lage) {
            case PmcLage::Amd:
                return ::comdare::cache_engine::measurement::pmc_vendor_info(
                           ::comdare::cache_engine::measurement::PmcVendor::Amd)
                    .id;
            case PmcLage::Intel:
                return ::comdare::cache_engine::measurement::pmc_vendor_info(
                           ::comdare::cache_engine::measurement::PmcVendor::Intel)
                    .id;
            case PmcLage::Unbrauchbar: break;
        }
        return {};
    }

    /// Das Lage-Wort fuer Ausgabe und Test-Diagnose.
    [[nodiscard]] std::string_view lage_label() const noexcept {
        switch (lage) {
            case PmcLage::Amd: return "amd";
            case PmcLage::Intel: return "intel";
            case PmcLage::Unbrauchbar: break;
        }
        return "unbrauchbar";
    }

    /// nenner_zeile() -- der Befund als EINE Zeile fuer die emittierte Kind-Pipeline und fuers Planer-Log.
    /// Sie nennt IMMER beide Mengen (geprueft / gebissen) und immer die Grenze der Aussage. Auch der
    /// unbrauchbar-Fall bekommt eine Zeile: eine Emission, die zu PMC schweigt, ist von einer ungeprueften
    /// nicht zu unterscheiden (T-2: Aussage statt Anwesenheit).
    [[nodiscard]] std::string nenner_zeile() const {
        std::string s{"PMC-BEFUND lage="};
        s += lage_label();
        s += " vendor=";
        s += cpuid_vendor.empty() ? std::string{"unbekannt"} : cpuid_vendor;
        s += " events=";
        s += std::to_string(events_gebissen);
        s += "/";
        s += std::to_string(events_geprueft);
        s += " biss=[";
        s += biss_vektor;
        s += "] quelle=";
        s += probe_gefahren ? "perf_event_open+cpuid+koeder" : "nicht_erhoben";
        if (!fehlgrund.empty()) {
            s += " grund=";
            s += fehlgrund;
        }
        s += " beweist-nicht=runner-host";
        return s;
    }
};

namespace detail_pmc_probe {

/// Der KOEDER. Ein Pointer-Chase ueber einen Puffer weit jenseits jeder L1-Groesse, mit grosser Schrittweite,
/// damit L1D-Read-Misses, Last-Level-Misses UND DTLB-Misses real anfallen. Die Kette ist eine Permutation
/// (jeder Slot genau einmal), also nicht vom Prefetcher vorherzusehen, und der Rueckgabewert wird vom
/// Aufrufer verbraucht -- sonst optimierte der Compiler den ganzen Koeder weg und der Zaehler saehe nichts.
/// KEIN Zufall aus /dev/urandom: die Kette muss REPRODUZIERBAR beissen, sonst wackelt die Erkennung.
[[nodiscard]] inline std::uint64_t koeder_pointer_chase() noexcept {
#if defined(__linux__)
    // 64 MiB: groesser als jeder heutige L2 und groesser als der L3-Anteil eines Kerns -- der Chase faellt
    // damit bis in den Hauptspeicher durch. 4096 Byte Schrittweite trifft je Sprung eine neue Seite und
    // erzeugt so auch DTLB-Misses.
    constexpr std::size_t    kBytes  = 64u * 1024u * 1024u;
    constexpr std::size_t    kStride = 4096u;
    constexpr std::size_t    kSlots  = kBytes / kStride;
    std::vector<std::size_t> kette(kSlots, 0);
    // Ein einfacher, deterministischer Zyklus ueber alle Slots: Schrittweite teilerfremd zu kSlots.
    constexpr std::size_t kSchritt = 4099u; // Primzahl, teilerfremd zu kSlots (2^14)
    std::size_t           pos      = 0;
    for (std::size_t i = 0; i < kSlots; ++i) {
        std::size_t const next = (pos + kSchritt) % kSlots;
        kette[pos]             = next;
        pos                    = next;
    }
    std::uint64_t summe = 0;
    std::size_t   p     = 0;
    for (std::size_t i = 0; i < kSlots * 4; ++i) {
        p = kette[p];
        summe += p;
    }
    return summe;
#else
    return 0;
#endif
}

} // namespace detail_pmc_probe

/// REALE Strategie: perf_event_open + cpuid + Koeder. Statisch eingesetzt (kein virtual).
struct RealePmcHostStrategie {
    /// Der cpuid-Vendor-String dieses Hosts ("AuthenticAMD" / "GenuineIntel" / ... / "" auf non-x86).
    [[nodiscard]] static std::string cpuid_vendor() {
        return ::comdare::cache_engine::platform_probe::probe_cpuid().vendor;
    }

    /// Oeffnet EIN Event self-monitoring, laesst den Koeder laufen, liest zurueck. true nur, wenn der
    /// Zaehler geoeffnet hat UND danach einen Wert > 0 traegt (K13: der Koeder MUSS beissen).
    [[nodiscard]] static bool event_beisst(::comdare::cache_engine::measurement::PmcEventSpec const& ev) noexcept {
#if defined(__linux__)
        struct ::perf_event_attr attr;
        std::memset(&attr, 0, sizeof(attr)); // Muellbits in Reserve-Feldern => EINVAL; immer memset
        attr.size           = sizeof(attr);
        attr.type           = ev.type;
        attr.config         = ev.config;
        attr.disabled       = 1;
        attr.exclude_kernel = 1;
        attr.exclude_hv     = 1;
        long const fd       = ::syscall(__NR_perf_event_open, &attr, /*pid=*/0, /*cpu=*/-1, /*group_fd=*/-1,
                                        /*flags=*/0UL);
        if (fd < 0) return false;
        int const f = static_cast<int>(fd);
        (void)::ioctl(f, PERF_EVENT_IOC_RESET, 0);
        (void)::ioctl(f, PERF_EVENT_IOC_ENABLE, 0);
        std::uint64_t const koeder = detail_pmc_probe::koeder_pointer_chase();
        (void)::ioctl(f, PERF_EVENT_IOC_DISABLE, 0);
        std::uint64_t wert = 0;
        bool const    ok   = ::read(f, &wert, sizeof(wert)) == static_cast<::ssize_t>(sizeof(wert));
        (void)::close(f);
        // koeder wird verbraucht, damit der Pointer-Chase nicht wegoptimiert wird (er ist der ganze Punkt).
        return ok && wert > 0 && koeder != 0xFFFFFFFFFFFFFFFFULL;
#else
        (void)ev;
        return false;
#endif
    }
};

/// probe_pmc_host<Strategie>() -- DIE Erkennung. Ein Aufruf je Planer-Lauf.
///
/// Reihenfolge ist Absicht: erst der Vendor (billig, und ein unbekannter Vendor macht jede Zaehlerei
/// gegenstandslos), dann die Events. Der Biss-Vektor wird IMMER vollstaendig erhoben, auch wenn schon das
/// erste Event beisst -- sonst stuende in der Ausgabe eine Teilaussage, und der naechste Leser haelt sie
/// fuer die ganze.
template <class Strategie = RealePmcHostStrategie>
[[nodiscard]] inline PmcHostBefund probe_pmc_host() {
    namespace cme = ::comdare::cache_engine::measurement;

    PmcHostBefund b;
    b.probe_gefahren  = true;
    b.events_geprueft = cme::kPmcEventCount;
    b.cpuid_vendor    = Strategie::cpuid_vendor();

    // FAIL-CLOSED (1): keine Event-Liste => nichts pruefbar (Nicht-Linux).
    if (cme::kPmcEventCount == 0) {
        b.lage      = PmcLage::Unbrauchbar;
        b.fehlgrund = "keine_event_liste_auf_dieser_plattform";
        return b;
    }

    // Der Biss-Vektor -- immer vollstaendig, immer benannt.
    for (std::size_t i = 0; i < cme::kPmcEventCount; ++i) {
        bool const beisst = Strategie::event_beisst(cme::kPmcEvents[i]);
        if (beisst) ++b.events_gebissen;
        if (i != 0) b.biss_vektor += ';';
        b.biss_vektor += std::string{cme::kPmcEvents[i].name};
        b.biss_vektor += beisst ? "=1" : "=0";
    }

    // FAIL-CLOSED (2): unbekannter Vendor => unbrauchbar, AUCH wenn Zaehler beissen. Eine Hardwareform, die
    // die Registry nicht kennt, kann im CEB-Stempel nicht benannt werden -- und ein unbenannter Stempel
    // waere genau die stille Ununterscheidbarkeit, gegen die diese Achse gebaut ist.
    cme::PmcVendorInfo const* const vi = cme::pmc_vendor_from_cpuid(b.cpuid_vendor);
    if (vi == nullptr) {
        b.lage      = PmcLage::Unbrauchbar;
        b.fehlgrund = "unbekannte_hardwareform";
        return b;
    }

    // FAIL-CLOSED (3): kein einziger Biss => unbrauchbar. Genau hier faellt der Fall "oeffnet, zaehlt aber
    // nie" (t_running==0, Rechte, Container ohne perf) auf die ehrliche Seite.
    if (b.events_gebissen == 0) {
        b.lage      = PmcLage::Unbrauchbar;
        b.fehlgrund = "koeder_hat_nicht_gebissen";
        return b;
    }

    b.lage = vi->vendor == cme::PmcVendor::Amd ? PmcLage::Amd : PmcLage::Intel;
    return b;
}

} // namespace comdare::cache_engine::planner
