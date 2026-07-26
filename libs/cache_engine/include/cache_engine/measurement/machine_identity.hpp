// measurement/machine_identity.hpp -- Maschinen-Identifikation (Lane C, Paket O-4, Bauplan IV.2.6,
// Ledger §70.6; 26.07.2026).
//
// WAS O-4 LOEST: bis hierher konnte niemand sagen, WELCHE der deklarierten Maschinen gerade laeuft.
// active_machine_signature() (simd_build_gate.hpp:187) gab {} zurueck, ohne dass es einen Weg gab, je
// etwas anderes zu liefern. Dieser Header baut den fehlenden Weg -- und NUR ihn.
//
// AUSDRUECKLICHE ABGRENZUNG: die DREI Gate-Stubs (simd_build_gate.hpp:185-187) bleiben in O-4 LEER.
// Ihre GEMEINSAME Fuellung ist Paket C-3a (Bauplan D2.3/D2.11 + §70.9-Identitaets-Auflage), gegatet auf
// C-3b (Beleg liegt vor) + dieses Paket. O-4 hat deshalb KEINEN Produktions-Konsumenten und ist
// byte-neutral.
//
// -- DIE ZWEI IDENTITAETS-BEGRIFFE, die NICHT verwechselt werden duerfen --------------------------
//   HOSTNAME              = auf WELCHER KISTE laufe ich gerade. Eine INSTANZ-Eigenschaft.
//                           -> nur LOOKUP-Schluessel zur Laufzeit, NIE Teil der Identitaet.
//   cpu_fabrication + ram_pair = WAS FUER EINE Maschine das ist. Eine KLASSEN-Eigenschaft.
//                           -> DAS ist die Identitaet, und nur sie darf in den Stempel.
// §70.6 verlangt woertlich: "bei exaktem Eigenschafts-Match gilt die Achse als WIEDERVERWENDBAR (auch
// ueber formal verschiedene Maschinentypen); Wirkung NUR auf die Stempel-Identitaet (gleiche
// Kombination => gleicher Stempel)". Stuende der Hostname im Stempel, haetten zwei baugleiche Maschinen
// verschiedene Stempel -- genau das ist damit verboten. Der Ist-Code sagt dasselbe: validate_profile.hpp
// :1105-1117 prueft den SCHLUESSEL (cpu_fabrication + ram_pair, Pflicht), waehrend hostname_hint
// ausdruecklich "nur ein Hinweis" ist.
//
// -- EIN KANAL (§73.1: "eine XML als Quelle fuer den gesamten Prozess") ---------------------------
// Es wird hier KEINE zweite Hostname-Tabelle aufgemacht. Die Zuordnung Hostname -> Maschine steht
// bereits in der Anwender-XML und wird bereits geparst:
//     <machines><machine id cpu_fabrication ram_pair hostname_hint/>
//     xml_config_parser.cpp:529-538 -> ExperimentMachine (xml_config_parser.hpp:360-365)
//     Beispiel-Deklaration: tests/unit/thesis_tiere/experiment_golden_kern.xml
//         <machine id="prod1" cpu_fabrication="amd_zen5_avx512" ram_pair="ddr5_2x32" hostname_hint="prod1"/>
// Die Aufloesung laeuft daher in ZWEI Stufen, jede in ihrer eigenen Welt:
//     (1) Hostname -> (cpu_fabrication, ram_pair)   : aus der XML, zur Laufzeit (nicht hier)
//     (2) (cpu_fabrication, ram_pair) -> TYP        : hier, compile-time deklariert
// Stufe (2) muss in C++ liegen, weil ihr Ziel ein TYP ist (die Maschinen-Signatur) -- Typen koennen
// nicht in der XML stehen. Das ist kein zweiter Kanal, sondern der Resolver des einen Kanals.
//
// WARUM DIE XML-id NICHT der Schluessel ist: die XML fuehrt id="prod1", die CT-Signaturen fuehren
// machine_id()=="prod1_zen5" (machine_simd_signature.hpp:78/:92/:102). Die Namen sind ABSICHTLICH
// verschieden und werden NICHT angeglichen -- machine_id() wird vom Registry-Generator reflektiert
// (tools/system_axis_registry_gen/main.cpp:312-314), eine Umbenennung waere ein Byte-Ereignis. Gebunden
// wird ueber das EIGENSCHAFTS-TUPEL, und das ist ohnehin die von §70.6 geforderte Semantik.
//
// -- KEINE EXTERNE ABHAENGIGKEIT (Owner-Erlaubnis nicht in Anspruch genommen, §73.1) --------------
// Die CPU-Kern-Kennung kommt aus <cpuid.h> -- einem Header, den der Compiler selbst mitbringt. Weder
// libcpuid noch cpu_features werden vendored: sie waeren Wrapper um genau diese Instruktion. Es wird
// auch KEIN Prozess aufgerufen; ein dmidecode-Aufruf im Bau-Kanal wuerde den Bau von root-Rechten
// abhaengig machen (in der 8er-Docker-Matrix reihenweise rot) und waere genau der Nebenkanal, den §73.1
// gerade einsammeln laesst.
//
// -- HERKUNFT DER DEKLARIERTEN WERTE (fuer das User-Manual, V-6) ----------------------------------
// Die deklarierten Werte werden OFFLINE einmalig abgeleitet und danach hier bzw. in der XML gepflegt;
// die Werkzeuge sind NICHT Teil des Build-Graphen:
//     vendor / family / model / stepping / brand  <- CPUID, in-Prozess (dieser Header) ODER offline
//                                                    `lscpu` / `cat /proc/cpuinfo` / `cpuid_tool`
//     cpu_fabrication (symbolisches Etikett)      <- vom Menschen aus obigen Werten vergeben
//     ram_pair (Frequenz/CAS/Bestueckung)         <- `sudo dmidecode -t memory` bzw. `decode-dimms`
//                                                    (braucht root/i2c -> deshalb NIE im Bau)
// DRIFT-GEGENPROBE: verify_declared_cpu() unten prueft die CPU-Seite live gegen die Deklaration. Die
// RAM-Seite ist in-Prozess NICHT pruefbar (SMBIOS braucht Privilegien) und bleibt deklarations-basiert.
//
// Metaprog: constexpr-Tabelle + Freifunktionen, keine vtable, kein std::variant, kein Runtime-Switch
// auf Achsen-Typen. Die Host-Abfrage ist bewusst LAUFZEIT (welcher Host laeuft, ist keine Eigenschaft
// des Achsen-Raums) und sitzt an der CEB-Freigabe-Naht, nie im Hot-Path.

#pragma once

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/machine_simd_signature.hpp>
#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>

// Plattform-Kopfe fuer die beiden Live-Abfragen. <cpuid.h> bringt der Compiler selbst mit (GCC/Clang)
// -- keine ext/-Abhaengigkeit, kein Vendoring, kein Prozess-Aufruf (§73.1).
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. DIE CPU-KERN-KENNUNG
// =================================================================================================

/// Die normalisierte Kern-Kennung einer CPU (Bauplan IV.2.6). `declared == false` heisst: fuer diese
/// Maschine liegen die Werte NOCH NICHT vor -- dann wird NICHT geraten, sondern ehrlich gemeldet.
struct MachineCoreCpuId {
    std::string_view vendor{};     ///< CPUID-Leaf 0, z.B. "AuthenticAMD" / "GenuineIntel"
    std::uint32_t    family   = 0; ///< display family (mit ext_family verrechnet)
    std::uint32_t    model    = 0; ///< display model (mit ext_model verrechnet)
    std::uint32_t    stepping = 0; ///< CPUID-Leaf 1, Bits 0-3
    std::string_view brand{};      ///< CPUID-Leaf 0x80000002..4, Praefix-Vergleich genuegt
    bool             declared = false;
};

/// Gleichheit der Kern-Kennung. Der Marken-String wird als PRAEFIX verglichen, weil ihn die CPU
/// rechtsbuendig mit Leerzeichen auffuellt (live beobachtet: "AMD Ryzen 9 9950X3D 16-Core Processor  ").
[[nodiscard]] constexpr bool core_cpu_matches(MachineCoreCpuId const& declared, MachineCoreCpuId const& live) noexcept {
    if (!declared.declared) return false;
    if (declared.vendor != live.vendor) return false;
    if (declared.family != live.family || declared.model != live.model) return false;
    if (declared.stepping != live.stepping) return false;
    return declared.brand.empty() || live.brand.starts_with(declared.brand);
}

// =================================================================================================
// 2. DIE DEKLARIERTEN MASCHINEN (Stufe 2 der Aufloesung: Eigenschafts-Tupel -> Typ)
// =================================================================================================

/// Eine deklarierte Maschine: das Eigenschafts-TUPEL (die Identitaet, §70.6) plus das, was daran
/// haengt -- die CT-Signatur und die CPU-Kern-Kennung fuer die Drift-Gegenprobe.
struct DeclaredMachine {
    std::string_view                 cpu_fabrication; ///< == <machine cpu_fabrication=..> der XML
    std::string_view                 ram_pair;        ///< == <machine ram_pair=..> der XML
    std::string_view                 machine_id;      ///< == MachineSimdSignature::machine_id()
    MachineCoreCpuId                 core;            ///< fuer verify_declared_cpu()
    std::span<SimdFeatureFlag const> signature;       ///< die deklarierte SIMD-Signatur
};

/// prod1 -- LIVE ABGELEITET am 26.07.2026 auf dem Host `prod1` und gegen /proc/cpuinfo gegengeprueft
/// (vendor_id AuthenticAMD / cpu family 26 / model 68 / stepping 0). Beide Wege stimmten exakt ueberein.
inline constexpr MachineCoreCpuId kProd1Zen5Core{.vendor   = "AuthenticAMD",
                                                 .family   = 26,
                                                 .model    = 68,
                                                 .stepping = 0,
                                                 .brand    = "AMD Ryzen 9 9950X3D",
                                                 .declared = true};

/// prod2 / odroid -- die CPU-Kern-Kennung ist NOCH NICHT abgeleitet. Sie wird hier BEWUSST NICHT
/// geraten: der Cluster ist fuer mich read-only, ich konnte die beiden Hosts nicht abfragen. Bis die
/// Werte offline erhoben sind (siehe Herkunfts-Block im Kopf), meldet verify_declared_cpu() ehrlich
/// NichtDeklariert statt eines erfundenen Treffers.
inline constexpr MachineCoreCpuId kUndeclaredCore{};

/// Die Deklarations-Tabelle. Die Tupel-Werte sind NICHT hier erfunden, sondern WOERTLICH aus der
/// Anwender-XML uebernommen (tests/unit/thesis_tiere/experiment_golden_kern.xml):
///     <machine id="prod1" cpu_fabrication="amd_zen5_avx512" ram_pair="ddr5_2x32" hostname_hint="prod1"/>
///     <machine id="prod2" cpu_fabrication="intel_avx2"      ram_pair="ddr4_2x32" hostname_hint="prod2"/>
///
/// WARUM DIE ODROID-KLASSE FEHLT: fuer OdroidGracemontSignature deklariert HEUTE keine XML ein
/// Eigenschafts-Tupel. Ein Tupel hier zu erfinden waere eine Falsch-Aussage ueber echte Hardware -- die
/// CT-Signatur existiert, aber ihre Maschinen-KLASSE ist noch nicht deklariert. Sobald eine XML sie
/// fuehrt, tritt sie hier als vierte Zeile ein. Bis dahin loest sie ehrlich auf nullptr auf.
/// Eine neue Maschine tritt IMMER doppelt ein: hier als Zeile UND in der XML als <machine>.
inline constexpr std::array<DeclaredMachine, 2> kDeclaredMachines{{
    {"amd_zen5_avx512", "ddr5_2x32", Prod1Zen5Signature::machine_id(), kProd1Zen5Core, Prod1Zen5Signature::signature()},
    {"intel_avx2", "ddr4_2x32", Prod2RaptorLakeSignature::machine_id(), kUndeclaredCore,
     Prod2RaptorLakeSignature::signature()},
}};

/// Stufe 2: exakter EIGENSCHAFTS-Match (§70.6 -- ausdruecklich NICHT ueber den Namen). Zwei formal
/// verschiedene Maschinentypen mit identischem Tupel loesen auf dieselbe Zeile auf; genau das ist die
/// vom Owner geforderte Wiederverwendbarkeit. Kein Treffer -> nullptr, KEIN Ersatz-Treffer.
[[nodiscard]] constexpr DeclaredMachine const* resolve_machine_by_properties(std::string_view cpu_fabrication,
                                                                             std::string_view ram_pair) noexcept {
    for (auto const& m : kDeclaredMachines)
        if (m.cpu_fabrication == cpu_fabrication && m.ram_pair == ram_pair) return &m;
    return nullptr;
}

// =================================================================================================
// 3. DIE LIVE-ABFRAGE (Laufzeit, an der CEB-Freigabe-Naht -- nie im Hot-Path)
// =================================================================================================

/// Der Hostname dieses Rechners. NUR Lookup-Schluessel gegen <machine hostname_hint=..>, nie Identitaet.
/// Leerer String = nicht ermittelbar -> der Aufrufer MUSS das als "keine Maschine gewaehlt" behandeln.
///
/// Bewusst `inline` statt eigener .cpp: measurement/ ist durchgaengig header-only, und eine neue
/// Quelldatei haette einen CMake-Eintrag gebraucht (§73.1: CMake sparsam). Kein Prozess, kein Skript --
/// gethostname(2) bzw. GetComputerNameA sind Betriebssystem-Aufrufe.
[[nodiscard]] inline std::string live_hostname() {
#if defined(_WIN32)
    char  buf[256]{};
    DWORD len = static_cast<DWORD>(sizeof(buf));
    return GetComputerNameA(buf, &len) ? std::string{buf, len} : std::string{};
#elif defined(__unix__) || defined(__APPLE__)
    char buf[256]{};
    if (::gethostname(buf, sizeof(buf) - 1) != 0) return {};
    buf[sizeof(buf) - 1] = '\0';
    return std::string{buf};
#else
    return {}; // unbekannte Plattform -> keine Maschine gewaehlt (kein Raten)
#endif
}

/// Die CPU-Kern-Kennung dieses Rechners. Auf Nicht-x86 bleibt `declared == false` (CPUID gibt es dort
/// nicht) -- die Drift-Gegenprobe meldet dann NichtDeklariert statt eines falschen Urteils.
///
/// Die string_view-Felder zeigen auf `thread_local`-Puffer: der Rueckgabewert ist damit fuer den
/// aufrufenden Thread stabil, ohne dass der Typ (der auch constexpr-Deklarationen traegt) auf
/// std::string umgestellt werden muesste.
[[nodiscard]] inline MachineCoreCpuId live_core_cpu_id() {
#if defined(__x86_64__) || defined(__i386__)
    thread_local char vendor_buf[13]{};
    thread_local char brand_buf[49]{};
    unsigned          a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid(0, &a, &b, &c, &d) == 0) return {};
    std::memcpy(vendor_buf, &b, 4);
    std::memcpy(vendor_buf + 4, &d, 4);
    std::memcpy(vendor_buf + 8, &c, 4);
    vendor_buf[12] = '\0';

    if (__get_cpuid(1, &a, &b, &c, &d) == 0) return {};
    std::uint32_t const base_family = (a >> 8) & 0xFu;
    std::uint32_t const base_model  = (a >> 4) & 0xFu;
    std::uint32_t const ext_family  = (a >> 20) & 0xFFu;
    std::uint32_t const ext_model   = (a >> 16) & 0xFu;

    MachineCoreCpuId out{};
    out.vendor   = std::string_view{vendor_buf};
    out.family   = (base_family == 0xFu) ? base_family + ext_family : base_family;
    out.model    = (base_family == 0x6u || base_family == 0xFu) ? base_model + (ext_model << 4) : base_model;
    out.stepping = a & 0xFu;

    unsigned r[4]{};
    if (__get_cpuid(0x80000000u, &r[0], &r[1], &r[2], &r[3]) != 0 && r[0] >= 0x80000004u) {
        for (unsigned i = 0; i < 3; ++i) {
            if (__get_cpuid(0x80000002u + i, &r[0], &r[1], &r[2], &r[3]) == 0) break;
            std::memcpy(brand_buf + i * 16, r, 16);
        }
        brand_buf[48] = '\0';
        out.brand     = std::string_view{brand_buf};
    }
    out.declared = true;
    return out;
#else
    return {}; // keine x86-CPUID -> ehrlich NichtDeklariert statt eines falschen Urteils
#endif
}

// =================================================================================================
// 4. DAS URTEIL (kein stiller Fallback)
// =================================================================================================

/// Ergebnis der Identitaets-Pruefung. Jeder Wert ist ein eigener, benannter Ausgang -- es gibt keinen
/// Zweig, der stillschweigend eine fremde Maschinen-Identitaet annimmt.
enum class MachineIdentityVerdict : std::uint8_t {
    Match              = 0, ///< Deklaration und Host stimmen ueberein
    UnbekannteMaschine = 1, ///< kein Eigenschafts-Tupel getroffen -> KEINE Identitaet (Basis CPU-only)
    NichtDeklariert    = 2, ///< Maschine bekannt, aber ihre CPU-Kern-Kennung ist noch nicht erhoben
    Abweichung         = 3, ///< Deklaration und Host widersprechen sich -> Fehlerklasse
};

inline constexpr std::size_t kMachineIdentityVerdictCount = 4;

[[nodiscard]] constexpr std::string_view machine_identity_verdict_label(MachineIdentityVerdict v) noexcept {
    switch (v) {
        case MachineIdentityVerdict::Match: return "match";
        case MachineIdentityVerdict::UnbekannteMaschine: return "unbekannte_maschine";
        case MachineIdentityVerdict::NichtDeklariert: return "nicht_deklariert";
        case MachineIdentityVerdict::Abweichung: return "abweichung";
    }
    return "unbekannt";
}

/// Die Drift-Gegenprobe: stimmt der Host mit dem ueberein, was er zu sein behauptet?
[[nodiscard]] constexpr MachineIdentityVerdict verify_declared_cpu(DeclaredMachine const*  declared,
                                                                   MachineCoreCpuId const& live) noexcept {
    if (declared == nullptr) return MachineIdentityVerdict::UnbekannteMaschine;
    if (!declared->core.declared || !live.declared) return MachineIdentityVerdict::NichtDeklariert;
    return core_cpu_matches(declared->core, live) ? MachineIdentityVerdict::Match : MachineIdentityVerdict::Abweichung;
}

/// Abbildung auf die BESTEHENDE Fehlerklassen-Taxonomie (axis_error.hpp) -- ohne neue Klasse.
///
/// WARUM KEINE NEUE KLASSE: eine Abweichung heisst in der Praxis, dass der Host die deklarierten
/// ISA-/Beschleuniger-Eigenschaften NICHT hat -- und genau das ist die Definition von
/// HardwareErweiterungFehlt (axis_error.hpp:42). Die Klasse passt ehrlich, es braucht keine fuenfte.
/// Zusaetzlich waere eine fuenfte Klasse JETZT eine Nummern-Kollision: RF-3 (§70.3) baut
/// BetriebssystemFeatureFehlt als Count 4->5 in einem EIGENEN Paket. Zwei Pakete koennen nicht beide
/// die Fuenfte sein.
/// UnbekannteMaschine und NichtDeklariert sind KEINE Fehler, sondern definierte Zustaende: ohne
/// Identitaet gilt die Basis (CPU-only), und die laeuft per Halbordnung ueberall. Sie liefern deshalb
/// nullopt -- was ausdruecklich NICHT dasselbe ist wie "stiller Fallback": der Aufrufer bekommt das
/// benannte Urteil und muss es behandeln.
[[nodiscard]] constexpr std::optional<CompilerCompilerErrorClass>
error_class_of_verdict(MachineIdentityVerdict v) noexcept {
    if (v == MachineIdentityVerdict::Abweichung) return CompilerCompilerErrorClass::HardwareErweiterungFehlt;
    return std::nullopt;
}

/// Das Gesamt-Ergebnis einer Identifikation. `machine == nullptr` heisst IMMER "keine Identitaet";
/// es gibt keinen Zustand, in dem eine Signatur ohne bestaetigte Zuordnung geliefert wird.
struct MachineIdentityResult {
    DeclaredMachine const* machine = nullptr;
    MachineIdentityVerdict verdict = MachineIdentityVerdict::UnbekannteMaschine;

    /// Die freigegebene Signatur -- LEER, solange die Identitaet nicht bestaetigt ist.
    [[nodiscard]] constexpr std::span<SimdFeatureFlag const> signature() const noexcept {
        return (machine != nullptr && verdict == MachineIdentityVerdict::Match) ? machine->signature
                                                                                : std::span<SimdFeatureFlag const>{};
    }
};

/// Stufe 2 + Drift-Gegenprobe in einem Schritt. Die Eingabe ist das Eigenschafts-Tupel AUS DER XML --
/// diese Funktion liest selbst KEINE Konfiguration und kennt keinen Hostnamen (Ein-Kanal-Doktrin).
[[nodiscard]] constexpr MachineIdentityResult
identify_machine(std::string_view cpu_fabrication, std::string_view ram_pair, MachineCoreCpuId const& live) noexcept {
    MachineIdentityResult r{};
    r.machine = resolve_machine_by_properties(cpu_fabrication, ram_pair);
    r.verdict = verify_declared_cpu(r.machine, live);
    return r;
}

// =================================================================================================
// 5. WOHLGEFORMTHEIT (compile-time)
// =================================================================================================
static_assert(kDeclaredMachines.size() == 2,
              "Nur die in einer XML deklarierten Maschinen-Klassen stehen hier (prod1, prod2). "
              "OdroidGracemontSignature hat noch kein Eigenschafts-Tupel -- nicht erfinden.");
// Die Tupel muessen paarweise verschieden sein -- sonst waere die Aufloesung mehrdeutig.
static_assert(
    [] {
        for (std::size_t i = 0; i < kDeclaredMachines.size(); ++i)
            for (std::size_t j = i + 1; j < kDeclaredMachines.size(); ++j)
                if (kDeclaredMachines[i].cpu_fabrication == kDeclaredMachines[j].cpu_fabrication &&
                    kDeclaredMachines[i].ram_pair == kDeclaredMachines[j].ram_pair)
                    return false;
        return true;
    }(),
    "Eigenschafts-Tupel muessen paarweise verschieden sein (sonst mehrdeutige Aufloesung).");
// Jede Zeile zeigt auf eine der drei echten CT-Signaturen (Namens-Drift bricht hier).
static_assert(kDeclaredMachines[0].machine_id == std::string_view{"prod1_zen5"});
static_assert(kDeclaredMachines[1].machine_id == std::string_view{"prod2_raptor_lake"});
// Eigenschafts-Match trifft, Namens-Match ist gar nicht erst moeglich (XML-id != machine_id).
static_assert(resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32") == &kDeclaredMachines[0]);
static_assert(resolve_machine_by_properties("amd_zen5_avx512", "ddr4_2x32") == nullptr,
              "Teil-Treffer ist KEIN Treffer -- das Tupel gilt als Ganzes (§70.6 'exakter Match').");
static_assert(resolve_machine_by_properties("prod1", "ddr5_2x32") == nullptr,
              "Die XML-id ist NICHT der Schluessel; gebunden wird ueber das Eigenschafts-Tupel.");
// Kein Treffer -> keine Identitaet, keine Signatur. Der Kern der "kein stiller Fallback"-Auflage.
static_assert(identify_machine("gibt_es_nicht", "auch_nicht", kProd1Zen5Core).machine == nullptr);
static_assert(identify_machine("gibt_es_nicht", "auch_nicht", kProd1Zen5Core).verdict ==
              MachineIdentityVerdict::UnbekannteMaschine);
static_assert(identify_machine("gibt_es_nicht", "auch_nicht", kProd1Zen5Core).signature().empty());
// prod1 gegen seine eigenen, live abgeleiteten Werte: Match, und erst DANN gibt es eine Signatur.
static_assert(identify_machine("amd_zen5_avx512", "ddr5_2x32", kProd1Zen5Core).verdict ==
              MachineIdentityVerdict::Match);
static_assert(!identify_machine("amd_zen5_avx512", "ddr5_2x32", kProd1Zen5Core).signature().empty());
// Noch nicht erhobene Kern-Kennung -> ehrlich NichtDeklariert und KEINE Signatur (statt Raten).
static_assert(identify_machine("intel_avx2", "ddr4_2x32", kProd1Zen5Core).verdict ==
              MachineIdentityVerdict::NichtDeklariert);
static_assert(identify_machine("intel_avx2", "ddr4_2x32", kProd1Zen5Core).signature().empty());
// Abweichung -> D1 HardwareErweiterungFehlt; die uebrigen Urteile sind KEINE Fehler.
static_assert(error_class_of_verdict(MachineIdentityVerdict::Abweichung) ==
              CompilerCompilerErrorClass::HardwareErweiterungFehlt);
static_assert(!error_class_of_verdict(MachineIdentityVerdict::Match).has_value());
static_assert(!error_class_of_verdict(MachineIdentityVerdict::UnbekannteMaschine).has_value());
static_assert(!error_class_of_verdict(MachineIdentityVerdict::NichtDeklariert).has_value());
// Die Taxonomie bleibt bei VIER Klassen -- O-4 fuegt bewusst keine hinzu (RF-3-Kollision, s.o.).
static_assert(kCompilerCompilerErrorClassCount == 4,
              "O-4 erweitert die Fehlerklassen NICHT. Wer hier eine fuenfte einzieht, muss die "
              "RF-3-Kollision (BetriebssystemFeatureFehlt, §70.3) mit aufloesen.");
// Verdict-Etiketten sind eindeutig und nicht leer.
static_assert(machine_identity_verdict_label(MachineIdentityVerdict::Match) == std::string_view{"match"});
static_assert(machine_identity_verdict_label(MachineIdentityVerdict::Abweichung) !=
              machine_identity_verdict_label(MachineIdentityVerdict::NichtDeklariert));

} // namespace comdare::cache_engine::measurement
