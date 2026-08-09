#pragma once
// measurement/pmc_counter_outcome.hpp -- B-5: die FEINE Taxonomie EINES PMC-Zaehlers.
//
// GEGENSTAND. Der Ledger (:4663) fuehrt B-5 als "ehrlicher Status-Token statt stiller 0" fuer
// pmc_cache_misses_l3, L2 und coherence. Der Zell-Token dafuer existiert seit dem 06.08. und ist der
// richtige (axis_error.hpp: SampleStatus::SourceUnavailable -> "n/a"). Was FEHLTE, ist die Stufe
// darunter: der Bestand kannte nur `bool open()` und warf damit drei verschiedene Naturen in EINEN
// Zustand -- "diesen Zaehler gibt es auf dieser Hardware nicht", "es gibt ihn, aber wir duerfen nicht"
// und "wir haben ihn falsch angefordert". Diese Datei trennt sie.
//
// WARUM DAS NICHT NUR KOSMETIK IST -- am Objekt gemessen (prod1, AMD Ryzen 9 9950X3D, Zen 5,
// family 26 / model 68, Kernel 6.17.0-35-generic, perf_event_paranoid=1), eigene perf_event_open-Sonde:
//   PERF_TYPE_HW_CACHE / LL / READ / MISS      pid=0  cpu=-1  -> errno=2  (ENOENT)
//   amd_l3 type=17 config=0x104 (l3_miss)      pid=-1 cpu=0   -> errno=13 (EACCES)
//   amd_l3 type=17 config=0x104 per-task       pid=0  cpu=-1  -> errno=22 (EINVAL)
// Die drei Zeilen sind DREI Aussagen: das generische Last-Level-Event existiert auf Zen 5 wirklich
// nicht; der echte L3-Zaehler existiert sehr wohl (der amd_l3-Uncore-PMU ist geladen), ist aber unter
// paranoid=1 fuer nicht-privilegierte Prozesse gesperrt; und per-Task ist er strukturell nicht
// oeffenbar, weil ein Uncore-PMU je CCX zaehlt und keinen Prozess kennt (cpumask 0,8).
// Ein einzelnes `false` haette alle drei zu "Quelle nicht da" eingeebnet -- und genau daraus ist im
// Bestand eine falsche Begruendung entstanden (s. RICHTIGSTELLUNG unten).
//
// RICHTIGSTELLUNG einer Bestands-Aussage (belegt, nicht behauptet). ce `5c102e05` begruendet den
// Verzicht auf den AMD-Weg woertlich mit: "die dafuer noetige amd_l3-Uncore-PMU ist auf identischer
// Hardware/Kernel wie prod1 als Modul vorhanden, aber NICHT GELADEN -- eine Infra-, keine Code-Frage."
// Das ist am Objekt widerlegt: `/sys/bus/event_source/devices/amd_l3/type` liefert 17, und
// `.../amd_l3/format/` traegt die vollstaendige Feldbelegung (event=config:0-7, umask=config:8-15,
// coreid 42-44, enallslices 46, enallcores 47, sliceid 48-50, threadmask 56-57). Der PMU IST geladen.
// Dieselbe Aussage steht in pmc_source.hpp und im Kopf von linux_perf_pmc_source.hpp; beide sind in
// diesem Paket mitgezogen. Die Lehre ist die des Regressions-Dossiers: eine Messung war fuer sich
// korrekt und beantwortete die falsche Frage ("ist der PMU da?" statt "warum scheitert das Oeffnen?").
//
// BAUFORM. Exakt das Muster von pruef_dock/dock_error_classification.hpp, das im Haus fuer denselben
// Zweck steht: eine EIGENE, feine Taxonomie, die (a) den fremden Transport-Wert klassifiziert, (b) auf
// die EINE D2-Zell-Taxonomie abbildet und (c) ein stabiles, disjunktes Log-Etikett liefert. Es wird
// deshalb WEDER SampleStatus erweitert (das Zell-Vokabular der CSV bleibt unberuehrt, kein
// golden-Risiko) NOCH ein zweites Zell-Vokabular erfunden -- die Zelle liest weiter "n/a" bzw.
// "failed", nur das LOG wird aussagekraeftig. Dieselbe Begruendung, mit der sample_status_label neben
// den Zell-Tokens steht: sonst klassifiziert das Log nichts, sondern wiederholt nur die Zelle.
//
// DOKTRIN: C++23, header-only, constexpr, statischer Dispatch; kein std::variant (Verbotskanon).
// ASCII-only. Rein additiv -- diese Datei hat bis zu ihrer Verdrahtung KEINEN Aufrufer und aendert
// KEINE CSV-Byte-Folge.

#include "axis_error.hpp" // SampleStatus / sample_status_token / probe_label_ist_disjunkt

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

// -- Die Naturen eines Zaehler-Zustands ------------------------------------------------------------
// Geschnitten nach NATUR, nicht nach errno-Zahl -- mehrere errno-Werte duerfen in eine Klasse fallen,
// aber keine Klasse darf zwei Naturen tragen. Die Trennlinien folgen der Owner-Schwere-Leiter
// (Memory feedback_pmc_je_mikroarchitektur_p_und_e_core_getrennt, Punkt 8):
//   * "eine grundlegende Systemeigenschaft fehlt"  -> es gibt keinen Wert, aber es ist kein Versagen
//   * "es ist etwas schiefgegangen"                -> es gibt keinen Wert, und es ist eines
// Der Unterschied traegt bis in die Zelle: die erste Gruppe rendert "n/a", die zweite "failed".
enum class PmcCounterOutcome : std::uint8_t {
    Offen = 0, // der Zaehler wurde geoeffnet UND gelesen -> die Zahl steht in der Zelle
    // -- Plattform-Eigenschaften: kein Wert, aber auch kein Versagen ("n/a") ------------------------
    EventNichtVorhanden = 1, // ENOENT/ENODEV: diese CPU/dieser Kernel kennt das Event hier nicht
    ZugriffVerweigert   = 2, // EACCES/EPERM: das Event EXISTIERT, die Rechte fehlen (paranoid/CAP_PERFMON)
    // -- Versagen: kein Wert, und wir sind die Ursache ("failed") -----------------------------------
    OeffnungUngueltig   = 3, // EINVAL: die ANFORDERUNG war falsch geformt -- ein Bau-, kein Plattform-Fehler
    RessourceErschoepft = 4, // EMFILE/ENFILE/ENOSPC/EBUSY: die Zaehler-/fd-Ressource war belegt
    NichtGelesen        = 5, // geoeffnet, aber read() lieferte nichts (Multiplexing-Verdraengung)
    UnbekannterFehler   = 6, // jedes andere errno -- SICHTBAR, nie auf eine Nachbar-Klasse geraten
};
/// Single-Source der Klassenzahl (bei JEDER neuen Klasse mit hochzaehlen; der Drift-Guard unten faengt
/// Vergessen -- dieselbe Bauform wie kSampleStatusCount / kCompilerCompilerErrorClassCount).
inline constexpr std::size_t kPmcCounterOutcomeCount = 7;

// -- Transport-Pin: die fremden errno-Konstanten, gegen die diese Abbildung gebaut ist --------------
// Dreht eine Plattform eine dieser Zahlen, bricht die Uebersetzung HIER und nicht still in der
// Auswertung (Muster: dock_error_classification.hpp).
static_assert(ENOENT == 2, "ENOENT-Transportwert gedreht -- die Abbildung unten ist neu zu pruefen.");
static_assert(EACCES == 13, "EACCES-Transportwert gedreht -- die Abbildung unten ist neu zu pruefen.");
static_assert(EINVAL == 22, "EINVAL-Transportwert gedreht -- die Abbildung unten ist neu zu pruefen.");

/// classify_pmc_open_errno(e) -- die Natur eines perf_event_open-Fehlschlags. e == 0 heisst "kein
/// Fehler" und liefert Offen; jeder unbekannte Wert bekommt UnbekannterFehler statt einer geratenen
/// Nachbar-Klasse. Die Abbildung ist TOTAL: es gibt kein errno ohne Klasse.
[[nodiscard]] constexpr PmcCounterOutcome classify_pmc_open_errno(int e) noexcept {
    switch (e) {
        case 0: return PmcCounterOutcome::Offen;
        // Das Event ist auf dieser Mikroarchitektur nicht abgebildet (generisches LL auf Zen 5) oder
        // das adressierte Geraet existiert nicht. Beides ist eine Eigenschaft der Maschine.
        case ENOENT:
        case ENODEV: return PmcCounterOutcome::EventNichtVorhanden;
        // Der Zaehler ist da, die Berechtigung fehlt. Behebbar OHNE andere Hardware -- und deshalb
        // eine andere Aussage als EventNichtVorhanden, auch wenn beide Zellen "n/a" lesen.
        case EACCES:
        case EPERM: return PmcCounterOutcome::ZugriffVerweigert;
        // Die Anforderung selbst war ungueltig: falsche attr-Belegung, oder ein Uncore-PMU per-Task
        // geoeffnet. Das ist unser Fehler, nicht der der Maschine.
        case EINVAL: return PmcCounterOutcome::OeffnungUngueltig;
        case EMFILE:
        case ENFILE:
        case ENOSPC:
        case EBUSY: return PmcCounterOutcome::RessourceErschoepft;
        default: return PmcCounterOutcome::UnbekannterFehler;
    }
}

/// Traegt dieser Zustand einen gueltigen Zahlenwert? Genau EINE Klasse tut das. Die Funktion ist die
/// Single-Source dieser Frage -- Aufrufer fragen sie, statt gegen Offen zu vergleichen.
[[nodiscard]] constexpr bool pmc_outcome_hat_wert(PmcCounterOutcome o) noexcept {
    return o == PmcCounterOutcome::Offen;
}

/// Die Abbildung auf die EINE D2-Zell-Taxonomie. Die Trennlinie ist die Owner-Schwere-Leiter, nicht
/// die Lautstaerke: fehlt eine Systemeigenschaft (Event nicht da, Zugriff verwehrt), wurde NICHT
/// gemessen und die Zelle liest ehrlich "n/a"; ging etwas schief (falsche Anforderung, Ressource
/// belegt, Zaehler lief nicht, unbekanntes errno), ist es ein Mess-Fehler und die Zelle liest
/// "failed". In KEINEM Fall entsteht eine 0 -- das ist der ganze Punkt von B-5.
[[nodiscard]] constexpr SampleStatus pmc_outcome_sample_status(PmcCounterOutcome o) noexcept {
    switch (o) {
        case PmcCounterOutcome::Offen: return SampleStatus::Ok;
        case PmcCounterOutcome::EventNichtVorhanden:
        case PmcCounterOutcome::ZugriffVerweigert: return SampleStatus::SourceUnavailable;
        case PmcCounterOutcome::OeffnungUngueltig:
        case PmcCounterOutcome::RessourceErschoepft:
        case PmcCounterOutcome::NichtGelesen:
        case PmcCounterOutcome::UnbekannterFehler: return SampleStatus::Failed;
    }
    return SampleStatus::Failed; // sicherer Default, in dieselbe Richtung wie sample_status_token
}

/// Der CSV-Zell-Token -- ueber die EINE D2-Taxonomie geleitet, nie als rohes Literal. Damit kann diese
/// Datei das Zell-Vokabular strukturell nicht erweitern, auch nicht versehentlich.
[[nodiscard]] constexpr std::string_view pmc_outcome_cell(PmcCounterOutcome o) noexcept {
    return sample_status_token(pmc_outcome_sample_status(o));
}

/// Das LOG-Etikett -- das eigentliche Ergebnis dieser Datei. Der Zell-Token bildet vier Klassen auf
/// "failed" und zwei auf "n/a" ab (so soll die CSV lesen); im Log muss unterscheidbar bleiben, ob ein
/// Zaehler auf dieser Hardware fehlt oder ob nur das Recht fehlt -- sonst klassifiziert das Log nichts.
/// Das `pmc_`-Praefix haelt das Vokabular gegen jede fremde Taxonomie disjunkt (unten verwacht).
[[nodiscard]] constexpr std::string_view pmc_outcome_label(PmcCounterOutcome o) noexcept {
    switch (o) {
        case PmcCounterOutcome::Offen: return "pmc_offen";
        case PmcCounterOutcome::EventNichtVorhanden: return "pmc_event_nicht_vorhanden";
        case PmcCounterOutcome::ZugriffVerweigert: return "pmc_zugriff_verweigert";
        case PmcCounterOutcome::OeffnungUngueltig: return "pmc_oeffnung_ungueltig";
        case PmcCounterOutcome::RessourceErschoepft: return "pmc_ressource_erschoepft";
        case PmcCounterOutcome::NichtGelesen: return "pmc_nicht_gelesen";
        case PmcCounterOutcome::UnbekannterFehler: return "pmc_fehler_unbekannt";
    }
    return "pmc_fehler_unbekannt";
}

// ==================================================================================================
// Selbst-Beweise (compile-time): die Abbildung ist total, disjunkt und driftet nicht.
// ==================================================================================================
static_assert(std::is_same_v<std::underlying_type_t<PmcCounterOutcome>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<PmcCounterOutcome>);
static_assert(static_cast<std::uint8_t>(PmcCounterOutcome::Offen) == 0,
              "Offen MUSS 0 bleiben -- value-initialisierte Zustaende sonst still 'gueltig'.");

// Drift-Guard in BEIDE Richtungen: waechst das Enum ohne die Zahl, schlaegt die erste Zeile an;
// waechst die Zahl ohne das Enum, faellt die zweite auf den Fallback und schlaegt an.
static_assert(kPmcCounterOutcomeCount == static_cast<std::size_t>(PmcCounterOutcome::UnbekannterFehler) + 1,
              "kPmcCounterOutcomeCount und das Enum sind auseinandergelaufen.");
static_assert(pmc_outcome_label(static_cast<PmcCounterOutcome>(kPmcCounterOutcomeCount)) ==
                  std::string_view{"pmc_fehler_unbekannt"},
              "Eine neue Klasse ohne Etikett faellt still auf den Fallback -- Etikett nachziehen.");

// Die Abbildung, an den drei am Objekt GEMESSENEN errno-Werten festgenagelt (s. Kopf).
static_assert(classify_pmc_open_errno(0) == PmcCounterOutcome::Offen);
static_assert(classify_pmc_open_errno(ENOENT) == PmcCounterOutcome::EventNichtVorhanden,
              "Der generische LL-Zaehler auf Zen 5 -- das Event gibt es hier nicht.");
static_assert(classify_pmc_open_errno(EACCES) == PmcCounterOutcome::ZugriffVerweigert,
              "Der amd_l3-Uncore unter paranoid=1 -- der Zaehler ist da, das Recht fehlt.");
static_assert(classify_pmc_open_errno(EINVAL) == PmcCounterOutcome::OeffnungUngueltig,
              "Ein Uncore-PMU per-Task geoeffnet -- unsere falsche Anforderung, nicht die Maschine.");
static_assert(classify_pmc_open_errno(4242) == PmcCounterOutcome::UnbekannterFehler,
              "Ein unbekanntes errno wird SICHTBAR, nicht auf eine Nachbar-Klasse geraten.");

// Die beiden Naturen fallen wirklich auseinander -- und tun es dort, wo es zaehlt.
static_assert(PmcCounterOutcome::EventNichtVorhanden != PmcCounterOutcome::ZugriffVerweigert,
              "Die Kern-Unterscheidung dieses Pakets darf nicht kollabieren.");
static_assert(pmc_outcome_label(PmcCounterOutcome::EventNichtVorhanden) !=
                  pmc_outcome_label(PmcCounterOutcome::ZugriffVerweigert),
              "Im LOG muessen 'gibt es nicht' und 'darf nicht' unterscheidbar bleiben.");
static_assert(pmc_outcome_cell(PmcCounterOutcome::EventNichtVorhanden) ==
                  pmc_outcome_cell(PmcCounterOutcome::ZugriffVerweigert),
              "In der ZELLE sind beide dasselbe: es wurde nicht gemessen. Das ist Absicht.");

// KEIN Zustand rendert je eine Zahl, ausser dem einen, der wirklich einen Wert hat.
static_assert(pmc_outcome_hat_wert(PmcCounterOutcome::Offen));
static_assert(!pmc_outcome_hat_wert(PmcCounterOutcome::EventNichtVorhanden));
static_assert(!pmc_outcome_hat_wert(PmcCounterOutcome::ZugriffVerweigert));
static_assert(!pmc_outcome_hat_wert(PmcCounterOutcome::NichtGelesen));
static_assert(pmc_outcome_cell(PmcCounterOutcome::EventNichtVorhanden) == std::string_view{"n/a"});
static_assert(pmc_outcome_cell(PmcCounterOutcome::ZugriffVerweigert) == std::string_view{"n/a"});
static_assert(pmc_outcome_cell(PmcCounterOutcome::OeffnungUngueltig) == std::string_view{"failed"});
static_assert(pmc_outcome_cell(PmcCounterOutcome::NichtGelesen) == std::string_view{"failed"},
              "Ein Zaehler, der lief aber nichts lieferte, ist ein Mess-Fehler -- nie eine stille 0.");

// Das neue Log-Vokabular ist gegen JEDE bestehende Taxonomie disjunkt. probe_label_ist_disjunkt laeuft
// ueber die Count-Single-Sources der fremden Enums -- waechst dort eines, waechst diese Pruefung mit.
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::Offen)));
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::EventNichtVorhanden)));
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::ZugriffVerweigert)));
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::OeffnungUngueltig)));
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::RessourceErschoepft)));
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::NichtGelesen)));
static_assert(probe_label_ist_disjunkt(pmc_outcome_label(PmcCounterOutcome::UnbekannterFehler)));

} // namespace comdare::cache_engine::measurement
