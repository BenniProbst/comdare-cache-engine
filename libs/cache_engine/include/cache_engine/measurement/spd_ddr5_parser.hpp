#pragma once
// measurement/spd_ddr5_parser.hpp -- DDR5-SPD-Bytes -> JEDEC-Nennrate (Task #7, Paket P1).
//
// WAS DIESE DATEI IST: ein reiner BYTE-Parser. Er nimmt einen fertig gelesenen Puffer entgegen und
// gibt Zahlen oder eine Fehlerklasse zurueck -- KEIN Datei-Zugriff, KEIN Prozess-Aufruf, keine
// Abhaengigkeit. Das Lesen der Quelle (sysfs-Pfad, Pfad-Injektion, Docker-Fall) gehoert in die
// Erhebungs-Kette von P2; hier ist es bewusst NICHT, damit der Parser constexpr-faehig bleibt und in
// Tests ohne Hardware exerziert werden kann.
//
// WARUM EIGENER PARSER STATT BIBLIOTHEK (Recherche §4, Vendoring-Doktrin): hwinfo/hwloc/infoware/
// libcpuid/fastfetch loesen die Frage NICHT -- sie kapseln dieselben zwei Quellen (SPD/DMI) und
// liefern entweder keine Frequenz oder verlangen Privilegien. Eine Fremdbibliothek zu vendoren, die
// die Kernfrage nicht beantwortet, waere teurer als diese knapp 100 Zeilen.
//
// -- DIE QUELLE UND IHR WERT (gemessen auf prod1, 27.07.2026) ------------------------------------
// /sys/bus/i2c/devices/1-0051/eeprom ist 0444 (world-readable) und liefert 1024 Byte. Das ist der
// EINZIGE unprivilegierte Kanal zu RAM-Daten: SMBIOS Type 17 (der konfigurierte Ist-Takt) ist
// root-only. Was hier herauskommt, ist deshalb ausdruecklich NUR die JEDEC-NENNRATE der Riegel --
// welches Profil das BIOS aktiviert hat, steht im SPD NICHT. Wer diesen Wert als Ist-Takt ausgibt,
// behauptet eine Messung, wo ein Datenblatt-Wert steht (Provenienz-Stufe spd_jedec_base, nie
// configured_measured).
//
// -- OFFSETS (JEDEC DDR5 SPD Annex, gegen den realen prod1-Dump verifiziert) ----------------------
//   Byte 2      : Key Byte / DRAM-Typ. 18 (0x12) = DDR5. Alles andere wird NICHT interpretiert.
//   Byte 20/21  : tCKAVGmin in Pikosekunden, LITTLE ENDIAN (prod1: 160,1 -> 416 ps -> 4800 MT/s).
//   Byte 640    : XMP-3.0-Magic 0x0C 0x4A.
//   Byte 832    : EXPO-Magic, ASCII "EXPO".
//
// -- WAS BEWUSST NICHT GEPARST WIRD --------------------------------------------------------------
// Die PROFIL-FREQUENZ von XMP/EXPO (prod1 traegt ein 5600er-Angebot) wird NICHT ausgelesen. Fuer die
// Magic-Bytes liegt eine belegte Offset-Angabe vor, fuer die Lage der Profil-Timings innerhalb der
// Bloecke nicht -- sie aus einem einzigen Dump zu erschliessen waere geraten, und ein falsch
// gelesenes Angebot koennte spaeter als Frequenz erscheinen. Der Parser meldet deshalb nur die
// PRAESENZ eines Angebots; die Frequenz bleibt offen, bis die Blockstruktur belegt ist.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/ram_frequency_reading.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>

namespace comdare::cache_engine::measurement {

/// Offsets und Kennungen als benannte Konstanten -- eine nackte 20 im Code waere in einem Byte-Parser
/// die schlechteste aller Varianten.
inline constexpr std::size_t  kSpdKeyByteOffset   = 2;
inline constexpr std::uint8_t kSpdKeyDdr5         = 18; // 0x12
inline constexpr std::size_t  kSpdTckAvgMinOffset = 20; // 2 Byte, little endian, Pikosekunden
inline constexpr std::size_t  kSpdXmpMagicOffset  = 640;
inline constexpr std::size_t  kSpdExpoMagicOffset = 832;
/// Kleinster Puffer, aus dem sich ueberhaupt eine Rate ableiten laesst (Byte 21 muss enthalten sein).
inline constexpr std::size_t kSpdMinBytesForRate = kSpdTckAvgMinOffset + 2;

/// Plausibilitaets-Fenster fuer tCKAVGmin. Bewusst WEIT: es soll Muell abweisen (0x0000 aus einer
/// halb gelesenen Datei, 0xFFFF aus geloeschtem EEPROM = 30 MT/s), nicht kuenftige Speed-Bins
/// aussperren. 100..2000 ps entspricht rund 1000..20000 MT/s und deckt DDR5 mit grossem Rand ab.
inline constexpr std::uint32_t kSpdTckPsMin = 100;
inline constexpr std::uint32_t kSpdTckPsMax = 2000;

/// Das Ergebnis eines gelungenen Parse-Laufs. jedec_mts ist die auf die JEDEC-Stufe gerundete Rate,
/// tck_ps der Rohwert -- beide, damit die Ableitung nachpruefbar bleibt und nicht geglaubt werden muss.
struct SpdDdr5Info {
    std::uint32_t tck_ps       = 0;
    std::uint32_t jedec_mts    = 0;
    bool          xmp_present  = false; ///< XMP-3.0-Angebot vorhanden (Frequenz NICHT gelesen, s. Kopf)
    bool          expo_present = false; ///< EXPO-Angebot vorhanden (dito)
};

/// Parser-Ausgabe: Werte ODER eine Erhebungs-Fehlerklasse -- nie ein halb gefuelltes Ergebnis.
using SpdParseResult = std::expected<SpdDdr5Info, HardwareProbeErrorClass>;

/// 16-Bit little endian ab `off`. Der Aufrufer stellt die Laenge sicher (im Parser geprueft).
[[nodiscard]] constexpr std::uint32_t spd_read_u16_le(std::span<std::uint8_t const> b, std::size_t off) noexcept {
    return static_cast<std::uint32_t>(b[off]) | (static_cast<std::uint32_t>(b[off + 1]) << 8);
}

/// Taktzeit -> JEDEC-Datenrate. DDR uebertraegt zwei Mal je Takt: MT/s = 2 * 10^6 / tCK[ps].
/// Die Division trifft die Nennrate nicht exakt (416 ps -> 4807,7), deshalb wird auf die naechste
/// 100er-Stufe gerundet -- so faellt 416 ps auf 4800 und 357 ps auf 5600, beides die JEDEC-/Profil-
/// Bezeichnungen. Ohne diese Rundung stuende eine krumme Zahl in der CSV, die keinem Datenblatt
/// entspricht.
[[nodiscard]] constexpr std::uint32_t spd_mts_from_tck_ps(std::uint32_t tck_ps) noexcept {
    if (tck_ps == 0) return 0;
    std::uint32_t const roh = 2000000U / tck_ps;
    return ((roh + 50U) / 100U) * 100U;
}

/// Der Parse-Lauf. Reihenfolge der Pruefungen ist Absicht: erst ob ueberhaupt genug Bytes da sind,
/// dann ob das Format unseres ist, dann ob der Wert plausibel ist. Ein zu kurzer Puffer ist KORRUPT
/// (die Quelle war da, lieferte aber nicht genug), ein fremder Key ist FORMAT_UNBEKANNT (die Quelle
/// ist in Ordnung, nur nicht unsere) -- diese Unterscheidung ist der Grund fuer zwei Fehlerklassen.
[[nodiscard]] constexpr SpdParseResult parse_spd_ddr5(std::span<std::uint8_t const> bytes) noexcept {
    if (bytes.size() <= kSpdKeyByteOffset) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
    if (bytes[kSpdKeyByteOffset] != kSpdKeyDdr5) return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);
    if (bytes.size() < kSpdMinBytesForRate) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    std::uint32_t const tck = spd_read_u16_le(bytes, kSpdTckAvgMinOffset);
    if (tck < kSpdTckPsMin || tck > kSpdTckPsMax) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    std::uint32_t const mts = spd_mts_from_tck_ps(tck);
    if (mts == 0) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

    SpdDdr5Info info{};
    info.tck_ps    = tck;
    info.jedec_mts = mts;
    // Die Angebots-Erkennung ist optional: ein kurzer (aber gueltiger) Puffer ist KEIN Fehler, er
    // traegt eben keine Profil-Bloecke. Deshalb hier nur Laengen-gesicherte Blicke, kein Abbruch.
    if (bytes.size() > kSpdXmpMagicOffset + 1)
        info.xmp_present = bytes[kSpdXmpMagicOffset] == 0x0CU && bytes[kSpdXmpMagicOffset + 1] == 0x4AU;
    if (bytes.size() > kSpdExpoMagicOffset + 3)
        info.expo_present = bytes[kSpdExpoMagicOffset] == static_cast<std::uint8_t>('E') &&
                            bytes[kSpdExpoMagicOffset + 1] == static_cast<std::uint8_t>('X') &&
                            bytes[kSpdExpoMagicOffset + 2] == static_cast<std::uint8_t>('P') &&
                            bytes[kSpdExpoMagicOffset + 3] == static_cast<std::uint8_t>('O');
    return info;
}

/// Bequemlichkeits-Naht zum Melde-Typ: aus einem gelungenen Parse wird ein Reading der Stufe 2.
/// Das Angebot wandert NICHT in mts (A7) -- es gibt hier bewusst keinen Weg, aus einer Praesenz
/// eine Frequenz zu machen.
[[nodiscard]] constexpr RamFrequencyReading reading_from_spd(SpdDdr5Info const& info) noexcept {
    RamFrequencyReading r{};
    r.mts        = info.jedec_mts;
    r.tck_ps     = info.tck_ps;
    r.provenance = RamFrequencyProvenance::SpdJedecBase;
    r.state      = RamReadingState::Erhoben;
    return r;
}

// -- Wachen: die Umrechnung an belegten Stuetzstellen, compile-time ------------------------------
// 416 ps ist der real gemessene prod1-Wert; 357 ps das dort vorhandene Profil-Angebot. Beide Zahlen
// stammen aus dem Dump bzw. der Recherche, nicht aus einer Beispiel-Rechnung.
static_assert(spd_mts_from_tck_ps(416) == 4800U, "prod1-Ist: 416 ps sind die JEDEC-Stufe 4800 MT/s");
static_assert(spd_mts_from_tck_ps(357) == 5600U, "prod1-Profil-Angebot: 357 ps entsprechen 5600 MT/s");
static_assert(spd_mts_from_tck_ps(625) == 3200U, "untere gebraeuchliche DDR5-Stufe");
static_assert(spd_mts_from_tck_ps(0) == 0U, "Division durch Null wird nie versucht");
static_assert(kSpdMinBytesForRate == 22, "Byte 21 muss im Puffer liegen, sonst ist die Rate nicht ableitbar");

} // namespace comdare::cache_engine::measurement
