#pragma once
// STEMPEL TEIL 2 (B-7/RN-78 EMITTER-HAELFTE, WEICHE A) -- H-23 TEIL B, Antwort L2 (=B1): DER HYBRID-MODUL-
// EMITTER. render_hybrid_module_source als STRATEGY-SCHWESTER der EINEN Rendering-Kernfunktion
// (modul_emitter.hpp; Vorbild der Sibling-Disziplin: adhoc_emitter_shaped.hpp), gespeist aus der geparsten
// HybridTierConfig (hybrid_config_xml.hpp: enabled / genus / max_docks / docks).
//
// BEWUSST EIGENER HEADER (Sibling-Disziplin wie adhoc_emitter_shaped.hpp): die Hybrid-Speisung zieht
// hybrid/hybrid_config_xml.hpp und damit den XML-Leser aus libs/common. modul_emitter.hpp bleibt davon
// frei; Konsumenten der Container-/SA-Strategien tragen den Include-Graph des Hybrids nicht mit.
//
// WAS EMITTIERT WIRD (byte-gleich zur Form der Hand-Fixture tests/unit/hybrid_tier_module.cpp): der
// ABI-Include hybrid/hybrid_module_abi_v1.hpp, der Makro-Aufruf COMDARE_DEFINE_HYBRID_MODULE(<Ziel-Genus als
// FQ-Enumerator>) und -- ANGEHAENGT, Weiche A -- der Stempel-Makro-Traeger + die Stempel-Zeile. Das Makro
// selbst (6 extern-C-Symbole, MaxDocks = 1 = F8-/CEB-Pruefdock-Doktrin) wird NICHT angefasst.
//
// F-17 (Owner 26.08.2026 13:00:03Z: "Die CEB ist kein hybrid und hat daher nur ein Pruefdock, nur bei der
// Hybrid-Tier-Binary ist das dynamisch wie empfohlen ... Aber korrekt ist dynamisch je Planer"; FINAL-f17 =
// Option C): max_docks ist ein PLANER-Datum. Dieser Emitter LIEST es aus der Konfiguration (Deckel-Wache
// [1, kHybridNodeObergrenzeDefault]) und TRAEGT es im Emissions-Deskriptor (dock_deckel_geplant) fuer die
// CEB-Bau-Naht -- er schreibt es NIE in den Quelltext: zwei Konfigurationen, die sich nur in max_docks
// unterscheiden, erzeugen BYTE-IDENTISCHE Modul-Quellen (Wache hybrid_erzeugnis_traegt_keine_dockzahl +
// test_stempel2_modul_emitter). Der Laufzeit-Deckel erreicht das Dock-Array ueber DockArray(laufzeit_deckel)
// (hybrid_dock_array.hpp, A2.5-Fix F-4); die Mehr-Dock-Delegation selbst ist HY-B/W3 (Proxy-Pin MaxDocks==1)
// -- dieser Emitter praejudiziert dort nichts, weil er nichts verdrahtet.
//
// STEMPEL-ZEILEN DES HYBRIDS: die System-Zeile ist kompositionsfrei (abi::system_stamp_line, die CEB-Naht
// vervollstaendigt sie per COMDARE_SYSTEM_CELL_VALUES innen). Die ORGAN-Zeile eines Hybrids traegt sein
// CT-Identitaetsdatum: das Reroute-Ziel ("reroute_ziel=<Genus>@<Version>"). Die Dock-BESTUECKUNG und der
// Dock-Deckel gehoeren NICHT hinein -- sie sind Laufzeit-Konfiguration ("die Dock-Bestueckung ist RUNTIME-
// Konfiguration und gehoert NIE in die binary_id", hybrid_dock_contract.hpp Deskriptor-Kopf); WELCHES Tier
// an WELCHER Zelle steckt, ist das Glied [9] (KompositMap), das die CEB-Naht je Hybrid-Bau als Compile-
// Define injiziert (anatomy_fingerprint.hpp: COMDARE_HYBRID_KOMPOSIT_GLIED) -- nicht Emitter-Text.
//
// FAIL-CLOSED, DURCHGEHEND (dieselbe Doktrin wie der Parser): enabled=false = Zweig nicht angefordert (kein
// Erzeugnis, kein Fehler -- die XOR-Regel je Kette); ein Ziel-Genus ausserhalb der deklarierten Reroute-
// Ziele (hybrid_binary_proxy.hpp kDeklarierteRerouteZielListe) wird VOR der Emission abgewiesen -- die
// emittierte TU braeche sonst an der CT-Sperre des Proxys, also weit weg vom Planer; max_docks ausserhalb
// des Deckels und ungueltige Dock-Vertrags-Ids ebenso. Jeder Zustand ist BENANNT (Totalitaets-Wache).
//
// @doku H-23 TEIL B L2 (B1) + FINAL-stempel-teil2 Abschnitt 4 Punkt 2 + FINAL-f17-pruefdock Abschnitt 3

#include "modul_emitter.hpp"

#include <hybrid/heuristik_adapter_synthese_matrix.hpp> // kHybridNodeObergrenzeDefault -- der EINE Deckel
#include <hybrid/hybrid_binary_proxy.hpp>               // reroute_ziel_gelistet (die deklarierte Ziel-Whitelist)
#include <hybrid/hybrid_config_xml.hpp>                 // HybridTierConfig / faehrt_hybrid_zweig (die Speisung)
#include <hybrid/hybrid_dock_contract.hpp>              // hybrid_dock_contract_id_gueltig

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::codegen {

// -----------------------------------------------------------------------------------------------------
// (1) DIE EMISSIONS-ZUSTAENDE -- benannt, total, mit Gegenprobe (Muster hybrid_status_* der Dock-Schicht).
// -----------------------------------------------------------------------------------------------------

enum class HybridEmissionStatus : std::uint8_t {
    ok                          = 0, ///< Quelle emittiert
    zweig_nicht_angefordert     = 1, ///< enabled=false: direkter Tier-Zweig, KEIN Hybrid-Erzeugnis (kein Fehler)
    ziel_genus_nicht_deklariert = 2, ///< Genus ausserhalb kDeklarierteRerouteZielListe (CT-Sperre des Proxys)
    max_docks_ausserhalb_deckel = 3, ///< max_docks == 0 oder > kHybridNodeObergrenzeDefault
    dock_vertrag_ungueltig      = 4, ///< ein Dock-Deskriptor adressiert keine Registry-Zeile
};

inline constexpr std::array<HybridEmissionStatus, 5> kAlleHybridEmissionStatus{
    HybridEmissionStatus::ok, HybridEmissionStatus::zweig_nicht_angefordert,
    HybridEmissionStatus::ziel_genus_nicht_deklariert, HybridEmissionStatus::max_docks_ausserhalb_deckel,
    HybridEmissionStatus::dock_vertrag_ungueltig};

[[nodiscard]] constexpr std::string_view hybrid_emission_status_name(HybridEmissionStatus s) noexcept {
    switch (s) {
        case HybridEmissionStatus::ok: return "ok";
        case HybridEmissionStatus::zweig_nicht_angefordert: return "zweig_nicht_angefordert";
        case HybridEmissionStatus::ziel_genus_nicht_deklariert: return "ziel_genus_nicht_deklariert";
        case HybridEmissionStatus::max_docks_ausserhalb_deckel: return "max_docks_ausserhalb_deckel";
        case HybridEmissionStatus::dock_vertrag_ungueltig: return "dock_vertrag_ungueltig";
    }
    return "unknown";
}

namespace detail {
[[nodiscard]] consteval bool alle_hybrid_emission_status_benannt() {
    for (std::size_t i = 0; i < kAlleHybridEmissionStatus.size(); ++i) {
        if (hybrid_emission_status_name(kAlleHybridEmissionStatus[i]) == std::string_view{"unknown"}) return false;
        for (std::size_t j = i + 1; j < kAlleHybridEmissionStatus.size(); ++j)
            if (hybrid_emission_status_name(kAlleHybridEmissionStatus[i]) ==
                hybrid_emission_status_name(kAlleHybridEmissionStatus[j]))
                return false;
    }
    return true;
}
} // namespace detail

static_assert(detail::alle_hybrid_emission_status_benannt(),
              "STEMPEL-2/L2: jeder Emissions-Zustand traegt einen eigenen Namen (stumme Zustaende sind die "
              "NAHT-1-Lehre der Dock-Schicht).");
static_assert(hybrid_emission_status_name(static_cast<HybridEmissionStatus>(99)) == std::string_view{"unknown"},
              "STEMPEL-2/L2: ein Nicht-Zustand meldet 'unknown' -- sonst ist die Wache darueber wertlos.");

// -----------------------------------------------------------------------------------------------------
// (2) DAS ZIEL-GENUS ALS QUELLTEXT -- der FQ-Enumerator, exakt wie in den Hand-Fixtures geschrieben.
// -----------------------------------------------------------------------------------------------------

/// genus_enumerator_fq(g) -- "::comdare::cache_engine::anatomy::AnatomyGenus::<Name>" fuer die fuenf
/// ABI-sichtbaren Genera; LEER fuer alles andere (der Reroute-Genus selbst ist NIE ein Ziel, Gate S1).
[[nodiscard]] constexpr std::string_view genus_enumerator_fq(anatomy::AnatomyGenus g) noexcept {
    switch (g) {
        case anatomy::AnatomyGenus::SearchAlgorithm:
            return "::comdare::cache_engine::anatomy::AnatomyGenus::SearchAlgorithm";
        case anatomy::AnatomyGenus::Set: return "::comdare::cache_engine::anatomy::AnatomyGenus::Set";
        case anatomy::AnatomyGenus::Sequence: return "::comdare::cache_engine::anatomy::AnatomyGenus::Sequence";
        case anatomy::AnatomyGenus::Adapter: return "::comdare::cache_engine::anatomy::AnatomyGenus::Adapter";
        case anatomy::AnatomyGenus::View: return "::comdare::cache_engine::anatomy::AnatomyGenus::View";
        case anatomy::AnatomyGenus::FunctionInterfaceReroute: return {};
    }
    return {};
}

static_assert(genus_enumerator_fq(anatomy::AnatomyGenus::FunctionInterfaceReroute).empty(),
              "STEMPEL-2/L2: der Reroute-Genus ist NIE ein Ziel-Genus (Gate S1) -- er hat keinen Enumerator-Text.");

// -----------------------------------------------------------------------------------------------------
// (3) DIE ORGAN-ZEILE DES HYBRIDS -- das Reroute-Ziel als CT-Identitaetsdatum; Docks bleiben draussen.
// -----------------------------------------------------------------------------------------------------

/// kHybridModulEmissionVersion -- die algo_version der Hybrid-Reroute-Modul-FORM (HY-A2 Proxy-Delegation,
/// Weg C). Flag-Grammatik v2 mit CPU-Flag (ce-eigene Version, COMDARE_VERSION_HW_FLAG_ENFORCE). Wer die
/// emittierte Form oder die Proxy-Semantik AENDERT, bumpt HIER den Minor (E-8-Auflage (ii)) -- die Stempel
/// aller emittierten Hybride bewegen sich dann deklariert mit.
inline constexpr std::string_view kHybridModulEmissionVersion = "1.0.0.c";
static_assert(
    ::comdare::cache_engine::measurement::version_is_parsable_or_documented_sentinel(kHybridModulEmissionVersion),
    "STEMPEL-2/L2: die Hybrid-Emissions-Version muss in der Flag-Grammatik v2 parsen.");

/// hybrid_organ_stamp_line(cfg) -- "reroute_ziel=<Genus>@1.0.0.c" (+ leerer Organ-Meta-Meta-Anhang).
/// Bewusst OHNE max_docks und OHNE Dock-Bestueckung (F-17 + Deskriptor-Doktrin, s. Datei-Kopf).
[[nodiscard]] inline std::string hybrid_organ_stamp_line(::comdare::cache_engine::hybrid::HybridTierConfig const& cfg) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    std::array<AxisVersionEntry, 1> const entries{
        {{"reroute_ziel", anatomy::genus_name(cfg.genus), kHybridModulEmissionVersion}}};
    return detail::organ_zeile_aus(entries);
}

// -----------------------------------------------------------------------------------------------------
// (4) DIE EMISSION
// -----------------------------------------------------------------------------------------------------

/// HybridModulEmission -- das Erzeugnis samt Bau-Entscheid-Daten fuer die CEB-Naht. dock_deckel_geplant ist
/// das Planer-Datum (max_docks) -- es reist HIER, nicht im Quelltext (F-17); heutiger Verbraucher = der
/// Emissions-Test (Wache), der produktive Verbraucher ist die Hybrid-Bau-Naht (HY-B/W3, DockArray(laufzeit_deckel)).
struct HybridModulEmission {
    HybridEmissionStatus  status              = HybridEmissionStatus::ok;
    anatomy::AnatomyGenus ziel_genus          = anatomy::AnatomyGenus::SearchAlgorithm;
    std::size_t           dock_deckel_geplant = 0;
    std::string           quelle;

    [[nodiscard]] bool emittiert() const noexcept { return status == HybridEmissionStatus::ok; }
};

/// hybrid_erzeugnis_traegt_keine_dockzahl(quelle) -- die F-17-Wache ueber das Erzeugnis: kein Dock-Token im
/// Quelltext (weder max_docks noch MaxDocks noch eine Dock-Bestueckung). Die Dock-Zahl ist dynamisch je Planer.
[[nodiscard]] constexpr bool hybrid_erzeugnis_traegt_keine_dockzahl(std::string_view quelle) noexcept {
    return quelle.find("max_docks") == std::string_view::npos && quelle.find("MaxDocks") == std::string_view::npos &&
           quelle.find("dock") == std::string_view::npos && quelle.find("Dock") == std::string_view::npos;
}

/// render_hybrid_module_source(cfg, idx, organ, system, measurement) -- die Kernfunktion mit der Hybrid-
/// Strategie, gespeist aus der Konfiguration. Stempel-Zeilen kommen als Parameter (dieselbe Signatur-
/// Disziplin wie render_adhoc_module_source); leere Organ-Zeile = stempellose Diagnose-Emission.
[[nodiscard]] inline HybridModulEmission
render_hybrid_module_source(::comdare::cache_engine::hybrid::HybridTierConfig const& cfg, int idx,
                            std::string_view organ_stamp = {}, std::string_view system_stamp = {},
                            std::string_view measurement_stamp = {}) {
    namespace hy = ::comdare::cache_engine::hybrid;
    HybridModulEmission e;
    e.ziel_genus          = cfg.genus;
    e.dock_deckel_geplant = cfg.max_docks;
    if (!hy::faehrt_hybrid_zweig(cfg)) {
        e.status = HybridEmissionStatus::zweig_nicht_angefordert;
        return e;
    }
    if (!hy::reroute_ziel_gelistet(cfg.genus) || genus_enumerator_fq(cfg.genus).empty()) {
        e.status = HybridEmissionStatus::ziel_genus_nicht_deklariert;
        return e;
    }
    if (cfg.max_docks == 0 || cfg.max_docks > hy::kHybridNodeObergrenzeDefault) {
        e.status = HybridEmissionStatus::max_docks_ausserhalb_deckel;
        return e;
    }
    for (auto const& d : cfg.docks) {
        if (!hy::hybrid_dock_contract_id_gueltig(d.contract_id)) {
            e.status = HybridEmissionStatus::dock_vertrag_ungueltig;
            return e;
        }
    }
    e.quelle = render_modul_source(kHybridModulStrategie, idx, genus_enumerator_fq(cfg.genus), organ_stamp,
                                   system_stamp, measurement_stamp);
    return e;
}

/// hybrid_modul_emission(cfg, idx, measurement) -- die GESPEISTE Form: Organ-Zeile aus der Konfiguration
/// (hybrid_organ_stamp_line), System-Zeile abi::system_stamp_line(), Mess-Zeile vom Aufrufer (leer = 2-arg).
[[nodiscard]] inline HybridModulEmission
hybrid_modul_emission(::comdare::cache_engine::hybrid::HybridTierConfig const& cfg, int idx,
                      std::string_view measurement_stamp = {}) {
    std::string const organ  = hybrid_organ_stamp_line(cfg);
    std::string const system = ::comdare::cache_engine::abi::system_stamp_line();
    return render_hybrid_module_source(cfg, idx, organ, system, measurement_stamp);
}

/// emit_hybrid_module(cfg, out_dir, idx, measurement) -- schreibt das Erzeugnis (Dateiname
/// <datei_praefix><idx>.cpp, Loader-Pattern) und liefert den Pfad; nullopt, wenn nicht emittiert wurde
/// (Status im optional zurueckgereichten Deskriptor).
[[nodiscard]] inline std::optional<std::filesystem::path>
emit_hybrid_module(::comdare::cache_engine::hybrid::HybridTierConfig const& cfg, std::filesystem::path const& out_dir,
                   int idx, std::string_view measurement_stamp = {}, HybridModulEmission* deskriptor_out = nullptr) {
    HybridModulEmission e = hybrid_modul_emission(cfg, idx, measurement_stamp);
    if (deskriptor_out != nullptr) *deskriptor_out = e;
    if (!e.emittiert()) return std::nullopt;
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    std::filesystem::path const f =
        out_dir / (std::string{kHybridModulStrategie.datei_praefix} + detail::dezimal(idx) + ".cpp");
    std::ofstream out(f, std::ios::trunc);
    out << e.quelle;
    return f;
}

/// HybridModulEmitter -- der Vertrags-Traeger der Hybrid-Gattung (ModulEmitterVertrag): Eingang ist die
/// Planer-Konfiguration, Ausgang die geschriebene Quelle.
struct HybridModulEmitter {
    static constexpr anatomy::AnatomyGenus genus = anatomy::AnatomyGenus::FunctionInterfaceReroute;

    ::comdare::cache_engine::hybrid::HybridTierConfig konfiguration{};
    int                                               idx = 0;

    [[nodiscard]] std::vector<std::filesystem::path> emittieren(std::filesystem::path const& out) const {
        std::vector<std::filesystem::path> files;
        if (auto const f = emit_hybrid_module(konfiguration, out, idx)) files.push_back(*f);
        return files;
    }
};

static_assert(ModulEmitterVertrag<HybridModulEmitter>,
              "STEMPEL-2/L4: der Hybrid-Emitter erfuellt den Vertrag der Andock-Flaeche.");

} // namespace comdare::cache_engine::builder::codegen
