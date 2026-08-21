// test_configure_enable_gnu -- B15 (Z21/##12-Rest): die GNU-Autoconf-Konvention
// --enable-FEATURE=no == --disable-FEATURE im configure.sh-Vorbau.
//
// BEFUND (am Objekt, 2026-08-21): configure.sh mappte --enable-*/--disable-* STUMPF auf ON/OFF
// (schalter_abbilden, :171-172); ein Wertteil lief in den NAMEN -- `--enable-x=no` ergab
// `-DCOMDARE_X=NO=ON` (tr behandelt '=' nicht). Die Autoconf-Konvention `--enable-FEATURE=no`
// == `--disable-FEATURE` fehlte ("schwerer offizieller Weg"-Doktrin; die XML-Haelfte von ##12
// ist gruen/E13).
//
// PROBE-MECHANIK: --with-cmake=/bin/echo ersetzt cmake durch echo -- der Konfigurations-Aufruf
// druckt seine Argumentzeile auf stdout, ohne irgendetwas zu konfigurieren. Jeder Fall faehrt in
// einem frischen Temp-CWD mit eigenem --build-dir (hermetisch).
//
// DIE VIER FORMEN (BAULISTE-Abnahme, je literal) + Durchreich-Form:
//   (1) --enable-x            -> -DCOMDARE_X=ON     (Bestandsform, byte-identisch zu vorher)
//   (2) --disable-x           -> -DCOMDARE_X=OFF    (Bestandsform, byte-identisch zu vorher)
//   (3) --enable-x=no         -> -DCOMDARE_X=OFF    (die fehlende Konvention; =yes -> ON)
//   (4) --disable-x=yes       -> -DCOMDARE_X=ON     (Spiegel; =no -> OFF)
//   (5) --enable-x=wert       -> -DCOMDARE_X=wert   (andere Werte reisen durch)
//
// T-11c-MUTATIONSANKER: das yes/no-Mapping invertieren -> (3)/(4) brechen literal.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace {

namespace fs = std::filesystem;

std::string g_configure; // argv[1]: Pfad zu configure.sh

struct Lauf {
    int         rc = -1;
    std::string out;
};

[[nodiscard]] Lauf konfiguriere(std::string const& extra_arg, std::string const& fall) {
    Lauf            l{};
    fs::path const  cwd = fs::temp_directory_path() / ("comdare_b15_" + fall);
    std::error_code ec;
    fs::remove_all(cwd, ec);
    fs::create_directories(cwd, ec);
    std::string cmd = "cd \"" + cwd.string() + "\" && sh \"" + g_configure +
                      "\" --with-cmake=/bin/echo --build-dir=bau " + extra_arg + " 2>/dev/null";
    std::FILE*  p   = ::popen(cmd.c_str(), "r");
    if (p == nullptr) return l;
    char puffer[1024];
    while (std::fgets(puffer, sizeof(puffer), p) != nullptr) l.out += puffer;
    int const status = ::pclose(p);
    l.rc             = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
    fs::remove_all(cwd, ec);
    return l;
}

TEST(ConfigureEnableGnu, BestandsformenBleibenByteIdentisch) {
    // (1) + (2): die zwei Alt-Formen -- der Umbau darf sie nicht bewegen.
    Lauf const an = konfiguriere("--enable-x", "an");
    EXPECT_EQ(an.rc, 0) << an.out;
    EXPECT_NE(an.out.find("-DCOMDARE_X=ON"), std::string::npos) << an.out;
    Lauf const aus = konfiguriere("--disable-x", "aus");
    EXPECT_EQ(aus.rc, 0) << aus.out;
    EXPECT_NE(aus.out.find("-DCOMDARE_X=OFF"), std::string::npos) << aus.out;
}

TEST(ConfigureEnableGnu, EnableMitNoIstOffUndMitYesIstOn) {
    // (3): die Autoconf-Konvention -- der Kern des Postens.
    Lauf const nein = konfiguriere("--enable-x=no", "eno");
    EXPECT_EQ(nein.rc, 0) << nein.out;
    EXPECT_NE(nein.out.find("-DCOMDARE_X=OFF"), std::string::npos)
        << "--enable-x=no muss OFF ergeben (Autoconf: == --disable-x):\n"
        << nein.out;
    EXPECT_EQ(nein.out.find("=NO=ON"), std::string::npos) << "der Alt-Defekt (Wert im Namen):\n" << nein.out;
    Lauf const ja = konfiguriere("--enable-x=yes", "eyes");
    EXPECT_EQ(ja.rc, 0) << ja.out;
    EXPECT_NE(ja.out.find("-DCOMDARE_X=ON"), std::string::npos) << ja.out;
}

TEST(ConfigureEnableGnu, DisableMitWertSpiegeltDieKonvention) {
    // (4): --disable-x=yes -> ON, --disable-x=no -> OFF.
    Lauf const ja = konfiguriere("--disable-x=yes", "dyes");
    EXPECT_EQ(ja.rc, 0) << ja.out;
    EXPECT_NE(ja.out.find("-DCOMDARE_X=ON"), std::string::npos) << ja.out;
    Lauf const nein = konfiguriere("--disable-x=no", "dno");
    EXPECT_EQ(nein.rc, 0) << nein.out;
    EXPECT_NE(nein.out.find("-DCOMDARE_X=OFF"), std::string::npos) << nein.out;
}

TEST(ConfigureEnableGnu, AndereWerteReisenDurch) {
    // (5): ein Nicht-yes/no-Wert wird als -DCOMDARE_NAME=WERT durchgereicht.
    Lauf const wert = konfiguriere("--enable-x=blau", "wert");
    EXPECT_EQ(wert.rc, 0) << wert.out;
    EXPECT_NE(wert.out.find("-DCOMDARE_X=blau"), std::string::npos) << wert.out;
    EXPECT_EQ(wert.out.find("-DCOMDARE_X=ON"), std::string::npos)
        << "ein expliziter Wert darf nicht still zu ON werden:\n"
        << wert.out;
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "Aufruf: %s <configure.sh>\n", argv[0]);
        return 2;
    }
    g_configure = argv[1];
    return RUN_ALL_TESTS();
}
