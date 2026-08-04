#pragma once
// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3,
// Liefergegenstand (e)) -- AdapterExecutionContext<Composition>.
//
// Die builder-seitige TREIBER-Flaeche der Adapter-Tier-Unterklasse, Schnitt und Begruendung identisch zu den
// Geschwistern set_/sequence_/view_execution_context.hpp: EIN Kontext-Objekt haelt die Anatomie, bietet die
// Gattungs-Operationen an und nimmt BEIDE Beobachtungs-Ebenen ab (observe_all + observe_axes).
//
// ADAPTER-BESONDERHEIT (benannt, nicht wegabstrahiert): die Disziplin FIFO/LIFO/Priority liegt in der
// API-NUTZUNG (front vs. back), NICHT in einer Achse -- Doku 14 Paragraf 28 kennt keine "ordering"-Achse
// (adapter_anatomy.hpp:10-12). Dieser Kontext reicht deshalb BEIDE Enden durch und fuehrt KEINEN
// Disziplin-Schalter ein; ein solcher waere genau die verworfene inner+ordering-Version.
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only, keine Beruehrung von abi_adapter.hpp, den
// Wire-PODs (adapter_tier.hpp), abi/*_decl.hpp oder den Stempel-/Fingerprint-Flaechen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md Paragraf 17.3 + 24 (+ 26.4 Adapter-API)

#include <anatomy/adapter_anatomy.hpp> // AdapterAnatomy / AdapterObserverSnapshot / AdapterAxisObservation

#include <cstddef>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana = ::comdare::cache_engine::anatomy;

/// AdapterExecutionContext<Composition> -- Builder-seitiger Halter + Treiber der Adapter-Anatomie.
template <class Composition>
class AdapterExecutionContext {
public:
    using anatomy_t          = ana::AdapterAnatomy<Composition>;
    using composition_t      = Composition;
    using element_type       = typename anatomy_t::element_type;
    using observer_t         = ana::AdapterObserverSnapshot;           ///< flache Gattungs-Sicht (Bestand)
    using axis_observation_t = typename anatomy_t::axis_observation_t; ///< per-Achsen-Sicht (E-24 C3)

    AdapterExecutionContext() = default;

    // -- Adapter-API (Paragraf 26.4): push + BEIDE Enden. Die Disziplin waehlt der Aufrufer. --
    void                                      push(element_type v) { anatomy_.push(v); }
    [[nodiscard]] std::optional<element_type> pop_front() { return anatomy_.pop_front(); } ///< FIFO (queue)
    [[nodiscard]] std::optional<element_type> pop_back() { return anatomy_.pop_back(); }   ///< LIFO (stack)
    [[nodiscard]] std::optional<element_type> front() const { return anatomy_.front(); }
    [[nodiscard]] std::optional<element_type> back() const { return anatomy_.back(); }
    void                                      clear() noexcept { anatomy_.clear(); }

    [[nodiscard]] std::size_t size() const noexcept { return anatomy_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return anatomy_.size() == 0; }

    // -- Beobachtung: beide Ebenen an EINER Stelle abnehmbar --
    [[nodiscard]] observer_t                   observe_all() const noexcept { return anatomy_.observe_all(); }
    [[nodiscard]] axis_observation_t           observe_axes() const noexcept { return anatomy_.observe_axes(); }
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return anatomy_t::observable_axis_count();
    }

    /// Zugriff auf die gehaltene Anatomie -- hier laufen die ORGAN-Accessoren (inner_container_organ() usw.).
    [[nodiscard]] anatomy_t&       anatomy() noexcept { return anatomy_; }
    [[nodiscard]] anatomy_t const& anatomy() const noexcept { return anatomy_; }

    // -- Composition-Inspection (durchgereicht); die Adapter-Bindung fuehrt zusaetzlich die Ebene-1-Gattung --
    static constexpr std::string_view    composition_name() noexcept { return anatomy_t::composition_name(); }
    static constexpr std::string_view    paper_id() noexcept { return anatomy_t::paper_id(); }
    static constexpr std::size_t         organ_count() noexcept { return anatomy_t::organ_count(); }
    static constexpr ana::AnatomyGenus   genus() noexcept { return anatomy_t::genus(); }
    static constexpr ana::AnatomyGattung gattung() noexcept { return anatomy_t::gattung(); }

private:
    anatomy_t anatomy_{};
};

} // namespace comdare::cache_engine::builder::anatomy_commands
