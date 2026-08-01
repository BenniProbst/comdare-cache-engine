#pragma once
// measurement/ram_probe_chain.hpp -- die RT-Erhebungs-Kette der RAM-Frequenz (Task #7, Paket P2).
//
// WAS HIER STEHT: die drei Erhebungs-Stufen als Chain of Responsibility (GoF) und die Traversierung.
// Was NICHT hier steht: die Wahl, WELCHE Kette gilt -- das ist die CT-Factory ueber die Achsen-Zelle
// (hardware_probe_factory.hpp). Diese Datei ist OS-NEUTRAL: sie liest ausschliesslich Pfade aus dem
// Kontext und kennt kein #ifdef. Die OS-Bindung sitzt in der Zelle (K4: "OS-Handles als CT-Typ-
// Bausteine"), nicht im Kettenglied -- deshalb laesst sich jede Stufe ohne die Zielplattform testen.
//
// -- WARUM EINE KETTE UND NICHT EIN IF-ELSE ------------------------------------------------------
// Die Quellen stehen in einer VERTRAUENS-Ordnung (Stufe 1 > 2 > 3, ram_frequency_reading.hpp), aber
// ihre Verfuegbarkeit steht erst zur Laufzeit fest: der Boot-Cache existiert nur mit Infra-Kontrakt
// (P6), der SPD-Knoten nur auf einem Kernel, der ihn anlegt, und in Docker gibt es beides nicht. Die
// Kette bricht deshalb bei der HOECHSTEN ERREICHBAREN Stufe ab, nicht beim ersten Wert -- und
// protokolliert, was sie unterwegs vorfand (Degradations-Pfad, Auflage A4).
//
// -- GoF-ETIKETT EHRLICH: CoR OHNE VTABLE --------------------------------------------------------
// Vorbild ist selection_filter_chain.hpp (cold-path-CoR mit Handler-Objekten). Der Unterschied ist
// bewusst: dort verkettet ein Zeiger (FilterHandler const* next_) gleichartige Glieder, hier sind die
// Glieder HETEROGENE Typen und die Verdrahtung ist eine Typliste (std::tuple). std::array ist dafuer
// strukturell unmoeglich -- es verlangt EINEN Elementtyp, und der gaebe es nur ueber eine gemeinsame
// Basis mit virtuellem probe(), also genau die vtable, die die CT-Doktrin hier nicht will. Die
// REIHENFOLGE (das Wesentliche an einer CoR) steht damit im TYP und nicht in einer Laufzeit-Liste:
// wer die Stufen vertauscht, aendert einen Typ, keinen Wert.
//
// -- DIE PROVENIENZ BINDET DIE KETTE, NICHT DER HANDLER ------------------------------------------
// Die liefernde Stufe bestimmt die Provenienz des Ergebnisses (run_ram_probe_chain setzt sie aus
// H::stage()). Ein Handler kann damit strukturell KEINE fremde Herkunft behaupten -- er koennte sonst
// einen SPD-Nennwert als configured_measured ausweisen, und genau diese Luege soll das ganze Paket
// verhindern. Dass die Handler das Feld zusaetzlich selbst setzen, ist Redundanz mit Wache, kein
// zweiter Kanal.
//
// -- KEIN IO IM BAU-GRAPHEN, KEIN HOT-PATH -------------------------------------------------------
// Die Kette liest Dateien. Sie laeuft deshalb ausschliesslich an der CEB-Freigabe-Naht (memoisiert,
// hardware_probe_factory.hpp), NIE in einer Statik-Initialisierung, NIE im gemessenen Pfad und NIE
// waehrend des Bauens. Alle Dateisystem-Zugriffe nutzen die error_code-Ueberladungen: eine Erhebung,
// die den Prozess mit einer Exception beendet, waere schlimmer als eine, die nichts findet.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/machine_identity.hpp>
#include <cache_engine/measurement/ram_frequency_reading.hpp>
#include <cache_engine/measurement/spd_ddr5_parser.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. DER KONTEXT (Pfad-Injektion) -- die einzige Verbindung der Kette zur Aussenwelt
// =================================================================================================

/// Alles, was die Stufen brauchen, an EINER Stelle. Die Pfade sind Daten, keine Konstanten im Code:
/// nur so laesst sich jede Stufe gegen einen Testbaum fahren, statt gegen die Live-Hardware der
/// Maschine, auf der die CI zufaellig laeuft (Auflage A9 -- "CI testet decline-Pfade + eingespielte
/// Puffer, nie Live-Hardware-Werte").
struct RamProbeContext {
    /// Stufe 1: die gefilterte Boot-Cache-Datei (Format-Kontrakt siehe unten).
    std::string_view boot_cache_path{};
    /// Stufe 1: Quelle der LAUFENDEN boot_id fuer die Frische-Wache.
    std::string_view boot_id_path{};
    /// Stufe 2: Wurzel, unter der nach `*/eeprom`-Knoten gesucht wird.
    std::string_view spd_device_root{};
    /// Stufe 3: der Eigenschafts-SCHLUESSEL der Anwender-XML. Er wird NIE umgeschrieben, normalisiert
    /// oder erraten -- er geht genau so in resolve_machine_by_properties, wie er hereinkam.
    std::string_view cpu_fabrication_key{};
    std::string_view ram_pair_key{};
    /// Test-Uhr. nullopt = echte Uhr (std::time). Ein Erhebungs-Zeitpunkt, den der Test nicht
    /// festhalten kann, macht jede Erwartung an ihn unpruefbar.
    std::optional<std::uint64_t> injected_now_unix_s{};
};

/// Der Zeitstempel der Erhebung. Ausgelagert, damit die Injektion an EINER Stelle greift.
[[nodiscard]] inline std::uint64_t probe_now_unix_s(RamProbeContext const& ctx) noexcept {
    if (ctx.injected_now_unix_s.has_value()) return *ctx.injected_now_unix_s;
    std::time_t const t = std::time(nullptr);
    return t < 0 ? 0U : static_cast<std::uint64_t>(t);
}

// =================================================================================================
// 2. DER BOOT-CACHE-KONTRAKT (Stufe 1) -- Format und Sicherheits-Auflage
// =================================================================================================
// Der konfigurierte Ist-Takt steht ausschliesslich in SMBIOS Type 17, und der ist gemessen root-only
// (/sys/firmware/dmi/tables/DMI = 0400). Es gibt dafuer keinen unprivilegierten Weg -- die einzige
// Loesung ist ein privilegierter Schreiber beim Boot und ein unprivilegierter Leser danach. Dieser
// Header beschreibt die LESER-Seite; den Schreiber liefert P6 per Infra-Kontrakt.
//
// SICHERHEITS-AUFLAGE A5 (der Grund, warum das Format GEFILTERT ist): ein roher `dmidecode --type 17`
// -Dump enthaelt DIMM-Seriennummern und Part-Numbers. Ihn world-readable abzulegen wuerde exakt das
// exponieren, wegen dessen der Kernel die DMI-Tabellen ueberhaupt gesperrt hat -- die Sperre waere
// umgangen statt respektiert. Der Kontrakt nennt deshalb NUR die vier benoetigten Felder; alles
// andere gehoert nicht in die Datei.
//
// FORMAT (Zeilen `schluessel=wert`, '#' leitet eine Kommentarzeile ein, Reihenfolge unerheblich):
//     # comdare hw dmi ram cache v1        <- Pflicht-Marker in der ERSTEN nicht-leeren Zeile
//     boot_id=<uuid>                       <- aus /proc/sys/kernel/random/boot_id
//     configured_speed_mts=<uint>          <- SMBIOS T17 "Configured Memory Speed"
//     speed_mts=<uint>                     <- SMBIOS T17 "Speed" (Nennwert; nur Beleg, nie der Wert)
//     memory_type=<DDR4|DDR5|...>          <- SMBIOS T17 "Type"
//     slots_populated=<uint>               <- Zahl bestueckter Slots
//
// FRISCHE-WACHE: die boot_id wechselt bei jedem Neustart. Stimmt die in der Datei nicht mit der
// laufenden ueberein, beschreibt die Datei einen FRUEHEREN Boot -- moeglicherweise mit anderem
// BIOS-Profil. Sie faellt dann durch, statt einen alten Wert als aktuellen auszugeben.

/// Der vorgeschlagene Ablage-Ort. /run ist ein tmpfs: der Inhalt verschwindet beim Neustart von
/// selbst, was zur Frische-Wache passt (eine stale Datei kann es dort gar nicht erst geben, solange
/// niemand sie kuenstlich konserviert -- die Wache deckt genau diesen Rest ab).
inline constexpr std::string_view kDefaultBootCachePath = "/run/comdare/hw/dmi_ram.cache";
/// Die laufende boot_id. Unprivilegiert lesbar (0444) auf jedem Linux mit procfs.
inline constexpr std::string_view kDefaultBootIdPath = "/proc/sys/kernel/random/boot_id";
/// Pflicht-Marker der ersten nicht-leeren Zeile. Fehlt er, ist die Datei nicht unsere (FormatUnbekannt).
inline constexpr std::string_view kBootCacheMarker = "# comdare hw dmi ram cache v1";
/// Die Schluessel des Kontrakts.
inline constexpr std::string_view kBootCacheKeyBootId          = "boot_id";
inline constexpr std::string_view kBootCacheKeyConfiguredSpeed = "configured_speed_mts";
inline constexpr std::string_view kBootCacheKeyMemoryType      = "memory_type";

/// Der unprivilegierte SPD-Baum. Auf prod1 gemessen: 1-0051/eeprom und 1-0053/eeprom, beide 0444.
inline constexpr std::string_view kDefaultSpdDeviceRoot = "/sys/bus/i2c/devices";
/// Der Dateiname des EEPROM-Knotens unter einem i2c-Geraet.
inline constexpr std::string_view kSpdEepromLeafName = "eeprom";

/// Plausibilitaets-Fenster fuer eine Datenrate aus dem Boot-Cache. Analog zum SPD-Fenster bewusst
/// weit: es soll Muell abweisen (0, eine halb geschriebene Zeile), nicht kuenftige Speed-Bins.
inline constexpr std::uint32_t kBootCacheMtsMin = 800;
inline constexpr std::uint32_t kBootCacheMtsMax = 20000;

// =================================================================================================
// 3. DIE STUFEN-SCHNITTSTELLE
// =================================================================================================

/// Ergebnis EINER Stufe: ein Reading ODER eine Erhebungs-Fehlerklasse -- nie ein halbes Ergebnis.
/// Derselbe Schnitt wie beim SPD-Parser (P1), damit die Klassen ungebrochen durchreisen.
using ProbeStageResult = std::expected<RamFrequencyReading, HardwareProbeErrorClass>;

/// Ein Kettenglied. stage() ist STATISCH: die Stufe eines Handlers ist eine Eigenschaft seines TYPS,
/// keine Laufzeit-Angabe -- sonst koennte ein Glied seine Position in der Vertrauens-Ordnung
/// waehrend des Laufs wechseln.
template <class H>
concept RamProbeStageHandler = requires(H const& h, RamProbeContext const& ctx) {
    { H::stage() } -> std::same_as<RamFrequencyProvenance>;
    { h.probe(ctx) } -> std::same_as<ProbeStageResult>;
};

// =================================================================================================
// 4. STUFE 1 -- BootCacheDmiProbe (configured_measured)
// =================================================================================================

namespace detail {

/// Eine Datei vollstaendig einlesen. Getrennte Rueckgabe fuer "gibt es nicht" und "gibt es, geht
/// nicht" -- das ist die Unterscheidung, die A4 traegt, und sie geht verloren, sobald man beides auf
/// einen leeren String abbildet.
[[nodiscard]] inline std::expected<std::string, HardwareProbeErrorClass>
read_whole_file(std::filesystem::path const& p) {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec) || ec) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);
    std::ifstream f(p, std::ios::binary);
    if (!f) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    std::string const s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    // Ein Lesefehler MITTEN im Strom ist etwas anderes als eine leere Datei: die Quelle war da und hat
    // sich dann verweigert. Ohne diese Pruefung stuende ein abgebrochener Lesevorgang als "leer" da.
    if (f.bad()) return std::unexpected(HardwareProbeErrorClass::QuelleUnlesbar);
    return s;
}

/// Rand-Leerraum abschneiden (auch das '\r' einer CRLF-Datei -- der Boot-Cache koennte von einem
/// anderen Werkzeug geschrieben werden als dem, das ihn liest).
[[nodiscard]] inline std::string_view trimmed(std::string_view s) noexcept {
    auto const is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && is_space(s.front())) s.remove_prefix(1);
    while (!s.empty() && is_space(s.back())) s.remove_suffix(1);
    return s;
}

/// Den Wert eines Schluessels aus einem `key=value`-Text ziehen. nullopt = Schluessel nicht vorhanden.
/// Bewusst KEIN Teiltreffer: "speed_mts" darf nicht auf "configured_speed_mts" anspringen, sonst
/// liefe der Nennwert als Ist-Takt durch -- das ist die teuerste Verwechslung dieses ganzen Pakets.
[[nodiscard]] inline std::optional<std::string_view> key_value_lookup(std::string_view text,
                                                                      std::string_view key) noexcept {
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t const      nl   = text.find('\n', pos);
        std::string_view const line = trimmed(text.substr(pos, nl == std::string_view::npos ? nl : nl - pos));
        if (!line.empty() && line.front() != '#') {
            std::size_t const eq = line.find('=');
            if (eq != std::string_view::npos && trimmed(line.substr(0, eq)) == key) return trimmed(line.substr(eq + 1));
        }
        if (nl == std::string_view::npos) break;
        pos = nl + 1;
    }
    return std::nullopt;
}

/// Die erste nicht-leere Zeile -- der Ort des Pflicht-Markers.
[[nodiscard]] inline std::string_view first_content_line(std::string_view text) noexcept {
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t const      nl   = text.find('\n', pos);
        std::string_view const line = trimmed(text.substr(pos, nl == std::string_view::npos ? nl : nl - pos));
        if (!line.empty()) return line;
        if (nl == std::string_view::npos) break;
        pos = nl + 1;
    }
    return {};
}

/// Vorzeichenlose Dezimalzahl. nullopt bei allem anderen -- insbesondere bei einer LEEREN Angabe und
/// bei Nachschrott ("4800 MT/s"), denn beides deutet auf eine halb geschriebene Zeile hin.
[[nodiscard]] inline std::optional<std::uint32_t> parse_u32(std::string_view s) noexcept {
    if (s.empty()) return std::nullopt;
    std::uint64_t v = 0;
    for (char const c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        v = v * 10U + static_cast<std::uint64_t>(c - '0');
        if (v > 0xFFFFFFFFULL) return std::nullopt;
    }
    return static_cast<std::uint32_t>(v);
}

} // namespace detail

/// Stufe 1: der konfigurierte Ist-Takt aus der gefilterten Boot-Cache-Datei.
///
/// HEUTIGER ZUSTAND: die Datei existiert nirgends -- der privilegierte Schreiber ist P6 (Infra-
/// Kontrakt). Diese Stufe faellt deshalb heute IMMER mit QuelleFehlt durch, und das ist kein Mangel,
/// sondern die ehrliche Auskunft: der Ist-Takt ist ohne Infra-Kanal strukturell unbeweisbar. Die
/// Stufe jetzt zu bauen kostet nichts und macht den Tag, an dem der Schreiber kommt, zu einem
/// Konfigurations- statt einem Code-Ereignis.
struct BootCacheDmiProbe {
    [[nodiscard]] static constexpr RamFrequencyProvenance stage() noexcept {
        return RamFrequencyProvenance::ConfiguredMeasured;
    }

    [[nodiscard]] ProbeStageResult probe(RamProbeContext const& ctx) const {
        if (ctx.boot_cache_path.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);

        auto const text = detail::read_whole_file(std::filesystem::path{ctx.boot_cache_path});
        if (!text.has_value()) return std::unexpected(text.error());

        // (a) Ist es ueberhaupt unsere Datei? Ein fremdes Format wird NICHT interpretiert.
        if (detail::first_content_line(*text) != kBootCacheMarker)
            return std::unexpected(HardwareProbeErrorClass::FormatUnbekannt);

        // (b) FRISCHE. Eine Datei ohne boot_id ist unvollstaendig, eine mit fremder boot_id beschreibt
        //     einen anderen Boot. Beides ist KORRUPT im Sinne von A4 -- vorhanden, lesbar, aber fuer
        //     diesen Lauf unbrauchbar -- und faellt sichtbar durch statt still einen alten Takt
        //     auszugeben. Ist die laufende boot_id nicht lesbar, laesst sich die Frische nicht
        //     beweisen; auch dann wird NICHT geglaubt.
        auto const datei_boot_id = detail::key_value_lookup(*text, kBootCacheKeyBootId);
        if (!datei_boot_id.has_value()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        auto const live_boot_id = detail::read_whole_file(std::filesystem::path{ctx.boot_id_path});
        if (!live_boot_id.has_value()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        if (*datei_boot_id != detail::trimmed(*live_boot_id))
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

        // (c) Der Wert. Gelesen wird AUSSCHLIESSLICH configured_speed_mts -- der Nennwert `speed_mts`
        //     steht im Kontrakt als Beleg, hat aber bewusst keinen Weg in das Ergebnis.
        auto const roh = detail::key_value_lookup(*text, kBootCacheKeyConfiguredSpeed);
        if (!roh.has_value()) return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);
        auto const mts = detail::parse_u32(*roh);
        if (!mts.has_value() || *mts < kBootCacheMtsMin || *mts > kBootCacheMtsMax)
            return std::unexpected(HardwareProbeErrorClass::QuelleKorrupt);

        RamFrequencyReading r{};
        r.mts                 = *mts;
        r.provenance          = stage();
        r.state               = RamReadingState::Erhoben;
        r.collected_at_unix_s = probe_now_unix_s(ctx);
        return r;
    }
};

// =================================================================================================
// 5. STUFE 2 -- SpdEepromProbe (spd_jedec_base)
// =================================================================================================

/// Stufe 2: die JEDEC-Nennrate aus dem SPD-EEPROM. Unprivilegiert, heute lieferfaehig.
///
/// WAS SIE LIEFERT UND WAS NICHT: die Nennrate der RIEGEL, nie den konfigurierten Takt. Ein
/// XMP/EXPO-Angebot wird als PRAESENZ erkannt (P1-Parser) und wandert bewusst nicht in die Frequenz.
///
/// MEHRERE KNOTEN: prod1 traegt zwei bestueckte Slots (1-0051, 1-0053, gemessen bitgleich). Die
/// Suche ist SORTIERT, weil directory_iterator keine Reihenfolge zusichert -- ohne Sortierung haenge
/// das Ergebnis bei heterogener Bestueckung von der Laune des Dateisystems ab. Ein Knoten, der kein
/// DDR5-SPD ist (auf einem i2c-Bus haengt mehr als Speicher), disqualifiziert die Stufe nicht: die
/// Suche geht weiter. Erst wenn KEIN Knoten traegt, faellt die Stufe durch -- mit dem Grund des
/// letzten Versuchs, damit "da war etwas, es war nur kaputt" nicht als "da war nichts" erscheint.
struct SpdEepromProbe {
    [[nodiscard]] static constexpr RamFrequencyProvenance stage() noexcept {
        return RamFrequencyProvenance::SpdJedecBase;
    }

    [[nodiscard]] ProbeStageResult probe(RamProbeContext const& ctx) const {
        if (ctx.spd_device_root.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);

        std::error_code             ec;
        std::filesystem::path const root{ctx.spd_device_root};
        if (!std::filesystem::is_directory(root, ec) || ec)
            return std::unexpected(HardwareProbeErrorClass::QuelleFehlt); // Docker-Fall: leerer Baum

        std::vector<std::filesystem::path> knoten;
        for (auto it = std::filesystem::directory_iterator(root, ec);
             !ec && it != std::filesystem::directory_iterator(); it.increment(ec)) {
            std::filesystem::path kandidat = it->path() / std::filesystem::path{kSpdEepromLeafName};
            std::error_code       ec2;
            if (std::filesystem::exists(kandidat, ec2) && !ec2) knoten.push_back(std::move(kandidat));
        }
        std::ranges::sort(knoten);
        if (knoten.empty()) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);

        HardwareProbeErrorClass letzter = HardwareProbeErrorClass::QuelleFehlt;
        for (auto const& k : knoten) {
            auto const bytes = detail::read_whole_file(k);
            if (!bytes.has_value()) {
                letzter = bytes.error();
                continue;
            }
            auto const spd = parse_spd_ddr5(
                std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(bytes->data()), bytes->size()});
            if (!spd.has_value()) {
                letzter = spd.error(); // A4: der Grund geht NICHT verloren
                continue;
            }
            RamFrequencyReading r = reading_from_spd(*spd);
            r.collected_at_unix_s = probe_now_unix_s(ctx);
            return r;
        }
        return std::unexpected(letzter);
    }
};

// =================================================================================================
// 6. STUFE 3 -- DeclaredProbe (declared_not_measured, TERMINAL)
// =================================================================================================

/// Stufe 3: der deklarierte Wert der Anwender-XML, aufgeloest ueber das Eigenschafts-Tupel.
///
/// TERMINAL heisst NICHT "liefert immer": ist fuer das Tupel nichts deklariert (oder steht dort die
/// 0-Marke), faellt auch diese Stufe durch und die Kette endet ehrlich ohne Wert. Ein Terminal-Glied,
/// das notfalls etwas erfindet, waere der stille Fallback, den O-4 fuer die Maschinen-Identitaet
/// bereits ausgeschlossen hat.
///
/// DER SCHLUESSEL WIRD NIE UMGESCHRIEBEN: cpu_fabrication_key und ram_pair_key gehen unveraendert in
/// resolve_machine_by_properties. Kein Trimmen, kein Kleinschreiben, kein Praefix-Match -- §70.6
/// verlangt den EXAKTEN Match, und jede "Bequemlichkeit" hier machte aus einem Nicht-Treffer still
/// einen Treffer auf eine fremde Maschine.
struct DeclaredProbe {
    [[nodiscard]] static constexpr RamFrequencyProvenance stage() noexcept {
        return RamFrequencyProvenance::DeclaredNotMeasured;
    }

    [[nodiscard]] ProbeStageResult probe(RamProbeContext const& ctx) const {
        DeclaredMachine const* m = resolve_machine_by_properties(ctx.cpu_fabrication_key, ctx.ram_pair_key);
        if (m == nullptr) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);
        // Die 0 ist die NICHT-DEKLARIERT-Marke (machine_identity.hpp), kein Messwert. Sie darf deshalb
        // nie als erhobener Wert herausfallen -- has_frequency() wuerde sie ohnehin verwerfen, aber
        // dann stuende ein Erhoben-Zustand ohne Wert im Ergebnis.
        if (m->ram_frequency_mhz == 0U) return std::unexpected(HardwareProbeErrorClass::QuelleFehlt);

        RamFrequencyReading r{};
        r.mts                 = m->ram_frequency_mhz; // Einheiten-Naht: das Feld heisst _mhz, traegt MT/s
        r.provenance          = stage();
        r.state               = RamReadingState::Erhoben;
        r.collected_at_unix_s = probe_now_unix_s(ctx);
        return r;
    }
};

// =================================================================================================
// 7. DIE TRAVERSIERUNG (Verdrahtung CT als Typliste, Ablauf RT)
// =================================================================================================

/// Die kanonische Reihenfolge. Sie steht hier EINMAL; jede Zelle der Factory verweist darauf, statt
/// sie zu wiederholen -- sonst koennte eine Zelle die Vertrauens-Ordnung unbemerkt umdrehen.
using LinuxRamProbeChain = std::tuple<BootCacheDmiProbe, SpdEepromProbe, DeclaredProbe>;
/// Die Kette einer Zelle ohne eigene Erhebung (A8/K3): nur das Terminal-Glied. Kein leeres Tupel --
/// eine Zelle ohne JEDE Stufe koennte nicht einmal die Deklaration ausweisen.
using DeclaredOnlyRamProbeChain = std::tuple<DeclaredProbe>;

/// Fuehrt die Kette in TYP-Reihenfolge aus und liefert das Ergebnis der hoechsten erreichbaren Stufe.
///
/// Der Degradations-Pfad wird GETRENNT vom Ergebnis gefuehrt und erst am Ende angeheftet: das
/// liefernde Reading kommt frisch aus dem Handler und wuerde einen im Ergebnis mitgefuehrten Pfad
/// sonst ueberschreiben -- die Auskunft ueber die durchgefallenen Stufen ginge genau dann verloren,
/// wenn sie interessant ist (eine Stufe hat geliefert, aber welche fielen davor durch?).
///
/// Faellt ALLES durch, ist das Ergebnis das ehrlich leere Reading (NichtErhoben) MIT vollstaendigem
/// Pfad -- nie ein geratener Wert.
template <class... Handlers>
    requires(RamProbeStageHandler<Handlers> && ...)
[[nodiscard]] inline RamFrequencyReading run_ram_probe_chain(std::tuple<Handlers...> const& chain,
                                                             RamProbeContext const&         ctx) {
    RamProbeTrail       trail{};
    RamFrequencyReading ergebnis{};
    bool                fertig = false;

    auto stufe = [&](auto const& h) {
        if (fertig) return; // eine bessere Stufe hat geliefert -> Rest bleibt NichtGefragt
        using H                  = std::remove_cvref_t<decltype(h)>;
        ProbeStageResult const r = h.probe(ctx);
        if (r.has_value()) {
            ergebnis = *r;
            // Die Provenienz kommt aus der STUFE, nicht aus dem Handler-Ergebnis: so kann kein Glied
            // eine fremde Herkunft behaupten (siehe Kopf-Block).
            ergebnis.provenance = H::stage();
            record_stage_success(trail, H::stage());
            fertig = true;
        } else {
            record_stage_failure(trail, H::stage(), r.error());
        }
    };
    // Komma-Fold: die Auswertungs-Reihenfolge ist links nach rechts sequenziert, also exakt die
    // Typ-Reihenfolge der Kette. Das ist die Stelle, an der aus der Typliste ein Ablauf wird.
    std::apply([&](auto const&... hs) { (stufe(hs), ...); }, chain);

    ergebnis.trail = trail;
    return ergebnis;
}

// =================================================================================================
// 8. WACHEN (compile-time)
// =================================================================================================
static_assert(RamProbeStageHandler<BootCacheDmiProbe>);
static_assert(RamProbeStageHandler<SpdEepromProbe>);
static_assert(RamProbeStageHandler<DeclaredProbe>);
// Die Handler sind zustandslos -- ein Kettenglied mit Zustand waere zwischen zwei Erhebungen nicht
// mehr dasselbe Glied, und die Memoisierung an der Freigabe-Naht haette einen zweiten Gedaechtnisort.
static_assert(std::is_empty_v<BootCacheDmiProbe> && std::is_empty_v<SpdEepromProbe> && std::is_empty_v<DeclaredProbe>);
// Kein virtual: die CoR ist hier statisch verdrahtet (siehe Kopf -- GoF-Etikett ehrlich halten).
static_assert(!std::is_polymorphic_v<BootCacheDmiProbe> && !std::is_polymorphic_v<SpdEepromProbe> &&
              !std::is_polymorphic_v<DeclaredProbe>);
// DIE ORDNUNGS-WACHE: die Ketten-Reihenfolge MUSS die Vertrauens-Ordnung sein. Ohne sie koennte
// jemand SpdEeprom vor BootCacheDmi haengen -- die Kette lieferte dann weiterhin Werte, nur eben
// systematisch die schlechteren, und kein Test wuerde es merken.
static_assert(provenance_trust_rank(std::tuple_element_t<0, LinuxRamProbeChain>::stage()) <
                      provenance_trust_rank(std::tuple_element_t<1, LinuxRamProbeChain>::stage()) &&
                  provenance_trust_rank(std::tuple_element_t<1, LinuxRamProbeChain>::stage()) <
                      provenance_trust_rank(std::tuple_element_t<2, LinuxRamProbeChain>::stage()),
              "Die Kette MUSS nach fallendem Vertrauen geordnet sein: Boot-Cache vor SPD vor "
              "Deklaration. Eine vertauschte Kette liefert weiterhin Werte -- nur die schlechteren.");
// Die Kette deckt JEDE Provenienz-Stufe genau einmal ab. Faellt eine Stufe weg, ist sie im
// Degradations-Pfad dauerhaft NichtGefragt und niemandem faellt auf, dass sie nie gefragt wurde.
static_assert(std::tuple_size_v<LinuxRamProbeChain> == kRamFrequencyProvenanceCount,
              "Die Linux-Kette MUSS jede Provenienz-Stufe tragen.");
// Das Terminal-Glied steht am Ende -- und die declared-only-Kette besteht genau aus ihm.
static_assert(
    std::is_same_v<std::tuple_element_t<kRamFrequencyProvenanceCount - 1, LinuxRamProbeChain>, DeclaredProbe>);
static_assert(std::is_same_v<std::tuple_element_t<0, DeclaredOnlyRamProbeChain>, DeclaredProbe>);
// Die Redundanz zwischen Handler-Ergebnis und Ketten-Bindung ist konsistent: reading_from_spd (P1)
// setzt dieselbe Stufe, die SpdEepromProbe deklariert. Faellt das auseinander, ueberschreibt die
// Kette den Handler stillschweigend -- richtig im Ergebnis, aber ein Zeichen fuer einen Denkfehler.
static_assert(reading_from_spd(SpdDdr5Info{}).provenance == SpdEepromProbe::stage());
// Der Marker ist eine KOMMENTAR-Zeile: der Parser ueberliest sie beim Schluessel-Lesen, und genau
// deshalb darf sie nicht versehentlich wie ein Schluessel aussehen.
static_assert(!kBootCacheMarker.empty() && kBootCacheMarker.front() == '#');
static_assert(kBootCacheMarker.find('=') == std::string_view::npos,
              "Der Marker darf kein '=' tragen -- sonst liest ihn key_value_lookup als Schluessel.");
// Die A5-Auflage als Wache: der Kontrakt nennt AUSSCHLIESSLICH Nicht-Identifizierendes. Kein
// Schluessel dieses Headers fragt nach Seriennummer, Part-Number oder Hersteller.
static_assert(kBootCacheKeyBootId != kBootCacheKeyConfiguredSpeed &&
              kBootCacheKeyConfiguredSpeed != kBootCacheKeyMemoryType);
// Die Verwechslung, die das Paket teuer machen wuerde: der Nennwert-Schluessel darf kein Praefix des
// Ist-Takt-Schluessels sein und umgekehrt (key_value_lookup vergleicht exakt -- diese Zeile haelt
// fest, dass auch ein spaeterer Praefix-Match nicht danebengreifen koennte).
static_assert(kBootCacheKeyConfiguredSpeed != std::string_view{"speed_mts"} &&
              kBootCacheKeyConfiguredSpeed.ends_with("speed_mts"));

} // namespace comdare::cache_engine::measurement
