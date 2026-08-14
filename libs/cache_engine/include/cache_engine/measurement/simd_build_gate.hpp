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

// -- Hook 1/3: die AKTIVEN Organ-Anforderungen ----------------------------------------------------
// Die Fassaden-Naht kennt KEINEN Organ-Kontext (sie sieht nur opt_flag + march_flag), der per-Binary
// genaue Weg liegt allein im Orchestrator (aggregate_required_for_axes ueber spec.axes). Dieser Hook
// ist deshalb die VEREINIGUNG ueber alle Organ-Klassen. Sie ist heute nachweislich LEER -- und solange
// sie leer ist, IST {} exakt die Vereinigung. Der Tripwire darunter erzwingt, dass diese Gleichung gilt.
[[nodiscard]] inline std::span<SimdFeatureFlag const> active_organ_required() noexcept { return {}; }

namespace detail {
[[nodiscard]] consteval std::size_t organ_required_union_size() {
    std::size_t n = 0;
    for (auto const& e : kSimdOrganRequirement) n += e.required.size();
    return n;
}
} // namespace detail
static_assert(detail::organ_required_union_size() == 0,
              "C-3a-TRIPWIRE: ein Organ deklariert jetzt required-Flags, damit ist active_organ_required() "
              "als leere Vereinigung FALSCH. Die globalen Hooks sind eine VEREINIGUNG ueber alle "
              "Organ-Klassen -- jede Binary bekaeme die Flags, auch die, die das Organ gar nicht nutzt. "
              "Per-Binary-Genauigkeit noetig: der Orchestrator-Weg (build_orchestrator.hpp "
              "aggregate_required_for_axes, Bauplan D3.4 UNBERUEHRT) braucht VOR der ersten "
              "required-Deklaration einen eigenen Owner-Paket-Entscheid.");

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
[[nodiscard]] inline std::vector<std::string> gate_extra_march_flags_for_build(SimdRoute   route,
                                                                               SimdDialect dialect = SimdDialect::Gpp) {
    SimdGateResult const r =
        pruef_dock(active_organ_required(), active_organ_meaningful(), active_machine_signature(), route, dialect);
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

[[nodiscard]] inline std::string gate_contribution_identity_text(SimdRoute   route,
                                                                 SimdDialect dialect = SimdDialect::Gpp) {
    return format_gate_contribution(gate_extra_march_flags_for_build(route, dialect));
}

/// Traegt das Gate auf IRGENDEINER Route etwas bei? Die eine Frage, die der Stempel-/Sidecar-Weg
/// stellen muss, bevor er ein Segment anlegt. Heute: nein.
[[nodiscard]] inline bool gate_contributes_anything(SimdDialect dialect = SimdDialect::Gpp) {
    for (auto route : {SimdRoute::NoExtension, SimdRoute::Avx2, SimdRoute::Avx512})
        if (!gate_extra_march_flags_for_build(route, dialect).empty()) return true;
    return false;
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
