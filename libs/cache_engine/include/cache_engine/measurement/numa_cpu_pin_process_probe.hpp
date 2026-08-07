// measurement/numa_cpu_pin_process_probe.hpp -- prozess-freie RT-Erhebung der target_isa-Unter-Achse
// core_class und der Zuordnungs-Faehigkeit dieser Maschine (Paket OD-11-RT).
//
// WOHER DAS PAKET KOMMT (Owner-KERN 06.08.2026, verbatim -- WORTLAUT UNVERAENDERT, auch der darin
// genannte alte Name): "numa page ist eine Cache-Seiten Koordination von Cache-Seiten lokalitaet.
// Jetzt brauchen wir ein pendant numa_process_probe dazu, welche sich damit beschaeftigt, wo
// Programme ausgefuehrt werden, nicht welche Speicherseiten wo liegen, sie sind aber beide
// strukturell aehnliche Unterachsen. Das ist also eine fehlende neue Unterachse, sie existiert nur
// im Plan, nicht gebaut." Dieser Header ist ihr Bau. Er spiegelt
// numa_page_probe.hpp Abschnitt fuer Abschnitt; wo er abweicht, steht der Grund darunter benannt.
//
// NAME (Owner-KERN 06.08.2026, NACH dem Bau, verbatim): "Ich moechte numa_process_probe besser
// numa_cpu_pin_process_probe nennen". Der neue Name benennt DAS PINNING als Gegenstand, nicht bloss
// den Prozess -- das ist die Sache selbst, nicht Kosmetik. Er gilt fuer die Probe: Dateien, Typen,
// Bezeichner und den probe_id-Praefix. Die XML-Achsen-Id der Unter-Achse bleibt davon UNBERUEHRT
// core_class (aus dem Plan, s. target_isa_sub_axes.hpp) -- die Probe ist das ERHEBUNGS-Verfahren,
// die Unter-Achse ist der erhobene Gegenstand; der Owner hat das Verfahren umbenannt, nicht die Achse.
//
// WAS ERHOBEN WIRD (zwei GETRENNTE Dinge, siehe "EINE ACHSE, ZWEI ERGEBNISSE" unten):
//   core_class -- die KERN-KLASSEN dieser Maschine samt der logischen Prozessoren je Klasse. linux:
//                 die Hybrid-PMU-Paare <pmu_root>/cpu_core/cpus + cpu_atom/cpus (die einzige Quelle,
//                 die Klassen BENENNT), sonst die L3-Domaenen aus <cpu_root>/cpuN/cache/index3
//                 (Groesse + shared_cpu_list), sonst ehrlich uniform; windows:
//                 GetLogicalProcessorInformationEx(RelationProcessorCore) mit EfficiencyClass; macos:
//                 KORREKTUR Review-Befund L-2 (2026-08-07, wf_f94d1373-f7b): diese Zeile behauptete
//                 vorher "keine prozessfreie Kern-Klassen-Schnittstelle" -- das ist FALSCH und
//                 widersprach dem eigenen macOS-Blatt. macOS HAT eine prozessfreie Quelle fuer die
//                 Klassen-GROESSEN: hw.nperflevels + hw.perflevel<N>.logicalcpu (sysctlbyname). Die
//                 benannte Luecke ist enger als die Zeile vorher sagte: Darwin liefert KEINE Zuordnung
//                 Kern-Id -> Ebene (die Ids werden aus den Anzahlen ABGELEITET, nicht gelesen), und die
//                 Zuordnungs-Erprobung meldet fuer diese Familie ehrlich KeineSchnittstelle (kein
//                 sched_setaffinity-Aequivalent). Vollstaendige Begruendung im macOS-Blatt-Kopf.
//   Zuordnungs-Faehigkeit (Pinning) -- KEINE Achse, sondern die BRAUCHBARKEITS-Aussage ueber die
//                 Achsen-Werte: darf dieser Thread ueberhaupt einem einzelnen Kern zugeordnet werden,
//                 und setzt der Kernel die Zuordnung wirklich durch. Ohne sie waeren die Kern-Klassen
//                 eine Karte ohne Weg.
//
// EINE ACHSE, ZWEI ERGEBNISSE -- BENANNT STATT VERSCHWIEGEN: numa_page_probe loest ZWEI Unter-Achsen
// auf und traegt deshalb zwei Achsen-Typen (numa_axis/page_axis). Diese Probe loest GENAU EINE auf
// (core_class), liefert aber trotzdem ZWEI unabhaengige std::expected -- weil die Pinning-Faehigkeit
// eine Aussage ueber die VERWENDBARKEIT der Achsen-Werte ist und keine Werte-Menge, die der Planer
// permutieren koennte. Sie deshalb in die Achse zu ziehen hiesse, eine Qualifikation als Auspraegung
// auszugeben; sie wegzulassen hiesse, eine Kern-Klassen-Karte auszuliefern, ohne zu sagen, ob man ihr
// folgen kann. Die Registry bekommt aus diesem Paket genau EINE neue Zeile.
//
// K2 PROZESS-FREI: ausschliesslich Datei-Reads unter sysfs und Familien-Schnittstellen im eigenen
// Prozess (sched_getaffinity/sched_setaffinity, GetLogicalProcessorInformationEx). Externe Programme,
// Shells und Prozess-Erzeugung sind kein Teil dieser API -- kein taskset, kein numactl, kein lscpu,
// kein hwloc. Der Slurm-Praefix-Weg (builder/experiment_tree/slurm_launcher.hpp) emittiert Skript-TEXT
// fuer den Cluster und ist ausdruecklich NICHT dieser Weg.
//
// DIE EINE MUTIERENDE STELLE, ausdruecklich und begrenzt: die Pinning-Faehigkeit laesst sich NICHT aus
// einer Datei lesen. "Der Kernel reicht die Zuordnung durch" ist nur durch HANDLUNG + RUECKPROBE
// feststellbar: sched_setaffinity kann 0 melden und die Maske trotzdem nicht auf genau einen Kern
// stehen. Die Familien-Erprobung (numa_cpu_pin_process_probe_pinning_<familie>) fuehrt deshalb genau vier
// Syscalls -- Vormaske lesen, Ziel setzen,
// zuruecklesen, Vormaske wiederherstellen -- auf dem AUFRUFENDEN Thread und auf keinem anderen. Sie ist
// idempotent, erzeugt keinen Prozess und keinen Thread, und ob die Wiederherstellung gelang, steht als
// eigenes Feld im Ergebnis statt als stille Annahme. WARUM SIE NICHT ScopedThreadPin BENUTZT
// (builder/measurement/thread_pinning.hpp): der Aktuator lebt im builder-Baum und ist der MESS-seitige
// Einsatz; dieser Header liegt in include/cache_engine/measurement und darf nicht rueckwaerts auf den
// builder zeigen. Er ersetzt den Aktuator auch nicht -- er stellt nur fest, ob es ihn zu benutzen lohnt.
//
// K4 FEHLER-EINORDNUNG (#29, exakt die OS-U3-/OD-10-RT-Aufteilung): HardwareProbeErrorClass heisst
// historisch "Hardware", bezeichnet im Framework aber den ZUGANG zu einer Quelle -- Datei fehlt /
// unlesbar / korrupt / unbekanntes Format reisen deshalb auch hier in dieser bestehenden Domaene. Nur
// eine auf dieser Plattform fehlende FAMILIEN-SCHNITTSTELLE oder ein gescheiterter Familien-Syscall
// erzeugt CompilerCompilerErrorClass::BetriebssystemFeatureFehlt. Es entsteht bewusst KEIN neues Enum
// (RF-3-Kollisionslehre -- eine neue Klasse in einem fremden Enum zieht dessen Count-Wachen und bricht
// sie still).
//
// DER WARN IST KEINE FEHLERKLASSE: der Owner-KERN verlangt, dass die Werte bei fehlender Zuordnungs-
// Faehigkeit TROTZDEM ausgegeben werden, mit "warn: no pinned locality on hybrid architecture" -- WARN,
// nicht FATAL. Ein Warn-Wert im K4-Enum waere genau der stille Count-Bruch, den die Doktrin verbietet.
// Der Warn ist deshalb ein PRAEDIKAT ueber den beiden Ergebnissen (warns_no_pinned_locality) plus der
// vom Owner woertlich vorgegebene Text (kNoPinnedLocalityWarning). Er wird hier NICHT ausgegeben --
// Ausgabe ist Sache des Konsumenten (siehe Paket-Grenze).
//
// 'N/A STATT NULL' (Owner-Doktrin, feedback_measurement_failure_visibility_csv_failed_not_null_plus_log):
// KEIN Ergebnis dieses Headers ist je eine 0 oder eine leere Liste, die "nichts gefunden" und "nicht
// erhoben" verschmelzen wuerde. Die Achse traegt ENTWEDER ihre Werte-Menge ODER eine benannte
// Fehlerklasse; eine leere Klassen-Liste und eine leere Kern-Liste sind strukturell unerreichbar (die
// Parser weisen sie als QuelleKorrupt zurueck). "Uniform" ist eine ECHTE Antwort und keine Leere:
// homogene Maschinen (prod1/Zen5) haben genau eine Kern-Klasse, und das ist ein Befund.
//
// NICHT GERATEN, SONDERN BENANNT (die zwei bewussten Auslassungen der Detektions-Kette):
//   acpi_cppc/highest_perf ist ein RANG je Kern, keine KLASSE. Auf prod1 traegt EINE L3-Domaene acht
//       verschiedene Werte (186/176/191/181/196/201/166/171) -- eine Klassen-Ableitung daraus braeuchte
//       einen Schwellwert, und ein Schwellwert ist geraten. Der Live-Test belegt das an der Maschine.
//   cpu_capacity ist aus demselben Grund draussen: eine Zahl je Kern, deren Klassen-Grenze niemand
//       liefert. Beide bleiben als moegliche QUELLEN benannt, nicht als stille Auslassung.
// Und drittens: mehr als ZWEI verschiedene L3-Domaenen-Groessen ergeben FormatUnbekannt statt einer
// erzwungenen Zweiteilung -- "NIE interpretieren, nie raten" gilt fuer die Topologie wie fuer das Format.
//
// K5 VERSIONIERTE VERFAHRENS-ID: probe_id() ist eine Eigenschaft des Probe-TYPS, kein frei setzbares
// Feld des Runtime-Ergebnisses. Aendert sich eine Quelle oder die Zusammensetzung der Werte, MUSS die
// Version steigen, damit Messungen aus verschiedenen Erhebungs-Verfahren unterscheidbar bleiben. Der
// Familien-Teil wird aus OsAxis::os_family_id() in einen constexpr-Puffer komponiert und nirgendwo als
// zweites Literal gepflegt (Muster kOsProbeVersion/kNumaPageProbeVersion). kNumaCpuPinProcessProbeVersion
// startet dreistellig mit CPU-Flag als "v1.0.0c"; COMDARE_VERSION_HW_FLAG_ENFORCE ist SCHARF, ein
// flagloses Literal braeche hier sofort. Die zentrale Naht-Liste in algo_semver.hpp fuehrt diese Quelle
// als Auspraegung derselben Klasse wie (f) OS-U3 und (g) OD-10-RT.
//
// ANSCHLUSS AN machine_resolved (das eigentliche Ziel des Pakets): der Probe-Typ fuehrt die Unter-Achse
// als TYP (core_axis) und verwacht compile-time, dass sie wirklich "machine_resolved" deklariert und am
// target_isa-Komplex haengt. Damit ist diese Probe nicht IRGENDEIN Erheber, sondern der von der
// statischen Einhaengung BENANNTE Aufloeser -- und ein Umschreiben der option_source auf einen
// CT-Katalog bricht hier, statt die Probe stillschweigend gegenstandslos zu machen.
//
// PAKET-GRENZE, DEKLARIERT (Muster numa_page_probe.hpp:66-69): dieses Paket ist das ANGEBOT, nicht sein
// Konsument. Die erhobenen Werte in die Planer-/CEB-Aufloesung, in die Permutation und in die
// Mess-Ausgabe zu tragen -- die CEB, die DIESELBE Tier-Binary einmal auf einen E-Core und einmal auf
// einen P-Core gepinnt startet, die getrennte Auswertung je Klasse, und die Ausgabe des WARN-Textes in
// Log und CSV -- ist das Folge-Paket OD-11-RT-K, exakt so wie OD-10-RT-K fuer die Seiten-Seite. Ebenso
// NICHT Teil dieses Pakets: das Pinnen der Mess-Naht selbst (der Aktuator ScopedThreadPin und seine
// Verdrahtung in perm_runner/experiment_driver) und die Hybrid-PMU-Wahl (cpu_core/cpu_atom-PMC). Das
// ist eine deklarierte Paket-Grenze, kein vergessener Anschluss.
//
// A-15 STEMPEL-NEUTRALITAET: CoreTopology und PinningCapability sind reine Runtime-Blaetter. Dieser
// Header kennt weder ABI-Stempel noch Registry, XML, Generator oder Binary-ID und bietet keine Naht
// dorthin. Kein erhobener Wert und auch keine probe_id wird in den Binary-Stempel geschrieben; die
// System-Achsen-Code-Version "target_isa" bleibt davon unberuehrt. Die Unter-Achse haengt an
// target_isa, und target_isa traegt binary_id="never" -- die Tier-Binary-Identitaet kann von hier aus
// strukturell nicht angefasst werden. Genau das ist die Bedingung des Owner-KERNs "reine
// Wiederverwendung, kein zweiter Bau": es entsteht KEIN Flotten-Neubau.
//
// Metaprog: CT-Familien-Dispatch ueber Template-Spezialisierung, kein Runtime-enum, kein switch ueber
// Familien, keine vtable. Der einzige std::variant ist die Fehler-Summe an der Expected/Result-Naht.

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/target_isa_sub_axes.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. DAS VOKABULAR DER AUSFUEHRUNGS-LOKALITAET
// =================================================================================================

/// WELCHE Kern-Klasse. Die fuenf Werte sind bewusst unterscheidbar, weil sie fuenf verschiedene
/// Aussagen ueber die Maschine sind und eine gemeinsame Vokabel sie unwiederbringlich verschmelzen
/// wuerde (dieselbe W-4-Lehre wie PageSizeAvailability in numa_page_probe.hpp):
///   Uniform        = die Maschine fuehrt GENAU EINE Kern-Klasse. Das ist eine ECHTE Antwort (prod1 ist
///                    so), nicht "nichts gefunden" -- siehe den 'n/a statt Null'-Block im Kopf,
///   HoheLeistung   = die leistungsstarke Klasse einer HYBRIDEN CPU (Intel P-Core / cpu_core-PMU),
///   HoheEffizienz  = die effiziente Klasse derselben CPU (Intel E-Core / cpu_atom-PMU),
///   GrosserCache   = die L3-groessere Domaene einer CACHE-asymmetrischen CPU (AMD V-Cache-CCD),
///   KleinerCache   = die L3-kleinere Domaene derselben CPU.
/// WARUM VIER STATT ZWEI: "P/E" und "V-Cache/Frequenz-CCD" sind NICHT dasselbe. Bei Intel-Hybrid
/// unterscheiden sich die Kerne in der Mikroarchitektur, bei AMD-X3D in der L3-Groesse bei gleicher
/// Mikroarchitektur. Beides als "HighIpc/LowIpc" zu fuehren waere eine Behauptung ueber die IPC, die
/// die Quelle nicht hergibt -- die L3-Quelle sagt etwas ueber CACHE, nicht ueber IPC.
/// ABGRENZUNG: das ist NICHT platform::CoreClass (platform/core_layout.hpp:14-19). Jenes ist das
/// MODELL-seitige Vokabular der Pinning-Policy (vtable, subject-under-study); dieses ist die
/// System-seitige Werte-Menge der Achse -- dieselbe Rollenteilung wie page gegen AllocPageHint.
enum class CoreClassKind : std::uint8_t {
    Uniform       = 0,
    HoheLeistung  = 1,
    HoheEffizienz = 2,
    GrosserCache  = 3,
    KleinerCache  = 4,
};
/// Single-Source der Klassen-Zahl (beide Drift-Wachen unten).
inline constexpr std::size_t kCoreClassKindCount = 5;

/// Log-/CSV-Etikett je Kern-Klasse. Der Fallback heisst BEWUSST nicht "unbekannt": dieses Wort ist in
/// axis_error.hpp bereits mehrfach vergeben, und die Disjunktheits-Wache unten soll eine echte Aussage
/// machen statt an einem geteilten Sammelbegriff zu scheitern.
[[nodiscard]] constexpr std::string_view core_class_kind_token(CoreClassKind k) noexcept {
    switch (k) {
        case CoreClassKind::Uniform: return "kern_uniform";
        case CoreClassKind::HoheLeistung: return "kern_hohe_leistung";
        case CoreClassKind::HoheEffizienz: return "kern_hohe_effizienz";
        case CoreClassKind::GrosserCache: return "kern_grosser_cache";
        case CoreClassKind::KleinerCache: return "kern_kleiner_cache";
    }
    return "kernklasse_unbekannt";
}

/// WORAUS die Klassifikation stammt. Sie reist MIT dem Ergebnis, weil zwei Maschinen mit derselben
/// Klassen-Zahl trotzdem verschieden klassifiziert sein koennen und eine Messreihe, die das verschweigt,
/// zwei unvergleichbare Erhebungen als eine ausweist.
///   HybridPmu = <pmu_root>/cpu_core/cpus + cpu_atom/cpus (LINUX). Die EINZIGE Quelle, die Kern-Klassen
///               selbst BENENNT (zwei getrennte PMUs, Intel Alder Lake und aufwaerts),
///   L3Domaene = genau ZWEI verschiedene L3-Groessen ueber <cpu_root>/cpuN/cache/index3 (LINUX). Sie
///               benennt DOMAENEN, nicht Mikroarchitekturen -- deshalb GrosserCache/KleinerCache und
///               nicht P/E,
///   Homogen   = keine Unterscheidung erhebbar; genau eine Klasse, und das ist der Befund. TRAEGT KEINE
///               plattform-spezifische Quelle -- "keine Unterscheidung" ist auf jeder Familie dieselbe
///               Aussage, deshalb geteilt.
///   WindowsEfficiencyClass = GetLogicalProcessorInformationEx(RelationProcessorCore).EfficiencyClass
///               (WINDOWS). Review-Befund L-1 (2026-08-07, wf_f94d1373-f7b): dieser Wert wurde vorher
///               FAELSCHLICH als HybridPmu gefuehrt, obwohl Windows weder cpu_core/cpu_atom noch
///               ueberhaupt sysfs kennt -- eine Provenienz-Vokabel, die zwei verschiedene Erhebungs-
///               Mechanismen unter demselben Wert verschmolz, ist dieselbe Fehlerklasse wie eine Spalte,
///               die etwas anderes behauptet als sie ist,
///   DarwinPerflevel = hw.nperflevels + hw.perflevel<N>.logicalcpu (MACOS). Dieselbe L-1-Korrektur: auch
///               macOS fuehrte HybridPmu, obwohl es ueber sysctlbyname und nicht ueber eine PMU-sysfs-
///               Datei erhebt.
/// WARUM ANGEHAENGT STATT UMSORTIERT: HybridPmu/L3Domaene/Homogen reisen als Zahlen 0/1/2 in Log und
/// Messergebnis (K5-Doktrin, wie bei axis_error.hpp CompilerCompilerErrorClass) -- ein Umsortieren
/// verschoebe eine Bestands-Nummer. Die beiden neuen Werte werden deshalb ANGEHAENGT (3/4), nicht
/// eingefuegt.
enum class CoreTopologySource : std::uint8_t {
    HybridPmu              = 0,
    L3Domaene              = 1,
    Homogen                = 2,
    WindowsEfficiencyClass = 3,
    DarwinPerflevel        = 4,
};
inline constexpr std::size_t kCoreTopologySourceCount = 5;

[[nodiscard]] constexpr std::string_view core_topology_source_token(CoreTopologySource s) noexcept {
    switch (s) {
        case CoreTopologySource::HybridPmu: return "quelle_hybrid_pmu";
        case CoreTopologySource::L3Domaene: return "quelle_l3_domaene";
        case CoreTopologySource::Homogen: return "quelle_homogen";
        case CoreTopologySource::WindowsEfficiencyClass: return "quelle_windows_efficiency_class";
        case CoreTopologySource::DarwinPerflevel: return "quelle_darwin_perflevel";
    }
    return "kernquelle_unbekannt";
}

/// WIE weit die Zuordnung eines Threads an EINEN Kern traegt. Die vier Werte sind die zwei
/// Owner-Bedingungen, getrennt gehalten: (a) "die CPU kann" ist die Kern-Klassen-Erhebung oben,
/// (b) "der Kernel reicht es durch" ist genau diese Stufe.
///   Durchgesetzt       = gesetzt UND per Rueckprobe als GENAU der Zielkern bestaetigt,
///   NichtDurchgesetzt  = der Aufruf meldete Erfolg, die Rueckprobe zeigt eine ANDERE Menge. Das ist
///                        der gefaehrlichste Fall (cpuset-Migration, Virtualisierung) und er ist
///                        deshalb ein EIGENER Wert und nicht "gescheitert",
///   VomKernAbgelehnt   = der Kernel hat die Zuordnung abgelehnt (Zielkern ausserhalb der effektiven
///                        Maske -- cgroup/cpuset --, fehlende Rechte, oder Kern offline),
///   KeineSchnittstelle = diese Familie bietet keine prozessfreie Thread-Zuordnung an.
/// Der Unterschied traegt: "gepinnt" und "gepinnt geblieben" sind verschiedene Zusicherungen, und eine
/// Messreihe, die beides als "gepinnt" ausweist, behauptet eine Lokalitaet, die es nicht gibt.
enum class PinningAvailability : std::uint8_t {
    Durchgesetzt       = 0,
    NichtDurchgesetzt  = 1,
    VomKernAbgelehnt   = 2,
    KeineSchnittstelle = 3,
};
inline constexpr std::size_t kPinningAvailabilityCount = 4;

[[nodiscard]] constexpr std::string_view pinning_availability_token(PinningAvailability a) noexcept {
    switch (a) {
        case PinningAvailability::Durchgesetzt: return "pin_durchgesetzt";
        case PinningAvailability::NichtDurchgesetzt: return "pin_nicht_durchgesetzt";
        case PinningAvailability::VomKernAbgelehnt: return "pin_vom_kern_abgelehnt";
        case PinningAvailability::KeineSchnittstelle: return "pin_keine_schnittstelle";
    }
    return "pinfaehigkeit_unbekannt";
}

/// EINE Kern-Klasse mit ihren logischen Prozessoren. cpu_ids ist immer nichtleer, aufsteigend und
/// duplikatfrei -- eine leere Gruppe ist strukturell unerreichbar, weil die Parser sie als QuelleKorrupt
/// zurueckweisen ('n/a statt Null').
struct CoreClassGroup {
    CoreClassKind              kind = CoreClassKind::Uniform;
    std::vector<std::uint32_t> cpu_ids;
};

[[nodiscard]] inline bool operator==(CoreClassGroup const& a, CoreClassGroup const& b) noexcept {
    return a.kind == b.kind && a.cpu_ids == b.cpu_ids;
}

/// Die WERTE-MENGE der Unter-Achse core_class: die Klassen dieser Maschine samt Zuordnung, plus die
/// Quelle, aus der die Klassifikation stammt. Immer mindestens EINE Gruppe.
struct CoreClassMap {
    CoreTopologySource          source = CoreTopologySource::Homogen;
    std::vector<CoreClassGroup> groups;
};

/// Das Ergebnis der Zuordnungs-Erprobung. erlaubte_kerne ist die EFFEKTIVE Maske des aufrufenden
/// Threads (cpuset/cgroup bereits eingerechnet) -- sie ist die ehrliche Obergrenze dessen, was eine
/// Mess-Permutation ueberhaupt ansteuern darf. maske_wiederhergestellt haelt fest, dass die Erprobung
/// den Aufrufer nicht veraendert zurueckgelassen hat; ein false ist ein BEFUND und keine Nebensache.
struct PinningCapability {
    PinningAvailability availability            = PinningAvailability::KeineSchnittstelle;
    std::uint32_t       erlaubte_kerne          = 0;
    std::uint32_t       gepruefter_kern         = 0;
    bool                maske_wiederhergestellt = false;
};

// =================================================================================================
// 2. DIE ERGEBNIS-TRAEGER
// =================================================================================================

/// K4: exakt die Form von NumaPageProbeError/OsProbeError -- eine typisierte Summe an der
/// Expected/Result-Naht. Die erste Alternative klassifiziert den Quellen-ZUGANG, die zweite
/// ausschliesslich die fehlende OS-Faehigkeit. Diese ausdrueckliche variant-Ausnahme ist
/// Fehler-TRANSPORT, niemals Familien-Dispatch.
/// Die Summe selbst steht EINMAL in axis_error.hpp (ProbeErrorSum) -- dieses Paket hat den Befund
/// ausgeloest, der sie dorthin gehoben hat (Begruendung dort): drei Proben mit je einem type alias auf
/// DENSELBEN variant und je einer eigenen error_domain-Ueberladung sind drei Definitionen derselben
/// Funktion. Der sprechende Alias-Name bleibt hier, weil er sagt, WELCHE Erhebung den Fehler traegt.
using NumaCpuPinProcessProbeError = ProbeErrorSum;

/// Das bestehende stabile Etikett der jeweils getragenen #29-Domaene, ohne ein drittes Vokabular zu
/// erfinden -- wortgleich zur numa/page-Naht.
[[nodiscard]] constexpr std::string_view
numa_cpu_pin_process_probe_error_label(NumaCpuPinProcessProbeError const& error) noexcept {
    return std::visit(
        [](auto const value) noexcept -> std::string_view {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, HardwareProbeErrorClass>)
                return hardware_probe_label(value);
            else
                return error_class_label(value);
        },
        error);
}

using CoreClassMapResult      = std::expected<CoreClassMap, NumaCpuPinProcessProbeError>;
using PinningCapabilityResult = std::expected<PinningCapability, NumaCpuPinProcessProbeError>;

/// Das Ergebnis EINER Erhebung: ENTWEDER die Werte-Menge ODER eine benannte Fehlerklasse, je Feld
/// unabhaengig. Es gibt keinen dritten Zustand und keinen Ersatzwert.
///
/// KEIN AEUSSERES std::expected -- aus demselben Grund wie bei NumaPageTopology und wieder benannt
/// statt verschwiegen: die beiden Felder haben GETRENNTE Quellen (sysfs-Baum vs Scheduler-Syscall). Ein
/// aeusseres expected liesse eine vollstaendig erhobene Kern-Klassen-Karte hinter einer verweigerten
/// Zuordnung verschwinden -- und genau die Werte sollen laut Owner-KERN "trotzdem ausgegeben" werden.
struct ProcessLocalityTopology {
    CoreClassMapResult      core_classes;
    PinningCapabilityResult pinning;
};

/// DER OWNER-WARN, woertlich wie vorgegeben. Er steht als KONSTANTE hier und wird von diesem Paket
/// NICHT ausgegeben (Paket-Grenze) -- damit der Konsument ihn nicht ein zweites Mal formuliert.
inline constexpr std::string_view kNoPinnedLocalityWarning = "warn: no pinned locality on hybrid architecture";

/// Das WARN-PRAEDIKAT (kein Fehler, keine Fehlerklasse -- siehe Kopf-Block): die Maschine fuehrt mehr
/// als eine Kern-Klasse, aber die Zuordnung ist nicht durchgesetzt. Genau dann ist die Klassen-Karte
/// zwar ehrlich, ihre Verwendbarkeit fuer eine getrennte P/E-Messung aber fraglich.
///
/// Eine NICHT erhobene Seite loest den Warn NICHT aus: wo schon die Klassen fehlen, ist "keine gepinnte
/// Lokalitaet auf hybrider Architektur" keine wahre Aussage, sondern eine ueber eine unbekannte
/// Maschine. Die fehlende Erhebung traegt bereits ihre eigene, benannte Fehlerklasse.
[[nodiscard]] inline bool warns_no_pinned_locality(ProcessLocalityTopology const& topology) noexcept {
    if (!topology.core_classes.has_value() || !topology.pinning.has_value()) return false;
    return topology.core_classes->groups.size() > 1U &&
           topology.pinning->availability != PinningAvailability::Durchgesetzt;
}

// =================================================================================================
// 3. DIE OS-HANDLES ALS INJIZIERBARER KONTEXT
// =================================================================================================

/// Die realen Quellen-Wurzeln. Sie stehen HIER als Single-Source und werden von der ISA-x-OS-Zelle
/// (hardware_probe_factory.hpp) als Zell-Handles weitergereicht, nie ein zweites Mal hingeschrieben.
/// ZWEI Wurzeln, weil die beiden Quellen in verschiedenen sysfs-Baeumen liegen: die Hybrid-PMUs haengen
/// unter /sys/devices (neben "cpu"), die Kern-Verzeichnisse unter /sys/devices/system/cpu.
inline constexpr std::string_view kDefaultCpuRoot    = "/sys/devices/system/cpu";
inline constexpr std::string_view kDefaultCpuPmuRoot = "/sys/devices";

/// Der Blattname der ONLINE-Liste unter <cpu_root>. Bewusst "online" und NICHT "possible": possible ist
/// die zur Bauzeit des Kernels reservierte OBERGRENZE (regelmaessig groesser als die Realitaet), online
/// sind die tatsaechlich vorhandenen Prozessoren. Die Achse traegt die Werte der MASCHINE.
inline constexpr std::string_view kCpuOnlineLeafName = "online";
/// Die beiden HYBRID-PMU-Verzeichnisse unter <pmu_root>. Kernel-ABI seit Alder Lake: cpu_core fuehrt die
/// P-Kerne, cpu_atom die E-Kerne, je in ihrer "cpus"-Datei in der Kernel-Listen-Grammatik.
inline constexpr std::string_view kPerformanceCorePmuDirName = "cpu_core";
inline constexpr std::string_view kEfficiencyCorePmuDirName  = "cpu_atom";
inline constexpr std::string_view kPmuCpuListLeafName        = "cpus";
/// Der Namens-Kontrakt der Kern-Verzeichnisse unter <cpu_root>: "cpu<N>".
inline constexpr std::string_view kCpuDirPrefix = "cpu";
/// Der L3-Zugang je Kern: <cpu_root>/cpu<N>/cache/index3/{size,shared_cpu_list}.
inline constexpr std::string_view kCacheDirName           = "cache";
inline constexpr std::string_view kL3CacheIndexDirName    = "index3";
inline constexpr std::string_view kCacheSizeLeafName      = "size";
inline constexpr std::string_view kCacheSharedCpuListLeaf = "shared_cpu_list";
/// Der Groessen-Suffix der sysfs-Cache-Groesse ("98304K"). Der Kernel schreibt hier immer Kibibyte.
inline constexpr std::string_view kCacheSizeKibSuffix = "K";

/// Injizierbare, BESITZENDE OS-Handles. std::filesystem::path statt string_view verhindert (wie bei
/// NumaPageProbeContext), dass ein Test einen temporaeren Pfad-String eintraegt, dessen Speicher vor
/// collect() bereits abgelaufen ist. windows/macos brauchen fuer ihre Familien-Schnittstellen keinen
/// Pfad, teilen aber denselben Kontext-Typ -- ein zweiter Kontext waere eine zweite Wissensquelle.
///
/// erprobe_pinning ist BEWUSST Teil des Kontexts und BEWUSST per Default an: die Erprobung ist die
/// einzige Antwort auf Owner-Bedingung (b), aber sie ist die eine mutierende Stelle des Headers. Ein
/// Aufrufer, der in einem bereits gepinnten Mess-Fenster steht, muss sie abschalten koennen -- und wenn
/// er das tut, meldet das Ergebnis KeineSchnittstelle statt eines erfundenen Erfolgs.
struct NumaCpuPinProcessProbeContext {
    std::filesystem::path cpu_root;
    std::filesystem::path cpu_pmu_root;
    bool                  erprobe_pinning = true;
};

/// Die echten Handles an genau einer Stelle. Tests ersetzen sie durch Wegwerf-Baeume; die
/// Probe-Spezialisierungen enthalten dadurch keine versteckten, nicht injizierbaren Pfad-Konstanten.
[[nodiscard]] inline NumaCpuPinProcessProbeContext default_numa_cpu_pin_process_probe_context() {
    return {std::filesystem::path{kDefaultCpuRoot}, std::filesystem::path{kDefaultCpuPmuRoot}, true};
}

namespace detail {

/// Der EINE L6-Ausgang einer nicht-nativen Zelle: BEIDE Felder melden die fehlende Familien-
/// Schnittstelle. Er steht hier im Public Header und nicht in einem Blatt, damit die drei Blaetter nicht
/// je eine eigene Formulierung derselben Aussage tragen und kein Blatt ein anderes inkludieren muss.
[[nodiscard]] inline ProcessLocalityTopology numa_cpu_pin_process_os_feature_missing() {
    return ProcessLocalityTopology{
        std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}),
        std::unexpected(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt})};
}

} // namespace detail

// =================================================================================================
// 4. PLAUSIBILITAETS-DECKEL (fachliche Wachen, nicht arithmetische)
// =================================================================================================
// Sie weisen MUELL ab, nicht kuenftige Hardware. Die Werte liegen bewusst weit ueber jedem realen
// Stand: ein Ueberschreiten ist kein "neuer Server", sondern eine kaputte oder fremde Quelle.

/// Groesste plausible CPU-Id. CONFIG_NR_CPUS liegt in ueblichen x86_64-Konfigurationen bei 8192; 65535
/// laesst Luft, ohne eine 32-Bit-Muellzahl als Prozessor durchgehen zu lassen.
/// BEWUSST NICHT kMaxNumaNodeId (4095) aus numa_page_probe.hpp: die GRAMMATIK der beiden Listen ist
/// dieselbe, ihr DECKEL ist es nicht. Ein geteilter Deckel wuerde die CPU-Liste einer Grossmaschine als
/// QuelleKorrupt abweisen -- genau die Sorte stiller Fehlklassifikation, gegen die K4 gebaut ist.
inline constexpr std::uint32_t kMaxCpuId = 65535;
/// Groesste plausible Anzahl logischer Prozessoren. Verhindert, dass eine kaputte Bereichs-Angabe
/// ("0-4294967295") zu einer Milliarden-Element-Liste expandiert, bevor irgendeine Wache greift.
inline constexpr std::size_t kMaxCpuCount = 65536;
/// Groesste plausible L3-Groesse in Bytes (1 TiB). Heute liegen reale L3 bei 8 MiB bis ~1 GiB.
inline constexpr std::uint64_t kMaxL3SizeBytes = std::uint64_t{1} << 40;
/// Groesste Zahl UNTERSCHEIDBARER Kern-Klassen, die das Vokabular oben traegt -- angewandt auf die
/// L3-Domaenen-Groessen (linux) wie auf die Windows-EfficiencyClass-Werte. Zwei sind der reale Fall
/// (P/E bzw. V-Cache-CCD gegen Frequenz-CCD); MEHR wird NICHT auf zwei Klassen gezwungen, sondern als
/// FormatUnbekannt benannt (Kopf-Block "nicht geraten"). Das ist bewusst KEIN Muell-Deckel wie die drei
/// darueber, sondern die AUSDRUECKSGRENZE des Vokabulars -- und sie wird gesagt, nicht verschwiegen.
inline constexpr std::size_t kMaxDistinctCoreClasses = 2;

// =================================================================================================
// 5. DIE VERSIONIERTE VERFAHRENS-ID (K5)
// =================================================================================================

namespace detail {

inline constexpr std::string_view kNumaCpuPinProcessProbeIdPrefix = "numa_cpu_pin_process_probe.";
/// K5/A13-M1b: dreistellig, mit CPU-Hardware-Flag, NIE experimentell. COMDARE_VERSION_HW_FLAG_ENFORCE
/// ist scharf (Default 1) -- ein flagloses "v1.0.0" braeche die gated Wache unten sofort.
inline constexpr std::string_view kNumaCpuPinProcessProbeVersion = "v1.0.0c";

/// Ein fester constexpr-Puffer statt std::string: probe_id() muss auch compile-time aus dem von der
/// Achse gelieferten string_view komponierbar sein. 64 Bytes lassen Platz fuer kuenftige echte
/// Familien-IDs, ohne die heutige ID-Laenge als zweite Wissensquelle einzufrieren.
struct NumaCpuPinProcessProbeIdBuffer final {
    std::array<char, 64> bytes{};
    std::size_t          length = 0;

    [[nodiscard]] constexpr std::string_view view() const noexcept { return {bytes.data(), length}; }
};

[[nodiscard]] consteval NumaCpuPinProcessProbeIdBuffer
compose_numa_cpu_pin_process_probe_id(std::string_view family, std::string_view version) noexcept {
    NumaCpuPinProcessProbeIdBuffer result{};
    std::size_t const required = kNumaCpuPinProcessProbeIdPrefix.size() + family.size() + 1U + version.size();
    if (required > result.bytes.size()) return result;

    auto append = [&result](std::string_view part) {
        for (char const c : part) result.bytes[result.length++] = c;
    };
    append(kNumaCpuPinProcessProbeIdPrefix);
    append(family);
    result.bytes[result.length++] = '@';
    append(version);
    return result;
}

[[nodiscard]] constexpr std::size_t numa_cpu_pin_process_probe_at_count(std::string_view id) noexcept {
    std::size_t count = 0;
    for (char const c : id)
        if (c == '@') ++count;
    return count;
}

[[nodiscard]] constexpr std::string_view numa_cpu_pin_process_probe_family_part(std::string_view id) noexcept {
    if (!id.starts_with(kNumaCpuPinProcessProbeIdPrefix)) return {};
    std::size_t const at = id.find('@', kNumaCpuPinProcessProbeIdPrefix.size());
    if (at == std::string_view::npos) return {};
    return id.substr(kNumaCpuPinProcessProbeIdPrefix.size(), at - kNumaCpuPinProcessProbeIdPrefix.size());
}

} // namespace detail

/// K5-Helfer: schneidet den rohen vX.Y.Z-Teil hinter '@' aus der komponierten Verfahrens-ID. Die
/// separate genau-ein-'@'-Wache unten verhindert, dass ein mehrdeutiger Rest akzeptiert wird.
[[nodiscard]] constexpr std::string_view numa_cpu_pin_process_probe_version_part(std::string_view id) noexcept {
    std::size_t const at = id.find('@');
    if (at == std::string_view::npos) return {};
    return id.substr(at + 1U);
}

// =================================================================================================
// 6. DIE CT-FAMILIEN-FACTORY
// =================================================================================================

/// TOTALITAETS-WACHE: bewusst nur deklariert. Eine neue OS-Achsen-Auspraegung ohne ausdrueckliche
/// Probe-Entscheidung bleibt ein unvollstaendiger Typ und kann nicht still auf eine Familie fallen.
template <class OsAxis>
struct NumaCpuPinProcessProbe;

template <>
struct NumaCpuPinProcessProbe<LinuxOperatingSystem> {
    using os_axis = LinuxOperatingSystem;
    /// DER ANSCHLUSS: die Unter-Achse, deren machine_resolved-Werte hier aufgeloest werden -- als TYP,
    /// nicht als Etikett. Die Wachen am Dateiende pruefen, dass sie es wirklich ist.
    using core_axis = CoreClassSubAxis;

    inline static constexpr detail::NumaCpuPinProcessProbeIdBuffer kProbeId =
        detail::compose_numa_cpu_pin_process_probe_id(os_axis::os_family_id(), detail::kNumaCpuPinProcessProbeVersion);

    [[nodiscard]] static constexpr std::string_view probe_id() noexcept { return kProbeId.view(); }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept {
#if defined(__linux__)
        return true;
#else
        return false;
#endif
    }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
#if defined(__linux__)
        return {};
#else
        return "sysfs_cpu_baum_und_sched_affinity_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static ProcessLocalityTopology collect(NumaCpuPinProcessProbeContext const& context);
};

template <>
struct NumaCpuPinProcessProbe<WindowsOperatingSystem> {
    using os_axis   = WindowsOperatingSystem;
    using core_axis = CoreClassSubAxis;

    inline static constexpr detail::NumaCpuPinProcessProbeIdBuffer kProbeId =
        detail::compose_numa_cpu_pin_process_probe_id(os_axis::os_family_id(), detail::kNumaCpuPinProcessProbeVersion);

    [[nodiscard]] static constexpr std::string_view probe_id() noexcept { return kProbeId.view(); }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept {
#if defined(_WIN32)
        return true;
#else
        return false;
#endif
    }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
#if defined(_WIN32)
        return {};
#else
        return "logical_processor_information_ex_und_thread_affinity_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static ProcessLocalityTopology collect(NumaCpuPinProcessProbeContext const& context);
};

template <>
struct NumaCpuPinProcessProbe<MacosOperatingSystem> {
    using os_axis   = MacosOperatingSystem;
    using core_axis = CoreClassSubAxis;

    inline static constexpr detail::NumaCpuPinProcessProbeIdBuffer kProbeId =
        detail::compose_numa_cpu_pin_process_probe_id(os_axis::os_family_id(), detail::kNumaCpuPinProcessProbeVersion);

    [[nodiscard]] static constexpr std::string_view probe_id() noexcept { return kProbeId.view(); }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept {
#if defined(__APPLE__)
        return true;
#else
        return false;
#endif
    }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
#if defined(__APPLE__)
        return {};
#else
        return "sysctl_hw_perflevel_auf_dieser_build_plattform_nicht_verfuegbar";
#endif
    }
    [[nodiscard]] static ProcessLocalityTopology collect(NumaCpuPinProcessProbeContext const& context);
};

/// Der Vertrag jeder entschiedenen Familien-Zelle. os_axis bindet die Familien-Wahl als TYP, core_axis
/// bindet die AUFGELOESTE Unter-Achse als TYP; collect liefert je Feld genau einen Traeger oder einen
/// klassifizierten K4-Fehler, nie einen Ersatzwert.
template <class Probe>
concept NumaCpuPinProcessProbeConcept = requires(NumaCpuPinProcessProbeContext const& context) {
    typename Probe::os_axis;
    typename Probe::core_axis;
    requires OperatingSystemAxisConcept<typename Probe::os_axis>;
    requires TargetIsaSubAxisConcept<typename Probe::core_axis>;
    requires std::same_as<Probe, NumaCpuPinProcessProbe<typename Probe::os_axis>>;
    { Probe::probe_id() } -> std::same_as<std::string_view>;
    { Probe::has_native_probe() } -> std::same_as<bool>;
    { Probe::not_implemented_reason() } -> std::same_as<std::string_view>;
    { Probe::collect(context) } -> std::same_as<ProcessLocalityTopology>;
};

namespace detail {

/// Der gesamte K5-Vertrag in einer CT-Wache: Single-Source-Praefix, die Familie direkt aus dem
/// Achsen-Typ, genau ein Trenner und eine echte, dreistellige, ce-eigene, nicht-experimentelle Version.
template <class OsAxis>
[[nodiscard]] consteval bool numa_cpu_pin_process_probe_id_contract_is_satisfied() noexcept {
    std::string_view const id      = NumaCpuPinProcessProbe<OsAxis>::probe_id();
    std::string_view const version = numa_cpu_pin_process_probe_version_part(id);
    AlgoSemVer const       parsed  = parse_algo_semver(version);
    return id.starts_with(kNumaCpuPinProcessProbeIdPrefix) &&
           numa_cpu_pin_process_probe_family_part(id) == OsAxis::os_family_id() &&
           numa_cpu_pin_process_probe_at_count(id) == 1U && ce_owned_version_is_wellformed(version) &&
           !parsed.is_sentinel() && !parsed.experimental && version.find('e') == std::string_view::npos;
}

template <class OsAxis>
[[nodiscard]] consteval bool numa_cpu_pin_process_probe_version_satisfies_cpu_enforce() noexcept {
    return ce_owned_version_satisfies_cpu_enforce(
        numa_cpu_pin_process_probe_version_part(NumaCpuPinProcessProbe<OsAxis>::probe_id()));
}

/// ANSCHLUSS-WACHE: haengt die Zelle wirklich an der target_isa-Unter-Achse core_class, und deklariert
/// diese wirklich noch machine_resolved? Wuerde sie auf einen CT-Options-Katalog umgestellt, waere diese
/// Probe gegenstandslos -- sie soll dann brechen, nicht stumm weiterlaufen.
template <class OsAxis>
[[nodiscard]] consteval bool numa_cpu_pin_process_probe_is_anchored() noexcept {
    using Probe = NumaCpuPinProcessProbe<OsAxis>;
    return std::same_as<typename Probe::core_axis, CoreClassSubAxis> &&
           Probe::core_axis::option_source() == std::string_view{"machine_resolved"} &&
           Probe::core_axis::parent_axis_label() == std::string_view{"target_isa"};
}

} // namespace detail

static_assert(NumaCpuPinProcessProbeConcept<NumaCpuPinProcessProbe<LinuxOperatingSystem>>);
static_assert(NumaCpuPinProcessProbeConcept<NumaCpuPinProcessProbe<WindowsOperatingSystem>>);
static_assert(NumaCpuPinProcessProbeConcept<NumaCpuPinProcessProbe<MacosOperatingSystem>>);

static_assert(detail::numa_cpu_pin_process_probe_id_contract_is_satisfied<LinuxOperatingSystem>(),
              "K5: die Linux-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");
static_assert(detail::numa_cpu_pin_process_probe_id_contract_is_satisfied<WindowsOperatingSystem>(),
              "K5: die Windows-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");
static_assert(detail::numa_cpu_pin_process_probe_id_contract_is_satisfied<MacosOperatingSystem>(),
              "K5: die macOS-probe_id verletzt Praefix/Familie/Trenner/Versions-Grammatik.");

#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::numa_cpu_pin_process_probe_version_satisfies_cpu_enforce<LinuxOperatingSystem>(),
              "K5: die Linux-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
static_assert(detail::numa_cpu_pin_process_probe_version_satisfies_cpu_enforce<WindowsOperatingSystem>(),
              "K5: die Windows-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
static_assert(detail::numa_cpu_pin_process_probe_version_satisfies_cpu_enforce<MacosOperatingSystem>(),
              "K5: die macOS-probe_id-Version verletzt die CPU-only-Pflicht: sie MUSS auf 'c' enden und darf NIE "
              "'e' tragen (Owner-Q3/E2 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
#endif

// Paarweise verschieden, alle drei Vergleiche: zwei Familien duerfen nie als dasselbe Verfahren im Log
// oder Messergebnis erscheinen. Die IDs bleiben trotzdem vollstaendig aus os_family_id() abgeleitet.
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::probe_id() !=
                  NumaCpuPinProcessProbe<WindowsOperatingSystem>::probe_id() &&
              NumaCpuPinProcessProbe<LinuxOperatingSystem>::probe_id() !=
                  NumaCpuPinProcessProbe<MacosOperatingSystem>::probe_id() &&
              NumaCpuPinProcessProbe<WindowsOperatingSystem>::probe_id() !=
                  NumaCpuPinProcessProbe<MacosOperatingSystem>::probe_id());
// Und disjunkt zu BEIDEN bestehenden Verfahrens-Familien: dieselbe Maschine erhebt jetzt drei
// verschiedene Dinge, und die Provenienz-Kette muss sie unterscheiden koennen (das Praefix ist die
// einzige Trennung). "numa_cpu_pin_process_probe." darf ausserdem kein Praefix-Treffer von "numa_page_probe."
// sein -- ein Log-Grep auf die eine Familie darf die andere nicht mitlesen.
static_assert(!NumaCpuPinProcessProbe<LinuxOperatingSystem>::probe_id().starts_with(std::string_view{"os_probe."}));
static_assert(!NumaCpuPinProcessProbe<LinuxOperatingSystem>::probe_id().starts_with(std::string_view{
    "numa_page_probe."}));
static_assert(detail::kNumaCpuPinProcessProbeIdPrefix != std::string_view{"os_probe."});
static_assert(detail::kNumaCpuPinProcessProbeIdPrefix != std::string_view{"numa_page_probe."});
static_assert(!detail::kNumaCpuPinProcessProbeIdPrefix.starts_with(std::string_view{"numa_page_probe."}) &&
              !std::string_view{"numa_page_probe."}.starts_with(detail::kNumaCpuPinProcessProbeIdPrefix));

// -- Der ANSCHLUSS an die statische Einhaengung ---------------------------------------------------
static_assert(detail::numa_cpu_pin_process_probe_is_anchored<LinuxOperatingSystem>(),
              "OD-11-RT: die Linux-Zelle loest nicht mehr die machine_resolved-Unter-Achse core_class am "
              "target_isa-Komplex auf -- dann erhebt sie etwas anderes als das, was die statische "
              "Einhaengung angeboten hat.");
static_assert(detail::numa_cpu_pin_process_probe_is_anchored<WindowsOperatingSystem>());
static_assert(detail::numa_cpu_pin_process_probe_is_anchored<MacosOperatingSystem>());
// NAMENSFALLE (uebernommen aus target_isa_sub_axes.hpp, hier als Gegenprobe an der RT-Seite): die Achse
// heisst "core_class" -- "scheduling"/"hetero_core_dispatch" sind die CT-POLICY (was der Bau tun soll),
// nicht das RT-FAKTUM (was die Maschine hat), und "cpu_core"/"cpu_atom" sind die Intel-Rohnamen der
// QUELLE, nicht die Bedeutung der Achse.
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() == std::string_view{"core_class"});
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() != std::string_view{"scheduling"});
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() !=
              std::string_view{"hetero_core_dispatch"});
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() != kPerformanceCorePmuDirName);
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() != kEfficiencyCorePmuDirName);
// ABGRENZUNG zu den beiden SPEICHER-Unter-Achsen: dieselbe Mechanik, andere Sache. Die Wache steht hier,
// weil dieser Header der neue ist (Owner: "strukturell aehnliche Unterachsen" -- aehnlich, nicht gleich).
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() != NumaNodeSubAxis::axis_label());
static_assert(NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis::axis_label() != PageSubAxis::axis_label());
// Alle drei Familien loesen DIESELBE Achse auf -- die Familie waehlt die Quelle, nicht die Achse.
static_assert(std::same_as<NumaCpuPinProcessProbe<WindowsOperatingSystem>::core_axis,
                           NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis> &&
              std::same_as<NumaCpuPinProcessProbe<MacosOperatingSystem>::core_axis,
                           NumaCpuPinProcessProbe<LinuxOperatingSystem>::core_axis>);

// -- Die K4-Domaenen, textuell und semantisch an den Bestand gebunden ------------------------------
static_assert(error_domain(NumaCpuPinProcessProbeError{HardwareProbeErrorClass::QuelleFehlt}) ==
              ErrorDomain::HardwareProbe);
static_assert(numa_cpu_pin_process_probe_error_label(NumaCpuPinProcessProbeError{
                  HardwareProbeErrorClass::QuelleFehlt}) == std::string_view{"quelle_fehlt"});
static_assert(numa_cpu_pin_process_probe_error_label(NumaCpuPinProcessProbeError{
                  HardwareProbeErrorClass::QuelleKorrupt}) == std::string_view{"quelle_korrupt"});
static_assert(numa_cpu_pin_process_probe_error_label(NumaCpuPinProcessProbeError{
                  HardwareProbeErrorClass::FormatUnbekannt}) == std::string_view{"format_unbekannt"});
static_assert(error_domain(NumaCpuPinProcessProbeError{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}) ==
              ErrorDomain::CompilerCompiler);
static_assert(numa_cpu_pin_process_probe_error_label(NumaCpuPinProcessProbeError{
                  CompilerCompilerErrorClass::BetriebssystemFeatureFehlt}) ==
              std::string_view{"betriebssystem_feature_fehlt"});

// -- Die drei Vokabulare: BEIDE Drift-Richtungen (RF-3-Lehre) + volle Disjunktheit -----------------
static_assert(std::is_same_v<std::underlying_type_t<CoreClassKind>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<CoreTopologySource>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<PinningAvailability>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<CoreClassKind> && std::is_trivially_copyable_v<PinningCapability>);
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt.
static_assert(kCoreClassKindCount == static_cast<std::size_t>(CoreClassKind::KleinerCache) + 1);
static_assert(core_class_kind_token(static_cast<CoreClassKind>(kCoreClassKindCount)) ==
                  std::string_view{"kernklasse_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Kern-Klasse");
// L-1 (2026-08-07): der Namens-Pin zeigt jetzt auf DarwinPerflevel (=4), den NEUEN letzten Enumerator --
// dieselbe Wanderung wie axis_error.hpp CompilerCompilerErrorClass (FK-8): die Wache wandert mit dem
// letzten Wert, die Bestands-Nummern 0/1/2 stehen still.
static_assert(kCoreTopologySourceCount == static_cast<std::size_t>(CoreTopologySource::DarwinPerflevel) + 1);
static_assert(static_cast<std::uint8_t>(CoreTopologySource::HybridPmu) == 0 &&
                  static_cast<std::uint8_t>(CoreTopologySource::L3Domaene) == 1 &&
                  static_cast<std::uint8_t>(CoreTopologySource::Homogen) == 2,
              "L-1 haengt an (3/4) und verschiebt KEINE Bestands-Nummer.");
static_assert(static_cast<std::uint8_t>(CoreTopologySource::WindowsEfficiencyClass) == 3 &&
              static_cast<std::uint8_t>(CoreTopologySource::DarwinPerflevel) == 4);
static_assert(core_topology_source_token(static_cast<CoreTopologySource>(kCoreTopologySourceCount)) ==
                  std::string_view{"kernquelle_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Klassifikations-Quelle");
static_assert(core_topology_source_token(CoreTopologySource::WindowsEfficiencyClass) !=
                  core_topology_source_token(CoreTopologySource::HybridPmu),
              "L-1: Windows und Linux duerfen nie dieselbe Provenienz-Vokabel tragen -- verschiedene "
              "Erhebungs-Mechanismen sind verschiedene Aussagen.");
static_assert(core_topology_source_token(CoreTopologySource::DarwinPerflevel) !=
                  core_topology_source_token(CoreTopologySource::HybridPmu),
              "L-1: macOS und Linux duerfen nie dieselbe Provenienz-Vokabel tragen.");
static_assert(core_topology_source_token(CoreTopologySource::WindowsEfficiencyClass) !=
              core_topology_source_token(CoreTopologySource::DarwinPerflevel));
static_assert(kPinningAvailabilityCount == static_cast<std::size_t>(PinningAvailability::KeineSchnittstelle) + 1);
static_assert(pinning_availability_token(static_cast<PinningAvailability>(kPinningAvailabilityCount)) ==
                  std::string_view{"pinfaehigkeit_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Pinning-Stufe");
// Die tragenden Unterscheidungen: sie duerfen nie zusammenfallen, sonst behauptet eine Messreihe eine
// Lokalitaet bzw. eine Kern-Klasse, die es nicht gibt.
static_assert(pinning_availability_token(PinningAvailability::Durchgesetzt) !=
              pinning_availability_token(PinningAvailability::NichtDurchgesetzt));
static_assert(pinning_availability_token(PinningAvailability::VomKernAbgelehnt) !=
              pinning_availability_token(PinningAvailability::KeineSchnittstelle));
static_assert(core_class_kind_token(CoreClassKind::HoheLeistung) != core_class_kind_token(CoreClassKind::GrosserCache),
              "P/E-Hybrid und V-Cache-CCD sind verschiedene Aussagen -- ein geteiltes Etikett machte aus "
              "einer Cache-Groesse eine Mikroarchitektur-Behauptung.");
static_assert(core_class_kind_token(CoreClassKind::Uniform) != core_class_kind_token(CoreClassKind::HoheLeistung));
// Token-Disjunktheit gegen ALLE bestehenden Zell-/Fehler-Vokabeln (axis_error.hpp), jede Stufe einzeln
// plus der jeweilige Fallback -- ein Log-Grep auf diese Etiketten darf keine fremde Domaene mittreffen.
static_assert(probe_label_ist_disjunkt(core_class_kind_token(CoreClassKind::Uniform)));
static_assert(probe_label_ist_disjunkt(core_class_kind_token(CoreClassKind::HoheLeistung)));
static_assert(probe_label_ist_disjunkt(core_class_kind_token(CoreClassKind::HoheEffizienz)));
static_assert(probe_label_ist_disjunkt(core_class_kind_token(CoreClassKind::GrosserCache)));
static_assert(probe_label_ist_disjunkt(core_class_kind_token(CoreClassKind::KleinerCache)));
static_assert(probe_label_ist_disjunkt(core_class_kind_token(static_cast<CoreClassKind>(kCoreClassKindCount))));
static_assert(probe_label_ist_disjunkt(core_topology_source_token(CoreTopologySource::HybridPmu)));
static_assert(probe_label_ist_disjunkt(core_topology_source_token(CoreTopologySource::L3Domaene)));
static_assert(probe_label_ist_disjunkt(core_topology_source_token(CoreTopologySource::Homogen)));
static_assert(probe_label_ist_disjunkt(core_topology_source_token(CoreTopologySource::WindowsEfficiencyClass)));
static_assert(probe_label_ist_disjunkt(core_topology_source_token(CoreTopologySource::DarwinPerflevel)));
static_assert(
    probe_label_ist_disjunkt(core_topology_source_token(static_cast<CoreTopologySource>(kCoreTopologySourceCount))));
static_assert(probe_label_ist_disjunkt(pinning_availability_token(PinningAvailability::Durchgesetzt)));
static_assert(probe_label_ist_disjunkt(pinning_availability_token(PinningAvailability::NichtDurchgesetzt)));
static_assert(probe_label_ist_disjunkt(pinning_availability_token(PinningAvailability::VomKernAbgelehnt)));
static_assert(probe_label_ist_disjunkt(pinning_availability_token(PinningAvailability::KeineSchnittstelle)));
static_assert(
    probe_label_ist_disjunkt(pinning_availability_token(static_cast<PinningAvailability>(kPinningAvailabilityCount))));
// ... und gegen die Erhebungs-Klassen, gegen die probe_label_ist_disjunkt selbst nicht prueft.
static_assert(core_class_kind_token(CoreClassKind::Uniform) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleFehlt));
static_assert(pinning_availability_token(PinningAvailability::KeineSchnittstelle) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleKorrupt));

// -- Die Deckel sind echte Deckel (eine 0 waere ein Deckel, der alles verbietet) -------------------
static_assert(kMaxCpuId > 0 && kMaxCpuCount > 0 && kMaxL3SizeBytes > 0 && kMaxDistinctCoreClasses >= 2);
static_assert(kMaxCpuCount > kMaxCpuId / 2, "Der Anzahl-Deckel darf den Id-Deckel nicht entwerten.");
static_assert(kMaxL3SizeBytes >= (std::uint64_t{1} << 30), "1-GiB-L3 muss unter dem Deckel liegen.");
// Der CPU-Deckel ist AUSDRUECKLICH nicht der NUMA-Knoten-Deckel (Begruendung an der Konstante). Die
// Gegenprobe gegen kMaxNumaNodeId steht im TEST und nicht hier: dieser Header inkludiert
// numa_page_probe.hpp bewusst NICHT -- die beiden Proben sind Geschwister, keine Schichten, und ein
// Include waere die erste Kopplung, ueber die spaeter die Vokabulare zusammenlaufen.

// -- Der Default-Traeger behauptet nichts ('n/a statt Null', strukturell) --------------------------
// Ein default-konstruiertes Ergebnis traegt eine LEERE Gruppen-Liste -- deshalb gibt es keinen Pfad, der
// ihn als Erfolg erzeugt: jede collect-Zelle belegt beide Felder, und die Parser weisen die leere Menge
// als QuelleKorrupt zurueck. Diese Wache haelt fest, dass der Default NICHT die Bedeutung "keine
// Kern-Klassen / kein Pinning" tragen darf.
static_assert(std::is_default_constructible_v<ProcessLocalityTopology>);
static_assert(PinningCapability{}.availability == PinningAvailability::KeineSchnittstelle &&
                  !PinningCapability{}.maske_wiederhergestellt,
              "Ein default-konstruiertes PinningCapability behauptet KEINE Faehigkeit -- der Defaultwert "
              "ist die ehrliche Nicht-Aussage, nicht 'Pinning geht'.");

} // namespace comdare::cache_engine::measurement

// Die Definitionen liegen in Plattform-Blaettern. Jedes Blatt ist auf jeder Plattform uebersetzbar und
// inkludiert seinerseits diesen Public Header, sodass auch eine direkte Blatt-Inklusion gueltig ist.
#include <cache_engine/measurement/numa_cpu_pin_process_probe_linux.hpp>
#include <cache_engine/measurement/numa_cpu_pin_process_probe_macos.hpp>
#include <cache_engine/measurement/numa_cpu_pin_process_probe_windows.hpp>
