#pragma once
// mess_retry_klammer.hpp -- T-15b (KON26-04/KON28-02/KON37-06, gebaut 2026-08-20): DIE BINARY-RETRY-KLAMMER.
//
// BEGRIFFS-KANON (C-07-Praezisierung, KON26-04-Kopf -- der Owner-Satz GOAL VI.5 enthaelt ZWEI verschieden
// grosse Mechanismen, die vorher unter einem Wort liefen):
//   T-15a  Drift -> "ganzen Lauf neu starten": "ganzer Lauf" = die KAMPAGNE (Granularitaet KON19-06 =
//          ALLES). Das ist die Stufe UEBER der Zelle und lebt NICHT hier (Wiederaufsetzpunkt-Posten).
//   T-15b  "bis zu 5 Wiederholungen" = bis zu 5 VERSUCHE des GESAMTEN Pruefdock-Durchlaufs EINER
//          Tier-Binary (Owner verbatim KON37-06: "ein build oder eine Messung duerfen je 5 Mal
//          scheitern bis wir aufgeben"). DAS ist diese Klammer. Die Drift-Achse (drift_gated_cell.hpp,
//          max_reruns) ist eine DRITTE, davon getrennte Groesse (andere Bedingung/Einheit: Gruppen-Rerun
//          bei Streuung, nicht Wiederholung bei Fehlschlag).
//
// WOHER DIE 5 KOMMT UND WARUM SIE HIER SITZT: ce 4cd1ab91 (09.08.) legte die Owner-Zahl 5 auf
// DriftGateConfig::max_reruns -- per KON26-04 die FALSCHE der zwei im Commit selbst dokumentierten
// Lesarten. Der UMZUG: die 5 zieht HIERHER (Binary-Retry), der Drift-Default faellt auf den
// Mechanismus-Default 3 zurueck (Rueckbau ausdruecklich owner-entschieden, s. drift_gated_cell.hpp).
//
// SEMANTIK (KON28-02, Owner verbatim vollstaendig spezifiziert):
//   HART (Default, ALLE Achsen-Einstellungen): failt EINE Einstellung hart, scheitert die Messung der
//     Binary KOMPLETT -> diese Klammer wiederholt den GESAMTEN Durchlauf (bis zu max_versuche) ->
//     nach Erschoepfung steht der LETZTE Ausgang ("failed" in der Zelle, nie null -- die bestehende
//     INC-29.1-Doktrin), der Lauf misst weiter.
//   SOFT (EINZIGE Ausnahme: fehlende MESSEINRICHTUNG der Mess-Achsen-Kategorie, z.B. fehlendes PMC):
//     KEIN Retry, KEIN Komplett-Scheitern -- Warnung in die xlsx (Traeger: LazyRunResult::
//     mess_warnungen), die Messung laeuft so weit es geht. Das Praedikat hart_gescheitert des
//     Aufrufers darf eine Soft-Lage deshalb NIE als hart melden (Test-Koeder haelt das fest).
//   Der Ausloeser ist AUFRUFER-SACHE (das Outcome-Format kennt nur er): im Mess-Pfad load_failed
//   (die drei SourceUnavailable-Pfade) ODER SampleStatus::Failed EINZELNER Einstellungen (die
//   KON26-04-OFFEN-Frage ist durch KON28-02 mit JA geschlossen).
//
// CI-GATE (C-07 Punkt 3): dieses Bauteil ist ueber test_t15b_retry_warmup_paar ctest-registriert --
// die Registrierung IST das CI-Gate (jede CI-Testzelle faehrt ctest; .gitlab-ci.yml traegt keinen
// eigenen T-15-Job und braucht keinen).
//
// LEICHT (nur stdlib): separat testbar ohne die schwere Iterator-/DLL-Include-Kette -- dasselbe
// Muster wie measure_parallelism.hpp.

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

namespace comdare::cache_engine::builder::experiment {

/// Die Konfiguration der Binary-Retry-Klammer -- PRODUKTIV aus dem Profil-XML
/// (<binary_retry max_versuche=".."/>, xml_config_parser; eigenes Element, NICHT am <drift_gate>).
///
/// DIE ZAHL 5 IST OWNER-TEXT (KON37-06 verbatim): "ein build oder eine Messung duerfen je 5 Mal
/// scheitern bis wir aufgeben". JE 5 heisst: BUILD-Versuche und MESS-Versuche haben GETRENNTE
/// Budgets derselben Groesse (nicht 1+5) -- die Bau-Seite klammert BuildOrchestrator::provision_core
/// (BuildConfig::bau_max_versuche, DIESELBE Quelle), die Mess-Seite klammert measure_one_binary.
struct MessRetryKonfig {
    std::uint32_t max_versuche = 5;

    /// max_versuche == 0 waere "kein einziger Versuch" -- das ist keine Messlage, sondern eine
    /// Fehlkonfiguration; die Klammer normalisiert NICHT still, sondern faehrt genau einen Versuch
    /// (ein Lauf ohne jeden Versuch koennte kein ehrliches "failed" tragen).
    [[nodiscard]] constexpr std::uint32_t versuche_mindestens_einer() const noexcept {
        return max_versuche == 0u ? 1u : max_versuche;
    }
};

/// Das Ergebnis der Klammer: der LETZTE Ausgang + die Versuchs-Bilanz (Beweis-Traeger fuer Tests
/// und Log -- die Zahl reist NICHT in die CSV: das Zeilen-Schema bleibt byte-identisch, der
/// Resume-Schema-Vergleich unangetastet).
template <class Outcome>
struct MessRetryErgebnis {
    Outcome     outcome{};
    std::size_t versuche   = 0;     // gefahrene Durchlaeufe insgesamt (1 .. max_versuche)
    bool        erschoepft = false; // Budget verbraucht und der letzte Versuch war weiterhin hart
};

/// Wiederholt `versuch()` bis zu cfg.max_versuche mal, solange `hart_gescheitert(outcome)` wahr ist.
///
/// `versuch()` faehrt den GESAMTEN Pruefdock-Durchlauf EINER Tier-Binary (im Mess-Pfad:
/// measure_one_binary -- laden, Vertrag pruefen, alle Einstellungen messen, Ablage schreiben).
/// Jeder Wiederholungs-Versuch VERWIRFT den vorigen Ausgang vollstaendig (keine Zeilen-Doppelung:
/// die per-Binary-Ablage schreibt trunc, die Zeilen des Fehlversuchs reisen nie in die globale CSV).
/// `warn` nimmt die Retry-Meldungen (nullptr = stumm), `label` benennt die Binary im Log.
///
/// NIE STUMM: jeder Wiederholungs-Anlauf und die Erschoepfung melden sich einzeln ([t15b-retry]).
template <class Versuch, class HartGescheitert>
[[nodiscard]] auto mit_mess_retry(MessRetryKonfig const& cfg, Versuch&& versuch, HartGescheitert&& hart_gescheitert,
                                  std::ostream* warn = nullptr, std::string const& label = "binary")
    -> MessRetryErgebnis<std::decay_t<std::invoke_result_t<Versuch&>>> {
    using Outcome = std::decay_t<std::invoke_result_t<Versuch&>>;
    MessRetryErgebnis<Outcome> out;
    std::uint32_t const        budget = cfg.versuche_mindestens_einer();
    for (std::uint32_t v = 1; v <= budget; ++v) {
        out.outcome  = versuch();
        out.versuche = v;
        if (!hart_gescheitert(out.outcome)) {
            out.erschoepft = false;
            return out; // Erfolg (oder Soft-Lage) -- dieser Ausgang steht.
        }
        if (v < budget) {
            if (warn != nullptr)
                (*warn) << "[t15b-retry] " << label << ": Durchlauf hart gescheitert (Versuch " << v << "/" << budget
                        << ") -- der GESAMTE Pruefdock-Durchlauf wird wiederholt (KON28-02).\n";
        }
    }
    out.erschoepft = true;
    if (warn != nullptr)
        (*warn) << "[t15b-retry] " << label << ": Budget erschoepft (" << budget
                << " Versuche) -- der letzte Ausgang steht ('failed' in der Zelle, nie null; KON37-06 "
                   "'dann aufgeben'), der Lauf misst weiter.\n";
    return out;
}

/// DER MESS-TRIGGER (KON28-02): hart gescheitert ist ein Binary-Durchlauf GENAU DANN, wenn die
/// Mess-QUELLE fehlte (load_failed > 0 -- die drei SourceUnavailable-Pfade: .so nicht ladbar,
/// Mess-Vertrag verletzt, kein Mess-Interface am Dock) ODER irgendeine EINZELNE Einstellung als
/// `failed_status` klassifiziert wurde (SampleStatus::Failed; die KON26-04-OFFEN-Frage ist durch
/// KON28-02 mit JA geschlossen: der Fehlschlag EINER Einstellung scheitert die Binary KOMPLETT).
///
/// WAS AUSDRUECKLICH NICHT TRIGGERT (die SOFT-Seite, KON28-02): fehlende MESSEINRICHTUNG der
/// Mess-Achsen-Kategorie (PMC nicht eingerichtet -> pmc.available == false, Warnung in der xlsx).
/// Dieses Praedikat liest deshalb NUR load_failed und den Zeilen-Status -- nie ein
/// Verfuegbarkeits-Flag. Der Soft-kein-Trigger-Koeder im Test haelt genau das fest.
///
/// Template statt konkretem Typ, mit dem Status als ARGUMENT: das Outcome der Mess-Schleife
/// (CellOutcome) ist funktions-lokal im Iterator und SampleStatus lebt in der measurement-Schicht --
/// beide hierher zu ziehen koppelte die leichte Klammer an die schwere Include-Kette. Der Iterator
/// reicht measurement::SampleStatus::Failed; der Test reicht seinen eigenen Zwilling.
template <class Outcome, class Status>
[[nodiscard]] bool mess_durchlauf_hart_gescheitert(Outcome const& oc, Status failed_status) {
    if (oc.load_failed > 0) return true;
    for (auto const& row : oc.rows)
        if (row.sample_status == failed_status) return true;
    return false;
}

} // namespace comdare::cache_engine::builder::experiment
