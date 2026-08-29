// measurement/simd_build_gate.hpp -- flag-genaues Bau-Gate (Pruef-Dock-State-Pattern) (Bau Section 40.a, E4).
//
// User-Direktive (Section 37/40.a): System-Achsen geben HW-Features frei, Organ-Achsen setzen sie durch --
// "Organ-Nutzung <= Maschinen-Signatur GESCHNITTEN Organ-Sinnhaftigkeit". Dieses Gate sitzt (konzeptionell)
// an der CEB-Bau-Delegation (provision_all/CompileFn), NICHT im Tier/Planer. Es ist die aktive Durchsetzung
// der drei Section-40.a-Deklarationen: Maschinen-Signatur (machine_simd_signature.hpp), Organ-Sinnhaftigkeit
// (simd_organ_sensibility.hpp) und Flag-Katalog (simd_feature_flag.hpp).
//
// STATE PATTERN (GoF, zero-cost/constexpr realisiert -- KEINE vtable): das Pruef-Dock durchlaeuft
// Ungeprueft -> {NotApplicable | Freigegeben | Abgelehnt}. NotApplicable ist der Default, WENN ein Organ KEINE
// required-Flags deklariert -> das Gate ist dann inert und emittiert NICHTS (byte-/golden-neutral, Ist-Verhalten
// unveraendert; heute deklariert kein Organ required-Flags). Erst ein Organ, das required-Flags deklariert,
// aktiviert das Gate.
//
// E-10/ORG-19 SCHRITT 3 (26.08.2026, Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19), supersedierend zum
// Absatz oben: die Organ-Seite des Docks wird PER BINARY gebildet, nicht mehr global. Der fruehere
// globale Hook active_organ_required() (leere Vereinigung ueber alle Organ-Klassen) ist ENTFERNT; an
// seine Stelle treten organ_required_for_axes/organ_meaningful_for_axes (Aggregation ueber die
// (achse,wert)-Paare EINER Binary, 18+1-Register inkl. Organ-Meta-Meta) und gate_for_binary = DER EINE
// Helfer fuer Orchestrator-Admission, CompileFn-Naht (job.binary_id -> ceb_parse_path) und den
// Glied-[5]-/Suffix-Kanal. Der C-3a-TRIPWIRE ist zur POSITIVEN Zusage gedreht (Umschlag unten).
//
// FEHLERKLASSEN (an das bestehende measurement/axis_error.hpp angedockt, KEINE neue Taxonomie): eine Verletzung
// ist D1 CompilerCompilerErrorClass (Log, Experiment misst weiter -- kein Abbruch):
//   - HardwareErweiterungFehlt: required-Flag NICHT in der Maschinen-Signatur (Section 37: Organ <= Freigabe verletzt).
//   - CompileKombination:       required-Flag nicht sinnvoll (nicht in organ_meaningful) ODER von der Grob-Route
//                               (Runner-Tag) nicht zugelassen.
//   - ToolchainFehlt:           der aktive Compiler-Dialekt kennt das Flag nicht (leere Dialekt-Schreibweise).
//
// binary_id UNBERUEHRT: die effektiven Flags gehen an die CompileFn/den build_version-Suffix (Provenienz im
// H-10-Sidecar), NIE in die binary_id (golden==131072/320 neutral). Metaprog: constexpr, CRTP/Concept-frei
// (reine POD-Deskriptoren + Freifunktionen), kein std::variant, keine vtable.

#pragma once

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/machine_identity.hpp> // C-3a/O-4: welche Maschine laeuft
#include <cache_engine/measurement/simd_feature_flag.hpp>
#include <cache_engine/measurement/simd_organ_requirement.hpp> // C-3a: die required-Deklarationen
#include <cache_engine/measurement/simd_organ_sensibility.hpp> // C-3a: die Sinnhaftigkeits-Matrix

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::measurement {

// GoF-State-Pattern-Zustaende des Pruef-Docks (zero-cost, reine Enum -- keine polymorphe State-Klasse).
enum class SimdGateState : std::uint8_t {
    Ungeprueft    = 0, // Anfangszustand vor der Dock-Pruefung
    NotApplicable = 1, // Organ deklariert keine required-Flags -> nichts zu gaten (inert, byte-neutral)
    Freigegeben   = 2, // required <= Signatur GESCHNITTEN Sinnhaftigkeit GESCHNITTEN Route -> effective_flags frei
    Abgelehnt     = 3, // Verletzung -> D1-Fehlerklasse (Log, misst weiter; kein Abbruch)
};

// Grob-Route (Runner-Routing-Vorstufe: die grobe simd-Sub-Achse). Bestimmt, welche Flag-Tiers ueberhaupt
// zulaessig sind; die Feinsignatur entscheidet DANN die einzelnen Flags.
enum class SimdRoute : std::uint8_t {
    NoExtension = 0, // generisch, keine SIMD-Flags
    Avx2        = 1, // 256-bit VEX + Begleiter + Scalar (kein AVX-512)
    Avx512      = 2, // 512-bit kumulativ (alle Tiers)
};

// Compiler-Dialekt fuer die -m-Flag-Auswahl (ToolchainFehlt greift bei leerer Dialekt-Schreibweise).
enum class SimdDialect : std::uint8_t { Gpp = 0, Clang = 1 };

[[nodiscard]] constexpr bool route_allows(SimdRoute route, SimdFlagTier tier) noexcept {
    switch (route) {
        case SimdRoute::NoExtension: return false;
        case SimdRoute::Avx2: return tier != SimdFlagTier::Avx512; // 256-bit-Route laesst AVX-512 nicht zu
        case SimdRoute::Avx512: return true;                       // 512-bit-Route: alle Tiers kumulativ
    }
    return false;
}

// Grob-simd-Id (SimdNoExtOption/Avx2/Avx512Option::simd_id()) -> Route.
[[nodiscard]] constexpr SimdRoute route_of_simd_id(std::string_view simd_id) noexcept {
    if (simd_id == "avx512") return SimdRoute::Avx512;
    if (simd_id == "avx2") return SimdRoute::Avx2;
    return SimdRoute::NoExtension; // no_extension / unbekannt -> konservativ generisch
}

[[nodiscard]] constexpr std::string_view dialect_flag_of(SimdFeatureFlag const& f, SimdDialect d) noexcept {
    return d == SimdDialect::Clang ? f.clang : f.gpp;
}

[[nodiscard]] constexpr bool span_contains(std::span<SimdFeatureFlag const> set, SimdFeatureFlag const& f) noexcept {
    for (auto const& e : set)
        if (e.cpuinfo == f.cpuinfo) return true;
    return false;
}

// Ergebnis der Dock-Pruefung: Zustand + optionale Fehlerklasse (Abgelehnt) + effektive Flags (Freigegeben).
// Bounded auf die Katalog-Groesse (effective ist stets eine Teilmenge des Katalogs).
struct SimdGateResult {
    SimdGateState                                               state = SimdGateState::Ungeprueft;
    std::optional<CompilerCompilerErrorClass>                   error{}; // nur gesetzt, wenn state==Abgelehnt
    std::array<SimdFeatureFlag, kSimdFeatureFlagCatalog.size()> effective{};
    std::size_t effective_count = 0; // NotApplicable/Abgelehnt -> 0 (byte-neutral)
};

// Das Pruef-Dock: Ungeprueft -> {NotApplicable | Freigegeben | Abgelehnt}. Rein constexpr, static-dispatch.
//   organ_required   : die harten Voraussetzungen des Organs (LEER => NotApplicable => inert/byte-neutral)
//   organ_meaningful  : die Sinnhaftigkeits-Obergrenze (aus simd_organ_sensibility.hpp)
//   machine_signature : die deklarierte Maschinen-Signatur (aus machine_simd_signature.hpp)
//   route             : die Grob-Route (Runner-Tag)
//   dialect           : aktiver Compiler-Dialekt (ToolchainFehlt-Pruefung)
[[nodiscard]] constexpr SimdGateResult pruef_dock(std::span<SimdFeatureFlag const> organ_required,
                                                  std::span<SimdFeatureFlag const> organ_meaningful,
                                                  std::span<SimdFeatureFlag const> machine_signature, SimdRoute route,
                                                  SimdDialect dialect = SimdDialect::Gpp) noexcept {
    SimdGateResult r{};

    // Ungeprueft -> NotApplicable: kein required deklariert -> nichts zu gaten (inert, byte-neutral).
    if (organ_required.empty()) {
        r.state = SimdGateState::NotApplicable;
        return r;
    }

    for (auto const& f : organ_required) {
        // Section 37 harte Zulassung: required <= Maschinen-Signatur.
        if (!span_contains(machine_signature, f)) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::HardwareErweiterungFehlt;
            return r;
        }
        // Sinnhaftigkeit: required <= organ_meaningful.
        if (!span_contains(organ_meaningful, f)) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::CompileKombination;
            return r;
        }
        // Route (Grob-Tag) muss den Tier des required-Flags zulassen.
        if (!route_allows(route, f.tier)) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::CompileKombination;
            return r;
        }
        // Toolchain: der aktive Dialekt muss das Flag kennen.
        if (dialect_flag_of(f, dialect).empty()) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::ToolchainFehlt;
            return r;
        }
    }

    // Freigegeben: effektive Flags = Signatur GESCHNITTEN Sinnhaftigkeit GESCHNITTEN Route (kumulativ, nicht nur required).
    r.state = SimdGateState::Freigegeben;
    for (auto const& f : machine_signature)
        if (span_contains(organ_meaningful, f) && route_allows(route, f.tier) && !dialect_flag_of(f, dialect).empty())
            r.effective[r.effective_count++] = f;
    return r;
}

// Section 37 Zulassung an der CEB-Bau-Delegation (provision_core, per-Binary): organ_required <= machine_signature.
// Rueckgabe = D1-Fehlerklasse bei Verletzung (HardwareErweiterungFehlt), sonst kein Fehler. organ_required LEER
// -> kein Fehler (byte-neutral). Fokussierte Durchsetzung fuer den Bau-Delegations-Punkt (ohne Flag-Emission --
// die uebernimmt die per-Perm-CompileFn-Naht in der Fassade).
[[nodiscard]] constexpr std::optional<CompilerCompilerErrorClass>
admit_organ_on_machine(std::span<SimdFeatureFlag const> organ_required,
                       std::span<SimdFeatureFlag const> machine_signature) noexcept {
    for (auto const& f : organ_required)
        if (!span_contains(machine_signature, f)) return CompilerCompilerErrorClass::HardwareErweiterungFehlt;
    return std::nullopt;
}

// CompileFn-Emission (runtime, an der provision_all/CompileFn-Naht): die einzelnen -m-Flags des Ergebnisses.
// Freigegeben -> je effektives Flag ein -m<flag>; NotApplicable/Abgelehnt -> LEER (CompileFn unveraendert).
[[nodiscard]] inline std::vector<std::string> effective_march_flags(SimdGateResult const& r,
                                                                    SimdDialect           dialect = SimdDialect::Gpp) {
    std::vector<std::string> out;
    if (r.state != SimdGateState::Freigegeben) return out; // byte-neutral, wenn nicht freigegeben
    out.reserve(r.effective_count);
    for (std::size_t i = 0; i < r.effective_count; ++i) out.emplace_back(dialect_flag_of(r.effective[i], dialect));
    return out;
}

// -- CompileFn-Naht-Konsumtion (an provision_all/CompileFn, Bau Section 40.a-E4) -------------------
// Route aus dem bereits aufgeloesten Grob-march-Flag (-mavx2/-mavx512f/leer) -- die CompileFn-Naht kennt
// den march_flag-String, nicht die simd-id.
[[nodiscard]] constexpr SimdRoute route_of_march_flag(std::string_view march_flag) noexcept {
    if (march_flag.find("avx512") != std::string_view::npos) return SimdRoute::Avx512;
    if (march_flag.find("avx2") != std::string_view::npos) return SimdRoute::Avx2;
    return SimdRoute::NoExtension;
}

// =================================================================================================
// PAKET C-3a -- SCHARFSCHALTUNG DER DREI HOOKS (Bauplan D2.3/D2.11, Owner-Vorab-GO Ledger §69.9)
// =================================================================================================
// Die drei Hooks werden GEMEINSAM gefuellt. Einzeln waere es wirkungslos: :187 allein ist ein
// garantierter No-Op, weil pruef_dock:109 bei leerem required sofort NotApplicable zurueckgibt --
// unabhaengig davon, welche Maschinen-Signatur eingesetzt wird (C-3b-Beleg).
//
// BYTE-KLASSE: byte-neutral scharf. Der DOMINANTE Schalter ist die Organ-Seite, und die ist leer
// (simd_organ_requirement.hpp:88 static_assert -- alle 9 Klassen tragen kRequiredNone). Damit bleibt
// pruef_dock NotApplicable, die rsp-Zeilen sind zeichengleich, und build_orchestrator.hpp:783ff
// betritt seinen Gate-Block nie. Belegt per rsp-Diff je Route im unregistrierten C-3a-Test.
//
// NOT-AUS: ein Knopf, kein CMake-Schalter (§73.1 "keine Skripte ausser CMake, und CMake sparsam").
#ifndef COMDARE_SIMD_GATE_ARMED
#define COMDARE_SIMD_GATE_ARMED 1
#endif
inline constexpr bool kSimdGateArmed = (COMDARE_SIMD_GATE_ARMED != 0);

// -- Die AKTIVE Maschinen-Deklaration (Einmal-Belegung durch die CEB) -----------------------------
// Die CompileFn-Naht kennt keinen XML-Kontext. Die CEB belegt deshalb EINMAL beim Start -- VOR der
// Perm-Schleife -- welches Eigenschafts-Tupel (cpu_fabrication, ram_pair) fuer diesen Lauf gilt; es
// stammt aus <machines><machine> der Anwender-XML (Ein-Kanal-Doktrin §73.1, O-4).
// DEFAULT LEER => keine Identitaet => Gate inert. Das ist der NATUERLICHE Kill-Switch.
enum class MachineDeclarationSetResult : std::uint8_t {
    Gesetzt                 = 0, ///< erste Belegung, uebernommen
    BereitsGesetztIdentisch = 1, ///< erneute Belegung mit demselben Tupel -- geduldet, kein Effekt
    AbgelehntAbweichend     = 2, ///< erneute Belegung mit ANDEREM Tupel -- ABGELEHNT, Altwert bleibt
};

namespace detail {
struct ActiveMachineState {
    std::mutex        mu;
    std::atomic<bool> latched{false};
    std::string       cpu_fabrication;
    std::string       ram_pair;
};
[[nodiscard]] inline ActiveMachineState& active_machine_state() noexcept {
    static ActiveMachineState s;
    return s;
}
} // namespace detail

/// EINMAL-BELEGUNG mit benanntem Verhalten beim zweiten Aufruf. Ein stilles Ueberschreiben mitten in
/// der Perm-Schleife waere ein Determinismus-Bruch: die Haelfte der Binaries eines Laufs haette eine
/// andere Freigabe als die andere. Ein abweichender Zweit-Aufruf wird deshalb ABGELEHNT, nicht
/// uebernommen -- der Aufrufer bekommt das Urteil zurueck und muss es behandeln ([[nodiscard]]).
/// Thread-sicher: die Belegung laeuft unter Mutex, das Latch-Flag traegt release/acquire.
[[nodiscard]] inline MachineDeclarationSetResult set_active_machine_declaration(std::string_view cpu_fabrication,
                                                                                std::string_view ram_pair) {
    auto&                       s = detail::active_machine_state();
    std::lock_guard<std::mutex> lk(s.mu);
    if (s.latched.load(std::memory_order_relaxed)) {
        return (s.cpu_fabrication == cpu_fabrication && s.ram_pair == ram_pair)
                   ? MachineDeclarationSetResult::BereitsGesetztIdentisch
                   : MachineDeclarationSetResult::AbgelehntAbweichend;
    }
    s.cpu_fabrication.assign(cpu_fabrication);
    s.ram_pair.assign(ram_pair);
    s.latched.store(true, std::memory_order_release);
    return MachineDeclarationSetResult::Gesetzt;
}

/// NUR fuer Tests: die Einmal-Belegung zuruecksetzen. Im Produktions-Pfad gibt es kein Zuruecksetzen.
inline void reset_active_machine_declaration_for_test() {
    auto&                       s = detail::active_machine_state();
    std::lock_guard<std::mutex> lk(s.mu);
    s.cpu_fabrication.clear();
    s.ram_pair.clear();
    s.latched.store(false, std::memory_order_release);
}

// -- Hook 1/3: die Organ-Anforderungen PER BINARY (E-10 Schritt 3a) -------------------------------
// HISTORIE (supersedierend fortgeschrieben, nie geloescht): bis E-10 stand hier der globale Hook
// active_organ_required() -- die leere VEREINIGUNG ueber alle Organ-Klassen, compile-hart bewacht vom
// C-3a-TRIPWIRE ("jede Binary bekaeme die Flags, auch die, die das Organ gar nicht nutzt"). Seit dem
// 18+1-Register (organ_meta_meta_requirement.hpp) waere ein globaler Hook genau die Klasse Fehler, die
// der Tripwire ankuendigte: ein IO-Traeger-Glied stempelte/gatete JEDE Binary. Der Hook ist deshalb
// ENTFERNT (kein Alias, kein Wrapper -- jeder Aufrufer bricht LAUT) und durch die per-Binary-Formen
// ersetzt: die Aggregation laeuft ueber die (achse,wert)-Paare EINER Binary (Kern in
// simd_organ_requirement.hpp, vereinigt 18er-Tabelle UND Meta-Meta-Zeile des Traeger-Paares).
[[nodiscard]] inline std::vector<SimdFeatureFlag>
organ_required_for_axes(std::span<std::pair<std::string, std::string> const> binary_axes) {
    return aggregate_required_for_axes(binary_axes);
}
[[nodiscard]] inline std::vector<SimdFeatureFlag>
organ_meaningful_for_axes(std::span<std::pair<std::string, std::string> const> binary_axes) {
    return aggregate_meaningful_for_axes(binary_axes);
}

static_assert(detail::probe_per_binary_aggregation_ist_exakt(),
              "C-3a-UMSCHLAG (25.08.2026): die required-Vereinigung wird PER BINARY ueber "
              "aggregate_required_for_axes gebildet (Orchestrator-Admission, CompileFn-Naht, "
              "Laufzeit-Zwilling aus EINEM Helfer gate_for_binary); die consteval-Probe belegt: "
              "Achsen MIT Meta-Meta-Traeger -> Probe-Menge, OHNE -> leer. Ein globaler Hook "
              "existiert nicht mehr. (TRIPWIRE-UMSCHLAG-Muster: zur POSITIVEN Zusage gedreht, "
              "nicht geloescht.)");

// -- Hook 2/3: die Sinnhaftigkeits-Obergrenze -----------------------------------------------------
// Anders als die required-Seite ist die Sinnhaftigkeits-Matrix NICHT leer. Die Vereinigung wird daher
// compile-time wirklich gebildet (dedupliziert ueber den cpuinfo-Namen). Sie wird von pruef_dock erst
// NACH der required-Pruefung gelesen und ist heute folgenlos -- aber sie ist echt, nicht gestubbt.
namespace detail {
[[nodiscard]] consteval std::size_t sensibility_union_count() {
    std::array<std::string_view, kSimdFeatureFlagCatalog.size()> seen{};
    std::size_t                                                  n = 0;
    for (auto const& e : kSimdOrganSensibility)
        for (auto const& f : e.meaningful) {
            bool dup = false;
            for (std::size_t i = 0; i < n; ++i)
                if (seen[i] == f.cpuinfo) dup = true;
            if (!dup) seen[n++] = f.cpuinfo;
        }
    return n;
}
[[nodiscard]] consteval auto make_sensibility_union() {
    std::array<SimdFeatureFlag, sensibility_union_count()> out{};
    std::size_t                                            n = 0;
    for (auto const& e : kSimdOrganSensibility)
        for (auto const& f : e.meaningful) {
            bool dup = false;
            for (std::size_t i = 0; i < n; ++i)
                if (out[i].cpuinfo == f.cpuinfo) dup = true;
            if (!dup) out[n++] = f;
        }
    return out;
}
} // namespace detail

/// Die Vereinigung der Sinnhaftigkeits-Mengen aller Organ-Klassen (dedupliziert, compile-time).
inline constexpr auto kSensibilityUnion = detail::make_sensibility_union();

/// E-10 Schritt 3a: BLEIBT als VOLLMENGEN-Auskunft (die globale Obergrenze ueber alle Organ-Klassen),
/// wird aber am Dock nicht mehr verwendet -- die Dock-Eingabe ist seit E-10 die per-Binary-Obergrenze
/// organ_meaningful_for_axes (Hook 2/3 per Binary, Sinnhaftigkeit der Achsen DIESER Binary).
[[nodiscard]] inline std::span<SimdFeatureFlag const> active_organ_meaningful() noexcept {
    if constexpr (!kSimdGateArmed) return {};
    return kSensibilityUnion;
}

// -- Hook 3/3: die Signatur der laufenden Maschine ------------------------------------------------
// Ueber O-4: die von der CEB belegte Deklaration -> Eigenschafts-Match -> Drift-Gegenprobe gegen die
// echte CPU. Eine Signatur faellt AUSSCHLIESSLICH bei Verdict Match heraus; ohne Belegung, ohne
// Tupel-Treffer, bei nicht erhobener Kern-Kennung und bei Abweichung bleibt sie LEER. Es gibt keinen
// Zweig, der still eine fremde Maschinen-Identitaet annimmt.
[[nodiscard]] inline std::span<SimdFeatureFlag const> active_machine_signature() noexcept {
    if constexpr (!kSimdGateArmed) return {};
    auto const& s = detail::active_machine_state();
    if (!s.latched.load(std::memory_order_acquire)) return {}; // CEB hat nichts belegt -> inert
    return identify_machine(s.cpu_fabrication, s.ram_pair, live_core_cpu_id()).signature();
}

/// Das Urteil zur laufenden Maschine (fuer Log/Diagnose an der Freigabe-Naht -- kein stiller Fallback).
[[nodiscard]] inline MachineIdentityVerdict active_machine_verdict() noexcept {
    auto const& s = detail::active_machine_state();
    if (!s.latched.load(std::memory_order_acquire)) return MachineIdentityVerdict::UnbekannteMaschine;
    return identify_machine(s.cpu_fabrication, s.ram_pair, live_core_cpu_id()).verdict;
}

// Die einzelnen -m-Flags, die das Gate an der CompileFn-Naht fuer die gegebene Grob-Route beisteuert.
// Freigegeben -> die effektiven Flags; NotApplicable/Abgelehnt -> LEER (CompileFn byte-identisch). Heute stets leer.
// E-10 Schritt 3a: die alte parameterlose Form (globale Hooks) ist ENTFERNT -- required/meaningful kommen
// als PER-BINARY-Aggregate herein (organ_required_for_axes/organ_meaningful_for_axes der Binary-Achsen).
[[nodiscard]] inline std::vector<std::string>
gate_extra_march_flags_for_build(std::span<SimdFeatureFlag const> organ_required,
                                 std::span<SimdFeatureFlag const> organ_meaningful, SimdRoute route,
                                 SimdDialect dialect = SimdDialect::Gpp) {
    SimdGateResult const r = pruef_dock(organ_required, organ_meaningful, active_machine_signature(), route, dialect);
    return effective_march_flags(r, dialect);
}

// =================================================================================================
// §70.9 -- SICHTBARKEIT DES GATE-BEITRAGS IN DER IDENTITAET
// =================================================================================================
// Owner-Auflage §70.9 (aus dem C-3b-Beleg): "die Gate-Beitraege MUESSEN bei der Scharfschaltung in der
// Identitaet sichtbar werden (Sidecar/Stempel)". GRUND: die Gate-Flags sind DISJUNKT zum Codegen-Kanal
// ({-mgfni,-mavx512bitalg,-mavx512vpopcntdq} gegen {-mavx512f}), und die Sub-Feature-Flags implizieren
// ihrerseits AVX512F. Ohne Sichtbarkeit koennten zwei Binaries mit demselben Etikett "+ext=avx512"
// unterschiedlich compiliert sein.
//
// WAS HIER STEHT UND WAS NICHT (Manager-Entscheid Weiche 1 = Variante A): hier steht die kanonische
// TEXT-REPRAESENTATION des Beitrags -- eine reine Funktion, ohne Konsumenten. Die EINHAENGUNG in den
// build_version-Suffix (§70.6: System-Achsen tragen die Build-Version als Stempel-Variable) passiert
// ABSICHTLICH NICHT hier: der Suffix ist Lane-F-Gebiet (profile_run_facade.cpp:369-405,
// artifact_cache.hpp:245-249) und R3 fuehrt ihn gerade auf die EINE Traeger-Datei
// system_version_suffix.hpp zusammen. Ein fuenfter Beitragsort unmittelbar davor waere gegen §73.1.
//
// [NACHGEFUEHRT 2026-08-11, Task #61 -- DER O-8-PUNKT IST GESCHLOSSEN. Der Absatz darueber beschreibt
// weiter richtig, WARUM die Einhaengung nicht hier steht; seine offene Marke ist ueberholt. Die
// Einhaengung existiert seit dbdd2f9b8 ("T2-B", 2026-08-06) DREIFACH, und zwar genau dort, wo dieser
// Absatz sie verortet hat -- in Lane-F-Gebiet, nicht als fuenfter Beitragsort hier:
//   (1) system_version_suffix.hpp:70/88 -- der Suffix-Beitrag (O-8 Schritt 10),
//   (2) das Toolchain-Glied [5] Key[7]="gate" (abi/toolchain_stamp_glied.hpp) ueber
//       compose_toolchain_stamp_glied_for_perm (profile_facade/toolchain_stamp_naht.hpp),
//   (3) derselbe Wert im CEB-Laufzeit-Zwilling -- aus EINEM Aufruf, damit Bau-Kanal und Zwilling nicht
//       driften koennen (profile_run_entry.hpp, Perm-Schleife).
// Gebildet wird der Beitrag PER PERMUTATION (gate_contribution_identity_text), nicht lauf-konstant.
//
// DASS DER WERT HEUTE LEER IST, IST KEINE LUECKE, SONDERN DIE KORREKTE IDENTITAET: active_organ_required()
// ist {} und der C-3a-TRIPWIRE oben (:272-278) macht diese Gleichung compile-hart -- die erste
// required-Deklaration bricht den Bau, statt einen falschen Stempel entstehen zu lassen. Ein LEERES
// Segment ist damit die wahre Aussage "das Gate hat nichts beigetragen", nicht eine fehlende Aussage.
// Wer den Wert befuellen will, braucht laut Tripwire-Text zuerst den Owner-Paket-Entscheid.]
//
// [E-10/ORG-19 SCHRITT 3 (26.08.2026), supersedierend zum Absatz direkt darueber: active_organ_required()
// und der alte Tripwire existieren nicht mehr -- der Umschlag oben ist die POSITIVE per-Binary-Zusage.
// Die FLAGS entstehen seither PER BINARY in der per-Job-CompileFn (job.binary_id -> ceb_parse_path ->
// gate_for_binary); der Glied-[5]-/Suffix-WERT bleibt per Perm DURCHGEREICHT (T2-B-Naht-Doktrin) und
// traegt die leere INVARIANTE des EINEN Helfers: er ist fuer JEDE Binary compile-hart identisch leer
// (produktions_required_aggregat_ist_heute_leer, simd_organ_requirement.hpp). Die erste ECHTE
// required-Deklaration bricht dort den Bau und erzwingt den per-Binary-Nachzug des Glied-Kanals;
// zusaetzlich bricht die per-Job-CompileFn fail-closed, wenn ihr Beitrag nicht im Glied steht.]
//
// STABILITAET: die Ausgabe ist SORTIERT (nach cpuinfo-Namen), damit sie nicht von der Katalog- oder
// Signatur-Reihenfolge abhaengt -- eine Stempel-Variable darf nicht wackeln, wenn jemand eine
// Tabellen-Zeile verschiebt. LEERER String = das Gate hat nichts beigetragen (heutiger Stand).
/// Die reine Formatierung -- getrennt von der Quelle, damit die Stabilitaets-Zusage (Sortierung,
/// Klammerung, Leer-Fall) unabhaengig vom Gate-Zustand pruefbar ist.
[[nodiscard]] inline std::string format_gate_contribution(std::vector<std::string> flags) {
    if (flags.empty()) return {}; // kein Beitrag -> kein Segment (Ist-Stand, byte-neutral)
    std::sort(flags.begin(), flags.end());
    std::string out = "gate=[";
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i != 0) out += ',';
        out += flags[i];
    }
    out += ']';
    return out;
}

// E-10 Schritt 3a: parametrisierte Form (per-Binary-Aggregate herein; alte Route-only-Form ENTFERNT).
[[nodiscard]] inline std::string gate_contribution_identity_text(std::span<SimdFeatureFlag const> organ_required,
                                                                 std::span<SimdFeatureFlag const> organ_meaningful,
                                                                 SimdRoute                        route,
                                                                 SimdDialect dialect = SimdDialect::Gpp) {
    return format_gate_contribution(gate_extra_march_flags_for_build(organ_required, organ_meaningful, route, dialect));
}

/// Traegt das Gate fuer DIESE per-Binary-Aggregate auf irgendeiner Route etwas bei? Die eine Frage,
/// die der Stempel-/Sidecar-Weg stellen muss, bevor er ein Segment anlegt. Heute: nein.
/// E-10 Schritt 3a: ersetzt gate_contributes_anything() (globale Hooks; ENTFERNT).
[[nodiscard]] inline bool gate_contributes_for(std::span<SimdFeatureFlag const> organ_required,
                                               std::span<SimdFeatureFlag const> organ_meaningful,
                                               SimdDialect                      dialect = SimdDialect::Gpp) {
    for (auto route : {SimdRoute::NoExtension, SimdRoute::Avx2, SimdRoute::Avx512})
        if (!gate_extra_march_flags_for_build(organ_required, organ_meaningful, route, dialect).empty()) return true;
    return false;
}

// =================================================================================================
// E-10/ORG-19 SCHRITT 3 -- gate_for_binary: DER EINE HELFER DES PER-BINARY-GATE-WEGS
// =================================================================================================
// Aus den (achse,wert)-Paaren EINER Binary (job.binary_id -> ex::ceb_parse_path) werden required und
// meaningful per Binary aggregiert (18+1-Register: 18er-Tabelle + Organ-Meta-Meta-Zeile des
// Traeger-Paares) und durch das Pruef-Dock zu {flags, identity_text} verdichtet. Flags und Text
// stammen aus DEMSELBEN Aufruf -- CompileFn-Naht (rsp-Zeile), Glied [5] und build_version-Suffix
// koennen damit nicht auseinanderlaufen. Heute (beide Register ohne required) sind beide Felder fuer
// JEDE Binary leer -> byte-neutral; nur der RECHENWEG ist per Binary (Designplan a.5 B-3).
struct SimdGateBinaryContribution {
    std::vector<std::string> flags;         ///< die -m-Flags fuer die per-Job-Compile-/rsp-Zeile
    std::string              identity_text; ///< "gate=[...]" oder "" (Glied-[5]-/Suffix-Segment)
};

/// Kern mit injizierbarem Meta-Meta-Register UND injizierbarer Maschinen-Signatur -- fuer die
/// CT-/Test-Probe (2c-Probe-Register, Prod-Signaturen) OHNE Host-Bindung. Produktion: Huelle unten.
[[nodiscard]] inline SimdGateBinaryContribution
gate_for_binary_in(std::span<OrganMetaMetaRequirement const>            meta_meta_register,
                   std::span<SimdFeatureFlag const>                     machine_signature,
                   std::span<std::pair<std::string, std::string> const> binary_axes, SimdRoute route,
                   SimdDialect dialect = SimdDialect::Gpp) {
    std::vector<std::pair<std::string_view, std::string_view>> sichten;
    sichten.reserve(binary_axes.size());
    for (auto const& [axis, value] : binary_axes) sichten.emplace_back(axis, value);
    SimdFlagMenge const        req  = aggregate_required_for_axes_kern(sichten, meta_meta_register);
    SimdFlagMenge const        mean = aggregate_meaningful_for_axes_kern(sichten, meta_meta_register);
    SimdGateResult const       r    = pruef_dock(req.als_span(), mean.als_span(), machine_signature, route, dialect);
    SimdGateBinaryContribution out;
    out.flags         = effective_march_flags(r, dialect);
    out.identity_text = format_gate_contribution(out.flags);
    return out;
}

/// DIE Produktions-Form: Produktions-Register (kOrganMetaMetaRequirement) + aktive Maschinen-Signatur.
/// Aufrufer: Orchestrator-Admission (aggregate direkt), per-Job-CompileFn der Fassade (Schritt 3c),
/// Glied-[5]-/Suffix-Kanal (leere-Achsen-INVARIANTE, s. Kommentar am 70.9-Block oben).
[[nodiscard]] inline SimdGateBinaryContribution
gate_for_binary(std::span<std::pair<std::string, std::string> const> binary_axes, SimdRoute route,
                SimdDialect dialect = SimdDialect::Gpp) {
    return gate_for_binary_in(kOrganMetaMetaRequirement, active_machine_signature(), binary_axes, route, dialect);
}

// -- Wohlgeformtheit + State-Pattern-Uebergaenge (alles compile-time) -----------------------------
static_assert(route_allows(SimdRoute::Avx512, SimdFlagTier::Avx512));
static_assert(!route_allows(SimdRoute::Avx2, SimdFlagTier::Avx512)); // 256-bit-Route sperrt AVX-512
static_assert(route_allows(SimdRoute::Avx2, SimdFlagTier::Avx256) &&
              route_allows(SimdRoute::Avx2, SimdFlagTier::Companion));
static_assert(!route_allows(SimdRoute::NoExtension, SimdFlagTier::Avx256));
static_assert(route_of_simd_id("avx512") == SimdRoute::Avx512 &&
              route_of_simd_id("no_extension") == SimdRoute::NoExtension);
static_assert(route_of_march_flag("-mavx512f") == SimdRoute::Avx512 &&
              route_of_march_flag("-mavx2") == SimdRoute::Avx2);
static_assert(route_of_march_flag("") == SimdRoute::NoExtension);
// leere required-Menge -> NotApplicable, KEINE effektiven Flags (byte-/golden-neutral, Ist-Verhalten):
static_assert(pruef_dock({}, {}, {}, SimdRoute::Avx512).state == SimdGateState::NotApplicable);
static_assert(pruef_dock({}, {}, {}, SimdRoute::Avx512).effective_count == 0);
// Section 37 per-Binary-Zulassung: leere Anforderung -> kein Fehler (byte-neutral); fehlendes Flag -> D1.
static_assert(!admit_organ_on_machine({}, {}).has_value());

} // namespace comdare::cache_engine::measurement
