#pragma once
// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3,
// Liefergegenstand (e)) -- ViewExecutionContext<Composition>.
//
// Die builder-seitige TREIBER-Flaeche der View-Gattung, Schnitt und Begruendung identisch zu den
// Geschwistern set_/sequence_execution_context.hpp: EIN Kontext-Objekt haelt die Anatomie, bietet die
// Gattungs-Operationen an und nimmt BEIDE Beobachtungs-Ebenen ab (observe_all + observe_axes).
//
// VIEW-BESONDERHEIT (benannt, nicht weggeglaettet): die View-Gattung ist NON-OWNING (view_anatomy.hpp:4).
// Dieser Kontext besitzt deshalb ebenfalls KEINEN Puffer -- bind() reicht einen FREMDEN Puffer durch, dessen
// Lebensdauer beim Aufrufer bleibt. Ein Kontext, der hier einen eigenen Puffer haelte, machte aus der
// non-owning-Gattung stillschweigend eine besitzende und waere ein Gattungs-Bruch, kein Komfort.
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only, keine Beruehrung von abi_adapter.hpp, den
// Wire-PODs (view_tier.hpp), abi/*_decl.hpp oder den Stempel-/Fingerprint-Flaechen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md Paragraf 17.3 + 24

#include <anatomy/view_anatomy.hpp> // ViewAnatomy / ViewObserverSnapshot / ViewAxisObservation

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana = ::comdare::cache_engine::anatomy;

/// ViewExecutionContext<Composition> -- Builder-seitiger Halter + Treiber der View-Anatomie (non-owning).
template <class Composition>
class ViewExecutionContext {
public:
    using anatomy_t          = ana::ViewAnatomy<Composition>;
    using composition_t      = Composition;
    using element_type       = typename anatomy_t::element_type;
    using observer_t         = ana::ViewObserverSnapshot;              ///< flache Gattungs-Sicht (Bestand)
    using axis_observation_t = typename anatomy_t::axis_observation_t; ///< per-Achsen-Sicht (E-24 C3)

    ViewExecutionContext() = default;

    // -- View-Gattungs-Operationen (non-owning). Der Puffer gehoert dem AUFRUFER. --
    void bind(element_type const* data, std::size_t size) noexcept { anatomy_.bind(data, size); }
    [[nodiscard]] std::optional<element_type> read(std::uint64_t index) const noexcept { return anatomy_.read(index); }

    [[nodiscard]] std::size_t size() const noexcept { return anatomy_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return anatomy_.size() == 0; }

    // -- Beobachtung: beide Ebenen an EINER Stelle abnehmbar --
    [[nodiscard]] observer_t                   observe_all() const noexcept { return anatomy_.observe_all(); }
    [[nodiscard]] axis_observation_t           observe_axes() const noexcept { return anatomy_.observe_axes(); }
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return anatomy_t::observable_axis_count();
    }

    /// Zugriff auf die gehaltene Anatomie -- hier laufen die ORGAN-Accessoren (layout_policy_organ() usw.).
    [[nodiscard]] anatomy_t&       anatomy() noexcept { return anatomy_; }
    [[nodiscard]] anatomy_t const& anatomy() const noexcept { return anatomy_; }

    // -- Composition-Inspection (durchgereicht) --
    static constexpr std::string_view  composition_name() noexcept { return anatomy_t::composition_name(); }
    static constexpr std::string_view  paper_id() noexcept { return anatomy_t::paper_id(); }
    static constexpr std::size_t       organ_count() noexcept { return anatomy_t::organ_count(); }
    static constexpr ana::AnatomyGenus genus() noexcept { return anatomy_t::genus(); }

private:
    anatomy_t anatomy_{};
};

} // namespace comdare::cache_engine::builder::anatomy_commands
