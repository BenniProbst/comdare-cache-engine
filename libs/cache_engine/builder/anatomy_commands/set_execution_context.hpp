#pragma once
// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3,
// Liefergegenstand (e) "SetExecutionContext") -- SetExecutionContext<Composition>.
//
// WAS DIESE DATEI IST: die builder-seitige TREIBER-Flaeche der Set-Gattung, nach dem Vorbild des
// SearchAlgorithm-Pendants anatomy_execution_context.hpp (V41.F.6.1.R5.B). Sie bringt die Set-Anatomie in
// dieselbe Aufruf-Form, in der der Mess-Treiber die SA-Gattung schon heute treibt: EIN Kontext-Objekt, das
// die Anatomie haelt, die Gattungs-Operationen anbietet und die Beobachtung abnimmt.
//
// EIN UNTERSCHIED, DER BENANNT GEHOERT (nicht weggeglaettet): der SA-Kontext BESITZT den konstitutiven
// Speicher, weil die SA-Anatomie per User-Direktive gar keine Container-API mehr hat
// (search_algorithm_anatomy.hpp:177-189: "R5.B Container-API ENTFERNT"). Die SET-Anatomie dagegen TREIBT ihr
// search_algo-Kern-Organ selbst als Menge (set_anatomy.hpp, K=V) -- sie ist die Stelle, an der die
// Mengen-Semantik und ihr Observer zusammenliegen. Dieser Kontext baut deshalb KEINEN zweiten Speicher
// daneben (das waere ein zweiter Zustand fuer dieselbe Menge und damit eine Messluege), sondern ist die
// gattungs-einheitliche AUFRUF-Flaeche + die Stelle, an der die beiden Beobachtungs-Ebenen der Gattung
// zusammenlaufen: die flache Gattungs-Sicht (observe_all) und die per-Achsen-Sicht (observe_axes, C3).
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only, keine Beruehrung von abi_adapter.hpp, den
// Wire-PODs (set_tier.hpp), abi/*_decl.hpp oder den Stempel-/Fingerprint-Flaechen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md Paragraf 17.3 + 24

#include <anatomy/set_anatomy.hpp> // SetAnatomy / SetObserverSnapshot / SetAxisObservation

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana = ::comdare::cache_engine::anatomy;

/// SetExecutionContext<Composition> -- Builder-seitiger Halter + Treiber der Set-Gattungs-Anatomie.
template <class Composition>
class SetExecutionContext {
public:
    using anatomy_t          = ana::SetAnatomy<Composition>;
    using composition_t      = Composition;
    using key_type           = std::uint64_t;
    using observer_t         = ana::SetObserverSnapshot;               ///< flache Gattungs-Sicht (Bestand)
    using axis_observation_t = typename anatomy_t::axis_observation_t; ///< per-Achsen-Sicht (E-24 C3)

    SetExecutionContext() = default;

    // -- Set-Gattungs-Operationen (K-only). Durchgereicht an die Anatomie, die das Kern-Organ treibt. --
    bool               insert(key_type k) { return anatomy_.insert(k); }
    [[nodiscard]] bool contains(key_type k) const { return anatomy_.contains(k); }
    bool               erase(key_type k) { return anatomy_.erase(k); }
    void               clear() { anatomy_.clear(); }

    [[nodiscard]] std::size_t size() const noexcept { return anatomy_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return anatomy_.size() == 0; }

    // -- Beobachtung: beide Ebenen an EINER Stelle abnehmbar --
    [[nodiscard]] observer_t                   observe_all() const noexcept { return anatomy_.observe_all(); }
    [[nodiscard]] axis_observation_t           observe_axes() const noexcept { return anatomy_.observe_axes(); }
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return anatomy_t::observable_axis_count();
    }

    /// Zugriff auf die gehaltene Anatomie -- hier laufen die ORGAN-Accessoren (search_algo_organ() usw.),
    /// ueber die der Mess-Treiber die getragenen Achsen-Organe (und eingesetzte Sub-Organe) treibt.
    [[nodiscard]] anatomy_t&       anatomy() noexcept { return anatomy_; }
    [[nodiscard]] anatomy_t const& anatomy() const noexcept { return anatomy_; }

    // -- Composition-Inspection (durchgereicht, identische Form wie AnatomyExecutionContext:177-179) --
    static constexpr std::string_view  composition_name() noexcept { return anatomy_t::composition_name(); }
    static constexpr std::string_view  paper_id() noexcept { return anatomy_t::paper_id(); }
    static constexpr std::size_t       organ_count() noexcept { return anatomy_t::organ_count(); }
    static constexpr ana::AnatomyGenus genus() noexcept { return anatomy_t::genus(); }

private:
    anatomy_t anatomy_{};
};

} // namespace comdare::cache_engine::builder::anatomy_commands
