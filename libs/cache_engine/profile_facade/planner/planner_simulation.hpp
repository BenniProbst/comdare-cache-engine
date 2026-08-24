#pragma once
// planner_simulation.hpp -- S-19 PLANUNGS-SIMULATION (#7, 2026-08-20): die B-4-/Mess-Mengen-RECHNUNG.
//
// SELBSTCHECK: diese Datei RECHNET und BERICHTET -- mehr nicht. Sie liest kein XML, faehrt keinen
// Director-Walk, misst keinen Host und kennt keine Registry. Jede Zahl kommt als Eingang herein
// (Muster und geteilte Bausteine: planner_mengen_types.hpp -- MengenArt/MengenFaktor/mul_sicher/
// drift_faktor_rechnen werden GENUTZT, nicht nachgebaut). Das ist die T-3-Bedingung: ein Test darf
// hier von Hand gerechnete Zahlen einsetzen, ohne Profil, Registry oder DLL.
//
// == WAS DIESE SIMULATION IST (Owner-Weg KON30-03) =================================================
// "Die Achsen im Planer zur Simulation gruppieren und kategorisieren, um ueber den gesamten
// Experiment-B+-Baum die tatsaechliche Permutation zu errechnen." Die B-4-Zahl (Bau-Nenner des
// Triggers) ist KEINE Owner-Zahl und KEIN Kandidat aus einer Liste: sie ENTSTEHT hier aus den
// XML-FREIGABEN. Deshalb traegt diese Datei bewusst KEIN einziges Nenner-Literal (kein 320, kein
// 131072, kein 24, kein 48): wo unten multipliziert wird, stehen Eingaenge; wo eine Fakultaet
// gebildet wird, steht n! ueber einer GEMESSENEN Torlage. "24 oder 48" ist ein AUSGANG.
//
// == DIE DOPPELZAEHL-FALLE, GEGEN DIE HIER GEBAUT IST: repetition ==================================
// build_axis_levels (profile_to_tree.hpp:162-169) emittiert die Wiederholungs-Achse "repetition"
// IMMER als DynamicDim (KF-10, Default 3) -- und DIESELBE Zahl ist der KF-10-Faktor der Formel
// T-15 x KF-10 (C-04: 3 Drift-reps x 3 Berichts-Wiederholungen = 9 geplante Messungen je Zelle).
// Wer die Wiederholung im dynamic_dims-Produkt UND im Wiederholungs-Faktor mitnimmt, rechnet um
// Faktor |repetitions| zu gross. REGEL DIESER DATEI: dyn_dims kommt OHNE repetition herein (der
// Sammler filtert, der Bericht sagt es), repetition zaehlt GENAU EINMAL -- als kf10_repetitions.
//
// == D.7 ENTSCHIEDEN (KON26-04): ARENA-DECKEL = GESAMT-FAKTOR, 120er-BASIS =========================
// Die GEPLANTE Messungs-Zahl je Einstellung ist T-15 x KF-10 (Erfolgs-Pfad, jeder Wert einzeln
// persistiert, KON37-06). Der Arena-WORST-CASE ist der GESAMT-Faktor arena_gesamt_faktor =
// drift * paar * t15b_retry (Vorgabewerte 12*2*5 = 120; gerechnet in planner_mengen_types.hpp,
// Eigentuemer measure_storage/#13). drift_faktor_rechnen = reps * (max_reruns + 1) (bis 12) ist
// darin der Drift-TEIL-Faktor; die T-15b-Klammer (je bis zu N Fehlversuche fuer Build UND Messung,
// C-08: "x5 ZUSAETZLICH") geht in die ARENA-Kapazitaet multipliziert ein, in die geplante
// MESS-Menge weiterhin NICHT. Alle Faktoren stehen als EIGENE Zeilen im Bericht.
//
// == BAU-MENGE != MESS-MENGE (Owner-Auftrag check-size, datierter Nachtrag 09.08.2026) =============
// n_bau zaehlt DLL-Bauten (Organ-Freigabe-Produkt x System-Perms). Die Mess-Menge zaehlt Ops ueber
// die MESS-TEILMENGE der Binaries -- die hat heute KEINEN wirksamen XML-Traeger (run_options.cap ist
// No-op; D.5/O3). Ohne gereichte Teilmenge bleibt die Mess-Gesamtzahl UNBESTIMMBAR; gedruckt wird
// dann die OBERE SCHRANKE (Teilmenge == voller Bau) als ausdruecklich benannte Schranke.

#include "planner_mengen_types.hpp" // MengenArt/MengenFaktor/Deckelurteil/MengenEingang/mul_sicher/drift_faktor_rechnen

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits> // add_sicher: numeric_limits-Wache (Schwester von mul_sicher)
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::planner {

// ---------------------------------------------------------------------------------------------------------------
// EINGANGS-BAUSTEINE. Der Sammler (collect_simulation_eingang_facade) fuellt sie; Tests fuellen von Hand.
// ---------------------------------------------------------------------------------------------------------------

/// EINE freigegebene Achse mit ihrer Werte-LISTE. Die Liste (nicht nur die Zahl) reist mit, weil die
/// Kampagnen-Aggregation (f) den FULL JOIN je Achse ueber alle Paper bildet -- eine Union braucht Werte.
struct SimAchse {
    std::string              name;
    std::vector<std::string> werte;
    MengenArt                art = MengenArt::Unbestimmbar;
    std::string              nenner;
};

/// EINE dynamische Laufzeit-Dimension (nur die Maechtigkeit; Laufzeit-Schleifen brauchen keine Union).
struct SimDynDim {
    std::string   name;
    std::uint64_t werte = 0;
    MengenArt     art   = MengenArt::Unbestimmbar;
    std::string   nenner;
};

/// Der EINGANG der Simulation EINER XML. `mengen` ist der unveraenderte check-size-Eingang desselben
/// Walks (eine Quelle, keine zweite Wahrheit); der Rest ist die S-19-Zusatzernte.
struct SimulationsEingang {
    MengenEingang mengen; ///< Walk-Zahlen + n_ops/drift/korn/GN_TOTAL (collect_mess_menge_facade-Form)

    // -- (a) XML-Freigabe je Organ-Achse -------------------------------------------------------------------------
    std::vector<SimAchse> organ_achsen; ///< nur Organ-Kompositions-Achsen (is_organ_composition_axis)

    // -- System-Perm-Identitaeten (fuer die Kampagnen-Union; Zaehl-Wahrheit bleibt mengen.perm_count) -------------
    std::vector<std::string> system_perm_ids; ///< dedupliziert, "opt+simd" je Perm des Walks

    // -- Walk-Schritte (Thesis: Basis-Pass + Sweeps; Experiment: (merge x lebewesen)-Paesse) ----------------------
    std::uint64_t walk_schritte = 0; ///< on_step-Aufrufe EINER Perm-Runde des Walks (Selektions-, kein Bau-Faktor)

    // -- (b) dynamic_dims OHNE repetition (s. Doppelzaehl-Falle im Kopf) -----------------------------------------
    std::vector<SimDynDim> dyn_dims;

    // -- (c) Wiederholungs-Faktoren ------------------------------------------------------------------------------
    std::uint64_t kf10_repetitions = 0; ///< <repetitions count> bzw. Parser-Default; 0 == nicht erhebbar
    MengenArt     kf10_art         = MengenArt::Unbestimmbar;
    std::string   kf10_nenner;
    std::uint64_t t15b_fehlversuche = 5; ///< C-07/C-08: "bis zu 5" je Build UND Messung -- KLAMMER, kein Faktor
    MengenArt     t15b_art          = MengenArt::Default;

    // -- (d) Messgeraete + PMC-Tor -------------------------------------------------------------------------------
    std::uint64_t messgeraete_basis = 0; ///< Tokens der GROESSTEN Tooling-Kombination des Walks (leer == Angebot)
    MengenArt     messgeraete_art   = MengenArt::Unbestimmbar;
    std::string   messgeraete_nenner;
    bool          pmc_tor = false;         ///< GEMESSEN auf DIESEM Host (PmcHostBefund; fail-closed false)
    std::string   pmc_lage_label;          ///< "amd" | "intel" | "unbrauchbar"
    std::string   pmc_nenner;              ///< nenner_zeile des Befunds bzw. fehlgrund
    std::string   fremde_lane_label;       ///< DEKLARIERT (--fremde-lane=..); leer == nicht erhoben
    bool          fremde_lane_pmc = false; ///< nur gueltig, wenn fremde_lane_label nicht leer

    // -- Mess-Teilmenge (D.5/O3: kein XML-Traeger -- Interims-Eingang des Aufrufers) -----------------------------
    std::uint64_t mess_teilmenge = 0; ///< 0 == nicht gereicht => Mess-Gesamtzahl unbestimmbar

    // -- (e) Kalibrierungen/Deckel -- ALLES Aufrufer-Zutaten, je mit Art und Nenner ------------------------------
    double        bau_sekunden_je_dll = 0.0; ///< 0 == nicht gereicht
    MengenArt     bau_sekunden_art    = MengenArt::Unbestimmbar;
    std::string   bau_sekunden_nenner;
    std::uint64_t bytes_je_dll     = 0; ///< 0 == nicht gereicht (GN-9-Kalibrierwert des Aufrufers)
    MengenArt     bytes_je_dll_art = MengenArt::Unbestimmbar;
    std::string   bytes_je_dll_nenner;
    std::uint64_t lager_budget_bytes = 0;   ///< 0 == kein Lager-Gate verlangt
    double        t3_fenster_tage    = 0.0; ///< 0 == kein Zeit-Fenster verlangt (OV-4 = f(T-3), B.4)
};

// ---------------------------------------------------------------------------------------------------------------
// ERGEBNIS EINER XML.
// ---------------------------------------------------------------------------------------------------------------
struct SimulationsSicht {
    bool                      erhoben = false;
    std::string               grund;
    std::vector<MengenFaktor> faktoren;

    std::uint64_t organ_produkt = 0; ///< prod(|Freigabe-Werte|) ueber die Organ-Achsen
    std::uint64_t system_perms  = 0; ///< |opt x simd| (Walk)
    std::uint64_t n_bau         = 0; ///< organ_produkt * system_perms -- der B-4-BEITRAG dieser XML
    std::uint64_t scheiben      = 0; ///< aufgerundete 4096er-Claims (Gen-2) je Perm-Satz
    std::uint64_t zellen        = 0; ///< combo_count * perm_count (Gegenprobe zellen_gezaehlt)
    std::uint64_t dyn_produkt   = 0; ///< prod dynamic_dims (OHNE repetition)

    std::uint64_t messungen_je_einstellung = 0; ///< T-15 x KF-10 (geplant; 0 == unbestimmbar)
    std::uint64_t drift_worst              = 0; ///< reps * (max_reruns + 1) -- Drift-TEIL-Faktor der Arena (D.7)
    std::uint64_t mess_ops_je_binary       = 0; ///< n_ops * dyn_produkt * messungen_je_einstellung
    std::uint64_t mess_ops_gesamt          = 0; ///< nur mit gereichter Teilmenge (sonst 0 == n/a)
    std::uint64_t mess_ops_schranke        = 0; ///< OBERE SCHRANKE (Teilmenge == n_bau)

    std::uint64_t messgeraete         = 0; ///< basis + (pmc_tor ? 1 : 0)
    std::uint64_t rekombination       = 0; ///< messgeraete! -- AUSGANG dieses Laufs, nie Vorgabe
    std::uint64_t rekombination_fremd = 0; ///< nur wenn fremde Lane deklariert; sonst 0
    std::uint64_t rekombination_summe = 0; ///< lokal + fremd (nur wenn fremde Lane deklariert)

    bool ueberlauf = false;

    // -- (e) AUSGANG: Kapazitaet/ETA/Deckel ----------------------------------------------------------------------
    bool          bau_eta_bekannt  = false;
    double        bau_eta_h        = 0.0; ///< n_bau * bau_sekunden_je_dll / 3600 (EINLANIG, eine Maschine)
    bool          lager_bekannt    = false;
    std::uint64_t lager_bytes      = 0; ///< n_bau * bytes_je_dll
    Deckelurteil  lager_urteil     = Deckelurteil::KeinDeckel;
    bool          mess_eta_bekannt = false;
    double        mess_eta_h       = 0.0; ///< mess_ops_gesamt * sekunden_je_op / 3600
    bool          ov4_bekannt      = false;
    std::uint64_t ov4_deckel       = 0; ///< AUSGANG: max Binaries, so dass (Bau + 1-Thread-Messung) ins Fenster passt
    Deckelurteil  zeit_urteil      = Deckelurteil::KeinDeckel;

    [[nodiscard]] bool abgelehnt() const noexcept {
        return ueberlauf || lager_urteil == Deckelurteil::Gerissen || lager_urteil == Deckelurteil::Unbestimmbar ||
               zeit_urteil == Deckelurteil::Gerissen || zeit_urteil == Deckelurteil::Unbestimmbar;
    }
};

// ---------------------------------------------------------------------------------------------------------------
// Ueberlauf-sichere ADDITION -- die Schwester von mul_sicher, fuer die Kampagnen-Summe.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] constexpr bool add_sicher(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (b > (std::numeric_limits<std::uint64_t>::max() - a)) return false;
    out = a + b;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// FAKULTAET, ueberlauf-gewacht. 20! ist die letzte in uint64 darstellbare Fakultaet; alles darueber
// meldet false statt einer gewickelten Zahl -- dieselbe Richtung wie mul_sicher.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] constexpr bool fakultaet_sicher(std::uint64_t n, std::uint64_t& out) noexcept {
    if (n > 20u) return false;
    std::uint64_t f = 1u;
    for (std::uint64_t i = 2u; i <= n; ++i) f *= i;
    out = f;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// DIE RECHNUNG EINER XML -- Faktoren zuerst, jede mit Nenner; Produkte danach; alles gewacht.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline SimulationsSicht simulation_rechnen(SimulationsEingang const& e) {
    SimulationsSicht s{};
    auto             fak = [&s](std::string name, std::string wert, MengenArt art, std::string nenner) {
        s.faktoren.push_back(MengenFaktor{std::move(name), std::move(wert), art, std::move(nenner)});
    };

    // -- Vorbedingungen: lieber ehrlich nichts sagen als eine Zahl aus Luecken bauen -----------------------------
    if (e.organ_achsen.empty()) {
        s.grund = "Keine Organ-Achse freigegeben -- ohne Freigabe-Produkt gibt es keine Bau-Menge "
                  "(Freigabe-Quelle: permute_axes bzw. axes_default_lookup).";
        return s;
    }
    if (e.mengen.perm_count == 0u || e.mengen.combo_count == 0u) {
        s.grund = "Der Director-Walk lieferte keine Zellen (perm_count=" + std::to_string(e.mengen.perm_count) +
                  ", combo_count=" + std::to_string(e.mengen.combo_count) + ").";
        return s;
    }

    // == (a) XML-FREIGABE-PRODUKT JE ORGAN-ACHSE -- der B-4-Kern =================================================
    s.organ_produkt = 1u;
    for (auto const& ax : e.organ_achsen) {
        fak("freigabe[" + ax.name + "]", std::to_string(ax.werte.size()), ax.art, ax.nenner);
        if (ax.werte.empty()) {
            s.grund = "Organ-Achse '" + ax.name + "' ist freigegeben, traegt aber 0 Werte -- das Produkt waere 0 " +
                      "und saehe wie eine leere Kampagne aus statt wie ein Profil-Fehler.";
            return s;
        }
        if (!mul_sicher(s.organ_produkt, static_cast<std::uint64_t>(ax.werte.size()), s.organ_produkt)) {
            s.ueberlauf = true;
            s.grund     = "Organ-Freigabe-Produkt laeuft ueber uint64 bei Achse '" + ax.name + "'";
            return s;
        }
    }
    fak("organ_produkt", std::to_string(s.organ_produkt), MengenArt::Gerechnet,
        "prod(|Freigabe-Werte|) ueber " + std::to_string(e.organ_achsen.size()) +
            " Organ-Achsen -- Rechenprinzip experiment_setting_count/catalog_axis_product (arithmetisch, "
            "ohne Materialisierung)");

    // == System-Perms + n_bau ====================================================================================
    s.system_perms = e.mengen.perm_count;
    {
        std::string ids;
        for (auto const& id : e.system_perm_ids) {
            if (!ids.empty()) ids += ",";
            ids += id;
        }
        fak("system_perms", std::to_string(s.system_perms), MengenArt::Exakt,
            "PlanHeader.perm_count -- |opt x simd| aus dem Director-Walk" + (ids.empty() ? "" : " {" + ids + "}"));
    }
    if (!mul_sicher(s.organ_produkt, s.system_perms, s.n_bau)) {
        s.ueberlauf = true;
        s.grund     = "organ_produkt * system_perms laeuft ueber uint64";
        return s;
    }
    fak("n_bau", std::to_string(s.n_bau), MengenArt::Gerechnet,
        "organ_produkt * system_perms -- der B-4-BEITRAG dieser XML (Bau-Menge != Mess-Menge)");

    // GEGENPROBE gegen die env-Kampagnenbreite (COMDARE_GN_TOTAL), wenn gesetzt: zwei Quellen fuer dieselbe
    // Frage muessen sich decken -- tun sie es nicht, sagt es der Bericht, statt eine Seite zu waehlen.
    if (e.mengen.binaries_je_perm != 0u) {
        fak("gn_total_env", std::to_string(e.mengen.binaries_je_perm),
            e.mengen.binaries_aus_env ? MengenArt::Umgebung : MengenArt::Default,
            (e.mengen.binaries_je_perm == s.organ_produkt)
                ? "COMDARE_GN_TOTAL -- deckt sich mit organ_produkt"
                : "COMDARE_GN_TOTAL -- WEICHT von organ_produkt AB (s. HINWEISE)");
    }
    if (e.mengen.batch_korn != 0u) {
        s.scheiben = (s.n_bau + e.mengen.batch_korn - 1u) / e.mengen.batch_korn;
        fak("scheiben_aufgerundet", std::to_string(s.scheiben), MengenArt::Gerechnet,
            "aufgerundet(n_bau / batch_korn) -- Gen-2-Claims a kGnBatchSlice; eine Teil-Scheibe ist ein Claim");
    }

    // == Zellen (exakt aus dem Walk, mit Gegenprobe) =============================================================
    if (!mul_sicher(e.mengen.perm_count, e.mengen.combo_count, s.zellen)) {
        s.ueberlauf = true;
        s.grund     = "perm_count * combo_count laeuft ueber uint64";
        return s;
    }
    fak("zellen", std::to_string(s.zellen), MengenArt::Gerechnet, "perm_count * combo_count");
    fak("zellen_gezaehlt", std::to_string(e.mengen.zellen_gezaehlt), MengenArt::Exakt,
        (e.mengen.zellen_gezaehlt == s.zellen) ? "begin_perm-Aufrufe des Walks -- deckt sich mit dem Produkt"
                                               : "begin_perm-Aufrufe des Walks -- WEICHT VOM PRODUKT AB (s. HINWEISE)");
    fak("walk_schritte_gesamt", std::to_string(e.walk_schritte), MengenArt::Exakt,
        "on_step-Aufrufe des Walks (Thesis: (Basis+Sweeps) x Zellen; Experiment: Phasen-Paesse x Zellen) -- "
        "SELEKTIONS-Enumeration, geht NICHT in n_bau ein");

    // == (b) dynamic_dims-PRODUKT (OHNE repetition -- die zaehlt als KF-10 genau einmal) =========================
    s.dyn_produkt = 1u;
    for (auto const& d : e.dyn_dims) {
        fak("dyn[" + d.name + "]", std::to_string(d.werte), d.art, d.nenner);
        if (d.werte == 0u) {
            s.grund = "dynamic_dim '" + d.name + "' traegt 0 Werte -- eine leere Laufzeit-Schleife ist ein " +
                      "Sammler-Fehler, kein Faktor 0.";
            return s;
        }
        if (!mul_sicher(s.dyn_produkt, d.werte, s.dyn_produkt)) {
            s.ueberlauf = true;
            s.grund     = "dynamic_dims-Produkt laeuft ueber uint64 bei '" + d.name + "'";
            return s;
        }
    }
    fak("dyn_produkt", std::to_string(s.dyn_produkt), MengenArt::Gerechnet,
        "prod(|values|) ueber " + std::to_string(e.dyn_dims.size()) +
            " dynamic_dims OHNE repetition (repetition zaehlt als KF-10, s. Kopf: Doppelzaehl-Falle)");

    // == (c) T-15 x KF-10 GEPLANT + beide Worst-Case-Klammern (D.7: keine Entscheidung) ==========================
    fak("t15_drift_reps", std::to_string(e.mengen.drift_reps),
        e.mengen.drift_aus_xml ? MengenArt::Xml : MengenArt::Default,
        "<drift_gate reps> bzw. DriftGateConfig::reps -- die GEPLANTEN Erfolgs-Messungen je Einstellung");
    if (e.kf10_repetitions != 0u) {
        fak("kf10_repetitions", std::to_string(e.kf10_repetitions), e.kf10_art, e.kf10_nenner);
        if (!mul_sicher(e.mengen.drift_reps, e.kf10_repetitions, s.messungen_je_einstellung)) {
            s.ueberlauf = true;
            s.grund     = "t15 * kf10 laeuft ueber uint64";
            return s;
        }
        fak("messungen_je_einstellung", std::to_string(s.messungen_je_einstellung), MengenArt::Gerechnet,
            "T-15 x KF-10 -- geplante, je einzeln persistierte Messungen je (Binary x Einstellung) (C-04)");
    } else {
        fak("kf10_repetitions", "n/a", e.kf10_art, e.kf10_nenner);
        fak("messungen_je_einstellung", "n/a", MengenArt::Unbestimmbar,
            "ohne KF-10-Wiederholungszahl nicht berechenbar (Profil-Wurzel traegt kein <repetitions>)");
    }
    s.drift_worst = drift_faktor_rechnen(e.mengen.drift_reps, e.mengen.drift_max_reruns);
    fak("drift_worst_arena", std::to_string(s.drift_worst), MengenArt::Gerechnet,
        "reps * (max_reruns + 1) -- Drift-TEIL-Faktor des Arena-Deckels arena_gesamt_faktor (D.7 "
        "ENTSCHIEDEN, 120er-Basis); steht NEBEN der geplanten Zahl, nicht statt ihr");
    fak("t15b_fehlversuch_klammer", "je " + std::to_string(e.t15b_fehlversuche) + " (Build UND Messung)", e.t15b_art,
        "WORST-CASE-KLAMMER T-15b/C-08 (x" + std::to_string(e.t15b_fehlversuche) +
            " ZUSAETZLICH) -- NICHT in die geplante Menge multipliziert, wohl aber in den Arena-Deckel "
            "arena_gesamt_faktor = drift*paar*retry (120er-Basis, D.7 ENTSCHIEDEN); Eigentuemer measure_storage/#13");

    // == Mess-Seite: Ops je Binary der Teilmenge =================================================================
    fak("n_ops", std::to_string(e.mengen.n_ops), e.mengen.n_ops_aus_xml ? MengenArt::Xml : MengenArt::Default,
        e.mengen.n_ops_aus_xml ? "<run_options n_ops>" : "ExperimentRunArgs::n_ops -- NICHT im Profil erklaert");
    if (s.messungen_je_einstellung != 0u) {
        std::uint64_t je = 0u;
        if (!mul_sicher(e.mengen.n_ops, s.dyn_produkt, je) || !mul_sicher(je, s.messungen_je_einstellung, je)) {
            s.ueberlauf = true;
            s.grund     = "n_ops * dyn_produkt * messungen_je_einstellung laeuft ueber uint64";
            return s;
        }
        s.mess_ops_je_binary = je;
        fak("mess_ops_je_binary", std::to_string(je), MengenArt::Gerechnet,
            "n_ops * dyn_produkt * messungen_je_einstellung -- Ops EINER Binary der Mess-Teilmenge");
    } else {
        fak("mess_ops_je_binary", "n/a", MengenArt::Unbestimmbar, "haengt an messungen_je_einstellung");
    }

    // == (d) Messgeraete, PMC-Tor, Rekombination als AUSGANG =====================================================
    fak("messgeraete_basis", std::to_string(e.messgeraete_basis), e.messgeraete_art, e.messgeraete_nenner);
    fak("pmc_tor", e.pmc_tor ? "an" : "aus (PMC-Zeilen entfallen)", MengenArt::Exakt,
        "GEMESSEN auf diesem Host: Lage '" + e.pmc_lage_label + "' -- " + e.pmc_nenner);
    s.messgeraete = e.messgeraete_basis + (e.pmc_tor ? 1u : 0u);
    fak("messgeraete", std::to_string(s.messgeraete), MengenArt::Gerechnet,
        "messgeraete_basis + PMC-Tor -- die Torlage entscheidet, nicht eine Vorgabe");
    if (!fakultaet_sicher(s.messgeraete, s.rekombination)) {
        s.ueberlauf = true;
        s.grund     = "messgeraete! laeuft ueber uint64 (n > 20)";
        return s;
    }
    fak("rekombination", std::to_string(s.rekombination), MengenArt::Gerechnet,
        "messgeraete! -- Messgeraete-Rekombination zur Messfehler-Herausrechnung; AUSGANG dieses Laufs, "
        "NIE Vorgabe (Eingangshypothesen 3!/4! bleiben Hypothesen)");
    if (!e.fremde_lane_label.empty()) {
        std::uint64_t const fremd_geraete = e.messgeraete_basis + (e.fremde_lane_pmc ? 1u : 0u);
        if (!fakultaet_sicher(fremd_geraete, s.rekombination_fremd)) {
            s.ueberlauf = true;
            s.grund     = "fremde messgeraete! laeuft ueber uint64";
            return s;
        }
        s.rekombination_summe = s.rekombination + s.rekombination_fremd;
        fak("rekombination_fremde_lane", std::to_string(s.rekombination_fremd), MengenArt::Geschaetzt,
            "DEKLARIERTE Lage '" + e.fremde_lane_label + "' (--fremde-lane, NICHT gemessen)");
        fak("rekombination_summe_lanes", std::to_string(s.rekombination_summe), MengenArt::Gerechnet,
            "lokal + fremde Lane -- nur so belastbar wie die Deklaration darueber");
    } else {
        fak("rekombination_fremde_lane", "n/a", MengenArt::Unbestimmbar,
            "keine --fremde-lane deklariert -- die zweite Lane wird nicht erfunden (fail-closed)");
    }

    // == Mess-Gesamtzahl: Teilmenge oder ehrliche Schranke =======================================================
    if (e.mess_teilmenge != 0u) {
        fak("mess_teilmenge", std::to_string(e.mess_teilmenge), MengenArt::Geschaetzt,
            "--mess-teilmenge (Interims-Eingang; XML-Traeger fehlt, run_options.cap ist No-op -- D.5/O3)");
    } else {
        fak("mess_teilmenge", "n/a", MengenArt::Unbestimmbar,
            "kein XML-Traeger (run_options.cap ist No-op) und kein --mess-teilmenge gereicht -- D.5/O3");
    }
    if (s.mess_ops_je_binary != 0u) {
        auto gesamt_rechnen = [&](std::uint64_t basis, std::uint64_t& out) -> bool {
            std::uint64_t g = 0u;
            if (!mul_sicher(basis, s.mess_ops_je_binary, g)) return false;
            if (!mul_sicher(g, s.zellen, g)) return false;
            if (!mul_sicher(g, s.rekombination, g)) return false;
            out = g;
            return true;
        };
        if (e.mess_teilmenge != 0u) {
            if (!gesamt_rechnen(e.mess_teilmenge, s.mess_ops_gesamt)) {
                s.ueberlauf = true;
                s.grund     = "mess_ops_gesamt laeuft ueber uint64";
                return s;
            }
            fak("mess_ops_gesamt", std::to_string(s.mess_ops_gesamt), MengenArt::Gerechnet,
                "mess_teilmenge * mess_ops_je_binary * zellen * rekombination");
        } else {
            fak("mess_ops_gesamt", "n/a", MengenArt::Unbestimmbar, "ohne Mess-Teilmenge nicht berechenbar");
        }
        if (!gesamt_rechnen(s.n_bau, s.mess_ops_schranke)) {
            s.ueberlauf = true;
            s.grund     = "mess_ops_schranke laeuft ueber uint64";
            return s;
        }
        fak("mess_ops_schranke", std::to_string(s.mess_ops_schranke), MengenArt::Gerechnet,
            "OBERE SCHRANKE (Teilmenge == n_bau) * mess_ops_je_binary * zellen * rekombination -- "
            "ausdruecklich Schranke, keine Mess-Menge");
    }

    // == (e) AUSGANG: Bau-ETA, Lager, OV-4 = f(T-3) ==============================================================
    if (e.bau_sekunden_je_dll > 0.0) {
        s.bau_eta_bekannt = true;
        s.bau_eta_h       = static_cast<double>(s.n_bau) * e.bau_sekunden_je_dll / 3600.0;
        fak("bau_sekunden_je_dll", mengen_detail::sekunden_text(e.bau_sekunden_je_dll), e.bau_sekunden_art,
            e.bau_sekunden_nenner);
        fak("bau_eta_h_einlanig", mengen_detail::sekunden_text(s.bau_eta_h), MengenArt::Gerechnet,
            "n_bau * bau_sekunden_je_dll / 3600 -- EINE Lane; Flotten-/Lane-Teilung modelliert dieser "
            "Bericht nicht (Pflicht-Nachkalibrierung beim ersten realen 4096er-Batch, KON58-03)");
    } else {
        fak("bau_eta_h_einlanig", "n/a", MengenArt::Unbestimmbar,
            "keine --bau-sekunden-je-dll gereicht -- es existiert kein gemessener 16W-Bau-Trace (O4/GN-9)");
    }
    if (e.bytes_je_dll != 0u) {
        if (!mul_sicher(s.n_bau, e.bytes_je_dll, s.lager_bytes)) {
            s.ueberlauf = true;
            s.grund     = "n_bau * bytes_je_dll laeuft ueber uint64";
            return s;
        }
        s.lager_bekannt = true;
        fak("bytes_je_dll", std::to_string(e.bytes_je_dll), e.bytes_je_dll_art, e.bytes_je_dll_nenner);
        fak("lager_bytes", std::to_string(s.lager_bytes), MengenArt::Gerechnet,
            "n_bau * bytes_je_dll -- der Lager-Bedarf der Bau-Menge (GN-9-Feasibility)");
    } else {
        fak("lager_bytes", "n/a", MengenArt::Unbestimmbar, "keine --bytes-je-dll gereicht (GN-9-Kalibrierwert)");
    }
    if (e.lager_budget_bytes != 0u) {
        s.lager_urteil = !s.lager_bekannt                          ? Deckelurteil::Unbestimmbar
                         : (s.lager_bytes <= e.lager_budget_bytes) ? Deckelurteil::Haelt
                                                                   : Deckelurteil::Gerissen;
    }
    if (e.mengen.sekunden_je_op > 0.0 && s.mess_ops_gesamt != 0u) {
        s.mess_eta_bekannt = true;
        s.mess_eta_h       = static_cast<double>(s.mess_ops_gesamt) * e.mengen.sekunden_je_op / 3600.0;
        fak("mess_eta_h", mengen_detail::sekunden_text(s.mess_eta_h), MengenArt::Geschaetzt,
            "mess_ops_gesamt * sekunden_je_op / 3600 -- so belastbar wie die Kalibrierung");
    }
    if (e.t3_fenster_tage > 0.0) {
        // OV-4 = f(T-3) (B.4): der Deckel ist ein AUSGANG -- wie viele Binaries passen mit (Bau +
        // 1-Thread-Messung) in das Fenster? Ohne BEIDE Kalibrierungen bleibt er unbestimmbar (fail-closed;
        // genau der O4-Punkt: ohne GN-9-Kalibrierlauf ist der Zeit-Deckel nicht bestimmbar).
        if (e.bau_sekunden_je_dll > 0.0 && e.mengen.sekunden_je_op > 0.0 && s.mess_ops_je_binary != 0u) {
            double const je_binary_s =
                e.bau_sekunden_je_dll + static_cast<double>(s.mess_ops_je_binary) * e.mengen.sekunden_je_op;
            double const fenster_s = e.t3_fenster_tage * 86400.0;
            s.ov4_bekannt          = true;
            s.ov4_deckel           = (je_binary_s > 0.0) ? static_cast<std::uint64_t>(fenster_s / je_binary_s) : 0u;
            s.zeit_urteil          = (s.n_bau <= s.ov4_deckel) ? Deckelurteil::Haelt : Deckelurteil::Gerissen;
            fak("ov4_deckel", std::to_string(s.ov4_deckel), MengenArt::Gerechnet,
                "floor(T-3-Fenster / (bau_s_je_dll + mess_ops_je_binary * sekunden_je_op)) -- AUSGANG: "
                "die gedeckelte Teilmatrix VOR GO-Vorlage ##51 (B.4)");
        } else {
            s.zeit_urteil = Deckelurteil::Unbestimmbar;
            fak("ov4_deckel", "n/a", MengenArt::Unbestimmbar,
                "T-3-Fenster verlangt, aber Bau- und/oder Op-Kalibrierung fehlt (O4: GN-9-Slot oder "
                "deklarierte Herabstufung) -- fail-closed");
        }
    }

    s.erhoben = true;
    return s;
}

// ---------------------------------------------------------------------------------------------------------------
// (f) KAMPAGNEN-AGGREGATION ueber MEHRERE XMLs: Summe der n_bau + FULL JOIN je Achse (Owner B.8/3:
// "FULL JOIN je Achse ueber alle Paper; Paper = genau EIN Experiment-XML").
// ---------------------------------------------------------------------------------------------------------------
struct KampagnenPosten {
    std::string        profil; ///< Profil-Basename (reist im Plan-Kopf mit)
    SimulationsEingang eingang;
    SimulationsSicht   sicht;
};

struct KampagnenSicht {
    bool                      erhoben = false;
    std::string               grund;
    bool                      ueberlauf = false;
    std::vector<MengenFaktor> je_profil;               ///< eine n_bau-Zeile je XML
    std::vector<MengenFaktor> union_achsen;            ///< je Achse |Union der Werte| ueber alle Paper
    std::uint64_t             summe_n_bau         = 0; ///< die B-4-ZAHL der Kampagne (Summe der Bau-Mengen)
    std::uint64_t             union_organ_produkt = 0; ///< FULL-JOIN-Produkt (Beobachtung, keine Bau-Zusage)
    std::uint64_t             union_system_perms  = 0; ///< |Union der opt+simd-Ids|
    std::uint64_t             union_n_bau         = 0; ///< union_organ_produkt * union_system_perms
};

[[nodiscard]] inline KampagnenSicht kampagnen_aggregation(std::vector<KampagnenPosten> const& posten) {
    KampagnenSicht k{};
    if (posten.empty()) {
        k.grund = "Keine XML gereicht -- eine Kampagne ohne Profil hat keine Menge.";
        return k;
    }
    std::map<std::string, std::set<std::string>> uni; // Achse -> Union der freigegebenen Werte
    std::set<std::string>                        perms_union;
    for (auto const& p : posten) {
        if (!p.sicht.erhoben) {
            k.grund = "Profil '" + p.profil + "' ist nicht erhoben (" + p.sicht.grund + ") -- eine Kampagne mit " +
                      "einem Loch ist keine Kampagne (fail-closed).";
            return k;
        }
        k.je_profil.push_back(MengenFaktor{"n_bau[" + p.profil + "]", std::to_string(p.sicht.n_bau),
                                           MengenArt::Gerechnet, "organ_produkt * system_perms dieser XML"});
        if (!add_sicher(k.summe_n_bau, p.sicht.n_bau, k.summe_n_bau)) {
            k.ueberlauf = true;
            k.grund     = "Summe der n_bau laeuft ueber uint64";
            return k;
        }
        for (auto const& ax : p.eingang.organ_achsen) uni[ax.name].insert(ax.werte.begin(), ax.werte.end());
        perms_union.insert(p.eingang.system_perm_ids.begin(), p.eingang.system_perm_ids.end());
    }
    k.union_organ_produkt = 1u;
    for (auto const& [name, werte] : uni) {
        k.union_achsen.push_back(MengenFaktor{"union[" + name + "]", std::to_string(werte.size()), MengenArt::Gerechnet,
                                              "FULL JOIN der Freigaben ueber alle Paper"});
        if (!mul_sicher(k.union_organ_produkt, static_cast<std::uint64_t>(werte.size()), k.union_organ_produkt)) {
            k.ueberlauf = true;
            k.grund     = "FULL-JOIN-Produkt laeuft ueber uint64 bei Achse '" + name + "'";
            return k;
        }
    }
    k.union_system_perms = static_cast<std::uint64_t>(perms_union.size());
    if (k.union_system_perms == 0u) k.union_system_perms = 1u; // Walk garantiert >=1 Perm; leer nur ohne Ernte
    if (!mul_sicher(k.union_organ_produkt, k.union_system_perms, k.union_n_bau)) {
        k.ueberlauf = true;
        k.grund     = "union_organ_produkt * union_system_perms laeuft ueber uint64";
        return k;
    }
    k.erhoben = true;
    return k;
}

// ---------------------------------------------------------------------------------------------------------------
// KANDIDATEN-GEGENPROBE (C.4): die gerechnete Zahl GEGEN die vom Aufrufer gereichten Kandidaten stellen
// und die Abweichung benennen. Die Kandidaten kommen als EINGANG (CLI) -- diese Datei kennt keine.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline std::string kandidaten_gegenprobe(std::uint64_t n_bau, std::vector<std::uint64_t> const& kand) {
    std::string o;
    if (kand.empty()) {
        o += "  (keine --kandidaten gereicht -- keine Gegenprobe)\n";
        return o;
    }
    for (auto const& c : kand) {
        o += "  kandidat " + std::to_string(c) + ": ";
        if (c == n_bau) {
            o += "TRIFFT die gerechnete Zahl\n";
        } else if (n_bau != 0u && c > n_bau && c % n_bau == 0u) {
            o += "Faktor " + std::to_string(c / n_bau) + " GROESSER als gerechnet\n";
        } else if (c != 0u && n_bau > c && n_bau % c == 0u) {
            o += "Faktor " + std::to_string(n_bau / c) + " KLEINER als gerechnet\n";
        } else {
            o += "weicht ab (kein ganzzahliges Verhaeltnis zu " + std::to_string(n_bau) + ")\n";
        }
    }
    return o;
}

// ---------------------------------------------------------------------------------------------------------------
// BERICHT EINER XML -- Faktor-Tabelle zuerst (check-size-Form), dann der B-4-Block, dann die HINWEISE.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline std::string simulations_bericht(SimulationsSicht const& s, SimulationsEingang const& e) {
    std::string o;
    o += "simulate -- S-19 PLANUNGS-SIMULATION (rein rechnend: baut KEINE DLL, misst NICHT)\n";
    o += "profil=" + (e.mengen.profile_id.empty() ? std::string{"?"} : e.mengen.profile_id) +
         " wurzel=" + (e.mengen.source_kind.empty() ? std::string{"?"} : e.mengen.source_kind) + "\n\n";

    if (!s.erhoben && s.faktoren.empty()) {
        o += "ERHEBUNG NICHT MOEGLICH\n  " + s.grund + "\n";
        return o;
    }
    if (!s.erhoben) o += "ERHEBUNG ABGEBROCHEN\n  " + s.grund + "\n\ndie bis zum Abbruch erhobenen Faktoren:\n";

    o += mengen_detail::breit("FAKTOR", 30) + mengen_detail::breit("MENGE", 22) + mengen_detail::breit("ART", 14) +
         "NENNER (woher die Zahl kommt)\n";
    o += std::string(120, '-') + "\n";
    for (auto const& f : s.faktoren) {
        o += mengen_detail::breit(f.name, 30) + mengen_detail::breit(f.wert, 22) +
             mengen_detail::breit(art_wort(f.art), 14) + f.nenner + "\n";
    }
    if (!s.erhoben) {
        o += "\ns19_simulation erhoben=nein grund_klasse=" + std::string{s.ueberlauf ? "ueberlauf" : "eingang"} +
             " urteil=ABGELEHNT\n";
        return o;
    }

    o += "\nDECKEL/AUSGANG (e)\n";
    o += "  lager: " + std::string{urteil_wort(s.lager_urteil)};
    if (e.lager_budget_bytes == 0u) {
        o += "  (kein --lager-budget-bytes gesetzt -- es wird nur berichtet)";
    } else {
        o += "  (lager_bytes=" + (s.lager_bekannt ? std::to_string(s.lager_bytes) : std::string{"n/a"}) +
             " gegen --lager-budget-bytes=" + std::to_string(e.lager_budget_bytes) + ")";
    }
    o += "\n  zeit:  " + std::string{urteil_wort(s.zeit_urteil)};
    if (e.t3_fenster_tage > 0.0) {
        o += "  (n_bau=" + std::to_string(s.n_bau) +
             " gegen ov4_deckel=" + (s.ov4_bekannt ? std::to_string(s.ov4_deckel) : std::string{"n/a"}) + ")";
    } else {
        o += "  (kein --t3-fenster-tage gesetzt -- es wird nur berichtet)";
    }
    o += "\n";

    o += "\nHINWEISE -- was an dieser Rechnung NICHT gemessen ist\n";
    o += "  1. mess-teilmenge: Bau-Menge != Mess-Menge (Owner). Es gibt heute keinen wirksamen XML-Traeger\n"
         "     der Teilmenge (run_options.cap ist No-op); mess_ops_schranke ist eine SCHRANKE, keine Menge.\n";
    o += "  2. rekombination: die Fakultaet steht auf der GEMESSENEN Torlage DIESES Hosts; eine zweite Lane\n"
         "     geht nur DEKLARIERT ein. iw/ima/imi(/i{pmc})-Tore haben kein XML-Feld und werden nicht erfunden\n"
         "     (D.2 -- Hypothesen-Tore, Traeger #53/PMC-Design).\n";
    o += "  3. arena-deckel: D.7 ENTSCHIEDEN (KON26-04) -- der worst-case ist der GESAMT-Faktor\n"
         "     arena_gesamt_faktor = drift * paar * retry (120er-Basis, planner_mengen_types.hpp);\n"
         "     drift_worst_arena und t15b-Klammer sind TEIL-Faktoren, keine konkurrierenden Lesarten.\n";
    o += "  4. lastsequenz: das Registry-Angebot traegt heute 1 Framework (ycsb); Sequenz-Permutationen ohne\n"
         "     Werte-Katalog bleiben unterbestimmt (D.3) -- dyn[workload] zaehlt die XML-Auswahl.\n";
    // L-07 (s19-FUND-1, par.27.1.I; Anker frisch 24.08. @943c70ee-Kette): reiner Hinweis-String,
    // kein Rechen-Delta.
    o += "  hybrid: der Hybrid-Mehrfach-Aufbau (GOAL VI.2) ist nicht modelliert; Hybrid-Belegung kommt\n"
         "     per XML (KON42-01(4)) und zaehlt dann als Freigabe -- die Dock-Zahl ist Programm-Deckel\n"
         "     (KON42-01(3)), kein Faktor.\n";
    if (e.mengen.binaries_je_perm != 0u && e.mengen.binaries_je_perm != s.organ_produkt) {
        o += "  5. GN-DIVERGENZ: COMDARE_GN_TOTAL (" + std::to_string(e.mengen.binaries_je_perm) +
             ") und organ_produkt (" + std::to_string(s.organ_produkt) +
             ") stimmen NICHT ueberein. Eine der beiden\n     Quellen beschreibt eine andere Kampagne; dieser "
             "Bericht waehlt keine Seite.\n";
    }

    o += "\ns19_simulation profil=" + (e.mengen.profile_id.empty() ? std::string{"?"} : e.mengen.profile_id) +
         " organ_produkt=" + std::to_string(s.organ_produkt) + " system_perms=" + std::to_string(s.system_perms) +
         " n_bau=" + std::to_string(s.n_bau) + " zellen=" + std::to_string(s.zellen) + "/" +
         std::to_string(e.mengen.zellen_gezaehlt) + " dyn_produkt=" + std::to_string(s.dyn_produkt) +
         " messungen_je_einstellung=" +
         (s.messungen_je_einstellung != 0u ? std::to_string(s.messungen_je_einstellung) : std::string{"n/a"}) +
         " messgeraete=" + std::to_string(s.messgeraete) + " rekombination=" + std::to_string(s.rekombination) +
         " pmc_tor=" + (e.pmc_tor ? "an" : "aus") +
         " mess_ops_gesamt=" + (s.mess_ops_gesamt != 0u ? std::to_string(s.mess_ops_gesamt) : std::string{"n/a"}) +
         " schranke=" + (s.mess_ops_schranke != 0u ? std::to_string(s.mess_ops_schranke) : std::string{"n/a"}) +
         " lager=" + urteil_wort(s.lager_urteil) + " zeit=" + urteil_wort(s.zeit_urteil) +
         " urteil=" + (s.abgelehnt() ? "ABGELEHNT" : "ok") + "\n";
    return o;
}

// ---------------------------------------------------------------------------------------------------------------
// KAMPAGNEN-BERICHT (f) -- die B-4-Zahl der Kampagne mit Rechenweg + Kandidaten-Gegenprobe.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline std::string kampagnen_bericht(KampagnenSicht const& k, std::vector<std::uint64_t> const& kand) {
    std::string o;
    o += "\n================ KAMPAGNEN-AGGREGATION (FULL JOIN je Achse ueber alle Paper) ================\n";
    if (!k.erhoben) {
        o += "NICHT ERHOBEN\n  " + k.grund + "\ns19_kampagne erhoben=nein urteil=ABGELEHNT\n";
        return o;
    }
    for (auto const& f : k.je_profil) {
        o += mengen_detail::breit(f.name, 40) + mengen_detail::breit(f.wert, 22) + f.nenner + "\n";
    }
    o += "\nFULL-JOIN-BEOBACHTUNG je Achse (Union der Freigaben):\n";
    for (auto const& f : k.union_achsen) {
        o += mengen_detail::breit(f.name, 40) + mengen_detail::breit(f.wert, 22) + f.nenner + "\n";
    }
    o += "\nB-4-ZAHL (Bau-Nenner des Triggers) = SUMME der n_bau ueber die gereichten XMLs: " +
         std::to_string(k.summe_n_bau) + "\n";
    o += "FULL-JOIN-Produkt (Union je Achse x Union der System-Perms): " + std::to_string(k.union_organ_produkt) +
         " x " + std::to_string(k.union_system_perms) + " = " + std::to_string(k.union_n_bau) +
         "  (Beobachtung der Achsen-Abdeckung, KEINE Bau-Zusage)\n";
    o += "\nKANDIDATEN-GEGENPROBE gegen summe_n_bau:\n" + kandidaten_gegenprobe(k.summe_n_bau, kand);
    o += "\ns19_kampagne profile=" + std::to_string(k.je_profil.size()) +
         " summe_n_bau=" + std::to_string(k.summe_n_bau) + " union_produkt=" + std::to_string(k.union_n_bau) +
         " urteil=ok\n";
    return o;
}

} // namespace comdare::cache_engine::planner
