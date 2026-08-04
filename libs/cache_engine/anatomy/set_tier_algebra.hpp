#pragma once
// E-24 C6 / F2 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- ISetAlgebraTier: die NATIVE Set-ABI der
// Mengen-Algebra (extract / merge / set-ops), APPEND-ONLY.
//
// AUFTRAG: E24-DOSSIER:156 ("F2 -- native Set-ABI-V2 append-only (extract/merge/set-ops) auf
// ISetTier/SetObserverSnapshotV1", LEDGER:1580) + Bauplan 3.2-C6.
//
// APPEND-ONLY (Auflage 5), woertlich eingehalten:
//   * ISetTier bleibt Methode fuer Methode unveraendert -- KEIN vtable-Anhang. Die Algebra ist ein
//     EIGENES Sub-Interface, das der SetAbiAdapter zusaetzlich erbt; Host-Abfrage 1x kalt per
//     dynamic_cast<ISetAlgebraTier*>. Eine Alt-DLL liefert nullptr, und der Host kann dieselbe
//     Semantik dann per Schleife ueber die V1-Flaeche selbst herstellen (sauberes Degradieren).
//   * SetObserverSnapshotV1 bleibt byte-identisch. Die Algebra-Ops erzeugen KEINE neuen Zaehler:
//     sie laufen ueber DIESELBEN Anatomie-Ops wie die Einzel-Ops und tauchen deshalb in den
//     BESTEHENDEN Observer-Feldern auf (insert/erase + die C6-Versuchs-Zaehler). Das ist die
//     ehrliche Bauform -- eine Algebra mit eigener Buchhaltung waere eine zweite Wahrheit.
//
// WARUM EINE SCHLUESSEL-PUFFER-FLAECHE UND KEIN "merge(ISetTier const&)" (Entscheid, begruendet):
// std::set::merge nimmt eine andere Menge. Ueber die Modul-Grenze ginge das nur, wenn ISetTier eine
// AUFZAEHLUNG anboete -- die hat es am Ist NICHT (set_tier.hpp:41-62: insert/contains/erase/size/clear/
// observe, kein Iterator, keine Schluessel-Ausgabe), und das Kern-Organ SortedArrayKeySet haelt seine
// Schluessel privat. Ein "merge(ISetTier const&)" waere deshalb entweder unimplementierbar oder
// erzwaenge eine Aufzaehlungs-Op an der eingefrorenen V1-Flaeche -- also genau den vtable-Anhang, den
// Auflage 5 verbietet. Die ABI-Form ist darum ein flacher uint64-Schluessel-Puffer (Zeiger + Anzahl):
// ABI-stabil, STL-frei, und der Host haelt die zweite Menge ohnehin als Workload-Daten.
// DEKLARIERTE LUECKE (kein stiller Rest): eine Set-zu-Set-Algebra ueber ZWEI geladene Module braucht
// eine Aufzaehlungs-Op; die waere ein EIGENES Sub-Interface und ist in diesem Fenster nicht beauftragt.
//
// SEMANTIK-ANKER = std::set (das C6-Konformitaets-Orakel der Set-Gattung, Bauplan 1.1):
//   extract      -> std::set::extract: entfernt den Schluessel und meldet, ob er REAL vorhanden war.
//   merge        -> std::set::merge:   uebernimmt aus der Quelle NUR die noch nicht vorhandenen
//                                      Schluessel; Rueckgabe = Zahl der REAL neu entstandenen.
//   intersection -> |A geschnitten B| (nicht-mutierend)
//   difference   -> |B ohne A|        (nicht-mutierend; die Richtung ist im Namen und unten fixiert)

#include <cstdint>

namespace comdare::cache_engine::anatomy {

/// ISetAlgebraTier -- die native Mengen-Algebra der Set-Gattung ueber die Modul-Grenze.
/// EIGENSTAENDIG (erbt ISetTier NICHT): der Antrieb bleibt ISetTier, diese Flaeche ergaenzt ihn.
class ISetAlgebraTier {
public:
    virtual ~ISetAlgebraTier() = default;

    /// extract (std::set::extract-Analogie, K-only): entfernt key. Rueckgabe: true gdw. key REAL
    /// vorhanden war. Unterschied zu tier_set_erase: keiner im Verhalten -- der Unterschied liegt in
    /// der Absicht (Entnahme statt Loeschung) und darin, dass die Algebra-Flaeche sie zusammen mit
    /// merge/set-ops anbietet. Bewusst KEINE zweite Buchhaltung: der Observer sieht denselben Erase.
    [[nodiscard]] virtual bool tier_set_extract(std::uint64_t key) noexcept = 0;

    /// merge (std::set::merge-Analogie): uebernimmt aus dem Schluessel-Puffer alle Schluessel, die
    /// NOCH NICHT enthalten sind. Rueckgabe = Zahl der REAL neu entstandenen Schluessel (Duplikate
    /// zaehlen nicht). keys darf nullptr sein, wenn count == 0.
    [[nodiscard]] virtual std::uint64_t tier_set_merge(std::uint64_t const* keys, std::uint64_t count) noexcept = 0;

    /// |A geschnitten B| -- Zahl der Puffer-Schluessel, die AUCH in dieser Menge sind. Nicht-mutierend.
    /// PUFFER-SEMANTIK (ausdruecklich, statt still): der Puffer wird als FOLGE gelesen, nicht als
    /// Menge -- ein doppelt enthaltener Schluessel zaehlt doppelt. Die Entduplizierung ist Sache des
    /// Aufrufers: modul-seitig braeuchte sie Speicher, und eine Allokation im noexcept-Mess-Pfad ist
    /// weder zulaessig noch messtechnisch neutral. Fuer duplikatfreie Puffer (der Regelfall der
    /// Workload-Daten) faellt beides zusammen.
    [[nodiscard]] virtual std::uint64_t tier_set_intersection_count(std::uint64_t const* keys,
                                                                    std::uint64_t        count) const noexcept = 0;

    /// |B ohne A| -- Zahl der Puffer-Schluessel (B), die NICHT in dieser Menge (A) sind.
    /// Richtung ausdruecklich fixiert: gezaehlt wird der Puffer-Ueberschuss, nicht der Mengen-Ueberschuss
    /// (den koennte diese Flaeche ohne Aufzaehlung gar nicht bestimmen -- s. Kopf).
    [[nodiscard]] virtual std::uint64_t tier_set_difference_count(std::uint64_t const* keys,
                                                                  std::uint64_t        count) const noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
