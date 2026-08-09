// test_ck1_messkette_koeder.cpp -- DIE ABNAHME DER MESSKETTE (CK-1, Owner-KERN 09.08.2026).
//
// == WAS HIER ABGENOMMEN WIRD ======================================================================
// Die Kausalkette des Owner-Befunds, an der WURZEL statt am Symptom:
//     1. die variadische Mess-Template-Variable wird durch die Gattung+Genus-Kaskade gereicht
//     2. => der Kanal kommt im Tier-Binary an
//     3. => init() beider Arenen ist beauftragbar
//     4. => flush() ist JE ARENA GETRENNT beauftragbar
//     5. => am Pruefdock kommen Signale an
//
// == DIE DREI AUSGAENGE, DIE UNTERSCHIEDEN WERDEN MUESSEN ==========================================
// Ein Test taugt nur, wenn er (a) einen RISS, (b) einen ueberlebenden MUTANTEN und (c) KAPUTTES
// WERKZEUG auseinanderhalten kann. Hier ist das so geloest:
//   (a) RISS            -> der Koeder-Wert am fernen Ende ist FALSCH (Vergleich gegen das Orakel)
//   (b) MUTANT ueberlebt-> der Koeder-Wert FEHLT, obwohl der Ueberlauf-Befund sauber ist. Ein
//                          sauberer Befund plus fehlende Zeile heisst: es wurde nie geschrieben.
//   (c) WERKZEUG kaputt -> der Aufbau-Befund meldet die fehlende Arena, oder nm fehlt. Beides wird
//                          AUSDRUECKLICH gemeldet, nie stillschweigend als GRUEN gewertet.
//
// == WARUM DAS ORAKEL UNABHAENGIG IST (T-5) ========================================================
// Die Koeder-Achse rechnet `(r % 4093) + 17`. Das Orakel unten rechnet dieselbe Zahl ueber die
// DIVISION (`r - (r / 4093) * 4093 + 17`) -- ein anderer Ausdruck, ein anderer Befehlsweg, dasselbe
// Ergebnis. Es zieht den Sollwert NICHT aus dem Prueflung-Header.
//
// EHRLICH BENANNT, weil es die Grenze der Unabhaengigkeit ist: beide Wege benutzen dieselbe Konstante
// 4093 und dieselbe 17. Eine Aenderung der Konstanten IM Prueflung wuerde von diesem Orakel bemerkt
// (es haelt seine eigenen Zahlen), eine Aenderung der FORMEL ebenfalls. Was es nicht bemerkte, waere
// ein Fehler, der in beiden Ausdruecken identisch steckt -- dagegen hilft kein Orakel, sondern nur
// eine zweite Quelle, und die gibt es fuer eine frei gewaehlte Koeder-Formel nicht.
//
// == DER KOEDER (K13) ==============================================================================
// R kommt frisch aus /dev/urandom, bei JEDEM Lauf neu. Kein fester Startwert, keine Uhrzeit als
// Ersatz. Ein Koeder, der ueber Laeufe hinweg gleich bleibt, kann von einem einkompilierten Wert
// nicht unterschieden werden.
//
// ASCII-only.

#include <builder/measure_storage/checkpoint_measure.hpp>
#include <mess/genus_kaskade.hpp>
#include <mess/konfiguration.hpp>
#include <mess/mess_naht.hpp>
#include <mess/pilot_achsen.hpp>
#include <mess/steuer_dock.hpp>
#include <mess/pilot_suche_impl.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace {

namespace mk = comdare::cache_engine::mess;
namespace pl = comdare::cache_engine::mess::pilot;
namespace ms = comdare::cache_engine::builder::measure_storage;

// ==================================================================================================
// DIE OBJEKT-PFADE -- aus fremder Quelle (CMake), nicht geraten
// ==================================================================================================
std::string g_obj_voll;
std::string g_obj_leer;

// ==================================================================================================
// DER KOEDER -- frisch aus /dev/urandom, beide Richtungen
// ==================================================================================================

/// Wuerfelt 64 Bit aus /dev/urandom. Schlaegt der Griff fehl, wird das GEMELDET und der Test faellt --
/// ein Ersatz-Startwert (Uhrzeit, feste Zahl) waere genau der Koeder, der nicht beisst.
[[nodiscard]] bool koeder_wuerfeln(std::uint64_t& heraus) {
    std::ifstream q("/dev/urandom", std::ios::binary);
    if (!q) return false;
    std::array<char, sizeof(std::uint64_t)> roh{};
    if (!q.read(roh.data(), static_cast<std::streamsize>(roh.size()))) return false;
    std::uint64_t w = 0;
    for (std::size_t i = 0; i < roh.size(); ++i) {
        w |= static_cast<std::uint64_t>(static_cast<unsigned char>(roh[i])) << (i * 8u);
    }
    heraus = w;
    return true;
}

/// DAS ORAKEL -- eigener Rechenweg (Division statt Modulo-Operator), kein geteilter Header.
[[nodiscard]] std::uint64_t orakel_koeder(std::uint64_t r) noexcept {
    std::uint64_t const teiler = 4093ull;
    std::uint64_t const ganze  = r / teiler;
    std::uint64_t const rest   = r - (ganze * teiler);
    return rest + 17ull;
}

// ==================================================================================================
// DIE SENKEN -- das FERNE ENDE der Kette
// ==================================================================================================
//
// Sie stehen fuer die CEB-Seite: was hier ankommt, ist das, was die CEB nach dem flush in der Hand
// haelt. Die Mess-Senke kopiert die Zeilen, weil der Zeiger nur waehrend des Aufrufs gilt.

struct SammelMessSenke {
    std::vector<ms::MessCheckpointZeile> zeilen{};
    ms::UeberlaufBefund                  befund{};
    bool                                 bedient = false;

    void mess_aufnehmen(ms::AusleseErgebnis const& e) noexcept {
        zeilen.assign(e.zeilen.begin(), e.zeilen.end());
        befund  = e.befund;
        bedient = true;
    }
};

struct SammelStapelSenke {
    std::vector<ms::StapelEintrag> offene{};
    ms::StapelBefund               befund{};
    bool                           bedient = false;

    void stapel_aufnehmen(ms::StapelBefund const& b, std::span<ms::StapelEintrag const> o) noexcept {
        offene.assign(o.begin(), o.end());
        befund  = b;
        bedient = true;
    }
};

static_assert(ms::MessSenkeConcept<SammelMessSenke>, "die Mess-Senke muss die Senken-Flaeche erfuellen");
static_assert(ms::StapelSenkeConcept<SammelStapelSenke>, "die Stapel-Senke muss die Senken-Flaeche erfuellen");

// ==================================================================================================
// DER PILOT -- die Genus-Flaeche, die das Pruefdock sieht
// ==================================================================================================
template <class MK>
using PilotGenus = mk::GenusInterface<pl::SucheImplFabrik<pl::PilotKomposition>, MK>;

// ==================================================================================================
// DIE ERKENNUNGSMUSTER fuer die DOPPELFORM -- sauberer SFINAE-Kontext (s. Begruendung im Test)
// ==================================================================================================

/// Ist `genus.fahren(last)` -- also die STILLE Form OHNE Visitor -- aufrufbar?
template <class G, class L, class = void>
struct HatStilleForm : std::false_type {};
template <class G, class L>
struct HatStilleForm<G, L, std::void_t<decltype(std::declval<G const&>().fahren(std::declval<L const&>()))>>
    : std::true_type {};

/// Ist `genus.fahren(last, visitor)` -- also die MESSENDE Form MIT Visitor -- aufrufbar?
template <class G, class L, class MK, class = void>
struct HatMessendeForm : std::false_type {};
template <class G, class L, class MK>
struct HatMessendeForm<G, L, MK,
                       std::void_t<decltype(std::declval<G const&>().fahren(
                           std::declval<L const&>(), std::declval<mk::MessVisitor<MK>&>()))>> : std::true_type {};

/// Zaehlt die Zeilen einer Station/Richtung und liefert die erste davon. Der NENNER (wie viele Zeilen
/// insgesamt) wird immer mitgefuehrt -- eine Zahl ohne Nenner laesst offen, ob eine von zwei oder
/// eine von zwei Millionen gemeint ist.
struct Fund {
    bool          gefunden = false;
    std::uint64_t position = 0;
    std::uint64_t messwert = 0;
    std::uint64_t treffer  = 0;
};

[[nodiscard]] Fund zeile_suchen(std::vector<ms::MessCheckpointZeile> const& zeilen, std::uint32_t deskriptor_ix,
                                ms::MessEbene ebene, ms::Richtung richtung) {
    Fund f{};
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        auto const& z = zeilen[i];
        if (z.deskriptor_ix != deskriptor_ix) continue;
        if (ms::ebene_aus_tag(z.tag) != ebene) continue;
        if (ms::richtung_aus_tag(z.tag) != richtung) continue;
        ++f.treffer;
        if (!f.gefunden) {
            f.gefunden = true;
            f.position = static_cast<std::uint64_t>(i);
            f.messwert = z.messwert;
        }
    }
    return f;
}

// ==================================================================================================
// ABNAHME 1 -- EIN ECHTER, FRISCH GEWUERFELTER WERT KOMMT DURCH DIE KETTE AN
// ==================================================================================================
TEST(Ck1Messkette, KoederWertKommtAmFernenEndeAn) {
    std::uint64_t R = 0;
    ASSERT_TRUE(koeder_wuerfeln(R)) << "WERKZEUG KAPUTT: /dev/urandom nicht lesbar -- kein Ersatz-Startwert.";

    std::uint64_t const soll = orakel_koeder(R); // VOR dem Lauf gerechnet (Zeitrichtung)

    ms::CheckpointMeasure<mk::Voll> instrument{};
    ms::MessMasse const             masse{4096u, 64u, ms::VorabBeruehrung::Ja};
    auto const                      aufbau = instrument.init(masse);
    ASSERT_TRUE(aufbau.mess) << "WERKZEUG KAPUTT: die MESS-Arena steht nicht.";
    ASSERT_TRUE(aufbau.stapel) << "WERKZEUG KAPUTT: die STAPEL-Arena steht nicht.";

    PilotGenus<mk::Voll>      genus{};
    mk::MessVisitor<mk::Voll> visitor{instrument, 0u};
    pl::Last const            last{R, 3u};

    auto const erg = genus.fahren(last, visitor);

    SammelMessSenke senke{};
    auto const      flush = instrument.flush_mess(senke);
    ASSERT_TRUE(flush.senke_bedient);
    ASSERT_TRUE(senke.bedient);

    auto const fund =
        zeile_suchen(senke.zeilen, pl::StationAchseKoeder::deskriptor_ix, ms::MessEbene::Micro, ms::Richtung::Aus);

    // DER NENNER, in der Ausgabe -- nicht im Kommentar.
    std::printf("[CK-1] Koeder R=%llu soll=%llu | Zeilen gesamt=%llu (Kapazitaet %llu, verloren %llu) | "
                "Koeder-Treffer=%llu an Position %llu | Wert=%llu | Achsen gerufen=%llu\n",
                static_cast<unsigned long long>(R), static_cast<unsigned long long>(soll),
                static_cast<unsigned long long>(senke.zeilen.size()),
                static_cast<unsigned long long>(flush.befund.kapazitaet),
                static_cast<unsigned long long>(flush.befund.verloren), static_cast<unsigned long long>(fund.treffer),
                static_cast<unsigned long long>(fund.position), static_cast<unsigned long long>(fund.messwert),
                static_cast<unsigned long long>(erg.achsen_gerufen));

    // AUSGANG (c): Werkzeug kaputt -- sauberer Befund, aber keine Zeile.
    ASSERT_FALSE(flush.befund.hat_verlust())
        << "Ueberlauf: die Arena war zu klein -- der fehlende Koeder waere dann kein Riss, sondern Verlust.";
    ASSERT_TRUE(fund.gefunden) << "MUTANT UEBERLEBT: der Ueberlauf-Befund ist sauber, aber die Koeder-Zeile "
                                  "fehlt -- es wurde nie geschrieben. Die Kette ist durchtrennt.";

    // AUSGANG (a): Riss -- der Wert ist da, aber falsch.
    EXPECT_EQ(fund.messwert, soll) << "RISS: der Wert kam an, ist aber nicht f(R).";
    EXPECT_EQ(fund.treffer, 1u) << "genau EINE Koeder-AUS-Zeile je Durchlauf erwartet.";

    // Der Rueckgabeweg (nicht die Mess-Kette) muss dasselbe sagen -- zwei unabhaengige Wege, ein Wert.
    EXPECT_EQ(erg.koeder_ergebnis, soll);
    EXPECT_EQ(erg.achsen_gerufen, 2u);

    // Die erwartete Zeilenzahl kommt aus der FALTUNG, nicht aus einer gepflegten Konstante.
    EXPECT_EQ(senke.zeilen.size(), pl::zeilen_je_op<mk::Voll>())
        << "die Zahl der Zeilen muss der gefalteten Erwartung entsprechen.";
}

// ==================================================================================================
// ABNAHME 2 -- DIE STAPEL-ARENA WIRD GETRENNT BEAUFTRAGT UND MELDET INHALTLICH
// ==================================================================================================
TEST(Ck1Messkette, StapelArenaGetrenntUndAusgeglichen) {
    std::uint64_t R = 0;
    ASSERT_TRUE(koeder_wuerfeln(R)) << "WERKZEUG KAPUTT: /dev/urandom nicht lesbar.";

    ms::CheckpointMeasure<mk::Voll> instrument{};
    auto const                      aufbau = instrument.init(ms::MessMasse{4096u, 64u, ms::VorabBeruehrung::Ja});
    ASSERT_TRUE(aufbau.steht());

    PilotGenus<mk::Voll>      genus{};
    mk::MessVisitor<mk::Voll> visitor{instrument, 0u};
    (void)genus.fahren(pl::Last{R, 2u}, visitor);

    SammelStapelSenke senke{};
    auto const        flush = instrument.flush_stapel(senke);

    std::printf("[CK-2] Stapel: offen=%llu von Kapazitaet %llu | hoechststand=%llu | zu_tief=%llu | "
                "unpaarige_aus=%llu\n",
                static_cast<unsigned long long>(flush.befund.offen),
                static_cast<unsigned long long>(flush.befund.kapazitaet),
                static_cast<unsigned long long>(flush.befund.hoechststand),
                static_cast<unsigned long long>(flush.befund.zu_tief),
                static_cast<unsigned long long>(flush.befund.unpaarige_aus));

    ASSERT_TRUE(flush.senke_bedient);
    EXPECT_TRUE(flush.ausgeglichen()) << "ein ausgeglichener Lauf darf keine der drei Fehlerklassen melden.";
    EXPECT_EQ(flush.befund.offen, 0u);
    EXPECT_EQ(flush.befund.unpaarige_aus, 0u);
    EXPECT_EQ(flush.befund.zu_tief, 0u);
    // Die Schachtelung des Piloten ist Makro{ Mikro, Mikro } -- Hoechststand 2, nachgerechnet, nicht geraten.
    EXPECT_EQ(flush.befund.hoechststand, 2u) << "die Schachtelungstiefe des Piloten ist 2 (Makro ueber Mikro).";
    EXPECT_EQ(senke.offene.size(), 0u);
}

// ==================================================================================================
// ABNAHME 2b -- GEGENEINGANG (T-4): ein absichtlich offen gelassenes EIN muss BENANNT werden
// ==================================================================================================
TEST(Ck1Messkette, OffenesEinWirdBenanntUndNichtAufgeraeumt) {
    ms::CheckpointMeasure<mk::Voll> instrument{};
    ASSERT_TRUE(instrument.init(ms::MessMasse{256u, 16u, ms::VorabBeruehrung::Ja}).steht());

    mk::MessVisitor<mk::Voll> visitor{instrument, 7u};

    // Ein EIN ohne AUS -- die Regression, die der Stapel anzeigen SOLL.
    visitor.checkpoint_ein<ms::MessEbene::Micro, pl::StationAchseKoeder>(1234u);

    SammelStapelSenke senke{};
    auto const        flush = instrument.flush_stapel(senke);

    std::printf("[CK-2b] Gegeneingang: offen=%llu (erwartet 1), herausgegebene Eintraege=%llu\n",
                static_cast<unsigned long long>(flush.befund.offen),
                static_cast<unsigned long long>(senke.offene.size()));

    EXPECT_FALSE(flush.ausgeglichen()) << "ein offenes EIN ist eine REGRESSION und darf nicht als sauber gelten.";
    ASSERT_EQ(flush.befund.offen, 1u);
    // INHALT, nicht Existenz: WELCHES EIN offen blieb.
    ASSERT_EQ(senke.offene.size(), 1u) << "der Befund muss den offenen Eintrag HERAUSGEBEN, nicht nur zaehlen.";
    EXPECT_EQ(senke.offene[0].deskriptor_ix, pl::StationAchseKoeder::deskriptor_ix);
    EXPECT_EQ(senke.offene[0].ebene, static_cast<std::uint8_t>(ms::MessEbene::Micro));
    EXPECT_EQ(senke.offene[0].thread_nr, 7u);
}

// ==================================================================================================
// ABNAHME 3 -- BEI DEAKTIVIERT KOMMT KEIN WERT, UND ES ENTSTEHT NACHWEISLICH KEIN AUFRUF
// ==================================================================================================
//
// Der Nachweis ist AM OBJEKT und nicht als Behauptung: zwei Uebersetzungseinheiten, die sich in genau
// einer Zeile unterscheiden, werden in ihren Symboltabellen verglichen.
//
// GEMESSENE EINSICHT (09.08.2026, prod1, g++ 15.3), die den Test formt: bei -O2 verschwinden die
// BENANNTEN Template-Symbole aus BEIDEN Objekten (alles inline). Der Unterschied, der auf BEIDEN
// Stufen traegt, ist der externe Aufruf `mmap` -- die Arena besorgt ihre Seiten ueber ihn, und er
// laesst sich nicht wegoptimieren. Genau deshalb prueft dieser Test mmap und nicht die
// Template-Namen: eine Zusage, die nur bei -O0 gilt, waere im Release wertlos, und der Release ist
// die Stufe, auf der gemessen wird.

[[nodiscard]] bool nm_zaehlen(std::string const& objekt, std::string const& muster, std::uint64_t& treffer,
                              std::uint64_t& symbole_gesamt) {
    std::string const befehl = "nm -C '" + objekt + "' 2>/dev/null";
    std::FILE*        rohr   = ::popen(befehl.c_str(), "r");
    if (rohr == nullptr) return false;
    treffer        = 0;
    symbole_gesamt = 0;
    char zeile[4096];
    while (std::fgets(zeile, sizeof(zeile), rohr) != nullptr) {
        ++symbole_gesamt;
        if (std::string{zeile}.find(muster) != std::string::npos) ++treffer;
    }
    int const rc = ::pclose(rohr);
    return rc == 0 && symbole_gesamt > 0;
}

TEST(Ck1Messkette, DeaktiviertErzeugtKeinenAufruf) {
    ASSERT_FALSE(g_obj_voll.empty())
        << "WERKZEUG KAPUTT: der Pfad des VOLL-Objekts kam nicht an (CMake reicht ihn als Argument).";
    ASSERT_FALSE(g_obj_leer.empty()) << "WERKZEUG KAPUTT: der Pfad des LEER-Objekts kam nicht an.";

    std::uint64_t voll_mmap = 0, voll_ges = 0, leer_mmap = 0, leer_ges = 0;
    ASSERT_TRUE(nm_zaehlen(g_obj_voll, "mmap", voll_mmap, voll_ges))
        << "WERKZEUG KAPUTT: nm lieferte keine Symbole fuer " << g_obj_voll;
    ASSERT_TRUE(nm_zaehlen(g_obj_leer, "mmap", leer_mmap, leer_ges))
        << "WERKZEUG KAPUTT: nm lieferte keine Symbole fuer " << g_obj_leer;

    std::printf("[CK-3] VOLL: mmap=%llu von %llu Symbolen | LEER: mmap=%llu von %llu Symbolen\n",
                static_cast<unsigned long long>(voll_mmap), static_cast<unsigned long long>(voll_ges),
                static_cast<unsigned long long>(leer_mmap), static_cast<unsigned long long>(leer_ges));

    // DIE POSITIVE RICHTUNG: der Vergleicher KANN anschlagen. Ohne sie waere "leer hat 0" wertlos --
    // eine Pruefung, die immer 0 findet, findet auch dann 0, wenn sie kaputt ist.
    EXPECT_GT(voll_mmap, 0u) << "GEGENPROBE GESCHEITERT: auch das VOLL-Objekt zeigt keinen mmap-Aufruf. "
                                "Dann prueft dieser Test nichts -- der Koeder beisst nicht.";

    // DIE ZU BELEGENDE RICHTUNG: bei deaktivierter Messung entsteht KEIN Aufruf.
    EXPECT_EQ(leer_mmap, 0u) << "die deaktivierte Uebersetzung enthaelt einen Arena-Aufruf -- "
                                "die Messeinrichtung existiert dort, obwohl sie nicht existieren darf.";

    // Und der Groessenunterschied als zweite, unabhaengige Aussage.
    EXPECT_LT(leer_ges, voll_ges) << "das deaktivierte Objekt muss weniger Symbole tragen als das volle.";
}

// ==================================================================================================
// ABNAHME 3b -- DIE COMPILE-HARTEN RIEGEL (was der Uebersetzer erzwingt)
// ==================================================================================================
TEST(Ck1Messkette, CompileHarteRiegelStehen) {
    // Die leere Konfiguration ist inaktiv, die volle aktiv -- und die Ordnung ist Identitaet.
    static_assert(!mk::aktiv<mk::Leer>);
    static_assert(mk::aktiv<mk::Voll>);
    static_assert(mk::Konfiguration<mk::Wallclock, mk::Makro>::tag() !=
                  mk::Konfiguration<mk::Makro, mk::Wallclock>::tag());

    // Die Kaskade ist geschlossen: der Tag der Gattung ist der Tag der Konfiguration.
    static_assert(mk::kette_geschlossen<PilotGenus<mk::Voll>>);
    static_assert(mk::kette_geschlossen<PilotGenus<mk::Leer>>);

    // Die Doppelform: bei aktiv nimmt fahren() einen Visitor, bei leer NICHT.
    //
    // WARUM HIER DAS void_t-ERKENNUNGSMUSTER STEHT UND KEIN requires-AUSDRUCK (Befund am Objekt,
    // 09.08.2026): `static_assert(!requires(G g, L l){ g.fahren(l); })` ist mit g++ 15.3 ein HARTER
    // Uebersetzungsfehler statt eines `false` -- der Uebersetzer meldet "no matching function for
    // call" aus dem Inneren des requires-Ausdrucks heraus, statt die Substitution scheitern zu
    // lassen. Das void_t-Muster ist ein sauberer SFINAE-Kontext und liefert zuverlaessig false.
    //
    // Der Unterschied ist nicht kosmetisch: mit dem requires-Ausdruck haette dieser Test gar nicht
    // uebersetzt, und die Zusage "der Parameter existiert nicht" waere ungeprueft geblieben.
    static_assert(HatStilleForm<PilotGenus<mk::Leer>, pl::Last>::value,
                  "bei LEERER Konfiguration muss fahren(last) OHNE Visitor aufrufbar sein");
    static_assert(!HatStilleForm<PilotGenus<mk::Voll>, pl::Last>::value,
                  "bei aktiver Messung darf fahren() OHNE Visitor NICHT aufrufbar sein -- "
                  "der Visitor ist Pflichtparameter, kein Default");
    static_assert(HatMessendeForm<PilotGenus<mk::Voll>, pl::Last, mk::Voll>::value,
                  "bei aktiver Konfiguration muss fahren(last, visitor) aufrufbar sein");

    // Eine Ebene ohne einkompiliertes Instrument ist nicht aufrufbar.
    using NurWall = mk::Konfiguration<mk::Wallclock>;
    static_assert(!mk::EbeneEingebaut<NurWall, ms::MessEbene::Micro>);
    static_assert(mk::EbeneEingebaut<mk::Voll, ms::MessEbene::Micro>);

    std::printf("[CK-3b] compile-harte Riegel: 10 static_assert gehalten (Tag-Ordnung, Kette, Doppelform, Ebene)\n");
    SUCCEED();
}

// ==================================================================================================
// ABNAHME 4 -- DER STEUERKANAL: SECHS DOCKS, und jedes passt nur auf seine CEB
// ==================================================================================================
//
// Owner 09.08.2026: "der sich ebenfalls VARIADISCH AN DIE 3 FAKULTAET vorhandenen CEB Versionen
// jeweils anpassen muss, damit hat der Planer 6 STEUERDOCKS -- fuer jede CEB einen, der GENAU AUF
// VORHANDENE EINKOMPILIERTE MESSEINRICHTUNGEN PASST".
//
// Was hier NICHT abgenommen wird, damit die Zusage nicht groesser klingt als sie ist: der Betrieb des
// Kanals (Typestate-Fenster, fd-Kanarie, Freigabe/Durchsetzung) ist Paket P2 und ungebaut.
TEST(Ck1Messkette, SechsSteuerdocksUndPassgenauigkeit) {
    using DockVoll = mk::SteuerDock<mk::Voll>;
    using DockWall = mk::SteuerDock<mk::Konfiguration<mk::Wallclock>>;

    // Die SECHS -- gefaltet, nicht gezaehlt. Der NENNER steht im Code.
    static_assert(mk::detail::ZuDocks<mk::CebPermutationen>::anzahl == 6);
    // Und die Gegenprobe, dass die Faltung wirklich rechnet (sonst waere die 6 eine Konstante).
    static_assert(mk::detail::Permutationen<mk::detail::Liste<mk::Wallclock, mk::Makro>>::typ::anzahl == 2);

    // Ein Befehl, den kein einkompiliertes Instrument anbietet, ist nicht sendbar.
    static_assert(mk::BefehlErlaubt<mk::FlushMessBefehl, mk::Voll>);
    static_assert(mk::BefehlErlaubt<mk::StartBefehl, mk::Voll>);

    // PASSGENAUIGKEIT: die eigene CEB dockt an, eine fremde wird LAUT abgewiesen.
    mk::CebBeschreibung const eigen{DockVoll::mess_tag, DockVoll::befehls_zensus};
    mk::CebBeschreibung const fremd{DockWall::mess_tag, DockWall::befehls_zensus};

    std::printf("[CK-4] Steuerdocks=6 | Zensus Voll=%llu Wall=%llu | eigen->%d fremd->%d\n",
                static_cast<unsigned long long>(DockVoll::befehls_zensus),
                static_cast<unsigned long long>(DockWall::befehls_zensus),
                static_cast<int>(DockVoll::anschliessen(eigen)), static_cast<int>(DockVoll::anschliessen(fremd)));

    EXPECT_EQ(DockVoll::anschliessen(eigen), mk::AnschlussBefund::Angeschlossen);
    EXPECT_EQ(DockVoll::anschliessen(fremd), mk::AnschlussBefund::SD_01_TagFremd)
        << "eine fremd konfigurierte CEB muss LAUT abgewiesen werden -- keine Schnittmengenbildung.";

    // Der ZWEITE, UNABHAENGIGE Weg: gleicher Tag, abweichender Zensus.
    mk::CebBeschreibung const zensus_falsch{DockVoll::mess_tag, DockVoll::befehls_zensus + 1u};
    EXPECT_EQ(DockVoll::anschliessen(zensus_falsch), mk::AnschlussBefund::SD_03_ZensusAbweichung)
        << "der Zensus muss unabhaengig vom Tag greifen -- zwei Wege, die verschieden brechen.";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Die Objekt-Pfade kommen aus FREMDER QUELLE (CMake-Generatorausdruck), nicht aus einer Vermutung
    // dieses Tests ueber das Bauverzeichnis.
    if (argc > 1) g_obj_voll = argv[1];
    if (argc > 2) g_obj_leer = argv[2];
    return RUN_ALL_TESTS();
}
