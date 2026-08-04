#pragma once
// E-24 C6-V (2026-08-04, ABI-neutraler Vor-Baustein des b-Teils, Entscheid E5 LEDGER:3828) --
// ObservableGrowth<Policy>: die ObservableAxis-Huelle um die SEQUENCE-eigene Achse growth_policy.
// Katalog-Anker: docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md, Zeile C-A
// (Mindest-Feldsatz GrowthStatistics, 7 Felder) + Sektion 2 Punkt 1/2.
//
// MUSTER: exakt ObservableIndexOrg<Strategy> (axes/index_organization/axis_01_index_organization_observable.hpp:
// 60-134). Die Policy selbst (DoublingGrowth sequence_composition.hpp:24-30, GoldenRatio/FixedChunk/Exact
// topics/sequence/axis_growth/axis_growth_policies.hpp) traegt die verhaltens-tragende Op next_capacity(),
// aber KEIN statistics()/snapshot_t -- sie ist damit keine ObservableAxis, und der C3-Slot der Sequence-Gattung
// bleibt EmptyAxisSnapshot (observer_aggregate.hpp:27-33). Die Mess-Mechanik gehoert deshalb in diese Huelle,
// die next_capacity DURCHREICHT und dabei zaehlt.
//
// GATING exakt nach Praezedenz: snapshot_t/statistics()/reset() nur unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF ist die Huelle ein nackter Pass-Through (kein Zaehler-Footprint) und ObservableAxis<...> == false
// -> observe_axes() liefert fuer den Slot EmptyAxisSnapshot (Release-Pfad, korrekt).
//
// EHRLICHKEIT (honest-100%, #24 Option A -- Praezedenz axis_01...observable.hpp:20-27): jeder Zaehler ist
// die Zahl der REAL ausgefuehrten Achsen-Operationen bzw. die dabei real uebergebenen/zurueckgegebenen
// Groessen. KEIN Zaehler wird aus einer Policy-Eigenschaft (growth_factor()) mal n synthetisiert.
//
// MESS-OBJEKT (Owner-KERN NACHTRAG 3, Katalog 1N.1): gemessen wird der eingestellte ALGORITHMUS (die
// Wachstums-Policy), die Achse ist der Mess-KANAL. elements_copied/bytes_copied sind die vom
// Wachstums-EREIGNIS geforderte Umkopier-Menge -- die Kostenklasse des Algorithmus (ExactGrowth zeigt die
// O(n^2)-Amortisation literal). Sie sind AUSDRUECKLICH NICHT die interne Re-Allokation von std::vector:
// dass die Gattungs-Substrate heute noch ueber std::vector mit std::allocator speichern statt ueber die
// T6-Achse, ist die im Katalog 1N.3 benannte Substitutions-Luecke und ein eigenes Fenster nach C6.
//
// ABGRENZUNG: IN-PROCESS-Form. Die Promotion in die Wire-Ebene (SequenceObserverAggregate<9>) ist C6.

#include "sequence_composition.hpp" // GrowthPolicy-Concept + DoublingGrowth (Wiederverwendung, kein Duplikat)

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

/// GrowthStatistics -- der Mindest-Feldsatz der Achse growth_policy (Katalog Sektion 2 Punkt 2: 7 Felder).
/// NUR uint64 (E1-Konvention: keine double-Felder in einer C6-promotionsfaehigen Form -- der SA-double-Drop
/// observable_tier.hpp:66-68 darf sich auf der Gattungs-Seite NICHT wiederholen). standard_layout +
/// trivially_copyable, damit C6 die Form ohne Umbau in den Gattungs-Wire heben kann.
struct GrowthStatistics {
    std::uint64_t growth_events   = 0; ///< REAL gewaehrte Wachstums-Ereignisse (granted > current)
    std::uint64_t elements_copied = 0; ///< Summe der je Ereignis umzukopierenden Elemente (== current am Ereignis)
    std::uint64_t bytes_copied    = 0; ///< elements_copied * element_bytes (Element-Breite der Gattung)
    std::uint64_t requested_total = 0; ///< Summe der real angeforderten Kapazitaeten (Nenner der Ueberallokation)
    std::uint64_t granted_total   = 0; ///< Summe der real gewaehrten Kapazitaeten (Zaehler der Ueberallokation)
    std::uint64_t final_capacity  = 0; ///< zuletzt gewaehrte Kapazitaet (Endstand der Achse)
    std::uint64_t peak_slack      = 0; ///< max(granted - requested) ueber alle Ereignisse (Ueberallokations-Spitze)

    [[nodiscard]] bool operator==(GrowthStatistics const&) const noexcept = default;
};

/// ObservableGrowth<Policy, ElementBytes> -- ObservableAxis-Huelle um eine GrowthPolicy.
/// ElementBytes = Breite eines Sequence-Elements (Default sizeof(std::uint64_t) == SequenceAnatomy::element_type,
/// sequence_anatomy.hpp:85). Sie steht als Template-Parameter, damit bytes_copied eine ECHTE Groesse der
/// gemessenen Gattung ist und nicht geraten wird.
template <class Policy, std::size_t ElementBytes = sizeof(std::uint64_t)>
    requires GrowthPolicy<Policy>
class ObservableGrowth {
public:
    using policy_type = Policy;

    /// Element-Breite, mit der bytes_copied gerechnet wird (Deklaration statt Annahme).
    static constexpr std::size_t element_bytes = ElementBytes;

    /// Die verhaltens-tragende Achsen-Op, DURCHGEREICHT (Drop-in fuer sequence_anatomy.hpp:96-100).
    /// Gezaehlt wird nur ein REAL gewaehrtes Wachstum (granted > current) -- ein Aufruf, der die Kapazitaet
    /// nicht erhoeht, ist kein Wachstums-Ereignis und wird nicht als eines gezaehlt.
    /// const + mutable stats_: die Praezedenz der Gattungs-Anatomien (view_anatomy.hpp:179, set_anatomy.hpp:301)
    /// -- die Beobachtung darf die Konstheit der Achsen-Op nicht brechen.
    [[nodiscard]] std::size_t next_capacity(std::size_t current, std::size_t requested) const noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t const granted = policy_.next_capacity(current, requested);
        if (granted > current) {
            ++stats_.growth_events;
            // current == Element-Ist am Ereignis (Aufrufstelle: data_.size() == capacity_), also die Zahl der
            // Elemente, die dieses Wachstums-Ereignis real umzukopieren verlangt. Kein Flag-abgeleiteter Wert.
            stats_.elements_copied += static_cast<std::uint64_t>(current);
            stats_.bytes_copied += static_cast<std::uint64_t>(current) * static_cast<std::uint64_t>(ElementBytes);
            stats_.requested_total += static_cast<std::uint64_t>(requested);
            stats_.granted_total += static_cast<std::uint64_t>(granted);
            stats_.final_capacity = static_cast<std::uint64_t>(granted);
            std::uint64_t const slack =
                (granted > requested) ? static_cast<std::uint64_t>(granted - requested) : std::uint64_t{0};
            if (slack > stats_.peak_slack) stats_.peak_slack = slack;
        }
        return granted;
#else
        return policy_.next_capacity(current, requested);
#endif
    }

    /// Concept-Member der Achse, durchgereicht (GrowthPolicy verlangt beide Ops -- eine Huelle, die nur die
    /// halbe Achse forwarded, faellt aus dem Concept; Reference: Observable-Wrapper muss Concept-Member forwarden).
    [[nodiscard]] double growth_factor() const noexcept { return policy_.growth_factor(); }

    /// E1 (Skalierungs-Konvention double -> uint64 MILLI-Fixpunkt, LEDGER:3828): growth_factor() ist die
    /// EINZIGE double-Groesse dieser Achse. Der Mindest-Feldsatz (7) fuehrt sie NICHT als POD-Feld; diese
    /// Huelle haelt die E1-Form bereit, damit C6 sie ohne neue Konvention promoten kann (x1000, Suffix _milli,
    /// Praezedenz avg_density_milli/frag_milli). FixedChunkGrowth meldet 0.0 als additiv-Sentinel -> 0.
    [[nodiscard]] std::uint64_t growth_factor_milli() const noexcept {
        double const f = policy_.growth_factor();
        return (f <= 0.0) ? std::uint64_t{0} : static_cast<std::uint64_t>((f * 1000.0) + 0.5);
    }

    /// Zugriff auf die getragene Policy (Diagnose/Treiber; die Huelle versteckt den Algorithmus nicht).
    [[nodiscard]] policy_type&       policy() noexcept { return policy_; }
    [[nodiscard]] policy_type const& policy() const noexcept { return policy_; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = GrowthStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }
#endif

private:
    // Alle Ist-Policies sind leer; der Member kostet damit nur Padding (zero-cost-Doktrin gewahrt).
    policy_type policy_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable GrowthStatistics stats_{};
#endif
};

} // namespace comdare::cache_engine::anatomy
