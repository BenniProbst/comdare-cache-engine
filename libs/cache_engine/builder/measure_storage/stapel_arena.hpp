#pragma once
// stapel_arena.hpp -- MeasureStorage (2026-08-09): DIE ZWEITE DER ZWEI ARENEN -- die STAPEL-ARENA.
//
// SELBSTCHECK: diese Datei speichert KEINEN Messwert. Kein Feld dieser Datei traegt eine Zeit, eine
// Latenz oder eine Zahl aus der Messung. Der Stapel ist KEIN Zwischenspeicher fuer Messwerte,
// sondern ein POSITIONSANZEIGER -- er beantwortet ausschliesslich "wo bin ich gerade?". Wer hier
// einen Messwert findet, hat einen Defekt gefunden.
//
// == WAS DIESE ARENA IST (Owner-KERN 09.08.2026, woertlich) =========================================
//     "es gibt einen Stack, der von checkpoint_measure in einer GETRENNTEN custom Arena prueft, auf
//      welcher Mess-Ebene wir uns befinden und in welchem Modul+Funktion"
//
//   Zugriff:    LIFO -- hinauf beim Eintritt, herunter beim Austritt.
//   Wachstum:   AUF UND AB mit der Verschachtelung. Wird staendig zurueckgesetzt.
//   Dimension:  die maximale VERSCHACHTELUNGSTIEFE (typisch einstellig), nicht die Ereigniszahl.
//   Auswertung: WAEHREND des Laufs, bei jedem Checkpoint.
//   Inhalt:     VERWEISE (Index in die Mess-Arena, Index in die Deskriptor-Tabelle), keine Kopien.
//
// WARUM LIFO UND NICHT FIFO: Checkpoint-Paare sind eine Klammerstruktur. Ein AUS schliesst immer das
// JUENGSTE offene EIN seiner Ebene. FIFO ordnete ein AUS dem AELTESTEN offenen EIN zu und loeste
// damit JEDE Verschachtelung falsch auf -- aus "macro ruft micro" wuerde "micro ruft macro".
//
// == KEIN std::stack, KEIN std::vector -- drei unabhaengige Gruende =================================
// 1. Der Vorgabe-Behaelter von std::stack ist std::deque. libstdc++ stueckelt in Bloecke von
//    512/sizeof(T) Elementen -- schon das ERSTE Hinauflegen belegt die Zeiger-Landkarte plus einen
//    vollen Block; auf 64-Bit-libstdc++ ist das das Achtfache der Objektgroesse fuer ein einziges
//    Element, und jeder Blockuebertritt belegt erneut. Im Messfenster ist das disqualifizierend.
// 2. Der Adapter hat KEINE Kapazitaets-Schnittstelle: weder reserve() noch capacity(). Selbst
//    std::stack<T, std::vector<T>> laesst sich ueber den Adapter nicht vorreservieren, und nichts
//    hindert den Behaelter danach am umlagernden Wachsen. Es gaebe keine Kapazitaets-Zusage.
// 3. Der Adapter verbietet die Inspektion: nur top/push/pop. Der Befund "welches EIN blieb offen"
//    verlangt aber, die VERBLIEBENEN Eintraege AUSZULESEN und zu melden. Beim eigenen Feld ist das
//    ein Ausschnitt; beim Adapter ginge es nur zerstoerend.
// Gebaut ist deshalb: ein Feld plus ein Top-Zaehler. Mehr braucht ein Stapel nicht.
//
// == DIE UEBERLAUF-POLITIK -- EINE ANDERE FEHLERKLASSE ALS DIE MESS-ARENA ==========================
// Ein voller Stapel ist KEIN Kapazitaetsproblem und KEIN Datenverlust. Er bedeutet: die
// Verschachtelung ist tiefer, als das Programm sie haben darf -- ein PROGRAMMIERFEHLER. Deshalb hat
// er einen eigenen Befund-Typ und einen eigenen Meldetext, der das Wort "Datenverlust" NICHT
// enthaelt und die Zahl "verloren" nicht kennt. Die beiden Diagnosen teilen sich keine Meldung.
//
// Eine ZWEITE eigene Fehlerklasse kommt hier dazu, die die Mess-Arena nicht hat: ein AUS ohne
// vorheriges EIN (unpaariges Herunternehmen). Auch das ist ein Programmierfehler, aber ein anderer
// als "zu tief" -- eigener Zaehler, eigener Satz im Meldetext.
//
// == DIE IN/OUT-SYMMETRIE -- WAS AM ENDE LIEGEN BLEIBT ==============================================
// Ein EIN ohne AUS bleibt beim Auslesen auf dem Stapel liegen. Das ist eine REGRESSION und wird
// GEZAEHLT UND GEMELDET, nicht stillschweigend aufgeraeumt. Ein Stapel, der sich am Ende selbst
// leert, hat die Regression vernichtet, die er haette anzeigen sollen. offene() gibt die
// verbliebenen Eintraege heraus -- mit Deskriptor und Ebene, damit die Meldung sagen kann, WELCHES
// EIN offen blieb, und nicht nur, DASS eines offen blieb.

#include "mess_arena.hpp"
#include "mess_speicher_kanon.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <type_traits>

namespace comdare::cache_engine::builder::measure_storage {

// == Der Stapel-Eintrag -- 16 Byte, NUR VERWEISE ===================================================
//
// SELBSTCHECK: kein Feld traegt einen Messwert. log_index ist ein VERWEIS in die Mess-Arena
// ("im Log steht der Verweis, im Stack die Herkunft"), deskriptor_ix ein VERWEIS in die statische
// Deskriptor-Tabelle, die Modul und Funktion nennt. Kopien gaebe es nur, wenn man hier Zeichenketten
// oder Zeiten mitfuehrte -- genau das ist verboten und deshalb nicht gebaut.
struct StapelEintrag {
    std::uint64_t log_index = 0; // Verweis auf die EIN-Zeile in der Mess-Arena

    // Verweis in die statische Deskriptor-Tabelle: Modul + Funktion + Ziel-Quellort. Ihr Layout ist
    // Soll-Design OP-1/OP-2 und NICHT entschieden -- deshalb steht hier ein Index und keine Struktur.
    std::uint32_t deskriptor_ix = 0;

    std::uint16_t thread_nr  = 0;
    std::uint8_t  ebene      = 0; // static_cast<uint8_t>(MessEbene)
    std::uint8_t  reserviert = 0;
};

static_assert(sizeof(StapelEintrag) == 16, "Stapel-Eintrag muss 16 Byte sein (4 je Cacheline)");
static_assert(std::is_trivially_copyable_v<StapelEintrag>, "Stapel-Eintrag muss trivially copyable sein");
static_assert(std::is_standard_layout_v<StapelEintrag>, "Stapel-Eintrag muss standard_layout sein");

// == Der heisse Zustand des Stapels -- eigene Cacheline ============================================
//
// Er wird bei JEDEM Checkpoint GELESEN UND GESCHRIEBEN (die Mess-Arena nur geschrieben). Genau
// deshalb darf er nicht in derselben Cacheline liegen wie der heisse Zustand der Mess-Arena --
// das waere selbstgemachtes False Sharing in ausgerechnet dem Modul, das solche Effekte messen soll.
// Die Trennung wird in checkpoint_speicher.hpp compile-hart zugesichert, nicht behauptet.
struct alignas(kCacheLineBytes) StapelArenaHeiss {
    StapelEintrag* basis         = nullptr;
    std::uint32_t  kapazitaet    = 0;
    std::uint32_t  top           = 0; // Zahl der derzeit offenen Eintraege
    std::uint32_t  hoechststand  = 0; // groesste je erreichte Tiefe -- misst die offene Haus-Frage
    std::uint32_t  zu_tief       = 0; // Fehlerklasse A: Verschachtelung zu tief (Programmierfehler)
    std::uint32_t  unpaarige_aus = 0; // Fehlerklasse B: AUS ohne EIN (Programmierfehler, anderer)
    std::uint32_t  polster       = 0;
};

static_assert(std::is_trivially_copyable_v<StapelArenaHeiss>, "heisser Stapel-Zustand muss trivially copyable sein");
static_assert(std::is_standard_layout_v<StapelArenaHeiss>, "heisser Stapel-Zustand muss standard_layout sein");
static_assert(alignof(StapelArenaHeiss) == kCacheLineBytes, "heisser Stapel-Zustand beginnt auf einer Cacheline");
static_assert(sizeof(StapelArenaHeiss) % kCacheLineBytes == 0, "heisser Stapel-Zustand fuellt ganze Cachelines");

// == Der Befund der STAPEL-Arena -- eigene Fehlerklassen, eigener Text =============================
//
// SELBSTCHECK: dieser Typ hat KEIN Feld "verloren" und der Text unten enthaelt das Wort
// "DATENVERLUST" nicht. Das ist die Trennung der Fehlerklassen, gebaut statt versprochen.
struct StapelBefund {
    std::uint32_t kapazitaet    = 0;
    std::uint32_t offen         = 0; // EIN ohne AUS, beim Auslesen liegengeblieben -- REGRESSION
    std::uint32_t hoechststand  = 0;
    std::uint32_t zu_tief       = 0;
    std::uint32_t unpaarige_aus = 0;

    [[nodiscard]] constexpr bool sauber() const noexcept { return offen == 0u && zu_tief == 0u && unpaarige_aus == 0u; }
};

/// Schreibt die Stapel-Meldung in einen vom Aufrufer gestellten Puffer. Kein ostream, kein
/// Text-Objekt. Rueckgabe: geschriebene Zeichen ohne Abschluss-Null.
///
/// Der Text nennt die drei Befunde GETRENNT. Sie in eine Zahl zusammenzuziehen hiesse, drei
/// verschiedene Ursachen (zu tiefe Verschachtelung, unpaariges Austreten, offen gebliebenes
/// Eintreten) hinter einer Meldung zu verstecken.
[[nodiscard]] inline int befund_zeile(StapelBefund const& b, char* puffer, std::size_t n) noexcept {
    if (puffer == nullptr || n == 0u) return 0;
    if (b.sauber()) {
        return std::snprintf(puffer, n, "stapel_arena: ausgeglichen (Hoechststand %llu von %llu Plaetzen)",
                             static_cast<unsigned long long>(b.hoechststand),
                             static_cast<unsigned long long>(b.kapazitaet));
    }
    return std::snprintf(puffer, n,
                         "stapel_arena: PROGRAMMIERFEHLER -- zu_tief = %llu (Verschachtelung ueber %llu Plaetze), "
                         "unpaariges_aus = %llu, offen_geblieben = %llu (EIN ohne AUS, Hoechststand %llu)",
                         static_cast<unsigned long long>(b.zu_tief), static_cast<unsigned long long>(b.kapazitaet),
                         static_cast<unsigned long long>(b.unpaarige_aus), static_cast<unsigned long long>(b.offen),
                         static_cast<unsigned long long>(b.hoechststand));
}

// == DIE STAPEL-ARENA ==============================================================================
class alignas(kCacheLineBytes) StapelArena {
public:
    StapelArena() noexcept = default;

    StapelArena(StapelArena const&)            = delete;
    StapelArena& operator=(StapelArena const&) = delete;

    /// AUFBAU, vor dem Messfenster -- eine EIGENE Reservierung, nicht ein Abschnitt der Mess-Arena.
    ///
    /// Dimensioniert ueber die VERSCHACHTELUNGSTIEFE, nicht ueber die Ereigniszahl. Die maximale
    /// Tiefe ist im Haus nirgends festgelegt (benannte Luecke); 64 Plaetze sind der konservative
    /// Vorgabewert bei typisch einstelliger Verschachtelung. Welche Tiefe WIRKLICH auftritt, misst
    /// hoechststand() -- damit schliesst sich die Luecke am Objekt statt durch Schaetzung.
    [[nodiscard]] bool reservieren(std::uint32_t tiefe_plaetze, VorabBeruehrung vorab) noexcept {
        heiss_ = StapelArenaHeiss{};
        if (tiefe_plaetze == 0u) return false;
        std::size_t const bytes = static_cast<std::size_t>(tiefe_plaetze) * sizeof(StapelEintrag);
        if (!res_.reservieren(bytes, vorab)) return false;
        heiss_.basis      = static_cast<StapelEintrag*>(res_.basis());
        heiss_.kapazitaet = tiefe_plaetze;
        return true;
    }

    // [MS-HEISS]
    /// EINTRITT: legt den Verweis oben auf. Rueckgabe false = zu tief (gezaehlt, nicht abgebrochen).
    ///
    /// Bei Ueberlauf wird NICHT geschrieben und der Top-Zaehler NICHT erhoeht -- sonst zeigte der
    /// Stapel auf einen Platz, den es nicht gibt, und das folgende Heruntern naehme Unsinn herunter.
    [[nodiscard]] bool hinauf(StapelEintrag const& eintrag) noexcept {
        if (heiss_.top >= heiss_.kapazitaet) {
            ++heiss_.zu_tief;
            return false;
        }
        heiss_.basis[heiss_.top] = eintrag;
        ++heiss_.top;
        if (heiss_.top > heiss_.hoechststand) heiss_.hoechststand = heiss_.top;
        return true;
    }

    /// AUSTRITT: nimmt den obersten Verweis herunter. Rueckgabe false = unpaariges AUS.
    /// Bei leerem Stapel wird NICHT unter Null gezaehlt -- der Fall bekommt seinen eigenen Zaehler.
    [[nodiscard]] bool herunter(StapelEintrag* heraus) noexcept {
        if (heiss_.top == 0u) {
            ++heiss_.unpaarige_aus;
            return false;
        }
        --heiss_.top;
        if (heraus != nullptr) *heraus = heiss_.basis[heiss_.top];
        return true;
    }

    /// WO BIN ICH GERADE? Der oberste offene Eintrag, oder nullptr bei leerem Stapel.
    /// Das ist die Frage, fuer die diese Arena existiert -- sie wird WAEHREND des Laufs beantwortet.
    [[nodiscard]] StapelEintrag const* spitze() const noexcept {
        if (heiss_.top == 0u) return nullptr;
        return heiss_.basis + (heiss_.top - 1u);
    }
    // [MS-HEISS-ENDE]

    [[nodiscard]] std::uint32_t tiefe() const noexcept { return heiss_.top; }
    [[nodiscard]] std::uint32_t kapazitaet() const noexcept { return heiss_.kapazitaet; }
    [[nodiscard]] std::uint32_t hoechststand() const noexcept { return heiss_.hoechststand; }
    [[nodiscard]] std::uint32_t zu_tief() const noexcept { return heiss_.zu_tief; }
    [[nodiscard]] std::uint32_t unpaarige_aus() const noexcept { return heiss_.unpaarige_aus; }

    /// Was liegengeblieben ist: jedes EIN ohne AUS. NICHT aufgeraeumt, sondern herausgegeben.
    [[nodiscard]] std::span<StapelEintrag const> offene() const noexcept {
        if (heiss_.basis == nullptr) return {};
        return std::span<StapelEintrag const>{heiss_.basis, heiss_.top};
    }

    [[nodiscard]] StapelBefund befund() const noexcept {
        return StapelBefund{heiss_.kapazitaet, heiss_.top, heiss_.hoechststand, heiss_.zu_tief, heiss_.unpaarige_aus};
    }

    [[nodiscard]] void const* nutzlast_basis() const noexcept { return heiss_.basis; }
    [[nodiscard]] void const* heisse_basis() const noexcept { return &heiss_; }

private:
    StapelArenaHeiss   heiss_{}; // eigene Cacheline, Offset 0
    SeitenReservierung res_{};   // kalt: eigene Reservierung, eigene Seiten
};

static_assert(sizeof(StapelArena) % kCacheLineBytes == 0, "Stapel-Arena muss ganze Cachelines fuellen");
static_assert(alignof(StapelArena) == kCacheLineBytes, "Stapel-Arena muss auf einer Cacheline beginnen");
static_assert(std::is_standard_layout_v<StapelArena>, "Stapel-Arena muss standard_layout sein (offsetof-Beleg)");

} // namespace comdare::cache_engine::builder::measure_storage
