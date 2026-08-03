#pragma once
// measurement/hardware_probe_factory.hpp -- die CT-Factory der HW-Erkennung (Task #7, Paket P2).
//
// WAS HIER STEHT: die Zuordnung ACHSEN-ZELLE -> ERKENNUNGS-GERAET, compile-time ueber
// Template-Spezialisierung, plus die EINE memoisierte Erhebung an der CEB-Freigabe-Naht. Die Ketten
// selbst liegen in ram_probe_chain.hpp; hier wird nur gewaehlt, welche gilt.
//
// -- REALM-EINORDNUNG (Owner-Nachtrag 3, N1/N4) --------------------------------------------------
// Die HW-Erkennung ist eine MESS-Achse (sie "misst" Hardware-Eigenschaften aus), zweigeteilt ueber
// Planer-RT und CEB-CT. Sie ist AUSDRUECKLICH KEIN System-Achsen-Anbau: dieser Header LIEST die
// System-Achsen-Typen (target_isa_complex, operating_system) als Zell-Koordinaten und fuegt ihnen
// KEIN Mitglied hinzu, aendert keine ihrer Strukturen und stempelt nicht in ihren Slot. Die
// Erkennung ist Konsument der Achsen, nicht ihr Bestandteil.
//
// -- WARUM DIE ZELLE BEIDE ACHSEN SPANNT (Owner-Nachtrag 2, K2) ----------------------------------
// Owner verbatim: der Planer "muss zumindest die ISA x OS Komplex-Hauptachse grob erkennen um per
// Metaprogrammierung eine passende feingranulare Hardware-Erkennung und passende OS handles dafuer
// mitzugeben bzw. je ISA x OS einzukompilieren". K2 schaerft: die Factory-Wahl spannt BEIDE Achsen im
// TYP -- nicht OS-only mit der ISA als Methoden-Parameter. Genau so ist HardwareProbeDevice
// parametrisiert. Dass die drei OS-Zeilen heute ueber die ISA partiell spezialisiert sind (ein freier
// ISA-Parameter statt sechs ausgeschriebener Zellen), ist keine Halbierung, sondern die Aussage des
// SPEC-SCHLUSSES: das Linux-Geraet gilt fuer eine BREITE BANDBREITE von CPU-Familien, weil SPD und
// SMBIOS herstellerneutral sind. Braucht eine ISA-Familie spaeter ein eigenes Geraet, tritt sie als
// vollstaendig spezialisierte Zelle daneben, ohne dass sich hier sonst etwas aendert.
//
// -- TOTALITAET UND WACHSTUM (K3/K5) -------------------------------------------------------------
// Das primaere Template ist DEKLARIERT, aber NICHT DEFINIERT. Das ist der ganze Mechanismus: eine
// Zelle ohne bewusste Entscheidung ist ein unvollstaendiger Typ und bricht -- statt still auf
// irgendein Default-Verhalten zu fallen. Neue Eintraege in den Achsen-Registries ziehen damit sofort
// eine Zellen-Entscheidung nach (K5: Compile-Bruch statt stiller Luecke). Die Totalitaets-Wache unten
// prueft die VOLLE Matrix, nicht nur die heute benutzte Zelle.
//
// -- OD-10-RT: DIE ZWEITE ERHEBUNG AN DERSELBEN ZELLE --------------------------------------------
// Seit OD-10-RT haengt neben der RAM-Kette eine zweite Erhebung an dieser Factory: die numa/page-Probe
// (numa_page_probe.hpp). Sie waehlt ihre Familie ueber DIESELBE Zell-Koordinate und bezieht ihre
// Zugriffs-Wurzeln als Zell-Handles (numa_node_root/hugepage_root). WARUM HIER UND NICHT DANEBEN: die
// Aussage "auf dieser Plattform liegt der Zugang so" gehoert genau einmal in den Bestand. Ein zweiter,
// paralleler Zell-Raum haette dieselbe Plattform-Zuordnung ein zweites Mal behauptet -- und zwei
// Behauptungen ueber dieselbe Sache driften.
// ABGRENZUNG, die man nicht verwischen darf: has_native_probe() dieser Zelle ist eine Aussage ueber die
// RAM-KETTE (windows/macos sind dort declared-only, A8). Die numa/page-Probe hat ihr EIGENES
// has_native_probe() je Familie -- sie ist auf allen drei Familien eine echte Zelle. Ein leeres
// numa_node_root() heisst deshalb NICHT "keine Erhebung", sondern "der Zugang laeuft dort nicht ueber
// Pfade".
//
// -- EHRLICHE NICHT-IMPLEMENTIERUNG (A8) ---------------------------------------------------------
// Windows und macOS bekommen KEIN ungetestetes Backend. Windows-WMI waere ein Versprechen ohne
// Maschine dahinter; macOS system_profiler waere ein Prozess-Aufruf und damit Doktrin-verboten. Beide
// Zellen sind deshalb declared-only -- aber mit ausdruecklich BESETZTEN oberen Stufen
// (NotImplementedProbe), damit der Degradations-Pfad "gefragt, gibt es hier nicht" sagt und nicht
// "uebersprungen, weil etwas Besseres da war".

#include <cache_engine/measurement/machine_identity.hpp>
#include <cache_engine/measurement/numa_page_probe.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/ram_frequency_reading.hpp>
#include <cache_engine/measurement/ram_probe_chain.hpp>
#include <cache_engine/measurement/target_isa_complex_axis.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. DAS ERKENNUNGS-GERAET EINER ZELLE
// =================================================================================================

/// Was jede Zelle liefern muss. Die Pfad-Funktionen sind die OS-HANDLES (K4): OS-gebundene
/// Zugriffs-Bausteine als CT-Bestandteil der Zelle. Die Kette selbst kennt sie nicht -- sie bekommt
/// sie im Kontext gereicht und bleibt dadurch OS-neutral und testbar.
///
/// OD-10-RT: numa_node_root/hugepage_root treten additiv hinzu. Sie gehoeren NICHT zur RAM-Kette,
/// sondern zur NumaPageProbe -- aber sie sind Handles derselben ISA-x-OS-ZELLE, und genau das ist die
/// Aussage dieses Concepts: eine Zelle fuehrt ALLE OS-gebundenen Zugriffs-Bausteine ihrer Plattform an
/// EINER Stelle. Ein zweiter, paralleler Zell-Raum nur fuer die numa/page-Handles waere eine zweite
/// Wissensquelle darueber, welche Zelle welche Plattform ist.
template <class D>
concept HardwareProbeDeviceConcept = requires {
    typename D::isa_axis;
    typename D::os_axis;
    typename D::chain_type;
    { D::device_id() } -> std::same_as<std::string_view>;
    { D::has_native_probe() } -> std::same_as<bool>;
    { D::boot_cache_path() } -> std::same_as<std::string_view>;
    { D::boot_id_path() } -> std::same_as<std::string_view>;
    { D::spd_device_root() } -> std::same_as<std::string_view>;
    { D::numa_node_root() } -> std::same_as<std::string_view>;
    { D::hugepage_root() } -> std::same_as<std::string_view>;
};

/// Das Geraet der Zelle (IsaComplexAxis x OsAxis).
///
/// BEWUSST NUR DEKLARIERT: eine Zelle ohne Spezialisierung ist ein unvollstaendiger Typ. Wer eine
/// Achsen-Auspraegung hinzufuegt, ohne die Zelle zu entscheiden, bekommt einen Compile-Fehler statt
/// eines stillen Default-Verhaltens (K5). Ein definiertes primaeres Template waere genau die stille
/// Luecke, die die Totalitaets-Auflage ausschliesst.
template <class IsaComplexAxis, class OsAxis>
struct HardwareProbeDevice;

/// linux -- die volle Kette. Gilt fuer JEDE ISA-Auspraegung (freier Parameter): SPD-EEPROM und SMBIOS
/// sind herstellerneutrale JEDEC-/DMTF-Standards, das Geraet ist also nicht an eine CPU-Familie
/// gebunden. Genau das meint der SPEC-SCHLUSS mit "breite Bandbreite verschiedener CPU-Familien".
template <class IsaComplexAxis>
struct HardwareProbeDevice<IsaComplexAxis, LinuxOperatingSystem> {
    using isa_axis   = IsaComplexAxis;
    using os_axis    = LinuxOperatingSystem;
    using chain_type = LinuxRamProbeChain;

    /// Etikett fuer Log und (spaeter, P5) das Mess-Stempel-Segment. Die Version im Namen ist Absicht:
    /// aendert sich das Erhebungs-VERHALTEN, wird daraus v2 -- ein Messwert-Vergleich ueber zwei
    /// verschiedene Erhebungs-Verfahren muss als solcher erkennbar bleiben.
    [[nodiscard]] static constexpr std::string_view device_id() noexcept { return "linux_dmi_spd_v1"; }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept { return true; }

    // -- OS-Handles (K4) --
    [[nodiscard]] static constexpr std::string_view boot_cache_path() noexcept { return kDefaultBootCachePath; }
    [[nodiscard]] static constexpr std::string_view boot_id_path() noexcept { return kDefaultBootIdPath; }
    [[nodiscard]] static constexpr std::string_view spd_device_root() noexcept { return kDefaultSpdDeviceRoot; }

    // -- OS-Handles der numa/page-Erhebung (OD-10-RT). Beide sysfs-Baeume sind unprivilegiert lesbar
    //    (gemessen auf prod1: online 0444, hugepages-<KiB>kB/nr_hugepages 0644) -- deshalb gibt es hier
    //    keine Boot-Cache-Stufe wie bei der RAM-Kette. Die Literale kommen aus numa_page_probe.hpp und
    //    stehen nicht ein zweites Mal da.
    [[nodiscard]] static constexpr std::string_view numa_node_root() noexcept { return kDefaultNumaNodeRoot; }
    [[nodiscard]] static constexpr std::string_view hugepage_root() noexcept { return kDefaultHugepageRoot; }
};

/// windows -- declared-only. GRUND (A8, benannt statt verschwiegen): der Ist-Takt stuende unter
/// Windows in Win32_PhysicalMemory.ConfiguredClockSpeed, das aus demselben SMBIOS-Feld stammt. Ein
/// WMI-Backend ohne Windows-Maschine zum Gegenpruefen auszuliefern hiesse, eine ungetestete Erhebung
/// als Messung auszugeben -- teurer als ehrlich nicht zu erheben.
template <class IsaComplexAxis>
struct HardwareProbeDevice<IsaComplexAxis, WindowsOperatingSystem> {
    using isa_axis   = IsaComplexAxis;
    using os_axis    = WindowsOperatingSystem;
    using chain_type = DeclaredOnlyRamProbeChain;

    [[nodiscard]] static constexpr std::string_view device_id() noexcept { return "windows_declared_only_v1"; }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
        return "wmi_backend_ungetestet_nicht_ausgeliefert";
    }

    // Keine Handles: es gibt auf dieser Zelle keinen Zugriffsweg, und ein Pfad, der ins Leere zeigt,
    // waere eine Behauptung. Leer heisst hier "nicht vorhanden", und die Stufen sagen dasselbe.
    [[nodiscard]] static constexpr std::string_view boot_cache_path() noexcept { return {}; }
    [[nodiscard]] static constexpr std::string_view boot_id_path() noexcept { return {}; }
    [[nodiscard]] static constexpr std::string_view spd_device_root() noexcept { return {}; }

    // OD-10-RT: leer aus einem ANDEREN Grund als oben, und der Unterschied ist wichtig. Die
    // numa/page-Erhebung EXISTIERT auf Windows (NumaPageProbe<WindowsOperatingSystem> ist eine echte
    // Zelle), sie bezieht ihre Werte dort aber ueber GetLogicalProcessorInformationEx/GetSystemInfo --
    // also gar nicht ueber Pfade. Ein gesetzter Pfad waere hier keine fehlende Faehigkeit, sondern eine
    // FALSCHE Zugriffs-Aussage.
    [[nodiscard]] static constexpr std::string_view numa_node_root() noexcept { return {}; }
    [[nodiscard]] static constexpr std::string_view hugepage_root() noexcept { return {}; }
};

/// macos -- declared-only, und das ist eine bewusste Dauer-Entscheidung, kein Rueckstand. GRUND:
/// system_profiler SPMemoryDataType waere ein PROZESS-Aufruf (Doktrin-Verbot) und unterscheidet
/// zudem konfiguriert nicht von maximal -- selbst mit Aufruf gaebe es dort nur ein
/// Stufe-2-Aequivalent, also keinen Ist-Takt.
template <class IsaComplexAxis>
struct HardwareProbeDevice<IsaComplexAxis, MacosOperatingSystem> {
    using isa_axis   = IsaComplexAxis;
    using os_axis    = MacosOperatingSystem;
    using chain_type = DeclaredOnlyRamProbeChain;

    [[nodiscard]] static constexpr std::string_view device_id() noexcept { return "macos_declared_only_v1"; }
    [[nodiscard]] static constexpr bool             has_native_probe() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view not_implemented_reason() noexcept {
        return "system_profiler_ist_prozessaufruf_und_ohne_ist_takt";
    }

    [[nodiscard]] static constexpr std::string_view boot_cache_path() noexcept { return {}; }
    [[nodiscard]] static constexpr std::string_view boot_id_path() noexcept { return {}; }
    [[nodiscard]] static constexpr std::string_view spd_device_root() noexcept { return {}; }

    // OD-10-RT: leer, weil Darwin die Werte ueber sysctlbyname anbietet und fuer NUMA gar keine
    // prozessfreie Schnittstelle fuehrt (siehe numa_page_probe_macos.hpp). Wieder eine Aussage ueber
    // den ZUGRIFFSWEG, nicht ueber eine fehlende Erhebungs-Zelle.
    [[nodiscard]] static constexpr std::string_view numa_node_root() noexcept { return {}; }
    [[nodiscard]] static constexpr std::string_view hugepage_root() noexcept { return {}; }
};

// =================================================================================================
// 2. DER DEKLARIERTE ZELL-RAUM + TOTALITAETS-WACHE (K3/K5)
// =================================================================================================

/// Die Achsen-Auspraegungen als TYP-Listen. Sie sind der Zell-Raum, ueber den die Totalitaets-Wache
/// laeuft -- die bestehenden String-Arrays (kAll*Ids) koennen das nicht leisten, weil eine Zelle
/// ueber TYPEN gewaehlt wird. Die Drift-Wachen darunter binden beide Sichten aneinander, damit hier
/// keine zweite, handgepflegte Achsen-Liste entsteht.
using AllIsaComplexAxes      = std::tuple<Prod1Zen5TargetIsa, Prod2RaptorLakeTargetIsa>;
using AllOperatingSystemAxes = std::tuple<LinuxOperatingSystem, WindowsOperatingSystem, MacosOperatingSystem>;

/// Ist die Zelle entschieden? Bei nur deklariertem primaeren Template ist der Typ unvollstaendig und
/// der requires-Ausdruck damit false -- ohne harten Fehler, sodass die Wache unten eine LESBARE
/// Meldung geben kann statt eines Substitutions-Absturzes.
template <class Isa, class Os>
concept HasProbeDeviceCell = requires { typename HardwareProbeDevice<Isa, Os>::chain_type; };

namespace detail {

template <class Isa, class... Oss>
[[nodiscard]] consteval bool os_row_complete(std::tuple<Oss...>*) noexcept {
    return (HasProbeDeviceCell<Isa, Oss> && ...);
}
template <class... Isas>
[[nodiscard]] consteval bool matrix_complete(std::tuple<Isas...>*) noexcept {
    return (os_row_complete<Isas>(static_cast<AllOperatingSystemAxes*>(nullptr)) && ...);
}

/// Traegt jede Zelle ein Geraet, das den Concept erfuellt? Vollstaendigkeit allein genuegt nicht --
/// eine Zelle koennte definiert sein und trotzdem die Handles schuldig bleiben.
template <class Isa, class... Oss>
[[nodiscard]] consteval bool os_row_conforms(std::tuple<Oss...>*) noexcept {
    return (HardwareProbeDeviceConcept<HardwareProbeDevice<Isa, Oss>> && ...);
}
template <class... Isas>
[[nodiscard]] consteval bool matrix_conforms(std::tuple<Isas...>*) noexcept {
    return (os_row_conforms<Isas>(static_cast<AllOperatingSystemAxes*>(nullptr)) && ...);
}

/// Deckt sich die Typ-Liste elementweise mit der bestehenden String-Single-Source? Groesse allein
/// faengt nur Zuwachs, nicht Umbau: taeuscht jemand die Reihenfolge um, zeigte die Wache weiterhin
/// gruen, waehrend die Zellen an anderen Achsen haengen.
template <class... Oss, std::size_t... I>
[[nodiscard]] consteval bool os_ids_match(std::tuple<Oss...>*, std::index_sequence<I...>) noexcept {
    return ((std::tuple_element_t<I, AllOperatingSystemAxes>::os_family_id() == kAllOperatingSystemIds[I]) && ...);
}
template <class... Isas, std::size_t... I>
[[nodiscard]] consteval bool isa_ids_match(std::tuple<Isas...>*, std::index_sequence<I...>) noexcept {
    return ((std::tuple_element_t<I, AllIsaComplexAxes>::complex_id() == kAllTargetIsaComplexIds[I]) && ...);
}

} // namespace detail

// -- Die Drift-Wachen zwischen Typ-Liste und String-Single-Source --
static_assert(std::tuple_size_v<AllOperatingSystemAxes> == kAllOperatingSystemIds.size(),
              "Die OS-Typ-Liste dieser Datei und kAllOperatingSystemIds sind auseinandergelaufen. Eine "
              "neue OS-Familie MUSS hier als Zell-Koordinate eintreten, sonst waere sie eine Achse "
              "ohne Erkennungs-Entscheidung (K5).");
static_assert(std::tuple_size_v<AllIsaComplexAxes> == kAllTargetIsaComplexIds.size(),
              "Die ISA-Komplex-Typ-Liste dieser Datei und kAllTargetIsaComplexIds sind "
              "auseinandergelaufen -- siehe OS-Wache darueber.");
static_assert(detail::os_ids_match(static_cast<AllOperatingSystemAxes*>(nullptr),
                                   std::make_index_sequence<std::tuple_size_v<AllOperatingSystemAxes>>{}),
              "Die OS-Typ-Liste steht in anderer Reihenfolge als kAllOperatingSystemIds.");
static_assert(detail::isa_ids_match(static_cast<AllIsaComplexAxes*>(nullptr),
                                    std::make_index_sequence<std::tuple_size_v<AllIsaComplexAxes>>{}),
              "Die ISA-Komplex-Typ-Liste steht in anderer Reihenfolge als kAllTargetIsaComplexIds.");

// -- Die TOTALITAETS-WACHE (K3): jede Zelle der VOLLEN Matrix ist entschieden --
static_assert(detail::matrix_complete(static_cast<AllIsaComplexAxes*>(nullptr)),
              "TOTALITAETS-WACHE (K3): Mindestens eine Zelle des Raums ISA-Komplex x OS traegt kein "
              "Erkennungs-Geraet. Jede Zelle braucht eine BEWUSSTE Entscheidung -- entweder eine echte "
              "Erhebung oder eine ehrliche declared-only-Auspraegung mit benanntem Grund. Eine fehlende "
              "Spezialisierung ist keine der beiden.");
static_assert(detail::matrix_conforms(static_cast<AllIsaComplexAxes*>(nullptr)),
              "Mindestens eine Zelle erfuellt HardwareProbeDeviceConcept nicht -- sie ist definiert, "
              "bleibt aber Kette, Etikett oder OS-Handles schuldig.");

// -- Zell-Eigenschaften, die keine Matrix-Schleife braucht --
static_assert(HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::has_native_probe(),
              "Linux ist die Bau-/Mess-Flotte -- faellt hier die echte Erhebung weg, misst niemand mehr.");
static_assert(!HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::has_native_probe() &&
                  !HardwareProbeDevice<Prod1Zen5TargetIsa, MacosOperatingSystem>::has_native_probe(),
              "Windows/macOS sind bewusst declared-only (A8). Wer hier true setzt, verspricht eine "
              "Erhebung, die es nicht gibt.");
// Dieselbe ISA, verschiedene OS -> verschiedene Ketten. Das ist der Beleg, dass die OS-Achse wirklich
// im Typ wirkt und nicht bloss als Etikett mitreist.
static_assert(!std::is_same_v<HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::chain_type,
                              HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::chain_type>);
// Die Zell-Etiketten sind paarweise verschieden -- ein geteiltes device_id() liesse zwei
// Erhebungs-Verfahren im Log als eines erscheinen.
static_assert(HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::device_id() !=
                  HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::device_id() &&
              HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::device_id() !=
                  HardwareProbeDevice<Prod1Zen5TargetIsa, MacosOperatingSystem>::device_id());
// Eine Zelle ohne native Erhebung darf keine Handles behaupten: ein gesetzter Pfad, den niemand
// liest, waere eine Falschangabe ueber die Faehigkeiten der Zelle.
static_assert(HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::spd_device_root().empty() &&
              HardwareProbeDevice<Prod1Zen5TargetIsa, MacosOperatingSystem>::spd_device_root().empty());
// Und die Gegenrichtung: die native Zelle MUSS ihre Handles fuehren, sonst laeuft ihre volle Kette
// ins Leere und faellt still auf die Deklaration zurueck.
static_assert(!HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::spd_device_root().empty() &&
              !HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::boot_cache_path().empty() &&
              !HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::boot_id_path().empty());

// -- OD-10-RT: dieselben beiden Richtungen fuer die numa/page-Handles ------------------------------
// (a) Die Linux-Zelle MUSS beide Wurzeln fuehren -- ein leeres Handle wuerde die Erhebung dort als
//     QuelleFehlt beantworten, obwohl die Baeume da sind.
static_assert(!HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::numa_node_root().empty() &&
              !HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::hugepage_root().empty());
// (b) Die beiden Pfad-losen Zellen duerfen KEINE Wurzel behaupten: dort kommen die Werte ueber
//     Familien-Schnittstellen, nicht ueber das Dateisystem. Ein gesetzter Pfad waere eine falsche
//     Zugriffs-Aussage -- und zwar eine ANDERE Aussage als die declared-only-Leere der RAM-Kette.
static_assert(HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::numa_node_root().empty() &&
              HardwareProbeDevice<Prod1Zen5TargetIsa, WindowsOperatingSystem>::hugepage_root().empty() &&
              HardwareProbeDevice<Prod1Zen5TargetIsa, MacosOperatingSystem>::numa_node_root().empty() &&
              HardwareProbeDevice<Prod1Zen5TargetIsa, MacosOperatingSystem>::hugepage_root().empty());
// (c) Die Zell-Handles sind AUS numa_page_probe.hpp gezogen, nicht ein zweites Mal hingeschrieben.
//     Faellt das je auseinander, laesen Prober und Zelle verschiedene Baeume.
static_assert(HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::numa_node_root() == kDefaultNumaNodeRoot &&
              HardwareProbeDevice<Prod1Zen5TargetIsa, LinuxOperatingSystem>::hugepage_root() == kDefaultHugepageRoot);
// (d) Die beiden Wurzeln sind verschieden und keine ist Praefix der anderen -- sonst koennte eine
//     Verwechslung im Kontext unbemerkt bleiben.
static_assert(kDefaultNumaNodeRoot != kDefaultHugepageRoot && !kDefaultNumaNodeRoot.starts_with(kDefaultHugepageRoot) &&
              !kDefaultHugepageRoot.starts_with(kDefaultNumaNodeRoot));
// (e) Und sie kollidieren mit KEINEM Handle der RAM-Kette: zwei Erhebungen, zwei Quellen-Raeume.
static_assert(kDefaultNumaNodeRoot != kDefaultSpdDeviceRoot && kDefaultNumaNodeRoot != kDefaultBootCachePath &&
              kDefaultNumaNodeRoot != kDefaultBootIdPath && kDefaultHugepageRoot != kDefaultSpdDeviceRoot &&
              kDefaultHugepageRoot != kDefaultBootCachePath && kDefaultHugepageRoot != kDefaultBootIdPath);

// =================================================================================================
// 3. DIE ZELLE DIESES BAUS
// =================================================================================================

/// Die Zelle, fuer die DIESE CEB gebaut ist.
///
/// Heute stehen hier die Defaults der Achsen (linux + prod1-Klasse) -- beweglich, kein Pin. In P5
/// setzt der PLANER die Zelle: er erkennt zu SEINER Laufzeit die Plattform und emittiert die Wahl als
/// Compile-Define in die CEB-Bau-Zeile (der in anatomy_version_stamp.hpp vorgemerkte W10-Anschluss).
/// Bis dahin ist diese Zeile die einzige Stelle, an der die Zelle festgelegt wird -- bewusst EINE
/// Stelle, damit die spaetere Umstellung genau hier und nirgends sonst ansetzt.
using CebHardwareProbeDevice = HardwareProbeDevice<DefaultTargetIsaComplex, DefaultOperatingSystem>;

static_assert(HardwareProbeDeviceConcept<CebHardwareProbeDevice>);

/// Der Kontext aus den OS-Handles der Zelle. Die Schluessel kommen vom Aufrufer (aus <machines> der
/// Anwender-XML) und werden UNVERAENDERT durchgereicht -- Ein-Kanal-Doktrin, kein zweiter Kanal.
template <class Device>
    requires HardwareProbeDeviceConcept<Device>
[[nodiscard]] inline RamProbeContext make_cell_context(std::string_view cpu_fabrication_key,
                                                       std::string_view ram_pair_key) {
    RamProbeContext ctx{};
    ctx.boot_cache_path     = Device::boot_cache_path();
    ctx.boot_id_path        = Device::boot_id_path();
    ctx.spd_device_root     = Device::spd_device_root();
    ctx.cpu_fabrication_key = cpu_fabrication_key;
    ctx.ram_pair_key        = ram_pair_key;
    return ctx;
}

/// Die Kette der Zelle EINMAL durchlaufen. Kein Zustand, keine Memoisierung -- diese Funktion ist der
/// nackte Erhebungs-Vorgang und wird von Tests direkt mit einem eigenen Kontext gefahren.
template <class Device>
    requires HardwareProbeDeviceConcept<Device>
[[nodiscard]] inline RamFrequencyReading probe_ram_frequency(RamProbeContext const& ctx) {
    typename Device::chain_type const chain{};
    return run_ram_probe_chain(chain, ctx);
}

/// OD-10-RT: der numa/page-Kontext AUS DER ZELLE. Das ist die Einhaengung des neuen Probers in die
/// bestehende ISA-x-OS-Factory: die Zelle sagt, welche Wurzeln auf ihrer Plattform gelten, und der
/// Prober liest sie -- er kennt selbst keine Pfad-Konstante. Ein leeres Handle reist unveraendert
/// durch und wird vom Prober als QuelleFehlt klassifiziert; es wird NICHT still durch einen Default
/// ersetzt, weil genau das eine Zell-Aussage ueberschreiben wuerde.
template <class Device>
    requires HardwareProbeDeviceConcept<Device>
[[nodiscard]] inline NumaPageProbeContext make_numa_page_context() {
    NumaPageProbeContext ctx{};
    ctx.numa_node_root = std::filesystem::path{Device::numa_node_root()};
    ctx.hugepage_root  = std::filesystem::path{Device::hugepage_root()};
    return ctx;
}

/// Die numa/page-Erhebung der Zelle EINMAL durchlaufen. Wie probe_ram_frequency: kein Zustand, keine
/// Memoisierung -- der nackte Erhebungs-Vorgang. Die FAMILIEN-Wahl kommt aus der Zell-Koordinate
/// Device::os_axis, nicht aus einem Runtime-Schalter.
template <class Device = CebHardwareProbeDevice>
    requires HardwareProbeDeviceConcept<Device>
[[nodiscard]] inline NumaPageTopology probe_numa_page_topology(NumaPageProbeContext const& ctx) {
    return NumaPageProbe<typename Device::os_axis>::collect(ctx);
}

// =================================================================================================
// 4. DIE CEB-FREIGABE-NAHT: EINE Erhebung je Prozess
// =================================================================================================
// WO DAS HIER SITZT: neben live_hostname()/identify_machine() -- an der Naht, an der die CEB EINMAL
// beim Start klaert, auf welcher Maschine sie laeuft (simd_build_gate.hpp, O-4). Die Erhebung liest
// Dateien; sie darf deshalb NIE in einer Statik-Initialisierung stehen (Reihenfolge ueber
// Uebersetzungseinheiten hinweg undefiniert, und IO waehrend der Initialisierung ist in der
// 8er-Docker-Matrix reihenweise das erste, was bricht), NIE im Bau-Graphen und NIE im gemessenen
// Pfad. Lazy beim ersten Aufruf, danach memoisiert.
//
// WARUM NICHT SCHLICHT EIN MEYERS-STATIC MIT DEN PARAMETERN: ein Meyers-Static wuerde den ZWEITEN
// Aufruf mit einem ANDEREN Schluessel stillschweigend mit dem alten Ergebnis beantworten. Genau diese
// stille Antwort hat set_active_machine_declaration (O-4) fuer die Maschinen-Belegung schon
// ausgeschlossen; die Erhebung folgt demselben Muster: der Zustand wird BENANNT zurueckgegeben, und
// der Aufrufer muss ihn behandeln. Der Meyers-Static bleibt -- fuer den Zustands-Halter selbst
// (acquisition_state()), exakt wie active_machine_state().

/// Wie dieser Aufruf beantwortet wurde. Jeder Wert ist ein eigener Ausgang; es gibt keinen Zweig, der
/// ein Ergebnis unter einem fremden Schluessel als das eigene ausgibt, ohne es zu sagen.
enum class RamAcquisitionState : std::uint8_t {
    FrischErhoben               = 0, ///< dieser Aufruf hat die Kette ausgeloest
    Memoisiert                  = 1, ///< bereits erhoben, identischer Schluessel -> derselbe Wert
    MemoisiertFremderSchluessel = 2, ///< bereits erhoben, ANDERER Schluessel -> Altwert, benannt
};
inline constexpr std::size_t kRamAcquisitionStateCount = 3;

[[nodiscard]] constexpr std::string_view ram_acquisition_state_token(RamAcquisitionState s) noexcept {
    switch (s) {
        case RamAcquisitionState::FrischErhoben: return "frisch_erhoben";
        case RamAcquisitionState::Memoisiert: return "memoisiert";
        case RamAcquisitionState::MemoisiertFremderSchluessel: return "memoisiert_fremder_schluessel";
    }
    return "erhebungs_zustand_unbekannt";
}

struct RamAcquisitionResult {
    RamFrequencyReading reading{};
    RamAcquisitionState state = RamAcquisitionState::FrischErhoben;
};

namespace detail {
struct RamAcquisitionSlot {
    std::mutex          mu;
    bool                latched = false;
    std::string         cpu_fabrication_key;
    std::string         ram_pair_key;
    RamFrequencyReading reading{};
};
[[nodiscard]] inline RamAcquisitionSlot& acquisition_slot() noexcept {
    static RamAcquisitionSlot s; // Meyers-Singleton: erst beim ersten Aufruf, thread-sicher
    return s;
}
} // namespace detail

/// Die EINE Erhebung je Prozess (Mess-Doktrin: ein Prozess je Lauf).
///
/// Der Schluessel wird MITGEFUEHRT, nicht nur benutzt: nur so laesst sich ein zweiter Aufruf mit
/// abweichendem Tupel als solcher melden. Er wandert dabei unveraendert in die Kette.
template <class Device = CebHardwareProbeDevice>
    requires HardwareProbeDeviceConcept<Device>
[[nodiscard]] inline RamAcquisitionResult acquire_ram_frequency(std::string_view cpu_fabrication_key,
                                                                std::string_view ram_pair_key) {
    auto&                       s = detail::acquisition_slot();
    std::lock_guard<std::mutex> lk(s.mu);
    if (s.latched) {
        bool const gleich = (s.cpu_fabrication_key == cpu_fabrication_key && s.ram_pair_key == ram_pair_key);
        return {s.reading, gleich ? RamAcquisitionState::Memoisiert : RamAcquisitionState::MemoisiertFremderSchluessel};
    }
    s.reading = probe_ram_frequency<Device>(make_cell_context<Device>(cpu_fabrication_key, ram_pair_key));
    s.cpu_fabrication_key.assign(cpu_fabrication_key);
    s.ram_pair_key.assign(ram_pair_key);
    s.latched = true;
    return {s.reading, RamAcquisitionState::FrischErhoben};
}

/// NUR fuer Tests: die Memoisierung loesen. Im Produktions-Pfad gibt es kein Zuruecksetzen -- eine
/// zweite Erhebung mitten im Lauf haette einen anderen Zeitstempel und moeglicherweise einen anderen
/// Wert, und dann waere die Haelfte eines Laufs anders belegt als die andere.
inline void reset_ram_acquisition_for_test() {
    auto&                       s = detail::acquisition_slot();
    std::lock_guard<std::mutex> lk(s.mu);
    s.latched = false;
    s.cpu_fabrication_key.clear();
    s.ram_pair_key.clear();
    s.reading = RamFrequencyReading{};
}

// -- Wachen der Naht --
static_assert(kRamAcquisitionStateCount ==
              static_cast<std::size_t>(RamAcquisitionState::MemoisiertFremderSchluessel) + 1);
static_assert(ram_acquisition_state_token(static_cast<RamAcquisitionState>(kRamAcquisitionStateCount)) ==
                  std::string_view{"erhebungs_zustand_unbekannt"},
              "Drift: hinter dem Count liegt ein etikettierter Erhebungs-Zustand");
static_assert(probe_label_ist_disjunkt(ram_acquisition_state_token(RamAcquisitionState::FrischErhoben)));
static_assert(probe_label_ist_disjunkt(ram_acquisition_state_token(RamAcquisitionState::Memoisiert)));
static_assert(probe_label_ist_disjunkt(ram_acquisition_state_token(RamAcquisitionState::MemoisiertFremderSchluessel)));
// Das Default-Ergebnis der Naht ist der ehrliche Leerzustand -- eine Erhebung, die nie lief, behauptet
// nichts.
static_assert(!has_frequency(RamAcquisitionResult{}.reading));

// =================================================================================================
// 5. GOLDEN-NEUTRALITAET (A10, strukturell statt als Zusicherung)
// =================================================================================================
// Der Typ-Schnitt ist die eigentliche Wache und steht in dem, was hier FEHLT: es gibt in diesem
// Header keine Funktion, die ein RamFrequencyReading an eine Stempel-, Achsen- oder binary_id-API
// reicht, und keine Achse nimmt den Typ an. Die folgenden Zeilen halten fest, dass die Erkennung die
// System-Achsen ausschliesslich LIEST (N4) -- die Zell-Koordinaten sind unveraenderte Achsen-Typen,
// keine Ableitungen mit zusaetzlichen Mitgliedern.
static_assert(TargetIsaComplexAxisConcept<CebHardwareProbeDevice::isa_axis>,
              "Die ISA-Koordinate MUSS eine unveraenderte System-Achsen-Auspraegung sein (N4).");
static_assert(OperatingSystemAxisConcept<CebHardwareProbeDevice::os_axis>,
              "Die OS-Koordinate MUSS eine unveraenderte System-Achsen-Auspraegung sein (N4).");
// Die Erkennung fuegt den Achsen NICHTS hinzu: die Zell-Koordinaten sind exakt die Achsen-Typen
// selbst. Waere hier ein abgeleiteter Typ eingesetzt, waere das der Anbau, den N4 ausschliesst.
static_assert(std::is_same_v<CebHardwareProbeDevice::isa_axis, DefaultTargetIsaComplex> &&
              std::is_same_v<CebHardwareProbeDevice::os_axis, DefaultOperatingSystem>);
// OD-10-RT: die numa/page-Erhebung der CEB-Zelle ist die Probe IHRER OS-Familie -- die Familien-Wahl
// laeuft ueber denselben Zell-Typ wie die RAM-Kette, nicht ueber einen zweiten Kanal.
static_assert(std::is_same_v<
              decltype(probe_numa_page_topology<CebHardwareProbeDevice>(std::declval<NumaPageProbeContext const&>())),
              NumaPageTopology>);
static_assert(NumaPageProbeConcept<NumaPageProbe<CebHardwareProbeDevice::os_axis>>);
// A-15/A10 STRUKTURELL, wie bei RamFrequencyReading: es gibt in diesem Header keine Funktion, die eine
// NumaPageTopology an eine Stempel-, Achsen- oder binary_id-API reicht, und keine Achse nimmt den Typ
// an. Die Erhebung LIEST die System-Achsen als Zell-Koordinaten und fuegt ihnen nichts hinzu.
static_assert(TargetIsaSubAxisConcept<NumaPageProbe<CebHardwareProbeDevice::os_axis>::numa_axis> &&
              TargetIsaSubAxisConcept<NumaPageProbe<CebHardwareProbeDevice::os_axis>::page_axis>);

} // namespace comdare::cache_engine::measurement
