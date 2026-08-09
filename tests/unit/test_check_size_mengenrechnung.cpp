// test_check_size_mengenrechnung.cpp -- check-size (2026-08-09): die MENGENRECHNUNG gegen HANDGERECHNETE
// Soll-Werte, und gegen die Eingaenge, bei denen sie NICHT gelten darf.
//
// == T-3 / T-5: WOHER DIE SOLL-WERTE KOMMEN =======================================================
// KEIN Soll-Wert dieser Datei stammt aus mengen_rechnen(). Alle sind von Hand gerechnet und mit bc
// (GNU bc, ausserhalb dieses Programms) gegengerechnet, bevor sie hier eingefroren wurden:
//
//   A  zeilen_je_op        = 2 + 2*1                    = 4
//      drift_faktor        = 3*(5+1)                    = 18
//      zeilen_je_op_batch  = 10*4*18                    = 720
//      arena_bytes         = 720*32                     = 23040
//   B  ops_gesamt          = 10*8192*6*18               = 8847360
//      dauer_s             = 8847360*0.001              = 8847.36
//      dauer_tage          = 8847.36/86400              = 0.1024
//   H  DIE 160-MiB-REKONSTRUKTION (der eigentliche Anlass dieses Kommandos):
//      131072*40*32*1                                   = 167772160 B  = 160.0000 MiB
//      131072*40*32*18                                  = 3019898880 B = 2880 MiB = 2.8125 GiB
//      Die im Umlauf befindliche 160-MiB-Zahl ist damit exakt rekonstruiert: sie ist 2^17 Ops x 40
//      Zeilen/Op x 32 B bei GENAU EINEM Durchlauf. Mit dem Drift-Gate (18) sind es 2880 MiB. Das ist
//      der Faktor-18-Fehler, um den es geht -- hier beziffert statt behauptet.
//
// == T-4: DIE GEGENEINGAENGE ======================================================================
// Zu jeder Zusicherung ein Eingang, bei dem sie NICHT gilt: Gate AUS (Faktor 1 statt 18), Deckel
// gerissen UND die Grenze selbst (kein off-by-one), Zeit-Deckel ohne Kalibrierung (unbestimmbar,
// NICHT "haelt"), leerer Walk, n_ops=0, nur-Makro (die Achszahl faellt weg), Zell-Divergenz.
// Dazu G5a..G5g: JEDE der fuenf Ueberlauf-Wachen mit einem Eingang, der GENAU SIE ausloest, plus
// einen Gegeneingang, bei dem dieselbe Zahl harmlos ist (G5f). Warum einzeln und nicht gesammelt:
// s. den protokollierten Mutations-Befund bei G5a.
//
// == WARUM STANDALONE OHNE GTEST ==================================================================
// Rezept gespiegelt von test_ms1_arenen_kein_alloc_im_fenster / test_ms2_pre_touch_seitenfehler
// (tests/unit/CMakeLists.txt): der Prueflung ist ein reiner Header ohne Bibliotheks-Anhang. Ein
// Test ohne Linkage laeuft ueberall in der 8-Distributions-Matrix und braucht keinen Katalog.

#include <profile_facade/planner/planner_mengen_types.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

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

/// Der Basis-Eingang von Beispiel A. Jeder Test aendert genau das, was er prueft.
pl::MengenEingang eingang_a() {
    pl::MengenEingang e{};
    e.perm_count         = 3;
    e.combo_count        = 2;
    e.zellen_gezaehlt    = 6;
    e.profile_axis_count = 1;
    e.tooling_leer       = true; // leer == volles Angebot => macro UND micro aktiv
    e.n_ops              = 10;
    e.n_ops_aus_xml      = true;
    e.drift_reps         = 3;
    e.drift_max_reruns   = 5;
    e.drift_aus_xml      = true;
    e.binaries_je_perm   = 8192;
    e.binaries_aus_env   = true;
    e.batch_korn         = 4096;
    e.profile_id         = "orakel_a";
    e.source_kind        = "thesis";
    return e;
}

} // namespace

int main() {
    std::printf("test_check_size_mengenrechnung -- handgerechnete Orakel gegen die Mengenrechnung\n");

    // == A: DIE KERN-RECHNUNG gegen die von Hand eingefrorenen Zahlen ==============================
    {
        auto const s = pl::mengen_rechnen(eingang_a());
        pruefe(s.erhoben, "A: erhoben");
        pruefe_gleich(s.zellen, 6u, "A: zellen == 3*2");
        pruefe_gleich(s.zeilen_je_op, 4u, "A: zeilen_je_op == 2 + 2*1");
        pruefe_gleich(s.drift_faktor, 18u, "A: drift_faktor == 3*(5+1)");
        pruefe_gleich(s.zeilen_je_op_batch, 720u, "A: zeilen_je_op_batch == 10*4*18");
        pruefe_gleich(s.arena_bytes_je_op_batch, 23040u, "A: arena_bytes == 720*32");
        pruefe_gleich(s.ops_gesamt, 8847360u, "A: ops_gesamt == 10*8192*6*18");
        pruefe(!s.abgelehnt(), "A: ohne Deckel wird nicht abgelehnt");
    }

    // == A': DIE AUSGABE IST DAS PRODUKT -- die Faktoren muessen EINZELN darin stehen ==============
    // T-2: nicht "der Bericht ist nicht leer", sondern: genau diese Faktoren, mit ihrem Nenner.
    {
        auto const e = eingang_a();
        auto const s = pl::mengen_rechnen(e);
        auto const b = pl::mengen_bericht(s, e);
        pruefe_enthaelt(b, "drift_faktor", "A': Faktor drift_faktor steht einzeln im Bericht");
        pruefe_enthaelt(b, "zeilen_je_op", "A': Faktor zeilen_je_op steht einzeln im Bericht");
        pruefe_enthaelt(b, "n_ops", "A': Faktor n_ops steht einzeln im Bericht");
        pruefe_enthaelt(b, "binaries_je_perm", "A': Faktor binaries_je_perm steht einzeln im Bericht");
        pruefe_enthaelt(b, "batch_korn", "A': Faktor batch_korn steht einzeln im Bericht");
        // Der NENNER gehoert in die AUSGABE, nicht in den Kopf:
        pruefe_enthaelt(b, "reps * (max_reruns + 1)", "A': der Nenner des Drift-Faktors steht im Bericht");
        pruefe_enthaelt(b, "PlanHeader.perm_count", "A': der Nenner der Perm-Zahl steht im Bericht");
        pruefe_enthaelt(b, "kapazitaet_zeilen_rechnen", "A': die gerufene Bestands-Funktion ist benannt");
        // Was GESCHAETZT statt gemessen ist, muss ausdruecklich dastehen:
        pruefe_enthaelt(b, "HINWEISE", "A': der Hinweis-Abschnitt existiert");
        pruefe_enthaelt(b, "ANNAHME", "A': die Achsen-Gleichsetzung ist als Annahme markiert");
        pruefe_enthaelt(b, "unbestimmbar", "A': die fehlende Dauer-Kalibrierung ist als unbestimmbar markiert");
        // Die Schlusszeile traegt den Nenner der Zellen mit (zellen=ist/soll):
        pruefe_enthaelt(b, "zellen=6/6", "A': die Schlusszeile traegt Produkt UND Walk-Zaehlung");
    }

    // == B: DIE DAUER -- nur mit ausdruecklicher Kalibrierung ======================================
    {
        auto e           = eingang_a();
        e.sekunden_je_op = 0.001;
        auto const s     = pl::mengen_rechnen(e);
        pruefe(s.dauer_bekannt, "B: mit Kalibrierung ist die Dauer bekannt");
        pruefe(std::fabs(s.dauer_s - 8847.36) < 1e-6, "B: dauer_s == 8847360*0.001");
        pruefe(std::fabs(s.dauer_tage - 0.1024) < 1e-9, "B: dauer_tage == 8847.36/86400");
    }

    // == H: DIE 160-MiB-REKONSTRUKTION und ihr Faktor-18-Gegenstueck ===============================
    // Das ist der Grund, warum es dieses Kommando gibt: dieselbe Konfiguration, einmal mit einem
    // Durchlauf gerechnet und einmal mit dem Drift-Gate -- 160 MiB gegen 2880 MiB.
    {
        auto e               = eingang_a();
        e.n_ops              = 131072; // 2^17 == COMDARE_GN_TOTAL des Voll-Baus
        e.profile_axis_count = 19;     // die 19 Organ-Achsen => zeilen_je_op = 2 + 2*19 = 40

        auto e_ohne_gate       = e;
        e_ohne_gate.drift_reps = 1; // reps < 2 == GATE AUS => genau EIN Durchlauf
        auto const s_ohne      = pl::mengen_rechnen(e_ohne_gate);
        pruefe_gleich(s_ohne.zeilen_je_op, 40u, "H: zeilen_je_op == 2 + 2*19");
        pruefe_gleich(s_ohne.drift_faktor, 1u, "H: Gate AUS => Faktor 1, nicht reps und nicht 0");
        pruefe_gleich(s_ohne.arena_bytes_je_op_batch, 167772160u, "H: 2^17*40*32*1 == 160 MiB");

        auto const s_mit = pl::mengen_rechnen(e); // reps=3, max_reruns=5 => 18
        pruefe_gleich(s_mit.drift_faktor, 18u, "H: Gate AN => 18");
        pruefe_gleich(s_mit.arena_bytes_je_op_batch, 3019898880u, "H: 2^17*40*32*18 == 2880 MiB");
        // Die Aussage, um die es geht: das Drift-Gate vervielfacht die Arena um exakt 18.
        pruefe_gleich(s_mit.arena_bytes_je_op_batch, s_ohne.arena_bytes_je_op_batch * 18u,
                      "H: der Faktor zwischen beiden ist exakt 18");
    }

    // == T-4 GEGENEINGANG 1: der Speicher-Deckel REISST ===========================================
    {
        auto e         = eingang_a();
        e.deckel_bytes = 23039; // ein Byte unter dem Bedarf von 23040
        auto const s   = pl::mengen_rechnen(e);
        pruefe(s.speicher_urteil == pl::Deckelurteil::Gerissen, "G1: 23039 < 23040 => GERISSEN");
        pruefe(s.abgelehnt(), "G1: ein gerissener Deckel lehnt den Lauf ab");
        pruefe_enthaelt(pl::mengen_bericht(s, e), "GERISSEN", "G1: der Bericht sagt GERISSEN");
    }
    // == T-4 GEGENEINGANG 2: die GRENZE selbst haelt (kein Off-by-one) ============================
    {
        auto e         = eingang_a();
        e.deckel_bytes = 23040; // exakt der Bedarf
        auto const s   = pl::mengen_rechnen(e);
        pruefe(s.speicher_urteil == pl::Deckelurteil::Haelt, "G2: 23040 == 23040 => haelt");
        pruefe(!s.abgelehnt(), "G2: die Grenze selbst lehnt nicht ab");
    }
    // == T-4 GEGENEINGANG 3: Zeit-Deckel OHNE Kalibrierung ist UNBESTIMMBAR, nicht "haelt" ========
    // Das ist die Stellvertreter-Falle dieses Kommandos: ein Deckel, der gruen meldet, weil die Zahl
    // fehlt -- nicht, weil die Kampagne passt.
    {
        auto e        = eingang_a();
        e.deckel_tage = 4.5; // der genannte Mess-Deckel
        // KEINE sekunden_je_op gereicht.
        auto const s = pl::mengen_rechnen(e);
        pruefe(!s.dauer_bekannt, "G3: ohne Kalibrierung ist die Dauer nicht bekannt");
        pruefe(s.zeit_urteil == pl::Deckelurteil::Unbestimmbar, "G3: verlangter Zeit-Deckel ohne Zahl => unbestimmbar");
        pruefe(s.abgelehnt(), "G3: unbestimmbar faellt fail-closed durch, es gilt NICHT als bestanden");
    }
    // == T-4 GEGENEINGANG 4: Zeit-Deckel MIT Kalibrierung, beide Richtungen =======================
    {
        auto e           = eingang_a();
        e.deckel_tage    = 4.5;
        e.sekunden_je_op = 0.001; // => 0.1024 Tage
        auto const s     = pl::mengen_rechnen(e);
        pruefe(s.zeit_urteil == pl::Deckelurteil::Haelt, "G4: 0.1024 <= 4.5 => haelt");

        auto e2           = e;
        e2.sekunden_je_op = 1.0; // => 8847360 s == 102.4 Tage
        auto const s2     = pl::mengen_rechnen(e2);
        pruefe(s2.zeit_urteil == pl::Deckelurteil::Gerissen, "G4: 102.4 > 4.5 => GERISSEN");
        pruefe(s2.abgelehnt(), "G4: der gerissene Zeit-Deckel lehnt ab");
    }
    // == T-4 GEGENEINGANG 5: UEBERLAUF -- JEDE Wache EINZELN beobachtbar ==========================
    //
    // WARUM HIER NICHT NUR s.ueberlauf GEPRUEFT WIRD -- ein protokollierter Befund aus der
    // Mutationsprobe zu dieser Datei (09.08.2026): die Wegwerf-Mutation "Ueberlauf-Wache vor
    // kapazitaet_zeilen_rechnen entfernt" UEBERLEBTE, weil eine SPAETERE Wache (ops_gesamt) denselben
    // Eingang faengt und dasselbe Flag setzt. Ein exit-Zweig, den ein spaeterer mit demselben
    // beobachtbaren Ergebnis verdeckt, ist nicht beobachtbar -- dort ueberlebt jeder Mutant.
    // Die Abhilfe ist zweifach: (a) jeder Eingang isoliert GENAU EINE Wache, (b) geprueft wird der
    // unterscheidende Grund-Text, nicht das gemeinsame Flag.
    {
        // 5a -- die ARENA-Wache allein: zeilen_je_op ist riesig, die Kampagnen-Faktoren sind winzig,
        //       also kann die ops_gesamt-Wache diesen Eingang gar nicht faengen.
        //       achsen=2^60 => zeilen_je_op = 2 + 2*2^60 ; 16 * (2+2^61) > uint64-Max
        auto e               = eingang_a();
        e.perm_count         = 1;
        e.combo_count        = 1;
        e.zellen_gezaehlt    = 1;
        e.binaries_je_perm   = 4096;
        e.n_ops              = 16;
        e.drift_reps         = 1;                    // Gate AUS => Faktor 1, damit der Faktor nichts beitraegt
        e.profile_axis_count = 1152921504606846976u; // 2^60
        auto const s         = pl::mengen_rechnen(e);
        pruefe(s.ueberlauf, "G5a: die Arena-Wache erkennt den Ueberlauf");
        pruefe(!s.erhoben, "G5a: bei Ueberlauf wird KEINE Endzahl erhoben");
        pruefe(s.abgelehnt(), "G5a: Ueberlauf lehnt ab");
        pruefe(s.grund.find("kapazitaet_zeilen_rechnen") != std::string::npos,
               "G5a: es war die ARENA-Wache, nicht irgendeine spaetere");
        // Der Bericht muss beim Abbruch die bis dahin erhobenen Faktoren MITZEIGEN -- wer nur
        // "Ueberlauf" liest, weiss nicht, welcher Faktor zu gross war.
        auto const b = pl::mengen_bericht(s, e);
        pruefe_enthaelt(b, "ERHEBUNG ABGEBROCHEN", "G5a: der Abbruch ist als solcher benannt");
        pruefe_enthaelt(b, "zeilen_je_op", "G5a: die erhobenen Faktoren stehen trotz Abbruch im Bericht");
        pruefe_enthaelt(b, "grund_klasse=ueberlauf", "G5a: die Schlusszeile klassifiziert den Abbruch");
        // Und KEINE Deckel-Aussage ueber Zahlen, die es nicht gibt:
        pruefe(b.find("DECKEL") == std::string::npos, "G5a: kein Deckel-Urteil ohne Gegenstand");
    }
    {
        // 5b -- die BYTE-Wache allein: die Zeilenzahl passt noch in uint64, ihre 32 Byte nicht mehr.
        //       n_ops=2^59, zeilen_je_op=2 (nur macro), drift=1 => 2^60 Zeilen; 2^60*32 = 2^65
        auto e            = eingang_a();
        e.perm_count      = 1;
        e.combo_count     = 1;
        e.zellen_gezaehlt = 1;
        e.tooling_leer    = false;
        e.ebene_macro     = true;
        e.ebene_micro     = false;
        e.drift_reps      = 1;
        e.n_ops           = 576460752303423488u; // 2^59
        auto const s      = pl::mengen_rechnen(e);
        pruefe(s.ueberlauf, "G5b: die Byte-Wache erkennt den Ueberlauf");
        pruefe(s.grund.find("byte_je_zeile") != std::string::npos,
               "G5b: es war die BYTE-Wache -- die Zeilenzahl selbst lief nicht ueber");
    }
    {
        // 5c -- die ZELL-Wache allein: perm_count * combo_count laeuft ueber.
        auto e        = eingang_a();
        e.perm_count  = 9223372036854775808u; // 2^63
        e.combo_count = 4;
        auto const s  = pl::mengen_rechnen(e);
        pruefe(s.ueberlauf, "G5c: die Zell-Wache erkennt den Ueberlauf");
        pruefe(s.grund.find("perm_count * combo_count") != std::string::npos, "G5c: es war die ZELL-Wache");
    }
    {
        // 5d -- die KAMPAGNEN-Wache allein: Arena und Bytes passen, erst die Kampagnen-Ops laufen ueber.
        auto e         = eingang_a();
        e.n_ops        = 1152921504606846976u; // 2^60
        e.tooling_leer = false;
        e.ebene_macro  = false;
        e.ebene_micro  = false; // zeilen_je_op = 0 => Arena und Bytes bleiben 0, keine Wache schlaegt dort an
        auto const s   = pl::mengen_rechnen(e);
        pruefe(s.ueberlauf, "G5d: die Kampagnen-Wache erkennt den Ueberlauf");
        pruefe(s.grund.find("binaries_je_perm") != std::string::npos, "G5d: es war die KAMPAGNEN-Wache");
    }
    {
        // 5e -- die ACHSEN-Wache allein: 2*achsen selbst laeuft ueber, noch vor jedem Produkt.
        //       Ein umgelaufenes zeilen_je_op waere eine ZU KLEINE Arena -- derselbe Fehler eine Ebene tiefer.
        auto e               = eingang_a();
        e.profile_axis_count = 9223372036854775808u; // 2^63; 2*2^63 laeuft ueber
        auto const s         = pl::mengen_rechnen(e);
        pruefe(s.ueberlauf, "G5e: die Achsen-Wache erkennt den Ueberlauf");
        pruefe(s.grund.find("2 * achsen") != std::string::npos, "G5e: es war die ACHSEN-Wache");
    }
    {
        // 5f -- GEGENEINGANG zur Achsen-Wache: dieselbe Achszahl, aber Mikro-Ebene AUS => kein Ueberlauf,
        //       weil die Achszahl dann gar nicht multipliziert wird.
        auto e               = eingang_a();
        e.profile_axis_count = 9223372036854775808u;
        e.tooling_leer       = false;
        e.ebene_macro        = true;
        e.ebene_micro        = false;
        auto const s         = pl::mengen_rechnen(e);
        pruefe(!s.ueberlauf, "G5f: ohne Mikro-Ebene ist dieselbe Achszahl harmlos");
        pruefe_gleich(s.zeilen_je_op, 2u, "G5f: nur macro => 2, die Achszahl bleibt aussen vor");
    }
    {
        // 5g -- batch_korn=0: eigener Zweig, eigener Grund (sonst von n_ops=0 verdeckt).
        auto e       = eingang_a();
        e.batch_korn = 0;
        auto const s = pl::mengen_rechnen(e);
        pruefe(!s.erhoben, "G5g: batch_korn=0 => nicht erhoben");
        pruefe(s.grund.find("batch_korn") != std::string::npos, "G5g: der Grund benennt batch_korn");
    }
    // == T-4 GEGENEINGANG 6: leerer Walk und n_ops=0 =============================================
    {
        auto e       = eingang_a();
        e.perm_count = 0;
        auto const s = pl::mengen_rechnen(e);
        pruefe(!s.erhoben, "G6: perm_count=0 => nicht erhoben");
        pruefe(!s.grund.empty(), "G6: der Grund ist nie leer, wenn nicht erhoben");

        auto e2       = eingang_a();
        e2.n_ops      = 0;
        auto const s2 = pl::mengen_rechnen(e2);
        pruefe(!s2.erhoben, "G6: n_ops=0 => nicht erhoben (kein stiller Default)");
    }
    // == T-4 GEGENEINGANG 7: nur Makro-Ebene => 2 Zeilen je Op, die Achszahl faellt weg ===========
    {
        auto e               = eingang_a();
        e.tooling_leer       = false;
        e.ebene_macro        = true;
        e.ebene_micro        = false;
        e.profile_axis_count = 19; // darf das Ergebnis NICHT beeinflussen, wenn micro aus ist
        auto const s         = pl::mengen_rechnen(e);
        pruefe_gleich(s.zeilen_je_op, 2u, "G7: nur macro => 2, die 19 Achsen zaehlen nicht mit");
        pruefe_gleich(s.zeilen_je_op_batch, 360u, "G7: 10*2*18 == 360");
    }
    // == T-4 GEGENEINGANG 8: die ZELL-DIVERGENZ wird gemeldet, nicht geglaettet ===================
    {
        auto e            = eingang_a();
        e.zellen_gezaehlt = 5; // Produkt sagt 6 -- eine der beiden Zahlen ist falsch
        auto const s      = pl::mengen_rechnen(e);
        auto const b      = pl::mengen_bericht(s, e);
        pruefe_enthaelt(b, "ZELL-DIVERGENZ", "G8: die Divergenz steht als Hinweis im Bericht");
        pruefe_enthaelt(b, "zellen=6/5", "G8: die Schlusszeile zeigt beide Zahlen");
    }

    // == T-4 GEGENEINGANG 9: BYTE-DECKEL bei zeilen_je_op == 0 ist UNBESTIMMBAR, nicht "haelt" ====
    // Der Stellvertreter-Fall des Byte-Deckels (Lens-Befund B-4, 09.08.2026): wallclock-only rechnet
    // die Arena zu NULL Bytes. Ein Deckel, der dann "haelt", haelt, WEIL nichts gerechnet wurde --
    // ununterscheidbar von einem Tooling-Fehlparse. VORLAEUFIG fail-closed (Owner-Entscheid offen,
    // s. Kommentar an mengen_rechnen Abschnitt 9); DIESER Test nagelt die getroffene Wahl fest.
    {
        auto e         = eingang_a();
        e.tooling_leer = false;
        e.ebene_macro  = false;
        e.ebene_micro  = false; // wallclock-only => zeilen_je_op = 0 => arena_bytes = 0
        e.deckel_bytes = 1;     // ein VERLANGTER Deckel -- jede Arena > 0 wuerde ihn reissen
        auto const s   = pl::mengen_rechnen(e);
        pruefe_gleich(s.zeilen_je_op, 0u, "G9: wallclock-only => zeilen_je_op == 0");
        pruefe_gleich(s.arena_bytes_je_op_batch, 0u, "G9: die Arena rechnet zu 0 Bytes");
        pruefe(s.speicher_urteil == pl::Deckelurteil::Unbestimmbar,
               "G9: verlangter Byte-Deckel ueber einer Null-Rechnung => unbestimmbar, NICHT haelt");
        pruefe(s.abgelehnt(), "G9: unbestimmbar faellt fail-closed durch");
        auto const b = pl::mengen_bericht(s, e);
        pruefe_enthaelt(b, "speicher=unbestimmbar", "G9: die Schlusszeile sagt speicher=unbestimmbar");
        pruefe_enthaelt(b, "fail-closed", "G9: der DECKEL-Block benennt den Grund");
    }
    // G9-GEGENEINGANG a: DERSELBE Eingang OHNE verlangten Deckel bleibt reiner Bericht (kein_deckel,
    // nicht abgelehnt) -- fail-closed heisst nicht, dass wallclock-only nicht mehr berichtet werden darf.
    {
        auto e         = eingang_a();
        e.tooling_leer = false;
        e.ebene_macro  = false;
        e.ebene_micro  = false;
        auto const s   = pl::mengen_rechnen(e);
        pruefe(s.speicher_urteil == pl::Deckelurteil::KeinDeckel, "G9a: ohne Deckel bleibt es kein_deckel");
        pruefe(!s.abgelehnt(), "G9a: ohne verlangten Deckel wird nicht abgelehnt");
    }
    // G9-GEGENEINGANG b: sobald EINE Ebene aktiv ist (zeilen_je_op > 0), urteilt derselbe Deckel wieder
    // normal -- der Unbestimmbar-Zweig verschluckt das echte Urteil nicht.
    {
        auto e         = eingang_a();
        e.tooling_leer = false;
        e.ebene_macro  = true;
        e.ebene_micro  = false; // zeilen_je_op = 2 => arena = 10*2*18*32 = 11520
        e.deckel_bytes = 1;
        auto const s   = pl::mengen_rechnen(e);
        pruefe_gleich(s.arena_bytes_je_op_batch, 11520u, "G9b: arena == 10*2*18*32");
        pruefe(s.speicher_urteil == pl::Deckelurteil::Gerissen, "G9b: 11520 > 1 => GERISSEN, nicht unbestimmbar");
    }

    // == NENNER DER TESTAUSSAGE SELBST -- nie eine nackte Zahl ====================================
    std::printf("%s: %d/%d Zusicherungen bestanden (Grundgesamtheit: alle Zusicherungen dieser Datei)\n",
                rot == 0 ? "GRUEN" : "ROT", gesamt - rot, gesamt);
    return rot == 0 ? 0 : 1;
}
