#pragma once
// E-24 C6-V (2026-08-04, ABI-neutraler Vor-Baustein des b-Teils, Entscheid E5 LEDGER:3828) --
// die ObservableAxis-Huellen der DREI VIEW-eigenen Achsen extent_policy / layout_policy / accessor_policy.
// Katalog-Anker: docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md, Zeilen C-B/C-C/C-D
// (Mindest-Feldsaetze je 3 Felder) + Sektion 2 Punkt 1/2.
//
// AUSGANGSLAGE (Katalog): DynamicExtent/LayoutRight/DefaultAccessor tragen KEIN statistics()/snapshot_t --
// alle drei C3-Slots der View-Gattung waren EmptyAxisSnapshot (view_anatomy.hpp:44-48). Diese Huellen
// schliessen die Luecke nach dem Muster ObservableIndexOrg<Strategy>
// (axes/index_organization/axis_01_index_organization_observable.hpp:60-134): die Policy bleibt unangetastet,
// die Huelle reicht ihre Concept-Ops DURCH und zaehlt dabei.
//
// GATING exakt nach Praezedenz: snapshot_t/statistics()/reset() nur unter COMDARE_CE_ENABLE_STATISTICS;
// bei OFF nackter Pass-Through (kein Zaehler-Footprint), ObservableAxis<...> == false -> EmptyAxisSnapshot.
//
// EHRLICHKEIT (honest-100%, #24 Option A): jeder Zaehler ist eine REAL ausgefuehrte Operation bzw. eine
// REAL gelieferte Groesse. Insbesondere werden non_contiguous_steps/max_offset_jump aus den TATSAECHLICH
// zurueckgegebenen Offsets gerechnet und NIE aus dem Layout-TYP synthetisiert (Katalog C-C woertlich), und
// unaligned_accesses prueft die REALE Adresse, nicht den Accessor-Namen.
//
// ABGRENZUNG: IN-PROCESS-Formen. Die Promotion in die Wire-Ebene (ViewObserverAggregate<5>) ist C6.

#include "view_composition.hpp" // ExtentPolicy/LayoutPolicy/AccessorPolicy-Concepts + die drei Ist-Defaults

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

// ---------------------------------------------------------------------------
// axis_extent -- ExtentStatistics + ObservableExtent
// ---------------------------------------------------------------------------

/// ExtentStatistics -- Mindest-Feldsatz der Achse extent_policy (Katalog Sektion 2 Punkt 2, 3 Felder).
/// bounds_checks_performed == 0 bei STATISCHER Ausdehnung ist der ELISIONS-BEWEIS (Katalog C-B): die Achse
/// fuehrt dann keinen dynamischen Grenz-Check, und die ehrliche Null ist das Messergebnis, nicht eine Luecke.
struct ExtentStatistics {
    std::uint64_t bounds_checks_performed = 0; ///< real ausgefuehrte DYNAMISCHE Grenz-Pruefungen der Achse
    std::uint64_t oob_rejects             = 0; ///< davon real abgewiesene (off >= gebundene Grenze)
    std::uint64_t rebind_count            = 0; ///< Binde-Ereignisse, die eine BESTEHENDE Bindung ersetzen

    [[nodiscard]] bool operator==(ExtentStatistics const&) const noexcept = default;
};

/// Compile-time-Frage "traegt der Typ die Ausdehnung?" -- beantwortet ueber die additive Marke
/// is_static_extent (DynamicExtent view_composition.hpp, StaticExtent<N> topics/view/view_policies.hpp).
/// is_static() ist eine Laufzeit-Op und taugt fuer ein `if constexpr` NICHT; ein Typ ohne Marke gilt
/// konservativ als dynamisch (dann wird geprueft und gezaehlt -- die sichere Richtung).
template <class P>
[[nodiscard]] consteval bool extent_is_static() noexcept {
    if constexpr (requires {
                      { P::is_static_extent } -> std::convertible_to<bool>;
                  }) {
        return P::is_static_extent;
    } else {
        return false;
    }
}

/// ObservableExtent<Policy> -- ObservableAxis-Huelle um eine ExtentPolicy.
///
/// Die Achse hat am Ist KEINE von der Anatomie getriebene Rechen-Op (der Ausdehnungs-Vertrag ist eine
/// Zusicherung, keine Berechnung). Ihre Mess-Ereignisse sind deshalb die beiden REALEN Ereignisse der
/// View-Gattung, die den Vertrag beruehren: das Binden (bind) und die Grenz-Pruefung im Lesepfad (read).
/// ViewAnatomy meldet sie ueber note_bind/note_bounds_check -- `if constexpr`-gegated, eine nackte Policy
/// ohne diese Member bleibt voellig unberuehrt.
///
/// ABGRENZUNG (wichtig fuer die Auswertung): bounds_checks_performed zaehlt die Grenz-Pruefungen, die die
/// EXTENT-ACHSE verantwortet. Der Speicher-Sicherheits-Guard der Anatomie (data_ == nullptr || off >= size_,
/// view_anatomy.hpp) bleibt davon unberuehrt und ist NICHT Gegenstand dieses Zaehlers.
template <class Policy>
    requires ExtentPolicy<Policy>
class ObservableExtent {
public:
    using policy_type = Policy;

    /// Compile-time-Klasse der getragenen Policy (durchgereichte Marke).
    static constexpr bool is_static_extent = extent_is_static<Policy>();

    // Concept-Member der Achse, durchgereicht (ExtentPolicy verlangt beide).
    [[nodiscard]] bool        is_static() const noexcept { return policy_.is_static(); }
    [[nodiscard]] std::size_t static_extent() const noexcept { return policy_.static_extent(); }

    /// REALES Binde-Ereignis der Gattung. Gezaehlt wird nur das ERSETZENDE Binden (die zweite und jede
    /// weitere Bindung) -- die Gesamtzahl der Bindungen fuehrt der flache Gattungs-POD als bind_count
    /// (view_tier.hpp), hier steht die davon verschiedene Re-Binding-Groesse.
    void note_bind(std::size_t bound) const noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (bound_once_) ++stats_.rebind_count;
        bound_once_ = true;
#endif
        (void)bound;
    }

    /// REALE Grenz-Pruefung im Lesepfad. Bei STATISCHER Ausdehnung fuehrt die Achse sie nicht aus
    /// (die Grenze steht im Typ) -- kein Zaehler-Inkrement, und genau diese ehrliche Null ist der
    /// Elisions-Beweis aus Katalog C-B. Bei dynamischer Ausdehnung wird REAL geprueft und gezaehlt.
    void note_bounds_check(std::size_t off, std::size_t bound) const noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (!is_static_extent) {
            ++stats_.bounds_checks_performed;
            if (off >= bound) ++stats_.oob_rejects;
        }
#endif
        (void)off;
        (void)bound;
    }

    [[nodiscard]] policy_type&       policy() noexcept { return policy_; }
    [[nodiscard]] policy_type const& policy() const noexcept { return policy_; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = ExtentStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_      = {};
        bound_once_ = false;
    }
#endif

private:
    policy_type policy_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable ExtentStatistics stats_{};
    mutable bool             bound_once_ = false;
#endif
};

// ---------------------------------------------------------------------------
// axis_layout -- LayoutStatistics + ObservableLayout
// ---------------------------------------------------------------------------

/// LayoutStatistics -- Mindest-Feldsatz der Achse layout_policy (Katalog Sektion 2 Punkt 2 / Zeile C-C).
/// Alle drei Groessen kommen aus den REAL gelieferten Offsets, NIE aus dem Layout-Typ (Katalog woertlich).
struct LayoutStatistics {
    std::uint64_t index_translations   = 0; ///< real ausgefuehrte index_of()-Uebersetzungen
    std::uint64_t non_contiguous_steps = 0; ///< Uebergaenge mit off != vorheriger off + 1 (Lokalitaets-Verlust)
    std::uint64_t max_offset_jump      = 0; ///< groesster real aufgetretener |off - vorheriger off| (dTLB-/CLU-Proxy)

    [[nodiscard]] bool operator==(LayoutStatistics const&) const noexcept = default;
};

/// ObservableLayout<Policy> -- ObservableAxis-Huelle um eine LayoutPolicy. index_of() ist die verhaltens-
/// tragende Op der Achse (sie bestimmt, WELCHE Speicherzelle ein read(i) trifft) und wird durchgereicht.
/// const + mutable stats_, weil ViewAnatomy::read() const ist (view_anatomy.hpp:90-98).
template <class Policy>
    requires LayoutPolicy<Policy>
class ObservableLayout {
public:
    using policy_type = Policy;

    [[nodiscard]] std::size_t index_of(std::size_t i) const noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t const   off = policy_.index_of(i);
        std::uint64_t const cur = static_cast<std::uint64_t>(off);
        if (stats_.index_translations != 0) {
            if (cur != last_offset_ + 1) ++stats_.non_contiguous_steps;
            std::uint64_t const jump = (cur >= last_offset_) ? (cur - last_offset_) : (last_offset_ - cur);
            if (jump > stats_.max_offset_jump) stats_.max_offset_jump = jump;
        }
        ++stats_.index_translations;
        last_offset_ = cur;
        return off;
#else
        return policy_.index_of(i);
#endif
    }

    [[nodiscard]] policy_type&       policy() noexcept { return policy_; }
    [[nodiscard]] policy_type const& policy() const noexcept { return policy_; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = LayoutStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_       = {};
        last_offset_ = 0;
    }
#endif

private:
    policy_type policy_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable LayoutStatistics stats_{};
    mutable std::uint64_t    last_offset_ = 0;
#endif
};

// ---------------------------------------------------------------------------
// axis_accessor -- AccessorStatistics + ObservableAccessor
// ---------------------------------------------------------------------------

/// AccessorStatistics -- Mindest-Feldsatz der Achse accessor_policy (Katalog Sektion 2 Punkt 2 / Zeile C-D).
/// conversion_ops ist honest-0-faehig: die Ist-Accessoren (DefaultAccessor/AlignedAccessor) liefern das
/// Element unveraendert, also ist 0 das MESSERGEBNIS. Ein konvertierender Accessor meldet seine real
/// ausgefuehrten Konversionen ueber access_counted() zurueck (Muster index_org_scan_counted).
struct AccessorStatistics {
    std::uint64_t access_count       = 0; ///< real ausgefuehrte access()-Zugriffe
    std::uint64_t unaligned_accesses = 0; ///< davon auf einer REAL nicht vertrags-ausgerichteten Adresse
    std::uint64_t conversion_ops     = 0; ///< vom Accessor zurueckgemeldete, real ausgefuehrte Konversionen

    [[nodiscard]] bool operator==(AccessorStatistics const&) const noexcept = default;
};

/// Ausrichtungs-VERTRAG des Accessors: AlignedAccessor<Align> deklariert ihn als `alignment`
/// (topics/view/view_policies.hpp); ohne Deklaration gilt die natuerliche Element-Ausrichtung.
template <class P>
[[nodiscard]] consteval std::size_t accessor_alignment_contract() noexcept {
    if constexpr (requires {
                      { P::alignment } -> std::convertible_to<std::size_t>;
                  }) {
        return P::alignment;
    } else {
        return alignof(std::uint64_t);
    }
}

/// ObservableAccessor<Policy> -- ObservableAxis-Huelle um eine AccessorPolicy. access() ist die verhaltens-
/// tragende Op und wird durchgereicht; die Ausrichtungs-Pruefung laeuft auf der REALEN Zieladresse.
template <class Policy>
    requires AccessorPolicy<Policy>
class ObservableAccessor {
public:
    using policy_type = Policy;

    /// Der gepruefte Ausrichtungs-Vertrag (Deklaration statt Annahme).
    static constexpr std::size_t alignment_contract = accessor_alignment_contract<Policy>();

    [[nodiscard]] std::uint64_t access(std::uint64_t const* d, std::size_t i) const noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.access_count;
        // REALE Adresse gegen den REALEN Vertrag -- kein aus dem Accessor-Namen abgeleiteter Wert.
        auto const addr = reinterpret_cast<std::uintptr_t>(d + i);
        if ((addr % static_cast<std::uintptr_t>(alignment_contract)) != 0) ++stats_.unaligned_accesses;
        if constexpr (requires(Policy const& p, std::uint64_t const* dd, std::size_t ii, std::uint64_t& c) {
                          { p.access_counted(dd, ii, c) } -> std::convertible_to<std::uint64_t>;
                      }) {
            std::uint64_t       conversions = 0;
            std::uint64_t const value       = policy_.access_counted(d, i, conversions);
            stats_.conversion_ops += conversions;
            return value;
        } else {
            // Ist-Accessoren konvertieren nicht -> conversion_ops bleibt ehrlich 0 (Messergebnis, keine Luecke).
            return policy_.access(d, i);
        }
#else
        return policy_.access(d, i);
#endif
    }

    [[nodiscard]] policy_type&       policy() noexcept { return policy_; }
    [[nodiscard]] policy_type const& policy() const noexcept { return policy_; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = AccessorStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }
#endif

private:
    policy_type policy_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable AccessorStatistics stats_{};
#endif
};

} // namespace comdare::cache_engine::anatomy
