// test_warn1_d2_fingerprint_koeder -- der NACHGEZOGENE Koeder zu D2 (Warnungs-Review, 09.08.2026).
//
// D2 war ein SPEICHER-Fund: `PermutationResult::fingerprint` stand OHNE Initialisierer, als einziges
// Mitglied des Structs (record{} und succeeded=false trugen laengst einen). Der CI-Bau meldete an
// tests/unit/test_workload_and_experiment.cpp:125, wo der Copy-Konstruktor den unbestimmten Wert LIEST:
//   "'r.PermutationResult::fingerprint' is used uninitialized"
// Geheilt wurde im Struct (result_aggregator.hpp) -- die Ursache sass dort, nicht im Aufrufer.
//
// WARUM DIESER KOEDER NACHGEBAUT WERDEN MUSSTE: die erste Minimalprobe hat die Warnung NICHT
// ausgeloest, die Gegenprobe blieb stumm. Der Grund ist der Fehler, an dem solche Proben regelmaessig
// scheitern: ein frischer Stack-Rahmen ist meist schon 0, also besteht die Probe auch OHNE
// Initialisierer -- sie unterscheidet ihre zwei Hypothesen nicht. Ein Koeder, der nicht beisst,
// belegt nichts (K13).
//
// DIE KONSTRUKTION, die beisst: der Speicher wird VOR der Konstruktion mit einem frisch aus
// /dev/urandom gewuerfelten Muster beschrieben, und konstruiert wird per DEFAULT-Initialisierung
// (`new (roh) T`, ohne `{}`) -- genau die Form, bei der ein fehlender Initialisierer den Wert
// unbestimmt laesst. Wertinitialisierung (`T{}`) wuerde das Feld auch OHNE Initialisierer nullen und
// die Probe wieder blind machen.

#include <comdare/experiment/result_aggregator.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <new>

namespace {

// K13: frisch aus /dev/urandom gewuerfelt (09.08.2026), nicht aus einer Doku abgeschrieben.
// Muss von 0 verschieden sein -- sonst kann das Muster nicht von einer echten Nullung getrennt werden.
inline constexpr unsigned char kSchmutzMuster = 0x96;
static_assert(kSchmutzMuster != 0, "ein Nullmuster koennte eine echte Initialisierung nicht widerlegen");

/// Die zwei Zwillinge unterscheiden sich in GENAU EINEM Zeichenpaar: dem `{}`. Sie sind
/// standard-layout, damit der Rueckgriff auf die Byte-Darstellung wohldefiniert bleibt.
struct ZwillingOhneInitialisierer {
    std::uint64_t fingerprint;
};
struct ZwillingMitInitialisierer {
    std::uint64_t fingerprint{};
};

/// Schreibt das Muster, konstruiert per DEFAULT-Initialisierung darueber und liefert die
/// Byte-Darstellung des Feldes zurueck. Gelesen wird ueber memcpy aus dem Rohpuffer -- das ist
/// auch dann wohldefiniert, wenn das Feld selbst unbestimmt ist.
template <class T>
[[nodiscard]] std::uint64_t feld_nach_default_konstruktion() {
    alignas(T) unsigned char roh[sizeof(T)];
    std::memset(roh, kSchmutzMuster, sizeof roh);
    T* p = new (static_cast<void*>(roh)) T; // DEFAULT-init, bewusst ohne {}
    std::uint64_t gelesen = 0;
    std::memcpy(&gelesen, roh, sizeof gelesen); // fingerprint liegt bei beiden Zwillingen an Offset 0
    p->~T();
    return gelesen;
}

} // namespace

// ── DER GEGENEINGANG ZUERST (T-4): beisst die Probe ueberhaupt? ────────────────────────────────
// Ohne diesen Nachweis koennte der eigentliche Test konstant gruen sein.
TEST(Warn1D2FingerprintKoeder, DieProbeUnterscheidetIhreZweiHypothesen) {
    std::uint64_t const erwartetes_muster = []() {
        std::uint64_t m = 0;
        std::memset(&m, kSchmutzMuster, sizeof m);
        return m;
    }();

    // OHNE Initialisierer: das Muster ueberlebt die Konstruktion -> die Probe SIEHT den Defekt.
    EXPECT_EQ(feld_nach_default_konstruktion<ZwillingOhneInitialisierer>(), erwartetes_muster)
        << "Der Koeder beisst nicht: schon ohne Initialisierer kommt kein Schmutz durch. "
           "Dann belegt der Test unten nichts (genau der Fehler der ersten Minimalprobe).";

    // MIT Initialisierer: derselbe Speicher, dieselbe Konstruktionsform -> 0.
    EXPECT_EQ(feld_nach_default_konstruktion<ZwillingMitInitialisierer>(), 0u)
        << "Die Wache ist konstant rot -- dann unterscheidet sie ebenfalls nichts.";
}

// ── DIE ZUSICHERUNG AM ECHTEN TYP ──────────────────────────────────────────────────────────────
TEST(Warn1D2FingerprintKoeder, PermutationResultNulltSeineSkalareAuchAufSchmutzigemSpeicher) {
    using comdare::experiment::PermutationResult;

    alignas(PermutationResult) unsigned char roh[sizeof(PermutationResult)];
    std::memset(roh, kSchmutzMuster, sizeof roh);
    auto* r = new (static_cast<void*>(roh)) PermutationResult; // DEFAULT-init

    // DER NENNER: PermutationResult fuehrt drei skalare/triviale Mitglieder. Alle drei muessen
    // einen Ruhewert tragen -- vor der Heilung waren es zwei von drei.
    int erfuellt = 0;
    int const gesamt = 3;

    if (r->fingerprint == 0u) ++erfuellt;
    if (r->succeeded == false) ++erfuellt;
    bool record_genullt = true;
    {
        // record{} ist wertinitialisiert -> die gesamte Byte-Darstellung muss frei vom Muster sein.
        unsigned char const* rec = reinterpret_cast<unsigned char const*>(&r->record);
        for (std::size_t i = 0; i < sizeof(r->record); ++i)
            if (rec[i] == kSchmutzMuster) { record_genullt = false; break; }
    }
    if (record_genullt) ++erfuellt;

    EXPECT_EQ(r->fingerprint, 0u)
        << "D2 ist zurueck: fingerprint traegt Schmutz statt des Ruhewerts 0. "
           "0 heisst 'nicht berechnet' und ist von jedem echten Hash unterscheidbar.";
    EXPECT_EQ(erfuellt, gesamt) << "Ruhewert gesetzt bei " << erfuellt << " von " << gesamt
                                << " skalaren Mitgliedern von PermutationResult";

    r->~PermutationResult();
}
