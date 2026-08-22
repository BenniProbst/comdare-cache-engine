#pragma once
// builder/pmc_startup_pruefung.hpp -- #83: DER CEB-GEGENEINGANG AM LAUF-HOST (C-1(c)-Warnung, KON103).
//
// OWNER-WORTLAUT (17.08.2026, verbatim): "C-1: (a) und (b) und (c) alle ja. Aber Warnung bei (c) wenn PMC
// vorhanden, aber nicht verwendet."
//
// DIE LUECKE, DIE DIESE DATEI SCHLIESST. Die Planer-Host-Probe (profile_facade/planner/pmc_host_probe.hpp)
// sagt seit dem 10.08. selbst, was sie NICHT beweist: "Benutzbarkeit auf dem RUNNER-Host, der die
// emittierten Jobs spaeter faehrt [...] Aufgefangen: der CEB-Gegeneingang (builder/pmc_startup_pruefung.hpp)
// prueft die Lage am LAUF-Host hart nach." Diese Datei existierte bis #83 NUR als Verweis -- der
// Gegeneingang war versprochen, nicht gebaut. Ohne ihn ist der eine Fall unsichtbar, den der Owner
// ausdruecklich zur WARNUNG erklaert hat: die CEB laeuft OHNE einkompilierte PMC-Quelle auf einem Host,
// der PMC liefern KOENNTE -- ein Teil-Lauf, der keiner haette sein muessen.
//
// DIE DREI WEGE (KON106-04, verbatim: "Fehler bei fehlender Quelle / WARNUNG bei vorhandener-aber-
// ungenutzter Quelle / stiller Normalfall") und wo jeder wohnt:
//   * FEHLER bei fehlender Quelle: NICHT hier -- das ist die EMISSIONS-Seite. Der #37-Preflight der
//     dynamischen Kette scheitert seit #83 hart (exit 1), wenn der Befund Unbrauchbar ist und die
//     Emission deshalb kein -DCOMDARE_ENABLE_PMC traegt (experiment_plan_director.hpp, C-1(b)).
//   * WARNUNG bei vorhandener-aber-ungenutzter Quelle: HIER. Zwei Auspraegungen, beide laut:
//       - Quelle NICHT einkompiliert, aber der Host-Koeder beisst => der Lauf laesst Messwerte liegen,
//         die er haette haben koennen (DIE (c)-Warnung des Owners).
//       - Quelle einkompiliert, aber available()==false am Lauf-Host => Teil-Lauf trotz Einbau
//         (Rechte/paranoid/Container); die Ergebnis-Zeilen tragen Tokens, die Warnung nennt den Lauf.
//   * STILLER NORMALFALL: Quelle einkompiliert und live -- ODER nicht einkompiliert auf einem Host, der
//     ohnehin nichts liefern kann (dort gibt es nichts zu warnen; die Bau-Seite bleibt soft, KON28-02).
//
// WARUM DIESE DATEI IMMER UEBERSETZT WIRD (kein COMDARE_ENABLE_PMC-Guard): sie muss "PMC vorhanden"
// gerade DANN feststellen koennen, wenn das Makro FEHLT -- stuende sie unter dem Makro, koennte sie den
// Warn-Fall strukturell nie sehen (dieselbe Henne-Ei-Begruendung wie am Event-Set-Header).
//
// DER BISS-BEWEIS IST DER EINE: measurement/pmc_event_biss.hpp -- derselbe Koeder und dieselbe
// Anforderungsform wie die Planer-Probe (die seit #83 dorthin delegiert). Gefragt wird gegen DIE EINE
// Event-Liste (measurement::kPmcEvents) mit demselben ODER-Kriterium (>=1 beisst), das auch die Lage-
// Entscheidung des Planers traegt -- ein strengeres Kriterium hier wuerde warnen, wo der Planer nie
// eingebaut haette, ein weicheres wuerde den Owner-Fall verschlucken.
//
// LEHRBUCH-STRATEGY, statisch (Muster pmc_host_probe.hpp): die Biss-Schicht ist ein Template-Policy-
// Parameter. Default ist der REALE Kern; Tests setzen eine Fake-Strategie ein und erzeugen damit alle
// Lagen, auch die auf dieser Maschine nicht herstellbaren. Kein Runtime-Switch, keine vtable.
//
// header-only, C++23. ASCII-only.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include <cache_engine/measurement/pmc_event_biss.hpp> // der EINE Probe-Kern (Koeder + Biss)
#include <cache_engine/measurement/pmc_event_set.hpp>  // DIE EINE Event-Liste

namespace comdare::cache_engine::builder {

/// Ist die PMC-Quelle in DIESES Binary einkompiliert? Die EINE Antwort auf die Frage, die der Aufrufer
/// sonst je Stelle mit einem eigenen #ifdef beantworten muesste (und irgendwann eine Stelle vergisst).
#if defined(COMDARE_ENABLE_PMC)
inline constexpr bool kPmcQuelleEinkompiliert = true;
#else
inline constexpr bool kPmcQuelleEinkompiliert = false;
#endif

/// Die vier Lagen des Gegeneingangs. Genau ZWEI davon sind Warnungen; keine ist ein Fehler -- der
/// Fehler-Weg (fehlende Quelle in der dynamischen Kette) wohnt in der Emission (s. Kopf).
enum class PmcStartupLage : std::uint8_t {
    NormalfallMitQuelle = 0,   ///< einkompiliert und live -> still
    NormalfallOhnePmc,         ///< nicht einkompiliert, Host kann ohnehin nichts liefern -> still
    TeilLaufQuelleOhneZugriff, ///< einkompiliert, aber available()==false am Lauf-Host -> WARNUNG
    VorhandenNichtVerwendet,   ///< NICHT einkompiliert, aber der Host-Koeder beisst -> WARNUNG (Owner-Fall)
};
inline constexpr std::size_t kPmcStartupLageCount = 4;

/// Traegt diese Lage eine Warnung? Single-Source der Frage (Aufrufer vergleichen nie selbst gegen Enums).
[[nodiscard]] constexpr bool pmc_startup_ist_warnung(PmcStartupLage l) noexcept {
    return l == PmcStartupLage::TeilLaufQuelleOhneZugriff || l == PmcStartupLage::VorhandenNichtVerwendet;
}

/// Das stabile, disjunkte Log-Etikett (pmc_-Praefix wie pmc_outcome_label; grep-stabil).
[[nodiscard]] constexpr std::string_view pmc_startup_label(PmcStartupLage l) noexcept {
    switch (l) {
        case PmcStartupLage::NormalfallMitQuelle: return "pmc_startup_normalfall_mit_quelle";
        case PmcStartupLage::NormalfallOhnePmc: return "pmc_startup_normalfall_ohne_pmc";
        case PmcStartupLage::TeilLaufQuelleOhneZugriff: return "pmc_startup_teil_lauf_quelle_ohne_zugriff";
        case PmcStartupLage::VorhandenNichtVerwendet: return "pmc_startup_vorhanden_nicht_verwendet";
    }
    return "pmc_startup_vorhanden_nicht_verwendet"; // sicherer Default: lieber eine Warnung zu viel
}

/// Der Befund EINES Gegeneingangs -- mit seinem NENNER (V-1): wie viele Events wurden gefragt, wie viele
/// haben gebissen. Eine Warnung ohne diese Zahlen waere eine Behauptung.
struct PmcStartupBefund {
    PmcStartupLage lage                 = PmcStartupLage::NormalfallOhnePmc;
    bool           quelle_einkompiliert = false;
    bool           quelle_available     = false;
    std::size_t    events_geprueft      = 0; ///< 0 wenn nicht gefragt wurde (einkompilierte Faelle)
    std::size_t    events_gebissen      = 0;

    [[nodiscard]] bool warnung() const noexcept { return pmc_startup_ist_warnung(lage); }
};

/// REALE Biss-Strategie: der EINE Kern. Tests ersetzen sie durch Fakes (statisch, kein virtual).
struct RealeStartupBissStrategie {
    [[nodiscard]] static bool event_beisst(::comdare::cache_engine::measurement::PmcEventSpec const& ev) noexcept {
        return ::comdare::cache_engine::measurement::pmc_event_beisst(ev);
    }
};

/// DIE PRUEFUNG. `quelle_einkompiliert` und `quelle_available` kommen vom Aufrufer (Makro-Konstante +
/// IPmcSource::available()), damit die Funktion selbst testbar bleibt -- alle vier Lagen sind mit
/// Parametern und Fake-Strategie herstellbar, ohne diese Maschine umzubauen.
///
/// Der Host wird NUR im nicht-einkompilierten Fall gefragt (der Koeder kostet einen 64-MiB-Chase je
/// Event; im einkompilierten Fall ist available() bereits die Lauf-Host-Aussage der realen Quelle --
/// ein zweiter Koeder wuerde dieselbe Frage teurer noch einmal stellen).
template <class BissStrategie = RealeStartupBissStrategie>
[[nodiscard]] inline PmcStartupBefund pmc_startup_pruefung(bool quelle_einkompiliert, bool quelle_available) {
    namespace cme = ::comdare::cache_engine::measurement;

    PmcStartupBefund b;
    b.quelle_einkompiliert = quelle_einkompiliert;
    b.quelle_available     = quelle_available;

    if (quelle_einkompiliert) {
        b.lage = quelle_available ? PmcStartupLage::NormalfallMitQuelle : PmcStartupLage::TeilLaufQuelleOhneZugriff;
        return b;
    }

    // Nicht einkompiliert: KANN dieser Host liefern? Dasselbe ODER-Kriterium wie die Planer-Probe
    // (>=1 Event der EINEN Liste beisst). Eine leere Liste (Nicht-Linux) beisst nie -> still (fail-closed:
    // wo nichts beweisbar ist, wird auch keine verpasste Gelegenheit behauptet).
    b.events_geprueft = cme::kPmcEventCount;
    for (std::size_t i = 0; i < cme::kPmcEventCount; ++i) {
        if (BissStrategie::event_beisst(cme::kPmcEvents[i])) ++b.events_gebissen;
    }
    b.lage = b.events_gebissen > 0 ? PmcStartupLage::VorhandenNichtVerwendet : PmcStartupLage::NormalfallOhnePmc;
    return b;
}

/// Die Warnzeile als STRING -- damit Tests die exakte Aussage pruefen koennen, statt stderr zu schaben.
/// Leerer String fuer die beiden stillen Lagen (die Zeile existiert nur, wenn es etwas zu sagen gibt).
[[nodiscard]] inline std::string pmc_startup_warnzeile(PmcStartupBefund const& b) {
    if (!b.warnung()) return {};
    std::string s{"[PMC-WARN] klasse="};
    s += pmc_startup_label(b.lage);
    s += " quelle_einkompiliert=";
    s += b.quelle_einkompiliert ? "1" : "0";
    s += " quelle_available=";
    s += b.quelle_available ? "1" : "0";
    if (b.lage == PmcStartupLage::VorhandenNichtVerwendet) {
        s += " host_biss=";
        s += std::to_string(b.events_gebissen);
        s += "/";
        s += std::to_string(b.events_geprueft);
        s += " -- PMC VORHANDEN, ABER NICHT VERWENDET (Owner 17.08.: Warnung bei (c)); dieser Lauf ist ein "
             "Teil-Lauf, der keiner haette sein muessen (Zeilen tragen Tokens, pmc_available=0)";
    } else {
        s += " -- TEIL-LAUF: Quelle einkompiliert, aber am Lauf-Host ohne Zaehler-Zugriff "
             "(perf_event_paranoid/CAP_PERFMON/Container); Zeilen tragen Tokens statt stiller 0";
    }
    return s;
}

/// Bequemer Produktions-Einstieg: pruefen UND (nur bei Warnung) auf stderr melden. Liefert den Befund,
/// damit der Aufrufer die Lage zusaetzlich verwerten kann. fprintf + fflush: die Zeile muss auch dann im
/// Job-Log stehen, wenn der Lauf danach lange misst oder abbricht.
template <class BissStrategie = RealeStartupBissStrategie>
inline PmcStartupBefund pmc_startup_pruefe_und_melde(bool quelle_einkompiliert, bool quelle_available) {
    PmcStartupBefund const b = pmc_startup_pruefung<BissStrategie>(quelle_einkompiliert, quelle_available);
    if (b.warnung()) {
        std::string const zeile = pmc_startup_warnzeile(b);
        std::fprintf(stderr, "%s\n", zeile.c_str());
        std::fflush(stderr);
    }
    return b;
}

// -- Selbst-Beweise (compile-time) -----------------------------------------------------------------
static_assert(pmc_startup_ist_warnung(PmcStartupLage::TeilLaufQuelleOhneZugriff));
static_assert(pmc_startup_ist_warnung(PmcStartupLage::VorhandenNichtVerwendet));
static_assert(!pmc_startup_ist_warnung(PmcStartupLage::NormalfallMitQuelle));
static_assert(!pmc_startup_ist_warnung(PmcStartupLage::NormalfallOhnePmc));
static_assert(pmc_startup_label(PmcStartupLage::VorhandenNichtVerwendet) !=
                  pmc_startup_label(PmcStartupLage::TeilLaufQuelleOhneZugriff),
              "Die zwei Warn-Naturen muessen im Log unterscheidbar bleiben.");

} // namespace comdare::cache_engine::builder
