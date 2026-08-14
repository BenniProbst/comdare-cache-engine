// SPDX-License-Identifier: LicenseRef-Comdare-Research-1.0
// Copyright (c) 2026 BEP Venture UG (haftungsbeschraenkt), Marke Comdare
//
// test_lizenz_konsistenz -- die Wache ueber die Lizenz-Umstellung.        (2026-08-10)
// =============================================================================
// PRUEFLING: der Zustand dieses Repositoriums, nicht ein Skript.
//
// DER BEFUND, DER SIE AUSGELOEST HAT (am Objekt gemessen, 10.08.2026, ce 95cb3039):
//   LICENSE  trug seit 02.08. die Dual-Lizenz.
//   NOTICE:7 trug weiter "Licensed under the Apache License, Version 2.0".
//   36 Quelldatei-Koepfe trugen als SPDX-Bezeichner weiter Apache-2.0.
// (Der volle fremde Bezeichner steht in dieser Datei NIRGENDS als Literal -- die
//  Koeder setzen ihn zur Laufzeit aus Teilen zusammen. Sonst zeigte die Wache beim
//  Lauf ueber den echten Baum ihre eigene Quelldatei an.)
// Zwei Dateien, ein Repo, zwei Aussagen -- acht Tage lang, ohne jedes Signal.
// NOTICE ist ausgerechnet die Datei, auf die die Apache-Lizenz selbst verweist.
//
// WAS DIESE WACHE KANN, UND WAS NICHT (T-9, ehrlich):
//   SIE KANN     : dass alle Lizenz-Aussagen DIESES Baums denselben Bezeichner
//                  nennen; dass kein Copyleft-Schalter vorgabeweise AN steht;
//                  dass jede tatsaechlich eingezogene Fremdkomponente im NOTICE
//                  attribuiert ist.
//   SIE KANN NICHT: beurteilen, ob die Lizenz rechtlich traegt. Eine Umstellung
//                  mit Wirkung fuer Dritte gehoert vor einen Anwalt -- besonders
//                  der unwiderrufliche Apache-Grant fuer Revisionen vor dem
//                  02.08.2026, den niemand zurueckholen kann. Diese Wache misst
//                  Konsistenz, nicht Wirksamkeit.
//
// K13: jeder Koeder wird aus /dev/urandom gewuerfelt und beidseitig gefahren --
// mit Koeder ROT, ohne Koeder GRUEN. Der Gegeneingang ist nie optional: eine
// Wache, die immer beisst, beweist so wenig wie eine, die nie beisst.
//
// MUTATIONS-NACHWEIS (T-1): COMDARE_LIZENZ_WURZEL schiebt einen anderen Baum
// unter; welcher gefahren wurde, steht in der Ausgabe jedes Falls.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#include <gtest/gtest.h>

#include "support/lizenz_audit.hpp"
#include "support/wachen_werkbank.hpp"

#include <cstdlib>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;
using comdare::test::lizenz::ernte_schalter;
using comdare::test::lizenz::ernte_spdx;
using comdare::test::lizenz::ernte_vendor_einzuege;
using comdare::test::lizenz::kBezeichner;
using comdare::test::lizenz::kChangeDate;
using comdare::test::lizenz::lies_datei;
using comdare::test::lizenz::Schalter;
using comdare::test::lizenz::SpdxErnte;
using comdare::test::wachen::koeder;
using comdare::test::wachen::WegwerfRepo;

[[nodiscard]] fs::path wurzel() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_LIZENZ_WURZEL")) {
        if (*ueberschrieben != '\0') { return fs::path{ueberschrieben}; }
    }
    return fs::path{COMDARE_LIZENZ_REPO_WURZEL};
}

[[nodiscard]] bool enthaelt(std::string const& heu, std::string_view nadel) {
    return heu.find(nadel) != std::string::npos;
}

// Der fremde Bezeichner, zur Laufzeit zusammengesetzt. Als Literal wuerde er in dieser
// Datei stehen -- und die Wache liest die ganze Datei, nicht nur ihren Kopf. Sie wuerde
// sich selbst als Verstoss melden. Zusammensetzen ist hier kein Trick, sondern die
// einzige Form, in der ein Koeder seinen eigenen Prueflig nicht vergiftet.
[[nodiscard]] std::string fremder_bezeichner() { return std::string{"Apache"} + "-2.0"; }
[[nodiscard]] std::string fremder_kopf() {
    return "// " + std::string{"SPDX-License-"} + "Identifier: " + fremder_bezeichner() + "\n";
}

// Ein minimaler, in sich stimmiger Baum. Er ist der GEGENEINGANG jedes Koeders:
// erst muss er gruen sein, dann darf ein Koeder ihn rot machen. Ohne diesen
// Schritt bewiese ein roter Koeder-Lauf nur, dass die Wache ueberhaupt meckert.
void baue_sauberen_baum(WegwerfRepo const& repo, std::string const& marke) {
    ASSERT_TRUE(repo.schreibe("LICENSE", "Comdare Research License, Version 1.0\n"
                                         "SPDX-License-Identifier: " +
                                             std::string{kBezeichner} +
                                             "\n"
                                             "Change Date: " +
                                             std::string{kChangeDate} +
                                             "\n"
                                             "marke_" +
                                             marke + "\n"));
    ASSERT_TRUE(repo.schreibe("NOTICE", "SPDX-License-Identifier: " + std::string{kBezeichner} +
                                            "\n"
                                            "Project       : liburing\n"));
    ASSERT_TRUE(repo.schreibe("libs/echt.hpp", "// SPDX-License-Identifier: " + std::string{kBezeichner} + "\n"));
    ASSERT_TRUE(repo.schreibe("CMakeLists.txt",
                              "option(COMDARE_HAVE_KRAM \"Activate ext/x/kram (GPL-3)\" OFF)\n"
                              "set(COMDARE_WRITER_BACKEND \"portable\" CACHE STRING \"backend\")\n"
                              "add_subdirectory(\"${PROJECT_SOURCE_DIR}/ext/io/liburing\" \"${B}/x\")\n"));
}

} // namespace

// =============================================================================
// FALL 1 -- L1: EIN Repo, EIN Lizenz-Bezeichner.
// Das ist der Fall, der am 10.08. rot war: 36 Koepfe sagten Apache-2.0.
// =============================================================================
TEST(LizenzKonsistenz, KeinFremderSpdxBezeichnerAusserhalbExt) {
    SpdxErnte const e = ernte_spdx(wurzel());

    std::string liste;
    for (auto const& f : e.fremd) { liste += "\n    " + f.datei + ":" + std::to_string(f.zeile) + "  " + f.text; }
    std::string doku;
    for (auto const& f : e.doku_ausnahme) {
        doku += "\n    " + f.datei + ":" + std::to_string(f.zeile) + "  " + f.text;
    }

    // Die beiden Nenner zaehlen VERSCHIEDENE Dinge, und das muss dastehen: 'Dateien'
    // sind Dateien, die drei Zeilen darunter sind VORKOMMEN. Eine Datei kann den
    // Bezeichner mehrfach tragen (die Doku-Ausnahme tut es). Wer beides als eine Zahl
    // liest, findet eine Differenz, die keine ist.
    std::cout << "NENNER L1  wurzel=" << wurzel().string() << "\n"
              << "  Dateien durchsucht          : " << e.dateien_gesehen << "\n"
              << "  Dateien mit SPDX-Zeile      : " << e.dateien_mit_spdx << "\n"
              << "  Vorkommen eigener Bezeichner: " << e.eigen << " (" << kBezeichner << ")\n"
              << "  Vorkommen fremder Bezeichner: " << e.fremd.size() << " (SOLL 0)" << liste << "\n"
              << "  Vorkommen Doku-Ausnahme     : " << e.doku_ausnahme.size() << " unter docs/" << doku << "\n";

    EXPECT_GT(e.dateien_gesehen, 100U) << "zu wenige Dateien gesehen -- die Wache lief ins Leere (stille Null)";
    EXPECT_GT(e.eigen, 30U) << "der eigene Bezeichner kommt fast nicht vor -- Erhebung unglaubwuerdig";
    EXPECT_TRUE(e.fremd.empty()) << "Dateien behaupten eine Lizenz, die nicht mehr gilt:" << liste;
}

// =============================================================================
// FALL 2 -- L2: LICENSE und NOTICE sagen dasselbe.
// =============================================================================
TEST(LizenzKonsistenz, LicenseUndNoticeNennenDenselbenBezeichner) {
    std::string const lic  = lies_datei(wurzel() / "LICENSE");
    std::string const not_ = lies_datei(wurzel() / "NOTICE");

    int        erfuellt = 0;
    bool const a        = enthaelt(lic, kBezeichner);
    bool const b        = enthaelt(not_, kBezeichner);
    bool const c        = !enthaelt(not_, "Licensed under the Apache License");
    erfuellt            = static_cast<int>(a) + static_cast<int>(b) + static_cast<int>(c);

    std::cout << "NENNER L2  pruefungen " << erfuellt << "/3\n"
              << "  LICENSE nennt Bezeichner            : " << (a ? "ja" : "NEIN") << "\n"
              << "  NOTICE  nennt Bezeichner            : " << (b ? "ja" : "NEIN") << "\n"
              << "  NOTICE  behauptet NICHT mehr Apache : " << (c ? "ja" : "NEIN") << "\n";

    EXPECT_TRUE(a) << "LICENSE traegt den Bezeichner nicht";
    EXPECT_TRUE(b) << "NOTICE traegt den Bezeichner nicht";
    EXPECT_TRUE(c) << "NOTICE behauptet weiter Apache-2.0 -- genau der Widerspruch vom 10.08.2026";
}

// =============================================================================
// FALL 3 -- L7: der Change Date des Owner-Entscheids C-2 steht drin und ist datiert.
// Owner 10.08.2026: "Ja, cache engine nach 5 Jahren frei verfuegbar ab heute."
// =============================================================================
TEST(LizenzKonsistenz, ChangeDateStehtUndIstDerEntschiedene) {
    std::string const lic    = lies_datei(wurzel() / "LICENSE");
    bool const        datum  = enthaelt(lic, kChangeDate);
    bool const        feld   = enthaelt(lic, "Change Date");
    bool const        ziel   = enthaelt(lic, "Apache License, Version 2.0");
    bool const        hist_a = enthaelt(lic, "2026-08-02");
    bool const        hist_b = enthaelt(lic, "2026-08-10");

    std::cout << "NENNER L7  pruefungen "
              << (static_cast<int>(datum) + static_cast<int>(feld) + static_cast<int>(ziel) + static_cast<int>(hist_a) +
                  static_cast<int>(hist_b))
              << "/5\n"
              << "  Change Date " << kChangeDate << " : " << (datum ? "ja" : "NEIN") << "\n"
              << "  Feld 'Change Date'      : " << (feld ? "ja" : "NEIN") << "\n"
              << "  Ziel-Lizenz Apache-2.0  : " << (ziel ? "ja" : "NEIN") << "\n"
              << "  Historie 2026-08-02     : " << (hist_a ? "ja" : "NEIN") << "\n"
              << "  Historie 2026-08-10     : " << (hist_b ? "ja" : "NEIN") << "\n";

    EXPECT_TRUE(datum) << "der Change Date aus Owner-Entscheid C-2 fehlt";
    EXPECT_TRUE(feld) << "kein maschinenlesbares Feld 'Change Date'";
    EXPECT_TRUE(ziel) << "die Lizenz nach dem Change Date ist nicht benannt";
    EXPECT_TRUE(hist_a && hist_b) << "die Lizenz-Historie nennt nicht beide Wechsel -- alte Grants bleiben gueltig";
}

// =============================================================================
// FALL 4 -- L4/L5/L6: KEIN Copyleft-Schalter steht vorgabeweise AN.
//
// Hier wird die Auftrags-Vorbedingung KON2-24 gemessen statt geglaubt. Sie
// sprach von VIER Schaltern, ein Nachtrag von FUENF. Diese Wache zaehlt selbst
// und nennt beide Zahlen -- sie lernt keine Liste auswendig.
// =============================================================================
TEST(LizenzKonsistenz, KeinCopyleftSchalterStehtVorgabeweiseAn) {
    std::vector<Schalter> const alle = ernte_schalter(wurzel());

    std::vector<Schalter> copyleft;
    std::vector<Schalter> an;
    for (auto const& s : alle) {
        if (!s.copyleft) { continue; }
        copyleft.push_back(s);
        if (s.vorgabe == "ON") { an.push_back(s); }
    }

    std::string liste;
    for (auto const& s : copyleft) {
        liste += "\n    " + s.datei + ":" + std::to_string(s.zeile) + "  " + s.name + " = " + s.vorgabe;
    }

    std::cout << "NENNER L4  COMDARE-Schalter gesamt : " << alle.size() << "\n"
              << "  davon copyleft-tragend           : " << copyleft.size() << liste << "\n"
              << "  davon vorgabeweise AN            : " << an.size() << " (SOLL 0)\n";

    EXPECT_GT(alle.size(), 10U) << "zu wenige Schalter gefunden -- der Parser lief ins Leere (stille Null)";
    EXPECT_FALSE(copyleft.empty()) << "kein einziger Copyleft-Schalter gefunden -- unglaubwuerdig, ext/ traegt GPL-3";
    EXPECT_TRUE(an.empty()) << "ein Copyleft-Schalter steht vorgabeweise AN -- die proprietaere Huelle haelt nicht";
}

// =============================================================================
// FALL 5 -- L6: der SECHSTE Schalter, den die Vorbedingung nicht kannte.
//
// COMDARE_WRITER_BACKEND ist kein option(), sondern eine CACHE-STRING-Variable.
// Auf "io_uring" gesetzt linkt sie liburing in die ausgelieferte Binary
// cache_engine_builder. Genau deshalb steht sie hier -- und genau deshalb fehlte
// liburing bis zum 10.08.2026 im NOTICE.
// =============================================================================
TEST(LizenzKonsistenz, WriterBackendStehtVorgabeweiseAufPortable) {
    std::vector<Schalter> const alle = ernte_schalter(wurzel());

    Schalter const* gefunden = nullptr;
    for (auto const& s : alle) {
        if (s.name == "COMDARE_WRITER_BACKEND") { gefunden = &s; }
    }

    std::cout << "NENNER L6  COMDARE_WRITER_BACKEND gefunden : " << (gefunden != nullptr ? "ja" : "NEIN") << "\n";
    ASSERT_NE(gefunden, nullptr) << "der sechste Schalter fehlt -- entweder umbenannt oder der Parser ist blind";
    std::cout << "  " << gefunden->datei << ":" << gefunden->zeile << "  Vorgabe = " << gefunden->vorgabe
              << " (SOLL portable)\n";

    EXPECT_EQ(gefunden->vorgabe, "portable")
        << "Vorgabe zieht ein ext/-Bauteil in jede Binary -- Attribution und Lizenzlage aendern sich damit";
}

// =============================================================================
// FALL 6 -- L3: jede eingezogene Fremdkomponente ist im NOTICE attribuiert.
//
// Das ist die Pflicht, die permissive Lizenzen hinterlassen: MIT und BSD faerben
// nichts ein, verlangen aber Namensnennung. liburing fiel genau durch dieses
// Loch -- es hat keinen option()-Schalter, also stand es auf keiner Schalter-Liste.
//
// GRENZE, BENANNT (T-9): gezaehlt werden nur DIREKTE add_subdirectory-Einzuege aus
// Dateien ausserhalb `ext/`. TRANSITIVE Einzuege sieht dieser Fall nicht -- zlib
// z.B. wird von libxlsxwriter selbst hereingeholt, also aus einer Datei INNERHALB
// `ext/`. zlib steht heute im NOTICE, aber nicht weil dieser Fall es erzwingt.
// Wer die transitive Menge decken will, misst den gebauten Zielgraphen, nicht
// den CMake-Quelltext; das ist ein eigenes Paket und hier ausdruecklich offen.
// =============================================================================
TEST(LizenzKonsistenz, JedeEingezogeneFremdkomponenteStehtImNotice) {
    auto const        einzuege = ernte_vendor_einzuege(wurzel());
    std::string const notiz    = lies_datei(wurzel() / "NOTICE");

    std::vector<std::string> fehlend;
    std::string              gesehen;
    std::vector<std::string> eindeutig;
    for (auto const& v : einzuege) {
        bool schon = false;
        for (auto const& k : eindeutig) {
            if (k == v.komponente) { schon = true; }
        }
        if (!schon) { eindeutig.push_back(v.komponente); }
        gesehen += "\n    " + v.datei + ":" + std::to_string(v.zeile) + "  " + v.komponente;
    }
    for (auto const& k : eindeutig) {
        if (notiz.find(k) == std::string::npos) { fehlend.push_back(k); }
    }

    std::cout << "NENNER L3  add_subdirectory-Einzuege aus ext/ : " << einzuege.size() << gesehen << "\n"
              << "  eindeutige Komponenten                     : " << eindeutig.size() << "\n"
              << "  davon im NOTICE attribuiert                : " << (eindeutig.size() - fehlend.size()) << "/"
              << eindeutig.size() << "\n";

    EXPECT_FALSE(eindeutig.empty()) << "kein einziger ext/-Einzug gefunden -- der Parser lief ins Leere";
    for (auto const& k : fehlend) {
        ADD_FAILURE() << "Komponente '" << k << "' wird in den Bau gezogen, steht aber nicht im NOTICE -- "
                      << "die Attributionspflicht ihrer Lizenz ist damit verletzt";
    }
}

// =============================================================================
// FALL 7 -- L5: die GPL-3-Kopie des fuenften Schalters wird nicht uebersetzt.
//
// COMDARE_CE_ENABLE_ORIGINAL_CODE_VALIDATION steht auf ON und kopiert bei JEDEM
// Configure wh.c (GPL-3) in ein Legacy-Verzeichnis. Zulaessig ist das nur, solange
// die Kopie blosse Beifuegung bleibt: kein Target darf sie als Quelle fuehren.
// =============================================================================
TEST(LizenzKonsistenz, GplKopieDesFuenftenSchaltersIstKeineTargetQuelle) {
    std::vector<Schalter> const alle = ernte_schalter(wurzel());
    bool                        da   = false;
    for (auto const& s : alle) {
        if (s.name == "COMDARE_CE_ENABLE_ORIGINAL_CODE_VALIDATION") { da = true; }
    }

    // Ein Target, das wh.c als Quelle fuehrt, waere ein kombiniertes Werk mit GPL-3.
    std::size_t                      gesehen = 0;
    std::vector<std::string>         treffer;
    std::error_code                  ec;
    fs::recursive_directory_iterator it{wurzel(), fs::directory_options::skip_permission_denied, ec};
    fs::recursive_directory_iterator ende;
    for (; it != ende; it.increment(ec)) {
        if (ec) { break; }
        if (it->is_directory(ec)) {
            // EXAKTE Namen, kein Praefix: "build" als Praefix frisst "builder/" mit.
            // Genau dieser Fehler stand im ersten Lauf dieser Wache in lizenz_audit.hpp
            // und leerte den Nenner lautlos -- er wird hier nicht wiederholt.
            std::string const n = it->path().filename().string();
            if (n == "ext" || n == ".git" || n == "build" || n.rfind("build-", 0) == 0 || n.rfind("build_", 0) == 0) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file(ec)) { continue; }
        if (it->path().filename() != "CMakeLists.txt" && it->path().extension() != ".cmake") { continue; }
        gesehen++;
        std::string const inhalt = lies_datei(it->path());
        std::size_t       pos    = 0;
        while (true) {
            std::size_t const t = inhalt.find("wh.c", pos);
            if (t == std::string::npos) { break; }
            pos                  = t + 4;
            std::size_t const za = inhalt.find_last_of('\n', t);
            std::size_t const ze = inhalt.find('\n', t);
            std::string const zeile =
                inhalt.substr(za == std::string::npos ? 0 : za + 1,
                              (ze == std::string::npos ? inhalt.size() : ze) - (za == std::string::npos ? 0 : za + 1));
            std::string const roh =
                zeile.substr(zeile.find_first_not_of(" \t") == std::string::npos ? 0 : zeile.find_first_not_of(" \t"));
            if (roh.rfind('#', 0) == 0) { continue; } // Kommentar ist keine Quelle
            if (zeile.find("add_library") != std::string::npos || zeile.find("add_executable") != std::string::npos ||
                zeile.find("target_sources") != std::string::npos) {
                treffer.push_back(fs::relative(it->path(), wurzel(), ec).generic_string() + "  " + roh);
            }
        }
    }

    std::string liste;
    for (auto const& t : treffer) { liste += "\n    " + t; }
    std::cout << "NENNER L5  CMake-Dateien durchsucht : " << gesehen << "\n"
              << "  fuenfter Schalter deklariert      : " << (da ? "ja" : "NEIN") << "\n"
              << "  wh.c als Target-Quelle            : " << treffer.size() << " (SOLL 0)" << liste << "\n";

    EXPECT_TRUE(da) << "COMDARE_CE_ENABLE_ORIGINAL_CODE_VALIDATION nicht gefunden -- Vorbedingung unpruefbar";
    EXPECT_TRUE(treffer.empty()) << "GPL-3-Quelle in einem Target -- aus Beifuegung waere ein kombiniertes Werk";
}

// =============================================================================
// FALL 8 -- DER GEGENEINGANG. Ohne ihn beweisen die Koeder unten nichts.
// =============================================================================
TEST(LizenzKonsistenzKoeder, SauberterBaumIstGruen) {
    std::string const marke = koeder();
    WegwerfRepo const repo{"lizenz_sauber_" + marke};
    baue_sauberen_baum(repo, marke);

    SpdxErnte const   e     = ernte_spdx(repo.pfad());
    auto const        sc    = ernte_schalter(repo.pfad());
    auto const        vz    = ernte_vendor_einzuege(repo.pfad());
    std::string const notiz = lies_datei(repo.pfad() / "NOTICE");

    std::size_t an = 0;
    for (auto const& s : sc) {
        if (s.copyleft && s.vorgabe == "ON") { an++; }
    }
    std::size_t fehlend = 0;
    for (auto const& v : vz) {
        if (notiz.find(v.komponente) == std::string::npos) { fehlend++; }
    }

    std::cout << "GEGENEINGANG marke=" << marke << "  fremd=" << e.fremd.size() << " copyleft_an=" << an
              << " einzuege=" << vz.size() << " nicht_attribuiert=" << fehlend << "\n";

    EXPECT_TRUE(e.fremd.empty()) << "der saubere Baum ist schon ohne Koeder rot -- die Wache meckert grundlos";
    EXPECT_EQ(an, 0U);
    EXPECT_EQ(vz.size(), 1U) << "der Einzug wurde nicht erkannt -- Fall 6 waere im echten Baum eine stille Null";
    EXPECT_EQ(fehlend, 0U);
}

// =============================================================================
// FALL 9 -- KOEDER A: eine Datei behauptet Apache-2.0. L1 MUSS beissen.
// =============================================================================
TEST(LizenzKonsistenzKoeder, FremderSpdxBezeichnerWirdGefunden) {
    std::string const marke = koeder();
    WegwerfRepo const repo{"lizenz_koeder_spdx_" + marke};
    baue_sauberen_baum(repo, marke);

    std::string const pfad = "libs/koeder_" + marke + ".hpp";
    ASSERT_TRUE(repo.schreibe(pfad, fremder_kopf()));

    SpdxErnte const e = ernte_spdx(repo.pfad());
    std::string     gefunden;
    for (auto const& f : e.fremd) { gefunden += "\n    " + f.datei + ":" + std::to_string(f.zeile) + "  " + f.text; }

    std::cout << "KOEDER A marke=" << marke << "  erwartet 1 Fund, gefunden " << e.fremd.size() << gefunden << "\n";

    ASSERT_EQ(e.fremd.size(), 1U) << "der Koeder wurde nicht gefunden -- die Wache ist blind";
    EXPECT_EQ(e.fremd[0].datei, pfad) << "gefunden wurde etwas anderes als der gewuerfelte Koeder";
    EXPECT_EQ(e.fremd[0].text, "Apache-2.0");
}

// =============================================================================
// FALL 10 -- KOEDER B: ein Copyleft-Schalter auf ON. L4 MUSS beissen.
// =============================================================================
TEST(LizenzKonsistenzKoeder, CopyleftSchalterAufOnWirdGefunden) {
    std::string const marke = koeder();
    WegwerfRepo const repo{"lizenz_koeder_schalter_" + marke};
    baue_sauberen_baum(repo, marke);

    std::string const name = "COMDARE_HAVE_KOEDER_" + marke;
    ASSERT_TRUE(
        repo.schreibe("adapters/CMakeLists.txt", "option(" + name + " \"Activate ext/x/koeder (GPL-3)\" ON)\n"));

    auto const            sc = ernte_schalter(repo.pfad());
    std::vector<Schalter> an;
    for (auto const& s : sc) {
        if (s.copyleft && s.vorgabe == "ON") { an.push_back(s); }
    }

    std::cout << "KOEDER B marke=" << marke << "  schalter=" << sc.size() << " copyleft_an=" << an.size() << "\n";

    ASSERT_EQ(an.size(), 1U) << "ein Copyleft-Schalter auf ON blieb unbemerkt -- Fall 4 ist wirkungslos";
    EXPECT_EQ(an[0].name, name);
}

// =============================================================================
// FALL 11 -- KOEDER C: eine eingezogene Komponente fehlt im NOTICE. L3 MUSS beissen.
// Das ist der Koeder, der die liburing-Luecke vom 10.08.2026 nachstellt.
// =============================================================================
TEST(LizenzKonsistenzKoeder, NichtAttribuierterEinzugWirdGefunden) {
    std::string const marke = koeder();
    WegwerfRepo const repo{"lizenz_koeder_notice_" + marke};
    baue_sauberen_baum(repo, marke);

    std::string const komp = "fremdlib" + marke;
    ASSERT_TRUE(repo.schreibe("apps/CMakeLists.txt",
                              "add_subdirectory(\"${PROJECT_SOURCE_DIR}/ext/io/" + komp + "\" \"${B}/y\")\n"));

    auto const        vz    = ernte_vendor_einzuege(repo.pfad());
    std::string const notiz = lies_datei(repo.pfad() / "NOTICE");

    std::vector<std::string> fehlend;
    for (auto const& v : vz) {
        if (notiz.find(v.komponente) == std::string::npos) { fehlend.push_back(v.komponente); }
    }

    std::cout << "KOEDER C marke=" << marke << "  einzuege=" << vz.size() << " nicht_attribuiert=" << fehlend.size()
              << "\n";

    ASSERT_EQ(fehlend.size(), 1U) << "ein nicht attribuierter Einzug blieb unbemerkt -- genau die liburing-Klasse";
    EXPECT_EQ(fehlend[0], komp);
}

// =============================================================================
// FALL 12 -- KOEDER D: WRITER_BACKEND vorgabeweise auf io_uring. L6 MUSS beissen.
// =============================================================================
TEST(LizenzKonsistenzKoeder, WriterBackendVorgabeIoUringWirdGefunden) {
    std::string const marke = koeder();
    WegwerfRepo const repo{"lizenz_koeder_backend_" + marke};
    baue_sauberen_baum(repo, marke);

    ASSERT_TRUE(repo.schreibe("CMakeLists.txt",
                              "set(COMDARE_WRITER_BACKEND \"io_uring\" CACHE STRING \"backend " + marke + "\")\n"));

    auto const      sc       = ernte_schalter(repo.pfad());
    Schalter const* gefunden = nullptr;
    for (auto const& s : sc) {
        if (s.name == "COMDARE_WRITER_BACKEND") { gefunden = &s; }
    }

    std::cout << "KOEDER D marke=" << marke << "  vorgabe=" << (gefunden != nullptr ? gefunden->vorgabe : "FEHLT")
              << "\n";

    ASSERT_NE(gefunden, nullptr);
    EXPECT_EQ(gefunden->vorgabe, "io_uring") << "die geaenderte Vorgabe wurde nicht gelesen -- Fall 5 ist blind";
    EXPECT_NE(gefunden->vorgabe, "portable");
}

// =============================================================================
// FALL 13 -- KOEDER E: NOTICE behauptet wieder Apache. L2 MUSS beissen.
// Das ist der Rueckdreh-Koeder auf den Ausgangsbefund selbst.
// =============================================================================
TEST(LizenzKonsistenzKoeder, NoticeMitApacheBehauptungWirdGefunden) {
    std::string const marke = koeder();
    WegwerfRepo const repo{"lizenz_koeder_apache_" + marke};
    baue_sauberen_baum(repo, marke);

    ASSERT_TRUE(repo.schreibe("NOTICE", "Licensed under the Apache License, Version 2.0\nmarke_" + marke + "\n"));

    std::string const notiz  = lies_datei(repo.pfad() / "NOTICE");
    bool const        sauber = !enthaelt(notiz, "Licensed under the Apache License");

    std::cout << "KOEDER E marke=" << marke << "  NOTICE-behauptet-Apache=" << (sauber ? "nein" : "ja") << "\n";

    EXPECT_FALSE(sauber) << "die Apache-Behauptung im NOTICE blieb unbemerkt -- Fall 2 ist wirkungslos";
    EXPECT_FALSE(enthaelt(notiz, kBezeichner)) << "Gegenprobe: der Koeder hat den Bezeichner wirklich verdraengt";
}
