#pragma once
// V5-#26-SUBSTANZ — Mess-Quellen-Abstraktion für die 6 HW-Counter (PMC) des 16+6-Mess-POD.
//
// Re-Audit-Blocker 2: die „+6"-HW-Spalten (cache_misses L1/L2/L3, dtlb_misses, coherence_invalidations,
// energy) waren hartkodiert 0. Dieses Interface ist die SOFTWARE-Architektur, die sie speist — pluggable
// Mess-Quelle mit EHRLICHER Verfügbarkeits-Meldung (`available()`). Die REALEN Werte brauchen Intel PCM /
// RDPMC / RAPL-MSR (Vendor-Lib + Admin/MSR-Treiber auf der i7-1270P) = Beschaffung/Recht (P4, extern);
// bis dahin liefert `NullPmcSource` available()=false (statt stiller 0). Wenn die Hardware-Quelle verfügbar
// ist, ist sie ein Drop-in (eine weitere IPmcSource-Implementierung) — KEINE Änderung an POD/Pipeline/PDF.
//
// @doku docs/sessions/20260531-mess-abstraktion-cross-platform-architektur-plan.md (I2 PMC) + _v5_i8…/POD #50

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::measurement {

/// Die HW-Counter (Delta über ein Mess-Intervall) + Verfügbarkeits-Flag (= pmc_available im POD).
struct PmcCounters {
    std::uint64_t cache_misses_l1         = 0;
    std::uint64_t cache_misses_l2         = 0;
    std::uint64_t cache_misses_l3         = 0;
    std::uint64_t dtlb_misses             = 0;
    std::uint64_t branch_misses           = 0;
    std::uint64_t coherence_invalidations = 0;
    std::uint64_t energy_micro_joules     = 0;
    bool          available               = false; // false = NICHT real gemessen (ehrlich, kein Schein-0)
    // B5/M-2-KORREKTUR-2 (2026-08-06, AMD-L3-Befund): PRO-ZAEHLER Verfuegbarkeit fuer die drei Felder, die
    // NICHT auf jeder Plattform/jedem Vendor gleich real sind -- cache_misses_l3 scheitert auf AMD Zen5 beim
    // Oeffnen (PERF_TYPE_HW_CACHE/LL/READ/MISS -> ENOENT, am Objekt auf prod1 nachgemessen).
    // #82 (I-PMC-3, 2026-08-21): der Satz "cache_misses_l2/coherence_invalidations werden von KEINER
    // heutigen IPmcSource je geoeffnet" ist seither UEBERHOLT -- LinuxPerfPmcSource oeffnet beide ueber
    // PERF_TYPE_RAW, modell-gebunden aus measurement/pmc_raw_event_katalog.hpp (heute Zen 5, kreuzgeprobt);
    // Modelle ohne Katalog-Eintrag lesen weiter ehrlich "n/a" ueber genau diese Flags.
    // `available` bleibt die ZEILEN-weite Aussage ("mindestens ein Zaehler hat real geliefert"); diese drei
    // Flags sind die ZAEHLER-weite Verfeinerung darunter. Default false (Fail-Safe, wie SystemAxisSample,
    // axis_error.hpp). Nur wenn `available && !dieses-Flag` wird die CSV-Zelle zu SourceUnavailable/"n/a"
    // (axis_error.hpp) statt einer Zahl; bei `!available` (NullPmcSource/PMC-off) bleibt die bestehende
    // 0-Konvention der ganzen Zeile unveraendert (kein Verhaltenswechsel im Default-Build).
    bool cache_misses_l2_source_available         = false;
    bool cache_misses_l3_source_available         = false;
    bool coherence_invalidations_source_available = false;
    // B5/M-2-KORREKTUR-3 (2026-08-06, Owner-KERN "stiller Rueckfall ist verboten"): energy_micro_joules ist
    // best-effort (RAPL-sysfs, oft root-only seit Linux 5.10) und faellt beim Fehlschlag auf denselben POD-
    // Default 0 zurueck wie die drei obigen Felder -- dieselbe Fehlerklasse, dieselbe Behandlung.
    bool energy_micro_joules_source_available = false;
    // M-3a (2026-08-07): branch_misses war das LETZTE Feld OHNE eigenes Flag -- es konnte strukturell nie
    // "nicht erhoben" ausdruecken und stand in jeder CSV-Zeile als nackte 0, ununterscheidbar von einer
    // echten Nullmessung. Seit M-3a erhebt LinuxPerfPmcSource den Zaehler REAL (PERF_TYPE_HARDWARE /
    // PERF_COUNT_HW_BRANCH_MISSES -- generisch und portabel, oeffnet auf Intel UND AMD, anders als das
    // LL-Event, das auf AMD Zen5 mit ENOENT scheitert); dieses Flag traegt das Oeffnungsergebnis. Damit
    // gilt fuer branch_misses dieselbe Regel wie fuer l2/l3/coherence/energy: eine Zahl bedeutet "diese
    // Quelle hat wirklich geliefert" (auch eine ECHTE 0), "n/a" bedeutet "Quelle nicht da".
    // WindowsPcmPmcSource nennt das Feld nicht -> bleibt dort beim Default false (ehrlich).
    bool branch_misses_source_available = false;
    // B-5 (2026-08-08): die LETZTEN zwei Felder ohne eigenes Flag. Bis hierher galt fuer sie die
    // Begruendung "l1/dtlb bleiben zeilen-gebunden ... hier wird kein Flag erfunden" (system_axis.hpp) --
    // richtig beobachtet, aber es machte sie strukturell unfaehig, je "nicht erhoben" zu sagen. Dass die
    // Luecke auf prod1 nicht auffiel, ist Zufall der Plattform: dort oeffnen beide Zaehler (am Objekt
    // belegt). Auf WINDOWS ist die 0 dagegen heute schon gelogen -- windows_pcm_pmc_source.hpp sagt es
    // selbst woertlich ("L1 wird von der System-weiten PCM-Counter-State nicht direkt geliefert ... L1
    // bleibt ehrlich 0"), und dtlb_misses wird dort ueberhaupt nie gesetzt. Beide Felder tragen dort also
    // eine 0, die eine Messung behauptet. Mit diesen Flags lesen sie "n/a"; PCM setzt sie bewusst nicht.
    // Damit traegt JEDER der sieben Zaehler sein eigenes Quellen-Flag -- die Regel hat keine Ausnahme mehr.
    bool cache_misses_l1_source_available = false;
    bool dtlb_misses_source_available     = false;
    // B-5 Runde 2 (2026-08-08, Lead-Entscheid): der L3-Zaehler, den es auf AMD WIRKLICH gibt -- als
    // EIGENE Groesse, nicht in cache_misses_l3 hinein.
    //
    // WARUM EINE EIGENE SPALTE UND NICHT DIESELBE: die beiden Zahlen sind nicht dasselbe Mass.
    // cache_misses_l3 ist PER-TASK (der generische Last-Level-Zaehler, wie ihn Intel liefert);
    // dieser hier kommt aus dem amd_l3-UNCORE-PMU, der SYSTEM-WEIT je CCX zaehlt -- pid=-1 ist
    // zwingend, per-Task gibt EINVAL, weil ein Uncore keinen Prozess kennt (am Objekt gemessen,
    // cpumask 0,8 = zwei CCX). Beide unter EINER Ueberschrift zu fuehren waere der Datenbruch aus
    // Ledger :4506 -- "verschiedene Semantik unter derselben Ueberschrift" --, nur quer ueber
    // Vendoren statt ueber die Zeit. Der Name traegt die Geltung deshalb SELBST: wer die Spalte
    // liest, weiss ohne Nachschlagen, dass hier nicht der Pruefling allein gezaehlt wurde.
    //
    // GELTUNGS-VORBEHALT, der mitreist: waehrend des Messfensters zaehlt der Uncore ALLES auf diesem
    // CCX, auch fremde Last. Der Wert ist als OBERGRENZE zu lesen, nicht als Prueflings-Wert. Er ist
    // trotzdem mehr wert als nichts: auf einer Maschine mit CAP_PERFMON (oder paranoid<=0) ist er
    // die einzige reale L3-Miss-Quelle, die AMD hergibt, und auf prod1 heute sagt das Flag ehrlich,
    // dass er gesperrt war.
    std::uint64_t l3_miss_uncore_systemweit                  = 0;
    bool          l3_miss_uncore_systemweit_source_available = false;
};

/// Pluggable HW-Performance-Counter-Quelle. begin()→Op-Lauf→end() liefert das Counter-Delta.
class IPmcSource {
public:
    virtual ~IPmcSource()                                   = default;
    virtual void                           begin() noexcept = 0; ///< Zähler-Start (Intervall-Beginn)
    [[nodiscard]] virtual PmcCounters      end() noexcept   = 0; ///< Intervall-Ende → Delta (available je Quelle)
    [[nodiscard]] virtual bool             available() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept      = 0;
};

/// Default-Quelle, solange kein realer PMC angebunden ist: meldet EHRLICH „nicht verfügbar" (alle 0).
/// Ersetzt die früher hartkodierten 0-HW-Spalten durch ein explizites available=false (#26 / Re-Audit-Blocker 2).
class NullPmcSource final : public IPmcSource {
public:
    void                           begin() noexcept override {}
    [[nodiscard]] PmcCounters      end() noexcept override { return PmcCounters{}; } // available=false (Default)
    [[nodiscard]] bool             available() const noexcept override { return false; }
    [[nodiscard]] std::string_view name() const noexcept override { return "null-pmc (P4/HW-gated)"; }
};

} // namespace comdare::cache_engine::measurement
