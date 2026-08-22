#pragma once
// selektiver_rebuild.hpp -- #97/E-12 C-13: das ENTSCHEIDUNGS- und AUSWEIS-Modul der Skip-Oekonomie
// einer Flotte, auf dem HEUTIGEN Stempel-Stand.
//
// DIE STILLE KUERZUNG, DIE DIESES MODUL HEILT. Commit 813c3232 (2026-07-27, "V-4 Permutations-
// Alt-Kanal retired") entfernte mit dem Alt-Kanal die komplette V36.E-SELEKTIV-REBUILD-Maschinerie
// (tools/permutation_codegen/codegen.cmake + permutation_codegen_tool.cpp, zusammen ~1360 Zeilen
// Entscheidungslogik): je Permutation entschied `needs_rebuild` -- (a) Wrapper fehlt, (b) mode
// on_rebuild, (c) Achsen-Signatur abweichend (dann bump_minor), sonst SKIP -- und eine
// Abschlusszeile wies aus: "N geschrieben, M skipped, K minor-bumped (V36.E)". Seit dem Retire gab
// es KEIN Modul mehr, das die Skip-vs-Neubau-OEKONOMIE einer Flotte ENTSCHEIDET und AUSWEIST.
//
// WAS SICH SEITHER GEAENDERT HAT (und warum dies keine Abschrift des Alten ist): die Identitaet
// haengt heute am STEMPEL -- dem 128-hex-SHA-512 ueber die Preimage-Glieder (fingerprint_format=6,
// anatomy_fingerprint.hpp), der zugleich Cache-/Lager-SCHLUESSEL und SKIP-Marke ist (die fuenf
// Rollen des Stempels). Der per-Binary-Skip laeuft deshalb NICHT mehr ueber eine .version-Signatur,
// sondern ueber dll_is_current (build_orchestrator.hpp, A2-SHA512-only-Gate): der VOLLE Stempel
// entscheidet, inklusive des richtungs-behafteten bvset-Pfads aus Task #59 (Erweiterung der
// Achsen-Menge => Skip bleibt erlaubt, Einschraenkung => Neubau; SkipBvsetKontext).
//
// DIE FLOTTEN-REGEL (der zweite, NEUE Teil): eine SYSTEM-AENDERUNG erzwingt den VOLLFLOTTEN-NEUBAU.
// Traeger der System-Identitaet ist das SYSTEM-Glied des Stempels (kAnatomyFingerprintSystemGlied,
// Preimage-Glied [2]) -- dieselbe Semantik, die W12-B auf dem Cache-Key-Praefix traegt ("+ceb=
// <major>.<minor>": ein ABI-Bump invalidiert ALLE Binaries, artifact_cache.hpp). Zwar fiele jede
// einzelne Binary mit gewechselter System-Zeile auch am vollen Stempel durch (das SYSTEM-Glied
// steht im Preimage); die Flotten-Regel macht daraus aber eine EINMAL je Flotte getroffene,
// AUSGEWIESENE Entscheidung statt N still fehlgeschlagener Einzel-Skips: kein Sidecar-Lesen, kein
// bvset-Pfad, kein Deutungsspielraum -- der Ausweis nennt den Grund beim Namen.
//
// FAIL-CLOSED IN BEIDEN HAELFTEN (die Erbregel des A2-Gates, uebertragen auf die Flotte):
//   * per Binary: jeder Zweifelsfall von dll_is_current ist ein Neubau (Sidecar fehlt/leer/
//     ungueltig, Erwartung leer, DLL fehlt) -- ein falsches "bauen" kostet einen Bau, ein falsches
//     "skip" unterschluege eine Binary.
//   * je Flotte: ohne beidseitig BELEGTE System-Zeile (aufgezeichnet UND aktuell) gibt es keinen
//     nachweisbar unveraenderten System-Stand -- also Vollflotten-Neubau. Ein Skip braucht einen
//     Beleg; das Fehlen des Belegs ist keiner. (Dasselbe Prinzip traegt C-14 an der Presence-Naht:
//     SKIP nur bei belegtem Eintrag, lager_presence.hpp.)
//
// SCHICHTUNG: dieses Modul ist REGISTRY-FREI wie bvset_teilmenge.hpp -- beide System-Zeilen kommen
// als Strings vom Host (der sie aus seiner Stempel-Komposition bzw. dem Flotten-Stand kennt,
// bestandslog_factory.hpp belegt das SYSTEM-Glied). Es erfindet keine zweite Skip-Wahrheit: die
// per-Binary-Frage wird woertlich an dll_is_current DELEGIERT, nicht nachgebaut -- dieses Modul
// fuegt die Flotten-Ebene und den Ausweis hinzu, sonst nichts.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + build_orchestrator (die eine Skip-Quelle).
// Laeuft VOR dem Bau (Planer-/Orchestrierungs-Seite), nie im gemessenen Hot-Path.

#include "build_orchestrator/build_orchestrator.hpp" // dll_is_current + SkipBvsetKontext (die EINE per-Binary-Skip-Quelle)

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace comdare::cache_engine::builder::experiment {

// Der GRUND einer Entscheidung -- Teil des Ausweises, nicht nur des Zaehlens. Die V36.E-Pflicht,
// jede Skip-/Neubau-Entscheidung zu BEGRUENDEN, kehrt hier als Typ zurueck: ein bool "bauen" ohne
// Grund waere wieder die stille Oekonomie, deren Verschwinden #97 aufdeckte.
enum class RebuildGrund : unsigned char {
    skip_stempel_aktuell,      // voller Stempel aktuell (dll_is_current true, inkl. bvset-Erweiterungs-Pfad)
    neubau_ohne_bestand,       // am Zielpfad liegt keine Binary -- es gibt nichts zu skippen
    neubau_stempel_abweichung, // Bestand liegt vor, aber der volle Stempel traegt ihn nicht
                               // (Hash-Abweichung, Sidecar-Zweifelsfall oder bvset-Einschraenkung)
    neubau_systemwechsel,      // Flotten-Regel: System-Zeile gewechselt oder nicht beidseitig belegt
};

[[nodiscard]] constexpr std::string_view rebuild_grund_text(RebuildGrund g) noexcept {
    switch (g) {
        case RebuildGrund::skip_stempel_aktuell: return "skip_stempel_aktuell";
        case RebuildGrund::neubau_ohne_bestand: return "neubau_ohne_bestand";
        case RebuildGrund::neubau_stempel_abweichung: return "neubau_stempel_abweichung";
        case RebuildGrund::neubau_systemwechsel: return "neubau_systemwechsel";
    }
    return "unbekannt"; // nicht erreichbar; haelt den Switch warnungsfrei erschoepfend
}

// DIE FLOTTEN-FRAGE: erzwingt der System-Stand den Vollflotten-Neubau? true heisst: KEIN einzelner
// Skip-Versuch, jede Binary dieser Flotte wird gebaut.
//
//   recorded_system_zeile  die SYSTEM-Zeile (Preimage-Glied [2]), unter der der vorhandene
//                          Flotten-Bestand gebaut wurde (der Host liest sie aus seinem
//                          Flotten-Stand, z.B. dem Bestandslog-Stempelmaterial).
//   current_system_zeile   die SYSTEM-Zeile des HEUTIGEN Treibers (aus derselben Komposition,
//                          die das Glied [2] des Stempels belegt -- KEINE zweite Ableitung).
//
// FAIL-CLOSED: ist eine der beiden Seiten leer, gibt es keinen NACHWEIS eines unveraenderten
// System-Stands -- Vollflotte. Das kostet schlimmstenfalls verschenkte Skips (jede Binary faellt
// ohnehin am vollen Stempel durch oder wird korrekt neu gebaut), nie einen falschen Skip.
[[nodiscard]] inline bool system_wechsel_erzwingt_vollflotte(std::string_view recorded_system_zeile,
                                                             std::string_view current_system_zeile) noexcept {
    if (recorded_system_zeile.empty() || current_system_zeile.empty()) return true;
    return recorded_system_zeile != current_system_zeile;
}

// EINE Entscheidung: bauen oder skippen, mit Grund. Default ist die sichere Seite (bauen).
struct SelektiverRebuildEntscheid {
    bool         bauen = true;
    RebuildGrund grund = RebuildGrund::neubau_ohne_bestand;
};

// DIE PER-BINARY-ENTSCHEIDUNG. Reihenfolge ist Vertrag:
//   (1) FLOTTEN-REGEL ZUERST: unter flotten_neubau wird NICHT einmal versucht zu skippen -- auch
//       eine Binary, deren Stempel zufaellig aktuell aussieht, wird gebaut (System-Aenderung =>
//       Vollflotten-Neubau, ohne Einzel-Skip-Versuch). Das ist der Koeder-Fall der Tests.
//   (2) VOLLER STEMPEL: dll_is_current entscheidet -- Hash-Gleichheit oder der #59-Teilmengen-Pfad
//       ((a) RICHTUNG recorded subset current UND (b) BINDUNG per Neuberechnung), jeder
//       Zweifelsfall fail-closed. Diese Funktion DELEGIERT und deutet nicht um.
//   (3) GRUND-SCHAERFUNG fuer den Ausweis: ein Neubau ohne vorhandene Binary heisst
//       neubau_ohne_bestand, einer trotz vorhandener Binary neubau_stempel_abweichung.
[[nodiscard]] inline SelektiverRebuildEntscheid entscheide_selektiven_rebuild(std::filesystem::path const& output,
                                                                              std::string const& expected_fingerprint,
                                                                              SkipBvsetKontext const& bvset_ctx,
                                                                              bool                    flotten_neubau) {
    if (flotten_neubau) return {.bauen = true, .grund = RebuildGrund::neubau_systemwechsel};
    if (dll_is_current(output, expected_fingerprint, bvset_ctx))
        return {.bauen = false, .grund = RebuildGrund::skip_stempel_aktuell};
    std::error_code ec;
    bool const      bestand_liegt_vor = std::filesystem::exists(output, ec) && !ec;
    return {.bauen = true,
            .grund = bestand_liegt_vor ? RebuildGrund::neubau_stempel_abweichung : RebuildGrund::neubau_ohne_bestand};
}

// DER AUSWEIS -- die Wiederherstellung der V36.E-Pflicht, die Oekonomie eines Flotten-Laufs
// AUSZUWEISEN statt sie stumm geschehen zu lassen. Reines Zaehlwerk ohne I/O: WO die
// Abschlusszeile landet (Log, Testat, Marker), entscheidet der Host.
struct SelektiverRebuildAusweis {
    std::size_t geschrieben             = 0; // Entscheidungen "bauen" (alle Neubau-Gruende zusammen)
    std::size_t geskippt                = 0; // Entscheidungen "skip" (voller Stempel aktuell)
    std::size_t systemwechsel_erzwungen = 0; // davon geschrieben WEGEN der Flotten-Regel
    std::size_t ohne_bestand            = 0; // davon geschrieben, weil keine Binary vorlag
    std::size_t stempel_abweichung      = 0; // davon geschrieben, weil der volle Stempel abwich

    void verbuche(SelektiverRebuildEntscheid const& e) {
        if (!e.bauen) {
            ++geskippt;
            return;
        }
        ++geschrieben;
        switch (e.grund) {
            case RebuildGrund::neubau_systemwechsel: ++systemwechsel_erzwungen; break;
            case RebuildGrund::neubau_ohne_bestand: ++ohne_bestand; break;
            case RebuildGrund::neubau_stempel_abweichung: ++stempel_abweichung; break;
            case RebuildGrund::skip_stempel_aktuell: break; // nicht erreichbar (bauen==true)
        }
    }

    // Summe aller verbuchten Entscheidungen -- der Anker fuer den FREMDEN Nenner der Tests:
    // sie muss der Fenstergroesse des Aufrufers entsprechen, sonst hat das Zaehlwerk geschluckt.
    [[nodiscard]] std::size_t entschieden() const noexcept { return geschrieben + geskippt; }

    // Die Abschlusszeile im V36.E-Geist ("N geschrieben, M skipped, K minor-bumped (V36.E)"),
    // auf den heutigen Gegenstand uebertragen: minor-bump gibt es nicht mehr (die Identitaet
    // haengt am Stempel, nicht an einer fortgeschriebenen Version), an seine Stelle tritt der
    // ausgewiesene Systemwechsel-Zwang.
    [[nodiscard]] std::string abschlusszeile() const {
        return std::to_string(geschrieben) + " geschrieben, " + std::to_string(geskippt) + " geskippt, " +
               std::to_string(systemwechsel_erzwungen) + " davon systemwechsel-erzwungen (C-13 selektiver Rebuild)";
    }
};

} // namespace comdare::cache_engine::builder::experiment
