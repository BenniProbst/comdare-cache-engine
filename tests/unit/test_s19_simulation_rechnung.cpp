// test_s19_simulation_rechnung.cpp -- S-19 PLANUNGS-SIMULATION (#7, 2026-08-20): die Simulations-
// Rechnung gegen HANDGERECHNETE Soll-Werte, und gegen die Eingaenge, bei denen sie NICHT gelten darf.
//
// == T-3 / T-5: WOHER DIE SOLL-WERTE KOMMEN =======================================================
// KEIN Soll-Wert dieser Datei stammt aus simulation_rechnen(). Alle sind von Hand gerechnet (und mit
// GNU bc ausserhalb dieses Programms gegengerechnet), bevor sie hier eingefroren wurden:
//
//   S  organ_produkt        = 4*4*5*4*1                = 320
//      n_bau                = 320*2                    = 640
//      scheiben             = aufgerundet(640/4096)    = 1
//      zellen               = 2*1                      = 2
//      dyn_produkt          = 1*3*6*4                  = 72
//      messungen_je_einst.  = 3*3 (T-15 x KF-10)       = 9
//      drift_worst          = 3*(3+1)                  = 12   (Drift-Default 3 seit T-15b-Umzug; 120er-Basis)
//      mess_ops_je_binary   = 10*72*9                  = 6480
//      messgeraete          = 3+1 (PMC-Tor an)         = 4
//      rekombination        = 4!                       = 24
//      mess_ops_gesamt      = 5*6480*2*24              = 1555200
//      mess_ops_schranke    = 640*6480*2*24            = 199065600
//   E  bau_eta_h            = 640*9/3600               = 1.6
//      lager_bytes          = 640*428000               = 273920000
//      ov4_deckel           = floor(86400/(9+6480*0.001)) = floor(86400/15.48) = 5581
//                             (5581*15.48 = 86393.88 <= 86400; 5582*15.48 = 86409.36 > 86400)
//   F  Kampagne P1(2 Bauten) + P2(12 Bauten): summe = 14; Union A{a,b,c,d}=4 x B{x,y}=2 = 8;
//      Union-System-Perms = 2 -> union_n_bau = 16
//   R  Fakultaeten von Hand: 0!=1 1!=1 2!=2 3!=6 4!=24 5!=120 20!=2432902008176640000; 21! -> Wache
//
// == T-4: DIE GEGENEINGAENGE ======================================================================
// Keine Organ-Achse (nicht erhoben) | Achse mit 0 Werten | dyn-Dim mit 0 Werten | n_bau-Ueberlauf |
// mess_ops-Ueberlauf | 21!-Ueberlauf | KF-10 unbekannt (Experiment-Wurzel) -> geplante Messungen n/a,
// aber n_bau bleibt | Kampagne mit einem Loch (fail-closed) | Kandidat ohne ganzzahliges Verhaeltnis.
//
// == WARUM STANDALONE OHNE GTEST ==================================================================
// Rezept gespiegelt von test_check_size_mengenrechnung: der Pruefling ist ein reiner Header ohne
// Bibliotheks-Anhang -- laeuft ueberall in der Matrix, braucht weder Katalog noch Registry noch XML.

#include <profile_facade/planner/planner_simulation.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace pl = ::comdare::cache_engine::planner;

namespace {

int gesamt = 0;
int rot    = 0;

void pruefe(bool bedingung, char const* was) {
    ++gesamt;
    if (!bedingung) {
        ++rot;
        std::printf("  ROT : %s\n", was);
    }
}

void pruefe_gleich(std::uint64_t ist, std::uint64_t soll, char const* was) {
    ++gesamt;
    if (ist != soll) {
        ++rot;
        std::printf("  ROT : %s -- ist=%llu soll=%llu\n", was, static_cast<unsigned long long>(ist),
                    static_cast<unsigned long long>(soll));
    }
}

void pruefe_enthaelt(std::string const& heu, char const* nadel, char const* was) {
    ++gesamt;
    if (heu.find(nadel) == std::string::npos) {
        ++rot;
        std::printf("  ROT : %s -- '%s' fehlt in der Ausgabe\n", was, nadel);
    }
}

/// Der Basis-Eingang S (Kopf-Tabelle). Jeder Test aendert genau das, was er prueft.
pl::SimulationsEingang eingang_s() {
    pl::SimulationsEingang e{};
    auto                   achse = [&e](char const* name, std::vector<std::string> werte) {
        e.organ_achsen.push_back(pl::SimAchse{name, std::move(werte), pl::MengenArt::Xml, "Test von Hand"});
    };
    achse("search_algo", {"a", "b", "c", "d"});
    achse("node_type", {"n1", "n2", "n3", "n4"});
    achse("memory_layout", {"m1", "m2", "m3", "m4", "m5"});
    achse("prefetch", {"p1", "p2", "p3", "p4"});
    achse("mapping", {"x"});
    e.mengen.perm_count       = 2;
    e.mengen.combo_count      = 1;
    e.mengen.zellen_gezaehlt  = 2;
    e.mengen.n_ops            = 10;
    e.mengen.drift_reps       = 3;
    e.mengen.drift_max_reruns = 3; // T-15b-Umzug: Drift-Default 3 (die 5 zog zum Binary-Retry, KON26-04)
    e.mengen.batch_korn       = 4096;
    e.mengen.profile_id       = "s19_test";
    e.mengen.source_kind      = "thesis";
    e.system_perm_ids         = {"O2+avx2", "O3+no_extension"};
    auto dyn                  = [&e](char const* name, std::uint64_t n) {
        e.dyn_dims.push_back(pl::SimDynDim{name, n, pl::MengenArt::Xml, "Test von Hand"});
    };
    dyn("thread_count", 1);
    dyn("hw_prefetcher", 3);
    dyn("workload", 6);
    dyn("working_set", 4);
    e.kf10_repetitions   = 3;
    e.kf10_art           = pl::MengenArt::Default;
    e.kf10_nenner        = "Test von Hand";
    e.messgeraete_basis  = 3;
    e.messgeraete_art    = pl::MengenArt::Xml;
    e.messgeraete_nenner = "Test von Hand";
    e.pmc_tor            = true;
    e.pmc_lage_label     = "amd";
    e.pmc_nenner         = "Test von Hand";
    e.mess_teilmenge     = 5;
    return e;
}

// == S: der Erfolgs-Pfad mit allen Hand-Orakeln ===================================================
void s_basis() {
    auto const e = eingang_s();
    auto const s = pl::simulation_rechnen(e);
    pruefe(s.erhoben, "S: erhoben");
    pruefe(!s.ueberlauf, "S: kein Ueberlauf");
    pruefe_gleich(s.organ_produkt, 320u, "S: organ_produkt 4*4*5*4*1");
    pruefe_gleich(s.system_perms, 2u, "S: system_perms");
    pruefe_gleich(s.n_bau, 640u, "S: n_bau = 320*2");
    pruefe_gleich(s.scheiben, 1u, "S: scheiben aufgerundet 640/4096");
    pruefe_gleich(s.zellen, 2u, "S: zellen 2*1");
    pruefe_gleich(s.dyn_produkt, 72u, "S: dyn_produkt 1*3*6*4");
    pruefe_gleich(s.messungen_je_einstellung, 9u, "S: T-15 x KF-10 = 3*3");
    pruefe_gleich(s.drift_worst, 12u, "S: drift_worst 3*(3+1) -- Drift-Teil der 120er-Basis (D.7)");
    pruefe_gleich(s.mess_ops_je_binary, 6480u, "S: mess_ops_je_binary 10*72*9");
    pruefe_gleich(s.messgeraete, 4u, "S: messgeraete 3+1");
    pruefe_gleich(s.rekombination, 24u, "S: rekombination 4! (Hand-Tabelle)");
    pruefe_gleich(s.mess_ops_gesamt, 1555200u, "S: mess_ops_gesamt 5*6480*2*24");
    pruefe_gleich(s.mess_ops_schranke, 199065600u, "S: Schranke 640*6480*2*24");
    pruefe(!s.abgelehnt(), "S: ohne verlangte Deckel nicht abgelehnt");

    auto const b = pl::simulations_bericht(s, e);
    pruefe_enthaelt(b, "n_bau=640", "S: Schlusszeile traegt n_bau");
    pruefe_enthaelt(b, "rekombination=24", "S: Schlusszeile traegt rekombination");
    pruefe_enthaelt(b, "pmc_tor=an", "S: Schlusszeile traegt pmc_tor");
    pruefe_enthaelt(b, "NIE Vorgabe", "S: Rekombination als AUSGANG deklariert");
    pruefe_enthaelt(b, "OBERE SCHRANKE", "S: Schranke als Schranke benannt");
    pruefe_enthaelt(b, "NICHT in die geplante Menge multipliziert", "S: x5-Klammer nicht multipliziert");
    pruefe_enthaelt(b, "120er-Basis", "S: Arena-Deckel als D.7-entschiedene 120er-Basis deklariert (H-5)");
    pruefe_enthaelt(b, "urteil=ok", "S: Urteil ok");
}

// == R: die Fakultaets-Handtabelle + PMC-Tor-Zweige ===============================================
void r_fakultaet_und_tor() {
    struct Zeile {
        std::uint64_t n, soll;
    };
    Zeile const tabelle[] = {
        {0u, 1u}, {1u, 1u}, {2u, 2u}, {3u, 6u}, {4u, 24u}, {5u, 120u}, {20u, 2432902008176640000u}};
    for (auto const& z : tabelle) {
        std::uint64_t ist = 0u;
        pruefe(pl::fakultaet_sicher(z.n, ist), "R: fakultaet_sicher liefert eine Zahl");
        pruefe_gleich(ist, z.soll, "R: Fakultaets-Handtabelle");
    }
    std::uint64_t verworfen = 0u;
    pruefe(!pl::fakultaet_sicher(21u, verworfen), "R: 21! reisst die Wache (uint64)");

    // Tor AUS: dieselben 3 Geraete ohne PMC -> 3! = 6 (Hand). Das ist der Zweig, in dem "24 oder 48"
    // eben NICHT herauskommt -- der Ausgang haengt an der Torlage, nicht an einer Vorgabe.
    auto e       = eingang_s();
    e.pmc_tor    = false;
    auto const s = pl::simulation_rechnen(e);
    pruefe_gleich(s.messgeraete, 3u, "R: Tor aus -> 3 Geraete");
    pruefe_gleich(s.rekombination, 6u, "R: Tor aus -> 3! = 6 (Hand)");
    pruefe_enthaelt(pl::simulations_bericht(s, e), "PMC-Zeilen entfallen", "R: Tor-aus-Wort im Bericht");

    // Fremde Lane DEKLARIERT: intel mit PMC -> fremd 4! = 24, Summe 24+24 = 48 (Hand) -- die 48
    // ENTSTEHT aus zwei Torlagen, sie steht nirgends als Zahl im Code.
    auto e2              = eingang_s();
    e2.fremde_lane_label = "intel";
    e2.fremde_lane_pmc   = true;
    auto const s2        = pl::simulation_rechnen(e2);
    pruefe_gleich(s2.rekombination_fremd, 24u, "R: fremde Lane 4! = 24");
    pruefe_gleich(s2.rekombination_summe, 48u, "R: Summe beider Lanes 24+24 (Hand)");

    auto e3              = eingang_s();
    e3.fremde_lane_label = "unbrauchbar";
    e3.fremde_lane_pmc   = false;
    auto const s3        = pl::simulation_rechnen(e3);
    pruefe_gleich(s3.rekombination_fremd, 6u, "R: fremde Lane unbrauchbar 3! = 6");
    pruefe_gleich(s3.rekombination_summe, 30u, "R: Summe 24+6 (Hand)");
}

// == E: ETA/Lager/OV-4 als AUSGANG (Hand-Orakel Kopf-Tabelle E) ===================================
void e_deckel_ausgang() {
    auto e                  = eingang_s();
    e.bau_sekunden_je_dll   = 9.0;
    e.bau_sekunden_art      = pl::MengenArt::Geschaetzt;
    e.bau_sekunden_nenner   = "Test von Hand";
    e.bytes_je_dll          = 428000u;
    e.bytes_je_dll_art      = pl::MengenArt::Geschaetzt;
    e.bytes_je_dll_nenner   = "Test von Hand";
    e.lager_budget_bytes    = 300000000u;
    e.mengen.sekunden_je_op = 0.001;
    e.t3_fenster_tage       = 1.0;
    auto const s            = pl::simulation_rechnen(e);
    pruefe(s.bau_eta_bekannt, "E: Bau-ETA bekannt");
    pruefe(std::fabs(s.bau_eta_h - 1.6) < 1e-9, "E: bau_eta_h = 640*9/3600 = 1.6");
    pruefe_gleich(s.lager_bytes, 273920000u, "E: lager_bytes 640*428000");
    pruefe(s.lager_urteil == pl::Deckelurteil::Haelt, "E: Lager haelt unter 300 MB");
    pruefe(s.ov4_bekannt, "E: OV-4-Deckel berechnet");
    pruefe_gleich(s.ov4_deckel, 5581u, "E: ov4 = floor(86400/15.48) (Hand-Grenzprobe 5581/5582)");
    pruefe(s.zeit_urteil == pl::Deckelurteil::Haelt, "E: 640 <= 5581 haelt");
    pruefe(!s.abgelehnt(), "E: nichts gerissen");

    // Grenze und Riss: Budget GENAU auf der Zahl haelt; ein Byte darunter reisst (kein off-by-one).
    auto e2               = e;
    e2.lager_budget_bytes = 273920000u;
    pruefe(pl::simulation_rechnen(e2).lager_urteil == pl::Deckelurteil::Haelt, "E: Budget == Bedarf haelt");
    e2.lager_budget_bytes = 273919999u;
    auto const s2         = pl::simulation_rechnen(e2);
    pruefe(s2.lager_urteil == pl::Deckelurteil::Gerissen, "E: ein Byte darunter reisst");
    pruefe(s2.abgelehnt(), "E: gerissener Lager-Deckel lehnt ab");

    // Fail-closed: verlangte Deckel ohne Kalibrierung sind UNBESTIMMBAR, nicht "haelt".
    auto e3               = eingang_s();
    e3.lager_budget_bytes = 1u;
    e3.t3_fenster_tage    = 1.0;
    auto const s3         = pl::simulation_rechnen(e3);
    pruefe(s3.lager_urteil == pl::Deckelurteil::Unbestimmbar, "E: Lager ohne bytes_je_dll unbestimmbar");
    pruefe(s3.zeit_urteil == pl::Deckelurteil::Unbestimmbar, "E: OV-4 ohne Kalibrierungen unbestimmbar");
    pruefe(s3.abgelehnt(), "E: unbestimmbare verlangte Deckel lehnen ab");
    pruefe_enthaelt(pl::simulations_bericht(s3, e3), "fail-closed", "E: fail-closed benannt");
}

// == G: Gegeneingaenge -- wo die Rechnung NICHT gelten darf =======================================
void g_gegeneingaenge() {
    // G1: keine Organ-Achse -> nicht erhoben (keine Bau-Menge aus dem Nichts).
    auto g1 = eingang_s();
    g1.organ_achsen.clear();
    auto const s1 = pl::simulation_rechnen(g1);
    pruefe(!s1.erhoben, "G1: ohne Organ-Achsen nicht erhoben");
    pruefe_enthaelt(s1.grund, "Keine Organ-Achse", "G1: Grund benannt");

    // G2: eine freigegebene Achse mit 0 Werten ist ein Profil-Fehler, kein Faktor 0.
    auto g2 = eingang_s();
    g2.organ_achsen[2].werte.clear();
    auto const s2 = pl::simulation_rechnen(g2);
    pruefe(!s2.erhoben, "G2: 0-Werte-Achse nicht erhoben");
    pruefe_enthaelt(s2.grund, "memory_layout", "G2: Achsen-Name im Grund");

    // G3: dyn-Dim mit 0 Werten ist ein Sammler-Fehler, kein Faktor 0.
    auto g3              = eingang_s();
    g3.dyn_dims[1].werte = 0u;
    pruefe(!pl::simulation_rechnen(g3).erhoben, "G3: 0-Werte-DynDim nicht erhoben");

    // G4: n_bau-Ueberlauf (organ_produkt * system_perms) -> Wache, keine gewickelte Zahl.
    auto g4              = eingang_s();
    g4.mengen.perm_count = 0xFFFFFFFFFFFFFFFFull / 2u;
    auto const s4        = pl::simulation_rechnen(g4);
    pruefe(!s4.erhoben && s4.ueberlauf, "G4: n_bau-Ueberlauf gewacht");

    // G5: mess_ops-Ueberlauf (n_ops riesig) -> Wache.
    auto g5         = eingang_s();
    g5.mengen.n_ops = 1ull << 62;
    auto const s5   = pl::simulation_rechnen(g5);
    pruefe(!s5.erhoben && s5.ueberlauf, "G5: mess_ops-Ueberlauf gewacht");

    // G6: 21 Messgeraete -> 21!-Wache.
    auto g6              = eingang_s();
    g6.messgeraete_basis = 21u;
    g6.pmc_tor           = false;
    auto const s6        = pl::simulation_rechnen(g6);
    pruefe(!s6.erhoben && s6.ueberlauf, "G6: 21!-Ueberlauf gewacht");

    // G7: KF-10 unbekannt (Experiment-Wurzel) -> geplante Messungen n/a, aber n_bau bleibt eine Zahl.
    auto g7             = eingang_s();
    g7.kf10_repetitions = 0u;
    g7.kf10_art         = pl::MengenArt::Unbestimmbar;
    auto const s7       = pl::simulation_rechnen(g7);
    pruefe(s7.erhoben, "G7: ohne KF-10 trotzdem erhoben");
    pruefe_gleich(s7.n_bau, 640u, "G7: n_bau unabhaengig von KF-10");
    pruefe_gleich(s7.messungen_je_einstellung, 0u, "G7: geplante Messungen n/a");
    pruefe_gleich(s7.mess_ops_gesamt, 0u, "G7: keine Mess-Gesamtzahl aus Luecken");

    // G8: ohne Teilmenge bleibt die Gesamtzahl n/a -- nur die benannte Schranke steht.
    auto g8           = eingang_s();
    g8.mess_teilmenge = 0u;
    auto const s8     = pl::simulation_rechnen(g8);
    pruefe(s8.erhoben, "G8: ohne Teilmenge erhoben");
    pruefe_gleich(s8.mess_ops_gesamt, 0u, "G8: Gesamtzahl n/a");
    pruefe_gleich(s8.mess_ops_schranke, 199065600u, "G8: Schranke bleibt (Hand)");
}

// == F: Kampagnen-Aggregation (FULL JOIN je Achse) ================================================
void f_kampagne() {
    auto posten_von = [](char const* profil, std::vector<pl::SimAchse> achsen,
                         std::vector<std::string> perms) -> pl::KampagnenPosten {
        pl::KampagnenPosten p{};
        p.profil                         = profil;
        p.eingang                        = pl::SimulationsEingang{};
        p.eingang.organ_achsen           = std::move(achsen);
        p.eingang.system_perm_ids        = std::move(perms);
        p.eingang.mengen.perm_count      = static_cast<std::uint64_t>(p.eingang.system_perm_ids.size());
        p.eingang.mengen.combo_count     = 1u;
        p.eingang.mengen.zellen_gezaehlt = p.eingang.mengen.perm_count;
        p.eingang.mengen.n_ops           = 1u;
        p.eingang.mengen.batch_korn      = 4096u;
        p.eingang.kf10_repetitions       = 1u;
        p.eingang.messgeraete_basis      = 3u;
        p.sicht                          = pl::simulation_rechnen(p.eingang);
        return p;
    };
    auto a = [](char const* n, std::vector<std::string> w) {
        return pl::SimAchse{n, std::move(w), pl::MengenArt::Xml, "Test von Hand"};
    };
    std::vector<pl::KampagnenPosten> posten;
    posten.push_back(posten_von("p1.xml", {a("A", {"a", "b"}), a("B", {"x"})}, {"O3+no"}));
    posten.push_back(posten_von("p2.xml", {a("A", {"b", "c", "d"}), a("B", {"x", "y"})}, {"O3+no", "O2+avx"}));
    pruefe_gleich(posten[0].sicht.n_bau, 2u, "F: P1 n_bau 2*1*1");
    pruefe_gleich(posten[1].sicht.n_bau, 12u, "F: P2 n_bau 6*2");

    auto const k = pl::kampagnen_aggregation(posten);
    pruefe(k.erhoben, "F: Kampagne erhoben");
    pruefe_gleich(k.summe_n_bau, 14u, "F: Summe 2+12 (Hand)");
    pruefe_gleich(k.union_organ_produkt, 8u, "F: Union 4*2 (Hand)");
    pruefe_gleich(k.union_system_perms, 2u, "F: Union der Perm-Ids");
    pruefe_gleich(k.union_n_bau, 16u, "F: Union-Produkt 8*2");

    auto const b = pl::kampagnen_bericht(k, {640u, 14u, 7u, 5u});
    pruefe_enthaelt(b, "summe_n_bau=14", "F: Schlusszeile Summe");
    pruefe_enthaelt(b, "kandidat 14: TRIFFT", "F: Kandidat trifft");
    pruefe_enthaelt(b, "kandidat 7: Faktor 2 KLEINER", "F: Kandidat kleiner (14/7)");
    pruefe_enthaelt(b, "kandidat 5: weicht ab", "F: Kandidat ohne Verhaeltnis");
    pruefe_enthaelt(b, "KEINE Bau-Zusage", "F: Union als Beobachtung deklariert");

    // Fail-closed: ein Loch macht die Kampagne ungueltig, statt still weiterzurechnen.
    auto loch = posten;
    loch.push_back(pl::KampagnenPosten{"kaputt.xml", pl::SimulationsEingang{}, pl::SimulationsSicht{}});
    pruefe(!pl::kampagnen_aggregation(loch).erhoben, "F: Kampagne mit Loch nicht erhoben");
    pruefe(!pl::kampagnen_aggregation({}).erhoben, "F: leere Kampagne nicht erhoben");
}

} // namespace

int main() {
    std::printf("test_s19_simulation_rechnung -- Soll-Werte von Hand (T-3), Gegeneingaenge (T-4)\n");
    s_basis();
    r_fakultaet_und_tor();
    e_deckel_ausgang();
    g_gegeneingaenge();
    f_kampagne();
    std::printf("%s: %d Pruefungen, %d rot\n", (rot == 0) ? "GRUEN" : "ROT", gesamt, rot);
    return (rot == 0) ? 0 : 1;
}
