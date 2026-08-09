// test_ms1_arenen_kein_alloc_im_fenster.cpp -- MS-1: DER AUFNAHME-PFAD ALLOZIERT IM FENSTER NICHT.
//
// SELBSTCHECK: dieser Test behauptet NICHTS ueber Messwerte, Latenzen oder Statistik. Er behauptet
// genau eine Sache und prueft sie aus mehreren Richtungen: zwischen dem Scharfstellen und dem
// Entschaerfen des Zaehlers ruft der Aufnahme-Pfad KEIN operator new. Wer hier eine Zeitnahme oder
// eine Perzentil-Rechnung sucht, ist in der falschen Datei. Die Seitenfehler-Frage haelt
// test_ms2_pre_touch_seitenfehler, und zwar mit einer anderen Observablen (ru_minflt).
//
// == DIE ROTE AUSGABE VOR DEM BAU, WOERTLICH =======================================================
// Erste Fassung dieser Datei, gebaut und gefahren am 09.08.2026, BEVOR es die Arenen gab. Damals
// existierte im Haus nur EIN Aufnahme-Pfad -- der Bestands-Pfad -- und genau der lief durch das
// Fenster, mit der Forderung "0 Allokationen":
//
//     == MS-1: der Aufnahme-Pfad alloziert im Fenster nicht ==
//     -- (1) Bestands-Pfad InMemoryMeasurementBuffer::append_record --
//          Appends im Fenster = 4096, Allokationen = 13, angeforderte Bytes = 524224
//       [ERR] Bestands-Pfad: Allokationen im Mess-Fenster
//     -- (2) Gegenprobe: der Zaehler sieht drei absichtliche Allokationen --
//          erwartet = 3, gezaehlt = 3
//     FEHLGESCHLAGEN: 1 Pruefung(en)
//     RC=1
//
// 13 Umlagerungen und 524224 angeforderte Bytes -- ueber ein halbes Megabyte Belegung samt Umkopieren
// MITTEN in einem Fenster von 4096 Operationen, deren Latenz gemessen werden soll.
//
// == WAS SICH AN ABSCHNITT (1) SEITHER GEAENDERT HAT, UND WARUM DAS KEINE AUFWEICHUNG IST =========
// Abschnitt (1) fordert heute das GEGENTEIL: der Bestands-Pfad MUSS allozieren. Das ist keine
// entschaerfte Zusage, sondern ein Rollenwechsel. Der Bestands-Pfad ist nicht mehr der Pruefling --
// das sind seit Abschnitt (3) die beiden Arenen. Er bleibt als KONTRAST stehen, und er leistet dabei
// zwei Dinge, die kein Kommentar leisten koennte: er belegt fortlaufend, dass der Zaehler echte
// Allokationen sieht (waere er blind, zeigte er auch hier 0), und er haelt den Bestands-Defekt
// sichtbar, statt ihn wegzuloeschen. Die Zusage "0" ist unveraendert; sie steht in (3).
//
// == DER BESTANDS-DEFEKT, am Objekt =================================================================
// ThreadArena (include/cache_engine/measurement/thread_arena.hpp) traegt den richtigen Namen, das
// alignas(64) und den Kommentar "append-only" -- und haelt einen std::vector als Member (Z.35), in
// den append() ohne jedes reserve hineinlegt (Z.19). Die Funktion ist zusaetzlich als noexcept
// deklariert, obwohl der Weg darunter werfen kann. InMemoryMeasurementBuffer setzt darauf auf und
// nennt seinen Pfad im Kommentar "Hot-Path: pro-Thread-Arena, kein Lock" (Z.42). Wer diese beiden
// als Vorlage nimmt, baut den Defekt nach. Der static_assert auf MessArenaHeiss (mess_arena.hpp)
// faengt genau diese Fehlerklasse beim Uebersetzen: ein Vektor-Member zerstoert trivially_copyable.
//
// Build: Standalone int main() (kein gtest -- ein Test-Framework alloziert im Fenster und machte die
// Messung wertlos). Globale operator-new/delete-Ersetzung in DIESER TU gilt programmweit.
//
// KEIN ZUFALL AUSSER BEI KOEDERN (Haus-Doktrin). Die Koeder-Werte unten sind am 09.08.2026 frisch
// aus /dev/urandom gewuerfelt und NICHT aus einer Doku abgeschrieben.

#include <builder/measure_storage/checkpoint_speicher.hpp>
#include <builder/measure_storage/mess_arena.hpp>
#include <builder/measure_storage/stapel_arena.hpp>

#include <cache_engine/measurement/in_memory_measurement_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <new>
#include <string>
#include <string_view>

// ===================================================================================================
// DER INSTRUMENTIERTE ALLOCATOR -- globale Ersetzung, thread-unabhaengig (Measure ist 1-Faden).
//
// SELBSTCHECK: der Zaehler selbst darf NICHT allozieren. Deshalb rohe malloc/free und rohe
// std::uint64_t-Globale mit konstanter Initialisierung (kein dynamischer Initialisierer, keine
// Initialisierungs-Reihenfolge-Falle). Er zaehlt NUR, wenn er scharf ist -- alles ausserhalb des
// Fensters (Aufbau, Ausgabe, iostream) bleibt unberuehrt und darf frei allozieren.
//
// WAS ER NICHT SIEHT, ausdruecklich: C-malloc ohne operator new (z.B. in vendorierten C-Pfaden).
// Die Ersetzung von operator new faengt den C++-Pfad, nicht den C-Pfad. Fuer den hier geprueften
// Aufnahme-Pfad ist diese Luecke geschlossen, weil Abschnitt (9) zusaetzlich TEXTLICH prueft, dass
// die heissen Bloecke weder die eine noch die andere Belegung nennen -- zwei unabhaengige
// Beobachtungen derselben Zusage, von denen keine die andere verdeckt.
// ===================================================================================================
namespace ms_wache {

inline std::uint64_t g_neu_zaehler = 0;
inline std::uint64_t g_neu_bytes   = 0;
inline bool          g_scharf      = false;

inline void scharf_stellen() noexcept {
    g_neu_zaehler = 0;
    g_neu_bytes   = 0;
    g_scharf      = true;
}

[[nodiscard]] inline std::uint64_t entschaerfen() noexcept {
    g_scharf = false;
    return g_neu_zaehler;
}

[[nodiscard]] inline std::uint64_t bytes() noexcept { return g_neu_bytes; }

inline void* roh_holen(std::size_t n) noexcept {
    if (g_scharf) {
        ++g_neu_zaehler;
        g_neu_bytes += static_cast<std::uint64_t>(n);
    }
    return std::malloc(n != 0u ? n : 1u);
}

inline void* roh_holen_ausgerichtet(std::size_t n, std::size_t a) noexcept {
    if (g_scharf) {
        ++g_neu_zaehler;
        g_neu_bytes += static_cast<std::uint64_t>(n);
    }
    std::size_t const gepolstert = ((n != 0u ? n : 1u) + a - 1u) / a * a;
    return std::aligned_alloc(a, gepolstert);
}

} // namespace ms_wache

void* operator new(std::size_t n) {
    void* p = ms_wache::roh_holen(n);
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new[](std::size_t n) {
    void* p = ms_wache::roh_holen(n);
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new(std::size_t n, std::align_val_t a) {
    void* p = ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new[](std::size_t n, std::align_val_t a) {
    void* p = ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new(std::size_t n, std::nothrow_t const&) noexcept { return ms_wache::roh_holen(n); }
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept { return ms_wache::roh_holen(n); }
void* operator new(std::size_t n, std::align_val_t a, std::nothrow_t const&) noexcept {
    return ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a, std::nothrow_t const&) noexcept {
    return ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete[](void* p, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t, std::nothrow_t const&) noexcept { std::free(p); }

namespace {

int g_fail = 0;

void pruefe(bool ok, char const* was) {
    if (!ok) {
        ++g_fail;
        std::cout << "  [ERR] " << was << "\n";
    }
}

namespace mess = ::comdare::cache_engine::measurement;
namespace ms   = ::comdare::cache_engine::builder::measure_storage;

// Owner-KERN: Batch 4096. Der Haus-Anker fuer eine Mess-Gruppe, keine gegriffene Groesse.
inline constexpr std::uint64_t kAppends = 4096;

// == KOEDER, am 09.08.2026 frisch aus /dev/urandom gewuerfelt (K13: nie aus einer Doku) ============
inline constexpr std::uint64_t kKoederZeit1   = 3207928510ull; // passt in die Arena
inline constexpr std::uint64_t kKoederZeit2   = 2988986077ull; // passt
inline constexpr std::uint64_t kKoederZeit3   = 4129189578ull; // passt -- damit ist sie voll
inline constexpr std::uint64_t kKoederZeit4   = 139788480ull;  // MUSS verloren gehen
inline constexpr std::uint64_t kKoederStapel1 = 3044735384ull;
inline constexpr std::uint64_t kKoederStapel2 = 4182419343ull;
inline constexpr std::uint64_t kKoederStapel3 = 3816673831ull;
inline constexpr std::uint64_t kKoederStapel4 = 1938343354ull;
inline constexpr std::uint64_t kKoederZuTief  = 0xf9285cdf0d550d02ull; // MUSS abgewiesen werden
inline constexpr std::uint64_t kKoederOffen   = 0xa7ea73b0d04f92ecull; // MUSS offen liegenbleiben
inline constexpr std::size_t   kKoederBytes   = 38763;                 // Groesse der Koeder-Allokation

// Liest eine Datei ganz ein. Laeuft ausserhalb jedes Fensters -- hier darf belegt werden.
[[nodiscard]] bool datei_lesen(char const* pfad, std::string& hinein) {
    std::ifstream f(pfad, std::ios::binary);
    if (!f) return false;
    hinein.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

} // namespace

int main() {
    std::cout << "== MS-1: der Aufnahme-Pfad alloziert im Fenster nicht ==\n";

    // -- (1) KONTRAST: der Bestands-Pfad ------------------------------------------------------------
    // Die Thread-Arena wird VOR dem Fenster angelegt (ein Append ausserhalb), damit das Fenster
    // ausschliesslich den Anhaenge-Pfad misst und nicht den einmaligen std::map-Knoten. Gemessen wird
    // also der GUENSTIGSTE Fall des Bestands, nicht der schlechteste -- und er faellt trotzdem.
    {
        mess::InMemoryMeasurementBuffer puffer{};
        mess::MeasurementRecord         satz{};
        satz.timestamp_ns = 1;
        puffer.append_record(satz);

        ms_wache::scharf_stellen();
        for (std::uint64_t i = 0; i < kAppends; ++i) {
            satz.timestamp_ns = i;
            puffer.append_record(satz);
        }
        std::uint64_t const zahl  = ms_wache::entschaerfen();
        std::uint64_t const bytes = ms_wache::bytes();

        std::cout << "-- (1) Kontrast: Bestands-Pfad InMemoryMeasurementBuffer::append_record --\n";
        std::cout << "     Appends im Fenster = " << kAppends << ", Allokationen = " << zahl
                  << ", angeforderte Bytes = " << bytes << "\n";
        pruefe(zahl > 0, "Kontrast: der Bestands-Pfad muss allozieren (sonst waere der Zaehler blind)");
    }

    // -- (2) GEGENPROBE: sieht der Zaehler ueberhaupt etwas? -----------------------------------------
    // Ein Zaehler, der nie zaehlt, ist kein Zaehler. Genau drei angeforderte Blocks, drei erwartet.
    {
        // Die drei Bloecke werden auch BESCHRIEBEN UND GELESEN. Nicht aus Ordnungsliebe: ein
        // Uebersetzer darf eine Belegung, deren Ergebnis nie benutzt wird, ersatzlos entfernen
        // (C++ erlaubt das ausdruecklich fuer new/delete-Paare) -- dann zaehlte der Zaehler nichts
        // und die Gegenprobe waere selbst der wertlose Test, den sie ausschliessen soll.
        ms_wache::scharf_stellen();
        int* a                    = new int(7);
        int* b                    = new int[16];
        int* c                    = static_cast<int*>(::operator new(64));
        b[0]                      = 11;
        b[15]                     = 13;
        c[0]                      = 17;
        std::uint64_t const zahl  = ms_wache::entschaerfen();
        int const           summe = *a + b[0] + b[15] + c[0];
        delete a;
        delete[] b;
        ::operator delete(c);

        std::cout << "-- (2) Gegenprobe: der Zaehler sieht drei absichtliche Allokationen --\n";
        std::cout << "     erwartet = 3, gezaehlt = " << zahl << ", Nutzsumme = " << summe << "\n";
        pruefe(zahl == 3, "Gegenprobe: der Zaehler zaehlt");
        pruefe(summe == 48, "Gegenprobe: die Bloecke wurden wirklich benutzt (7+11+13+17)");
    }

    // -- (3) DIE ZUSAGE: die beiden Arenen im Fenster ------------------------------------------------
    // Aufbau AUSSERHALB (dort ist Belegung erlaubt), dann das Fenster: je Operation ein EIN und ein
    // AUS -- Stapel hinauf, Zeile an, Spitze lesen, Zeile an, Stapel herunter. Das ist der volle
    // Verkehr beider Arenen, nicht nur ein Anhaengen.
    {
        ms::CheckpointSpeicher sp;
        bool const             steht = ms::speicher_aufbauen(sp, 4u * kAppends, 64u, ms::VorabBeruehrung::Ja);
        pruefe(steht, "Aufbau: beide Arenen reserviert");

        ms::MessCheckpointZeile ein{};
        ein.tag = ms::tag_bauen(ms::MessEbene::Micro, ms::Richtung::Ein);
        ms::MessCheckpointZeile aus{};
        aus.tag = ms::tag_bauen(ms::MessEbene::Micro, ms::Richtung::Aus);

        std::uint64_t summe    = 0;
        std::uint64_t schlecht = 0;

        ms_wache::scharf_stellen();
        for (std::uint64_t i = 0; i < kAppends; ++i) {
            ein.zeit_ticks           = i;
            std::uint64_t const slot = sp.mess.anhaengen(ein);
            if (slot == ms::MessArena::kUeberlaufSlot) ++schlecht;

            ms::StapelEintrag e{};
            e.log_index     = slot;
            e.deskriptor_ix = static_cast<std::uint32_t>(i & 0xFFFFu);
            e.ebene         = static_cast<std::uint8_t>(ms::MessEbene::Micro);
            if (!sp.stapel.hinauf(e)) ++schlecht;

            ms::StapelEintrag const* oben = sp.stapel.spitze();
            if (oben != nullptr) summe += oben->log_index;

            aus.zeit_ticks = i;
            aus.messwert   = summe;
            if (sp.mess.anhaengen(aus) == ms::MessArena::kUeberlaufSlot) ++schlecht;
            if (!sp.stapel.herunter(nullptr)) ++schlecht;
        }
        std::uint64_t const zahl = ms_wache::entschaerfen();

        std::cout << "-- (3) DIE ZUSAGE: beide Arenen im Fenster --\n";
        std::cout << "     Anhaengen = " << (2u * kAppends) << ", Hinauf/Herunter = " << kAppends
                  << ", Spitze gelesen = " << kAppends << ", Allokationen = " << zahl << "\n";
        std::cout << "     Mess-Arena belegt = " << sp.mess.belegt() << " von " << sp.mess.kapazitaet()
                  << ", Stapel-Hoechststand = " << sp.stapel.hoechststand() << "\n";
        pruefe(zahl == 0, "DIE ZUSAGE: 0 Allokationen im Mess-Fenster");
        pruefe(schlecht == 0, "kein Ueberlauf im regulaeren Lauf");
        pruefe(sp.mess.belegt() == 2u * kAppends, "beide Zeilen je Operation liegen in der Arena");
        pruefe(sp.stapel.hoechststand() == 1, "der Stapel bleibt bei Tiefe 1 (kein Leck)");
        pruefe(sp.stapel.tiefe() == 0, "am Ende ist der Stapel leer (jedes EIN hat sein AUS)");
        pruefe(summe > 0, "die Spitze wurde wirklich gelesen (sonst misst (3) nichts)");
    }

    // -- (4) GEGENPROBE K13: eine absichtliche Allokation IM GEMESSENEN BEREICH ----------------------
    // Derselbe Arenen-Verkehr wie in (3), aber mit einem Koeder mitten drin. Der Zaehler MUSS ihn
    // sehen -- sonst beweist (3) nichts, weil dann jeder Pfad "0" ergaebe.
    // ZWEI unabhaengige Observablen: die ANZAHL und die BYTE-Zahl. Ein Mutant, der die Zaehlung
    // stillegte, faellt an beiden; einer, der nur die Byte-Summe verlore, faellt an der zweiten.
    {
        ms::CheckpointSpeicher sp;
        pruefe(ms::speicher_aufbauen(sp, 1024u, 64u, ms::VorabBeruehrung::Ja), "Aufbau (4)");

        ms::MessCheckpointZeile z{};
        z.tag = ms::tag_bauen(ms::MessEbene::Macro, ms::Richtung::Ein);

        ms_wache::scharf_stellen();
        (void)sp.mess.anhaengen(z);
        char* koeder             = new char[kKoederBytes]; // DER KOEDER -- absichtlich, im Fenster
        koeder[0]                = 1;
        koeder[kKoederBytes - 1] = 2;
        (void)sp.mess.anhaengen(z);
        std::uint64_t const zahl  = ms_wache::entschaerfen();
        std::uint64_t const bytes = ms_wache::bytes();
        delete[] koeder;

        std::cout << "-- (4) Gegenprobe K13: eine absichtliche Allokation im gemessenen Bereich --\n";
        std::cout << "     Koeder-Groesse = " << kKoederBytes << ", gezaehlt = " << zahl << ", Bytes = " << bytes
                  << "\n";
        pruefe(zahl == 1, "K13: der Zaehler sieht den Koeder im Arenen-Fenster");
        pruefe(bytes == kKoederBytes, "K13: die Byte-Zahl ist exakt die Koeder-Groesse");
    }

    // -- (5) KOEDER Mess-Arena: Ueberlauf ist DATENVERLUST -------------------------------------------
    // Kapazitaet 3, vier Zeilen angeboten. VIER unabhaengige Observablen, damit kein Mutant von einer
    // anderen Zeile verdeckt stirbt:
    //   (a) die Rueckgabe der vierten ist kUeberlaufSlot     -> faengt "Kapazitaets-Pruefung entfernt"
    //   (b) verloren == 1 und belegt == 3                    -> faengt "Verlust-Zaehlung entfernt"
    //   (c) der Platz HINTER der Kapazitaet ist unberuehrt   -> faengt "ueber die Kante geschrieben"
    //   (d) der Koeder-Wert steht in keiner ausgelesenen Zeile
    // Beobachtbarkeit geprueft: (a) und (c) unterscheiden sich zwischen MIT und OHNE Kapazitaets-
    // Pruefung; (b) unterscheidet sich zwischen MIT und OHNE weiterlaufendem Versuchs-Zaehler. Keine
    // der vier faengt den Mutanten der anderen, sie liegen auf verschiedenen Observablen.
    {
        ms::MessArena arena;
        pruefe(arena.reservieren(3u, ms::VorabBeruehrung::Ja), "Aufbau (5): Kapazitaet 3");

        ms::MessCheckpointZeile z{};
        z.zeit_ticks           = kKoederZeit1;
        std::uint64_t const s1 = arena.anhaengen(z);
        z.zeit_ticks           = kKoederZeit2;
        std::uint64_t const s2 = arena.anhaengen(z);
        z.zeit_ticks           = kKoederZeit3;
        std::uint64_t const s3 = arena.anhaengen(z);
        z.zeit_ticks           = kKoederZeit4; // der Koeder
        std::uint64_t const s4 = arena.anhaengen(z);

        ms::AusleseErgebnis const erg             = arena.auslesen();
        bool                      koeder_gefunden = false;
        for (auto const& zeile : erg.zeilen) {
            if (zeile.zeit_ticks == kKoederZeit4) koeder_gefunden = true;
        }

        auto const* roh   = static_cast<ms::MessCheckpointZeile const*>(arena.nutzlast_basis());
        bool const  kante = (roh != nullptr) && (roh[3].zeit_ticks == 0u);

        char text[256];
        (void)ms::befund_zeile(erg.befund, text, sizeof(text));

        std::cout << "-- (5) Koeder Mess-Arena: Ueberlauf ist DATENVERLUST --\n";
        std::cout << "     Slots = " << s1 << "," << s2 << "," << s3 << ","
                  << (s4 == ms::MessArena::kUeberlaufSlot ? std::uint64_t{9999} : s4)
                  << " (9999 == Ueberlauf-Kennung)\n";
        std::cout << "     " << text << "\n";
        pruefe(s1 == 0 && s2 == 1 && s3 == 2, "(5a) die ersten drei liegen auf 0,1,2");
        pruefe(s4 == ms::MessArena::kUeberlaufSlot, "(5a) die vierte meldet Ueberlauf");
        pruefe(erg.befund.verloren == 1 && erg.befund.belegt == 3, "(5b) verloren = 1 von 4, belegt = 3");
        pruefe(kante, "(5c) hinter der Kapazitaet wurde nicht geschrieben");
        pruefe(!koeder_gefunden, "(5d) der Koeder-Wert steht in keiner ausgelesenen Zeile");
        pruefe(std::string_view{text}.find("DATENVERLUST") != std::string_view::npos,
               "(5e) die Meldung nennt die Fehlerklasse Datenverlust");
        pruefe(std::string_view{text}.find("von 4") != std::string_view::npos,
               "(5f) die Meldung nennt den Nenner, nicht nur die Zahl");
    }

    // -- (6) KOEDER Stapel-Arena: Ueberlauf ist ein PROGRAMMIERFEHLER, kein Datenverlust -------------
    // Dieselbe Lage, andere Arena -- und die Meldung MUSS eine andere sein. Das ist die
    // Fehlerklassen-Trennung, gebaut statt versprochen: die beiden Texte duerfen sich nicht teilen.
    {
        ms::StapelArena stapel;
        pruefe(stapel.reservieren(4u, ms::VorabBeruehrung::Ja), "Aufbau (6): Tiefe 4");

        ms::StapelEintrag e{};
        e.log_index   = kKoederStapel1;
        bool const h1 = stapel.hinauf(e);
        e.log_index   = kKoederStapel2;
        bool const h2 = stapel.hinauf(e);
        e.log_index   = kKoederStapel3;
        bool const h3 = stapel.hinauf(e);
        e.log_index   = kKoederStapel4;
        bool const h4 = stapel.hinauf(e);
        e.log_index   = kKoederZuTief; // der Koeder -- eine Ebene zu tief
        bool const h5 = stapel.hinauf(e);

        bool koeder_gefunden = false;
        for (auto const& offen : stapel.offene()) {
            if (offen.log_index == kKoederZuTief) koeder_gefunden = true;
        }

        // unpaariges AUS: fuenfmal herunter bei vier offenen -> eigene Fehlerklasse, eigener Zaehler
        for (int i = 0; i < 5; ++i) (void)stapel.herunter(nullptr);

        ms::StapelBefund const b = stapel.befund();
        char                   text[320];
        (void)ms::befund_zeile(b, text, sizeof(text));

        std::cout << "-- (6) Koeder Stapel-Arena: Ueberlauf ist ein PROGRAMMIERFEHLER --\n";
        std::cout << "     " << text << "\n";
        pruefe(h1 && h2 && h3 && h4, "(6a) vier Ebenen passen");
        pruefe(!h5, "(6b) die fuenfte wird abgewiesen");
        pruefe(b.zu_tief == 1, "(6c) zu_tief = 1");
        pruefe(!koeder_gefunden, "(6d) der abgewiesene Koeder liegt nicht auf dem Stapel");
        pruefe(b.hoechststand == 4, "(6e) der Hoechststand ist 4, nicht 5");
        pruefe(b.unpaarige_aus == 1, "(6f) ein unpaariges AUS, eigener Zaehler");
        pruefe(stapel.tiefe() == 0, "(6g) der Top-Zaehler laeuft nicht unter Null");
        pruefe(std::string_view{text}.find("PROGRAMMIERFEHLER") != std::string_view::npos,
               "(6h) die Meldung nennt die Fehlerklasse Programmierfehler");
        pruefe(std::string_view{text}.find("DATENVERLUST") == std::string_view::npos,
               "(6i) die Stapel-Meldung nennt NIEMALS Datenverlust -- getrennte Fehlerklassen");
    }

    // -- (7) IN/OUT-SYMMETRIE: ein EIN ohne AUS bleibt liegen und wird gemeldet ----------------------
    // Drei hinauf, zwei herunter. Das dritte EIN ist eine REGRESSION -- es muss beim Auslesen noch da
    // sein, mit seinem Koeder-Wert, und in der Meldung auftauchen. Wuerde der Stapel sich am Ende
    // selbst leeren, waere genau die Regression vernichtet, die er anzeigen sollte.
    {
        ms::StapelArena stapel;
        pruefe(stapel.reservieren(8u, ms::VorabBeruehrung::Ja), "Aufbau (7): Tiefe 8");

        ms::StapelEintrag e{};
        e.log_index     = kKoederOffen; // DAS EIN, das offen bleibt
        e.deskriptor_ix = 0x0d55u;
        e.ebene         = static_cast<std::uint8_t>(ms::MessEbene::Macro);
        (void)stapel.hinauf(e);
        e.log_index = 11;
        e.ebene     = static_cast<std::uint8_t>(ms::MessEbene::Micro);
        (void)stapel.hinauf(e);
        e.log_index = 22;
        (void)stapel.hinauf(e);
        (void)stapel.herunter(nullptr);
        (void)stapel.herunter(nullptr);

        auto const             offen = stapel.offene();
        ms::StapelBefund const b     = stapel.befund();
        char                   text[320];
        (void)ms::befund_zeile(b, text, sizeof(text));

        std::cout << "-- (7) IN/OUT-Symmetrie: ein EIN ohne AUS bleibt liegen --\n";
        std::cout << "     offen = " << offen.size() << ", log_index der Leiche = 0x" << std::hex
                  << (offen.empty() ? 0ull : offen[0].log_index) << std::dec << "\n";
        std::cout << "     " << text << "\n";
        pruefe(offen.size() == 1, "(7a) genau ein EIN blieb offen");
        pruefe(!offen.empty() && offen[0].log_index == kKoederOffen, "(7b) es ist das Koeder-EIN, nicht irgendeines");
        pruefe(!offen.empty() && offen[0].ebene == static_cast<std::uint8_t>(ms::MessEbene::Macro),
               "(7c) seine Mess-Ebene reist mit -- die Meldung kann sagen, WELCHES offen blieb");
        pruefe(b.offen == 1 && !b.sauber(), "(7d) der Befund nennt es und gilt als nicht sauber");
        pruefe(std::string_view{text}.find("offen_geblieben = 1") != std::string_view::npos,
               "(7e) die Meldung nennt die offene Klammer ausdruecklich");
    }

    // -- (8) DIE HAUSKONSTANTE gegen die Maschine, und die Trennung am Objekt ------------------------
    {
        std::cout << "-- (8) Hauskonstante und Trennung --\n";
        std::string roh;
        bool const  gelesen = datei_lesen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", roh);
        if (gelesen) {
            long const gemessen = std::strtol(roh.c_str(), nullptr, 10);
            std::cout << "     sysfs coherency_line_size = " << gemessen << ", Hauskonstante = " << ms::kCacheLineBytes
                      << "\n";
            pruefe(static_cast<std::size_t>(gemessen) == ms::kCacheLineBytes,
                   "(8a) die Hauskonstante stimmt mit der Maschine ueberein");
        } else {
            // KEIN stiller Durchlauf: die Nichtpruefbarkeit wird ausgesprochen.
            std::cout << "     sysfs nicht lesbar -- die Hauskonstante ist auf dieser Maschine UNGEPRUEFT\n";
        }

        ms::CheckpointSpeicher sp;
        pruefe(ms::speicher_aufbauen(sp, 256u, 16u, ms::VorabBeruehrung::Ja), "Aufbau (8)");

        auto const linie_mess   = reinterpret_cast<std::uintptr_t>(sp.mess.heisse_basis()) / ms::kCacheLineBytes;
        auto const linie_stapel = reinterpret_cast<std::uintptr_t>(sp.stapel.heisse_basis()) / ms::kCacheLineBytes;
        auto const seite_mess   = reinterpret_cast<std::uintptr_t>(sp.mess.nutzlast_basis()) / ms::kSeitenBytes;
        auto const seite_stapel = reinterpret_cast<std::uintptr_t>(sp.stapel.nutzlast_basis()) / ms::kSeitenBytes;

        std::cout << "     heisse Cacheline-Nummern: mess = " << linie_mess << ", stapel = " << linie_stapel
                  << " (Abstand " << (linie_stapel - linie_mess) << " Linien)\n";
        std::cout << "     Nutzlast-Seiten: mess = " << seite_mess << ", stapel = " << seite_stapel << "\n";
        pruefe(linie_mess != linie_stapel, "(8b) die heissen Zustaende liegen in verschiedenen Cachelines");
        pruefe(seite_mess != seite_stapel, "(8c) die Nutzlasten liegen auf verschiedenen Seiten");
        pruefe(sizeof(ms::MessCheckpointZeile) == 32 && sizeof(ms::StapelEintrag) == 16,
               "(8d) die POD-Groessen sind 32 und 16 Byte");
    }

    // -- (9) TEXT-WACHE auf den heissen Bloecken ----------------------------------------------------
    // Sie schliesst die Luecke, die eine operator-new-Ersetzung strukturell offenlaesst: eine
    // C-Belegung waere fuer den Zaehler unsichtbar. ZWEI Zusicherungen, damit kein Mutant still
    // durchkommt: die Marker MUESSEN gefunden werden (sonst ist eine leere Wache eine gruene Wache),
    // UND der eingeklammerte Block muss frei von den verbotenen Woertern sein. Zusaetzlich muss der
    // Block die geprueften Funktionsnamen enthalten -- wer die Funktion aus der Klammer heraustraegt,
    // bricht die Wache statt sie zu umgehen.
    {
        std::cout << "-- (9) Text-Wache auf den heissen Bloecken --\n";
        char const* quellen[2] = {COMDARE_MS_QUELLE_MESS, COMDARE_MS_QUELLE_STAPEL};
        char const* anker[2]   = {"anhaengen", "hinauf"};
        char const* verboten[] = {"new",       "malloc",      "calloc",     "realloc",   "std::vector",
                                  "push_back", "std::string", "std::stack", "std::deque"};

        for (int q = 0; q < 2; ++q) {
            std::string inhalt;
            if (!datei_lesen(quellen[q], inhalt)) {
                pruefe(false, "(9) heisse Quelle lesbar");
                continue;
            }
            std::size_t const a         = inhalt.find("[MS-HEISS]");
            std::size_t const e         = inhalt.rfind("[MS-HEISS-ENDE]");
            bool const        marker_da = (a != std::string::npos) && (e != std::string::npos) && (e > a);
            pruefe(marker_da, "(9a) beide Marker stehen in der heissen Quelle");
            if (!marker_da) continue;

            std::string const block = inhalt.substr(a, e - a);
            pruefe(block.find(anker[q]) != std::string::npos,
                   "(9b) die gepruefte Funktion steht INNERHALB der Klammer");

            int treffer = 0;
            for (char const* wort : verboten) {
                if (block.find(wort) != std::string::npos) {
                    ++treffer;
                    std::cout << "     [FUND] verbotenes Wort im heissen Block: " << wort << "\n";
                }
            }
            std::cout << "     " << quellen[q] << ": Block = " << block.size()
                      << " Byte, verbotene Woerter = " << treffer << "\n";
            pruefe(treffer == 0, "(9c) der heisse Block nennt keine Belegung und keinen STL-Behaelter");
        }

        // GEGENPROBE zur Wache selbst: sie muss auf einem Text, der die Woerter enthaelt, anschlagen.
        // Ohne diese Zeile waere nicht gezeigt, dass die Suche ueberhaupt etwas finden KANN.
        std::string const probe    = "auto* p = new int; void* q = malloc(8); std::vector<int> v; v.push_back(1);";
        int               gefunden = 0;
        for (char const* wort : verboten) {
            if (probe.find(wort) != std::string::npos) ++gefunden;
        }
        std::cout << "     Gegenprobe der Wache: " << gefunden << " von 9 verbotenen Woertern im Probetext\n";
        pruefe(gefunden >= 4, "(9d) die Wache findet die Woerter, wenn sie da sind");
    }

    if (g_fail == 0) {
        std::cout << "OK: MS-1\n";
        return 0;
    }
    std::cout << "FEHLGESCHLAGEN: " << g_fail << " Pruefung(en)\n";
    return 1;
}
