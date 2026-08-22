#pragma once
// lager/bestand_schluessel_schema.hpp -- I-1: die Typ-1..4-Lager-SCHLUESSEL als CT-SCHEMA
// (#91-Vollzug W2-D, 2026-08-21; design91-v2 Abschnitt 4 KLASSE B, Eintrag I-1 -- der
// Kern-Identitaets-Entscheid des Unifikations-Designs; Owner-Wort KON110-04 + KON101 V-09R/V-10b).
//
// WAS DIESE DATEI IST -- UND WAS SIE NICHT IST. Sie ist der Anteil von I-1 AUSSERHALB der
// D-2-Lager-Kette: das SCHEMA der Bestands-Schluessel als Compile-Time-Deklaration, gegen die der
// D-2-Zug (#57(7)-(9), Di-25-Buendel: Bestand-3/4-Factory, Invalidierungs-Implementierung,
// N2-Schreibweg, Verbund1-Skip-Logik) seine Mechanik baut. Sie baut selbst KEINE Lager-Mechanik,
// keinen Digest und keinen Schreibweg -- Drift zwischen Schema und Mechanik beisst compile-time
// beim D-2-Bau, nicht erst im Lager.
//
// WARUM VOR DEM TRIGGER (Mi 26.08. 06:00): ab dem Trigger fuellt die Flotte Bestand 1/2 unter
// diesen Schluesseln; ein spaeterer Schluessel-Wechsel re-keyed das Lager (design91-v2, Massstab
// (c): Lager-Schluessel/persistierte Record-Schemata). Identitaets-Umbauten sind heute billig,
// ab dem Trigger teuer (KON34-04-Linie).
//
// QUELLEN AM OBJEKT (Bestands-Pflicht -- kein neues Verfahren, FORTSCHREIBUNG der gebauten Formen):
//   Bestand 1  builder/bestandslog/bestandslog_factory.hpp   BinaryKeyPolicy: die EINE SHA512-
//              Wahrheit (identisch abi::anatomy_fingerprint_hex) + Zell-Klammer.
//   Bestand 2  builder/bestandslog/messwert_key_source.hpp   Komponenten (feste Reihenfolge):
//              [0] 128-hex-Fingerprint der vermessenen Binary, [1] Hardware-Identitaet der
//              messenden Maschine; die ZELLE [d,e,f] wird DANEBEN geklammert
//              (Section 62-NACHTRAG-4), KEINE Fusion in den Digest (Section 66-N3).
//   Kanonische Formen: profile_facade/planner/plan_legend.hpp canonical_combo (dedupliziert +
//              sortiert = Reihenfolge-unabhaengige Identitaet) + builder/bestandslog/
//              planer_block_value.hpp ceb_key_sha512 + sha512/ctsha512 ("keine zweite
//              Hash-Wahrheit", planer_driven_build.hpp).
//   machine_id include/cache_engine/measurement/machine_identity.hpp (KLASSEN-Eigenschaft
//              cpu_fabrication + ram_pair; Hostname ist NIE Teil der Identitaet, Ledger 70.6).
//
// BEGRIFFS-HINWEIS (I-7/M13): Bestand 2 nennt seine Maschinen-Komponente am Objekt
// "Hardware-Identitaet", design91-v2 nennt die Bestand-3/4-Komponente "machine_id". Dieses Schema
// uebernimmt BEIDE Wortlaute verbatim (keine stille Glaettung); ob der D-2-Zug die Begriffe
// vereinigt oder als Alias in der Begriffs-Alias-Registry (naming/begriffs_alias_registry.hpp)
// deklariert, entscheidet er an seinem Fenster -- Alias VOR Rename (I-7).
//
// NAMENS-DISZIPLIN (R-2/KON112-09): die Genus-NAMEN der Bestaende 3/4 sind hier ABSICHTLICH leer.
// "Was ein Ding IST" (Namens-SCHEMA) bleibt separater Owner-Entscheid; die Vorlage faellt an der
// ersten Neu-Benennung -- also im D-2-Zug, der die Genus-Enumeratoren real anlegt. Dieses Schema
// adressiert die Bestaende ueber ihre NUMMER (Typ-1..4-Vokabular des Owners, KON110-04).
//
// SELBSTCHECK: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik, kein Stempel-/golden-
// Byte; header-only; ASCII-only; keine Beruehrung von axes/, topics/, heuristik/.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::lager {

// Single-Source: ein 5. Bestand bricht hier compile-time (statt still 4 zu bleiben).
inline constexpr std::size_t kBestandArtCount = 4;

// Maximale Komponentenzahl eines Bestands-Digests (Bestand 3 und 4 tragen je drei Komponenten).
inline constexpr std::size_t kMaxSchluesselKomponenten = 3;

/// EINE Zeile des Schluessel-Schemas: welcher Bestand, aus welchen Komponenten sein Digest in
/// welcher festen Reihenfolge entsteht, und was DANEBEN geklammert wird (nie in den Digest).
// Default-Initialisierer nach dem Muster des heuristik-Bestands (axis_optimization_catalog.hpp):
// jedes Feld auch bei kuenftiger Teil-Initialisierung bestimmt; cppcheck-gruen.
struct BestandSchluesselSchema {
    std::uint8_t     bestand_nr  = 0;  ///< 1..4 (Typ-1..4-Vokabular KON110-04)
    std::string_view genus_token = {}; ///< "binary"/"measurement" (Par.62-B, Bestand 1/2); LEER fuer
                                       ///< 3/4 (R-2-Namens-SCHEMA: Benennung = D-2-Fenster)
    /// Digest-Komponenten in FESTER Reihenfolge (Bestand 1/2 verbatim vom Objekt, Bestand 3/4
    /// verbatim design91-v2 Klasse B I-1). Leere Slots hinter komponenten_zahl bleiben leer.
    std::array<std::string_view, kMaxSchluesselKomponenten> komponenten      = {};
    std::size_t                                             komponenten_zahl = 0; ///< wie viele der Slots belegt sind
    /// Was DANEBEN geklammert wird (Section 62-NACHTRAG-4: Zelle NIE in den Digest, sonst
    /// dedupliziert avx2 gegen avx512 weg). LEER = Klammer-Frage liegt beim D-2-Fenster.
    std::string_view zell_klammer = {};
};

/// Das EINE Schluessel-Schema der vier Bestaende -- Index == bestand_nr - 1 (static_assert unten).
inline constexpr std::array<BestandSchluesselSchema, kBestandArtCount> kBestandSchluesselSchema{{
    // Bestand 1 -- Binary-Bau (gebaut): die EINE SHA512-Wahrheit ueber die Stempel-Zeilen
    // (identisch abi::anatomy_fingerprint_hex), Zelle [a,b,c] daneben geklammert.
    {1, "binary", {"voll_stempel_fingerprint_sha512"}, 1, "[a,b,c]"},
    // Bestand 2 -- Messwerte (gebaut, messwert_key_source.hpp verbatim): der SKIP haengt an der
    // IDENTITAET des Gemessenen, nicht an seinem Wert -- Messwerte gehen NIE in den Schluessel.
    {2, "measurement", {"binary_fingerprint_128hex", "hardware_identitaet"}, 2, "[d,e,f]"},
    // Bestand 3 -- Synthese-Batches (Typ 3, D-2 baut; Komponenten verbatim design91-v2 I-1):
    // machine_id x Voll-Stempel(+Fingerprint) x Mess-Ebenen-/Kanal-Referenz.
    {3, "", {"machine_id", "voll_stempel_fingerprint", "mess_ebenen_kanal_referenz"}, 3, ""},
    // Bestand 4 -- Loesungen (Typ 4, D-2 baut; Komponenten verbatim design91-v2 I-1):
    // XML-C14N-Hash x machine_id x Bestands-/Stempel-Referenzen.
    {4, "", {"xml_c14n_hash", "machine_id", "bestands_stempel_referenzen"}, 3, ""},
}};

// -- Die EINE Hash-Wahrheit (SHA-512-Linie) ------------------------------------------------------
// C14N-Verfahren = FORTSCHREIBUNG der bestehenden kanonischen Formen; KEIN neues Verfahren
// (design91-v2 I-1: "canonical_combo + ceb_key_sha512 + ctsha512-Digest -- EINE Hash-Wahrheit").
inline constexpr std::string_view                kEineHashWahrheit = "sha512/ctsha512";
inline constexpr std::array<std::string_view, 3> kKanonischeFormenBestand{"canonical_combo", "ceb_key_sha512",
                                                                          "ctsha512"};

// -- Invalidierungs-Regel (Owner-Wort KON110-04) -------------------------------------------------
// "Ergaenzung ja, Kernbestand bleibt": neue Eintraege ergaenzen das Lager; der Kernbestand wird
// bei Erweiterungen NICHT weggeworfen. Selektive Invalidierung laeuft ueber die lesbaren
// Versions-Tags (bestandslog_document.hpp v4, OE-C), nie ueber Schluessel-Fusion (Section 66-N3).
inline constexpr bool kInvalidierungErgaenzungJa = true;
inline constexpr bool kKernbestandBleibt         = true;

// -- Verbund1-Skip-Geltung (KON101 V-10b + L4) ---------------------------------------------------
// Der Verbund1-Skip prueft Vollstaendigkeit JE machine_id, NIE global; und er ist ein BAU-Skip,
// KEIN Mess-Skip ("Bau-SKIP ja / Mess-SKIP nein").
inline constexpr bool kVerbund1SkipPrueftJeMachineId = true;
inline constexpr bool kVerbund1SkipPrueftGlobal      = false;
inline constexpr bool kVerbund1BauSkip               = true;
inline constexpr bool kVerbund1MessSkip              = false;

namespace detail {
[[nodiscard]] consteval bool bestand_schluessel_schema_ist_vollstaendig() {
    for (std::size_t i = 0; i < kBestandArtCount; ++i) {
        BestandSchluesselSchema const& z = kBestandSchluesselSchema[i];
        if (z.bestand_nr != i + 1) return false; // Index == bestand_nr - 1
        if (z.komponenten_zahl == 0 || z.komponenten_zahl > kMaxSchluesselKomponenten) return false;
        for (std::size_t k = 0; k < z.komponenten_zahl; ++k)
            if (z.komponenten[k].empty()) return false; // belegte Slots nie leer
        for (std::size_t k = z.komponenten_zahl; k < kMaxSchluesselKomponenten; ++k)
            if (!z.komponenten[k].empty()) return false; // unbelegte Slots wirklich leer
    }
    return true;
}
} // namespace detail

static_assert(kBestandSchluesselSchema.size() == kBestandArtCount,
              "kBestandSchluesselSchema: Array-Groesse == kBestandArtCount (Anzahl-Anker).");
static_assert(detail::bestand_schluessel_schema_ist_vollstaendig(),
              "kBestandSchluesselSchema: 4 Zeilen, Index == bestand_nr - 1, Komponenten-Slots konsistent.");
// Namen-Anker (Pin-Doktrin, bewusste Literal-Gegenprobe wie run_methodology_registry): Drift der
// Genus-Tokens von Bestand 1/2 gegen Par.62-B bricht hier compile-time; Bestand 3/4 bleiben
// namenlos, bis das D-2-Fenster benennt (R-2).
static_assert(kBestandSchluesselSchema[0].genus_token == std::string_view{"binary"} &&
                  kBestandSchluesselSchema[1].genus_token == std::string_view{"measurement"} &&
                  kBestandSchluesselSchema[2].genus_token.empty() && kBestandSchluesselSchema[3].genus_token.empty(),
              "Genus-Tokens: Bestand 1/2 = {binary, measurement} (Par.62-B); 3/4 namenlos bis D-2 (R-2).");
// Komponenten-Anker Bestand 2 (messwert_key_source.hpp verbatim): [0] Fingerprint, [1] Hardware.
static_assert(kBestandSchluesselSchema[1].komponenten[0] == std::string_view{"binary_fingerprint_128hex"} &&
                  kBestandSchluesselSchema[1].komponenten[1] == std::string_view{"hardware_identitaet"},
              "Bestand 2: Komponenten-Reihenfolge [0] Binary-Fingerprint, [1] Hardware-Identitaet "
              "(messwert_key_source.hpp, Ledger-Formel 'voll-permutativ + Hardware-Identitaet').");
// Komponenten-Anker Bestand 3/4 (design91-v2 Klasse B I-1 verbatim, Text-Reihenfolge).
static_assert(kBestandSchluesselSchema[2].komponenten[0] == std::string_view{"machine_id"} &&
                  kBestandSchluesselSchema[3].komponenten[0] == std::string_view{"xml_c14n_hash"} &&
                  kBestandSchluesselSchema[3].komponenten[1] == std::string_view{"machine_id"},
              "Bestand 3/4: Komponenten verbatim design91-v2 I-1 (machine_id x Stempel x Kanal-Referenz; "
              "XML-C14N x machine_id x Referenzen).");

/// constexpr-Lookup ueber die Bestands-Nummer (1..4). Unbekannte Nummer => nullptr (Gegeneingang
/// testbar); die Mechanik-Seite (D-2) darf daraus NIE still einen Default machen.
[[nodiscard]] constexpr BestandSchluesselSchema const* bestand_schluessel_schema_of(std::uint8_t nr) noexcept {
    for (std::size_t i = 0; i < kBestandArtCount; ++i)
        if (kBestandSchluesselSchema[i].bestand_nr == nr) return &kBestandSchluesselSchema[i];
    return nullptr;
}

} // namespace comdare::cache_engine::lager
