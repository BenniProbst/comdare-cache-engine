// measurement/target_isa_complex_axis.hpp -- target_isa als KOMPLEX-Achse IN SICH
// (O-8 Schritt 6 / Bauplan IV.2.2; Owner OD-2, Ledger 3521).
//
// OWNER-WORTLAUT (OD-2, verbatim): "Die target_isa ist auch eine Komplex-Achse in sich, die rekursiv
// durch die Komplex-Achse target_isa x operating_system x ... gewrappt wird. Die target_isa bildet
// also eine feste Rekombination aus RAM-Frequenz und CAS und CPU-Fabrikation als 'neue statische
// Komplex-Haupt-System-Achse', die allein als Komplex-Wrapper die zugehoerigen target_isa
// Unter-System-Achsen erhaelt, die zuvor vereinbart waren. Also nichts von deinen Vorschlaegen."
//
// Der Schluss-Satz schliesst eine Laufzeit-Unter-Achsen-Loesung AUS. Die drei Glieder sind deshalb
// STATISCHE constexpr-Konstanten je benannter Auspraegung, keine RT-Werte.
//
// -- WAS EINE AUSPRAEGUNG IST -------------------------------------------------------------------
// Eine Auspraegung ist eine BENANNTE REKOMBINATION: eine Ziel-ISA plus die drei Maschinen-Glieder.
// Die Glieder werden hier NICHT wiederholt, sondern aus der einen Deklarations-Quelle GEZOGEN:
// kDeclaredMachines (machine_identity.hpp), die ihrerseits woertlich aus <machines> der Anwender-XML
// stammt (O-4b, Schritt 5). Eine Auspraegung nennt daher nur den Eigenschafts-SCHLUESSEL
// (cpu_fabrication + ram_pair) und bekommt die Glieder ueber resolve_machine_by_properties --
// Ein-Kanal-Doktrin (Ledger 73.1): es entsteht KEINE zweite Maschinen-Tabelle.
//
// -- DIE DREI GLIEDER ---------------------------------------------------------------------------
//   ram_frequency_mhz  <- <machine ram_frequency_mhz=..>  (0 = nicht deklariert)
//   cas_latency_cl     <- <machine cas_latency_cl=..>     (0 = nicht deklariert)
//   cpu_fabrication    <- das O-4a-Kern-Tupel vendor/family/model/stepping (MachineCoreCpuId).
// Das dritte Glied ist AUSDRUECKLICH nicht das symbolische XML-Etikett gleichen Namens: jenes ist
// (mit ram_pair) der Identitaets-SCHLUESSEL, ueber den hier aufgeloest wird. Die Fabrikation selbst
// ist das Tupel. Wer die beiden verwechselt, macht den Schluessel zum Wert und bricht die Aufloesung.
//
// -- RF-7: EIN FELD, NICHT DREI ------------------------------------------------------------------
// Der Komplex ist EIN Feld im System-Haupt-Achsen-Array (je Achsen-Typ EINE Array-Stempel-Zeile) --
// KEINE Klammer-Explosion je Glied. Deshalb traegt jede Auspraegung dasselbe axis_label()
// "target_isa": die Glieder sind Inhalt EINER Achse, nicht drei Achsen. Der static_assert am
// Dateiende haelt genau das fest.
//
// -- WIRKUNG (RF-6) ------------------------------------------------------------------------------
// NUR Stempel-Identitaet: gleiche Kombination => gleicher Stempel. KEINE Laufzeit-/Dispatch-Wirkung,
// binary_id="never". Ein nicht deklariertes Glied wird als nicht deklariert gestempelt, NIE geraten.
//
// Metaprog: CRTP + Concept, static-dispatch, keine vtable (Muster TargetIsaSystemAxis/OperatingSystemAxis).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <cache_engine/measurement/machine_identity.hpp>
#include <cache_engine/measurement/target_isa_system_axis.hpp>

#include <array>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// Familien-Repraesentant der ACHSE target_isa fuer Typ-Eltern-Bezuege (CebSubAxis<..., TargetIsaAxisTag>).
/// KEINE Auspraegung und kein Baustein: er traegt ausschliesslich das Achsen-Label, damit sich die
/// Unter-Achsen auf die ACHSE beziehen koennen statt auf eine willkuerlich herausgegriffene
/// Auspraegung. Das Label wird NICHT wiederholt, sondern aus der bestehenden Single-Source gezogen.
struct TargetIsaAxisTag final : CebSystemAxis<TargetIsaAxisTag> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return X86_64TargetIsa::axis_label(); }
};

/// Der innere Komplex-Wrapper. `IsaAxis` ist die gewrappte Ziel-ISA-Auspraegung (Typ, nicht String);
/// `Derived` nennt den Eigenschafts-Schluessel, ueber den die drei Glieder aufgeloest werden.
template <class Derived, class IsaAxis>
struct TargetIsaComplexAxis : CebSystemAxis<Derived> {
    /// Die gewrappte Ziel-ISA als TYP -- die Rekursion "Komplex wrappt Achse" ist damit maschinen-pruefbar.
    using isa_axis = IsaAxis;

    /// Der Komplex IST die Achse target_isa (RF-7: EIN Feld). Label aus der gewrappten Achse abgeleitet.
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return IsaAxis::axis_label(); }

    /// Name DIESER Rekombination (Baustein-Etikett im Registry-Block, Stempel-Auspraegung).
    [[nodiscard]] static constexpr std::string_view complex_id() noexcept { return Derived::do_complex_id(); }

    /// Die gewrappte Ziel-ISA-Kennung ("x86_64" | "aarch64") -- durchgereicht, nicht kopiert.
    [[nodiscard]] static constexpr std::string_view target_isa_id() noexcept { return IsaAxis::target_isa_id(); }

    /// Die aufgeloeste Deklarations-Zeile. nullptr = der Schluessel dieser Auspraegung steht in KEINER
    /// <machines>-Deklaration; dann ist die Rekombination unvollstaendig und darf nicht stempeln.
    [[nodiscard]] static constexpr DeclaredMachine const* declared_machine() noexcept {
        return resolve_machine_by_properties(Derived::do_cpu_fabrication_key(), Derived::do_ram_pair_key());
    }

    // -- Die DREI festen Glieder (constexpr, aus der Deklaration gezogen) --
    [[nodiscard]] static constexpr std::uint32_t ram_frequency_mhz() noexcept {
        auto const* m = declared_machine();
        return m != nullptr ? m->ram_frequency_mhz : 0U;
    }
    [[nodiscard]] static constexpr std::uint32_t cas_latency_cl() noexcept {
        auto const* m = declared_machine();
        return m != nullptr ? m->cas_latency_cl : 0U;
    }
    /// Glied 3: das O-4a-Kern-Tupel. `declared == false` heisst NICHT deklariert (kein Raten).
    [[nodiscard]] static constexpr MachineCoreCpuId cpu_fabrication() noexcept {
        auto const* m = declared_machine();
        return m != nullptr ? m->core : MachineCoreCpuId{};
    }

    /// Traegt die Rekombination alle drei Glieder? Ehrliche Teil-Deklaration ist erlaubt und bleibt
    /// sichtbar -- diese Frage ist der Ort, an dem der Stempel-Weg sie stellen kann.
    [[nodiscard]] static constexpr bool has_all_members_declared() noexcept {
        return ram_frequency_mhz() != 0U && cas_latency_cl() != 0U && cpu_fabrication().declared;
    }

protected:
    constexpr TargetIsaComplexAxis() noexcept = default;
};

template <class A>
concept TargetIsaComplexAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, TargetIsaComplexAxis<A, typename A::isa_axis>> &&
    TargetIsaSystemAxisConcept<typename A::isa_axis> &&
    std::is_empty_v<TargetIsaComplexAxis<A, typename A::isa_axis>> &&
    (!std::is_polymorphic_v<TargetIsaComplexAxis<A, typename A::isa_axis>>) && requires {
        { A::complex_id() } -> std::same_as<std::string_view>;
        { A::target_isa_id() } -> std::same_as<std::string_view>;
        { A::ram_frequency_mhz() } -> std::same_as<std::uint32_t>;
        { A::cas_latency_cl() } -> std::same_as<std::uint32_t>;
        { A::cpu_fabrication() } -> std::same_as<MachineCoreCpuId>;
    };

// -- Die benannten Rekombinationen. Je deklarierter Maschinen-KLASSE eine; der Schluessel steht
//    woertlich so in <machines> (experiment_golden_kern.xml). Eine neue Maschine tritt hier als
//    dritte Auspraegung ein, NACHDEM sie in kDeclaredMachines und in der XML steht. --

/// prod1-Klasse: x86_64 + Zen5-Fabrikation. RAM-Glieder heute nicht deklariert (siehe kDeclaredMachines).
struct Prod1Zen5TargetIsa final : TargetIsaComplexAxis<Prod1Zen5TargetIsa, X86_64TargetIsa> {
    [[nodiscard]] static constexpr std::string_view do_complex_id() noexcept { return "prod1_zen5"; }
    [[nodiscard]] static constexpr std::string_view do_cpu_fabrication_key() noexcept { return "amd_zen5_avx512"; }
    [[nodiscard]] static constexpr std::string_view do_ram_pair_key() noexcept { return "ddr5_2x32"; }
};

/// prod2-Klasse: x86_64 + Intel-Fabrikation. Kern-Tupel und RAM-Glieder heute nicht deklariert --
/// die Auspraegung existiert trotzdem, weil die KLASSE deklariert ist; sie stempelt ehrlich unvollstaendig.
struct Prod2RaptorLakeTargetIsa final : TargetIsaComplexAxis<Prod2RaptorLakeTargetIsa, X86_64TargetIsa> {
    [[nodiscard]] static constexpr std::string_view do_complex_id() noexcept { return "prod2_raptor_lake"; }
    [[nodiscard]] static constexpr std::string_view do_cpu_fabrication_key() noexcept { return "intel_avx2"; }
    [[nodiscard]] static constexpr std::string_view do_ram_pair_key() noexcept { return "ddr4_2x32"; }
};

/// CEB-Default = prod1-Klasse (die Bau-/Mess-Flotte laeuft heute dort) -- beweglicher Startwert, KEIN Pin.
using DefaultTargetIsaComplex = Prod1Zen5TargetIsa;

/// Single-Source der gueltigen Rekombinations-Ids (analog kAllTargetIsaIds/kAllOperatingSystemIds).
inline constexpr std::array<std::string_view, 2> kAllTargetIsaComplexIds = {Prod1Zen5TargetIsa::complex_id(),
                                                                            Prod2RaptorLakeTargetIsa::complex_id()};

static_assert(TargetIsaComplexAxisConcept<Prod1Zen5TargetIsa>);
static_assert(TargetIsaComplexAxisConcept<Prod2RaptorLakeTargetIsa>);
static_assert(TargetIsaAxisTag::axis_label() == std::string_view{"target_isa"});

// RF-7-WACHE: EIN Feld, nicht drei. Alle Auspraegungen des Komplexes tragen DASSELBE Achsen-Label --
// die drei Glieder sind Inhalt EINER Achse. Wer einem Glied ein eigenes axis_label() gibt, macht aus
// dem Komplex drei Achsen und bricht hier compile-time.
static_assert(Prod1Zen5TargetIsa::axis_label() == TargetIsaAxisTag::axis_label());
static_assert(Prod2RaptorLakeTargetIsa::axis_label() == TargetIsaAxisTag::axis_label());
static_assert(Prod1Zen5TargetIsa::axis_label() == Prod2RaptorLakeTargetIsa::axis_label());

// AUFLOESUNGS-WACHE: jede Auspraegung MUSS in kDeclaredMachines landen. Ein Tippfehler im Schluessel
// haengt die Rekombination sonst still ins Leere und stempelt lauter Nullen.
static_assert(Prod1Zen5TargetIsa::declared_machine() != nullptr,
              "Prod1Zen5TargetIsa: der Eigenschafts-Schluessel steht in keiner <machines>-Deklaration "
              "(kDeclaredMachines, machine_identity.hpp). Schluessel und Deklaration muessen zusammen "
              "gepflegt werden -- eine Maschine tritt IMMER doppelt ein.");
static_assert(Prod2RaptorLakeTargetIsa::declared_machine() != nullptr, "Prod2RaptorLakeTargetIsa: siehe oben.");
static_assert(Prod1Zen5TargetIsa::declared_machine() != Prod2RaptorLakeTargetIsa::declared_machine(),
              "Zwei Rekombinationen duerfen nicht auf dieselbe Deklarations-Zeile zeigen.");

// EHRLICHKEITS-ANKER (Ist-Stand, kein Soll): das Kern-Tupel ist heute NUR fuer prod1 erhoben, die
// RAM-Glieder fuer KEINE Maschine (Ledger #49: Erhebung beim Infra-Agenten). Diese Asserts sind die
// Stelle, an der eine spaetere Nachdeklaration bewusst quittiert werden muss, statt still zu wirken.
static_assert(Prod1Zen5TargetIsa::cpu_fabrication().declared,
              "prod1 hat eine erhobene CPU-Kern-Kennung (O-4, live gegengeprueft).");
// O-8 Schritt 5b: der O-4a-Nachzug ist VOLLZOGEN -- prod2s Kern-Kennung ist erhoben und deklariert.
// Diese Zeile stand vorher invertiert hier und hat den Nachzug erzwungen, statt ihn zu vergessen.
static_assert(Prod2RaptorLakeTargetIsa::cpu_fabrication().declared,
              "prod2s Kern-Kennung ist mit O-4a erhoben (GenuineIntel/6/151/2). Faellt sie wieder weg, "
              "ist das ein Rueckschritt und keine Bereinigung.");
static_assert(Prod1Zen5TargetIsa::cpu_fabrication().vendor != Prod2RaptorLakeTargetIsa::cpu_fabrication().vendor,
              "Die beiden Rekombinationen muessen sich im CPU-Fabrikations-Glied unterscheiden -- sonst "
              "waere eine der beiden Deklarationen aus der anderen abgeschrieben.");
static_assert(!Prod1Zen5TargetIsa::has_all_members_declared() && !Prod2RaptorLakeTargetIsa::has_all_members_declared(),
              "Solange RAM-Frequenz/CAS nicht erhoben sind, ist KEINE Rekombination vollstaendig. Wer "
              "diese Zeile bricht, hat Werte nachdeklariert -- dann gehoert sie angepasst, nicht entfernt.");

} // namespace comdare::cache_engine::measurement
