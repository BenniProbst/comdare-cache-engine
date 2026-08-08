#pragma once
// builder/lager_ablage/ergebnis_mappe.hpp -- A9-S3 (xlsx-Writer F3): Abstract-Factory-Interfaces +
// CSV-Fallback. Design-Dossier: docs/architecture/20260803-a9_xlsx_writer_f3_soll_design.md, Abschnitt
// 4.2 (Schnittstellen) + Abschnitt 4.2 Punkt 5 (CSV-Fallback = dieselbe Factory).
//
// OWNER-KERN 26.07. Section 6 (woertlich): "xlsx = kuenftig DEFAULT, CSV einstellbar + Fallback. CSVs
// werden im FACTORY PATTERN je Sheet einzeln gebaut; xlsx = EINE Datei mit EINEM Sheet je gewaehlter
// Unter-Achsen-Permutation + zusaetzlichem INFO-Sheet." Owner-Kanon 05.08.: "entweder CSV xor xlsx,
// xlsx ist default ... das ist ein strategy pattern, keine chain of responsibility" -- genau EINE
// aktive Strategie je Lauf, kein Doppelschreiben.
//
// BAUFORM (Dossier 4.1): geteilt. Interfaces + Factory + CSV-Fallback sind HEADER-ONLY (dieser Header).
// Das xlsx-Backend ist eine EIGENE TU (xlsx_ergebnis_writer.cpp, Target comdare_lager_ablage), die
// comdare_vendored_xlsxwriter PRIVATE linkt -- <xlsxwriter.h> darf NICHT in diese oeffentliche
// Schnittstelle lecken. Der Bruch zwischen header-only und TU sitzt an GENAU EINER Stelle: die freie
// Funktion detail::erzeuge_xlsx_ergebnis_mappe() ist hier nur DEKLARIERT; ihre Definition liegt in der
// TU. ErgebnisMappenFactory::oeffne() bleibt trotzdem inline im Header -- sie dispatcht nur.
//
// QUELLE BLEIBT CSV (Dossier 4.2 Verhaltensvertrag 1): dieser Writer definiert KEINE eigene
// Spaltenmenge. kopf()/zeile() nehmen bereits gesplittete Spalten/Felder entgegen (aus
// lazy_csv_header()/format_csv_row(), cache_engine_builder_iterator.hpp:470/:623) -- Schema-Hoheit
// bleibt dort. Zell-Token-Treue (Verhaltensvertrag 2): "failed"/"gesperrt"/"nicht_gebaut"/"n/a" werden
// woertlich durchgereicht, NIE uminterpretiert (axis_error.hpp Token-Vokabular, dort per static_assert
// zementiert).
//
// NAMENS-GRAMMATIK: der Dateiname-Stamm (ohne Endung) kommt vom Aufrufer bereits FERTIG aus
// bestandslog::blatt_dateiname() (lager_pfad_grammatik.hpp:391, Ownership-Entscheid L5) -- dieser
// Header baut KEINE eigene Dateinamens-Grammatik und forkt lager_pfad_grammatik.hpp nicht.
//
// DOKTRIN: C++23, header-only, statischer Dispatch fuer die Grammatik-Teile; die Format-Wahl
// (xlsx|csv) ist der EINE bewusste Laufzeit-Draht (Haupt=CT/Unter=RT-Regel, Rueckschrieb-Methoden sind
// eine Mess-UNTER-Achse) -- GoF Abstract Factory, KEIN std::variant (Verbotskanon). ASCII-only.
//
// FASSUNG 3 (Owner-Entscheid 08.08., additiv, s. Abschnitt 2B unten): eine ZWEITE Blattsorte in
// DERSELBEN Mappe fuer die compare/Macro/Micro-Ebenen (Aufrufer=Prozess+Thread, IN/OUT-Checkpoint-
// Zeilen, interne Hyperlinks zum Hinabsteigen). Gleiche Factory, gleiche Sheet-Namens-Wache, gleiches
// INFO-Sheet -- kein zweiter Writer, kein Umbau der Fassung-1/2-Schnittstellen.

#include "bestandslog/lager_pfad_grammatik.hpp"

#include <cache_engine/measurement/axis_error.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::lager_ablage {

namespace ms = ::comdare::cache_engine::measurement;

// =================================================================================================
// 1. FORMAT-WAHL (Strategy, kein std::variant)
// =================================================================================================

/// Die EINE Laufzeit-Weiche der Factory: xlsx ist DEFAULT (Owner-KERN 26.07.), csv der Fallback
/// DERSELBEN Factory. Genau eine aktive Strategie je Lauf (Owner-Kanon 05.08.), kein Doppelschreiben.
enum class ErgebnisFormat : std::uint8_t { xlsx, csv };

// =================================================================================================
// 2. DIE DREI UNTER-EBENEN GEGEN SHEET-EXPLOSION (Dossier Abschnitt 4.2, SheetSchluessel)
// =================================================================================================

/// EINE gewaehlte Unter-Achsen-Permutation. Reihenfolge = BINDENDE Sortierung Mess-Unter ->
/// System-Unter -> Organ-Unter (deckungsgleich mit lager_baum_writer.hpp Ebenen 9-11, dort dieselbe
/// Reihenfolge fuer den Baum-Pfad). Ein leeres Feld heisst "diese Unter-Ebene ist hier nicht noetig" --
/// dieselbe "nur soweit noetig"-Regel wie im Baum. Die REIHENFOLGE, in der ein Aufrufer verschiedene
/// SheetSchluessel bei blatt() vorlegt, bestimmt die Sheet-Nummerierung (S001, S002, ...) -- diese
/// Klasse selbst erzeugt KEINE mixed-radix-Permutation ueber alle moeglichen Werte (das ist die
/// CoR-Filterkette der Auswertung, A9-S4); sie ist nur der SCHLUESSEL, unter dem ein Sheet gefunden
/// bzw. neu angelegt wird.
struct SheetSchluessel {
    std::string mess_unter;
    std::string system_unter;
    std::string organ_unter;

    [[nodiscard]] bool operator==(SheetSchluessel const&) const = default;
};

/// Ein (achse, wert)-Meta-Eintrag fuer das INFO-Blatt -- dieselbe Grammatik-Idee wie KvPaar
/// (lager_pfad_grammatik.hpp), hier aber fuer die MENSCHLICHE Auswerte-Ausgabe statt fuer einen
/// Dateipfad, deshalb ein eigener, einfacherer Typ (owning std::string statt view: das INFO-Blatt wird
/// erst bei schliessen() gerendert, lange nachdem der Aufrufer seine temporaeren Werte freigegeben hat).
struct AchsenEintrag {
    std::string achse;
    std::string wert;
};

/// Die verwendeten HAUPT-Achsen aller drei Typen (Mess/System/Organ) -- Dossier 4.2 Punkt 7. Eine flache
/// Liste genuegt: Haupt-Achsen-Namen sind ueber die drei Typen disjunkt (kein "search_algo" im Mess- oder
/// System-Realm, kein "target_isa" im Organ-Realm), der Typ ist damit aus dem Namen selbst ablesbar.
using HauptAchsenBelegung = std::vector<AchsenEintrag>;

/// Die konstanten (weggelassenen) Unter-Achsen -- Dateinamens-Doktrin (KERN 26.07. Section 6): nie sich
/// aendernde Variablen werden aus dem Dateinamen weggelassen und stattdessen hier als Meta-Eintrag
/// gefuehrt, damit ihr Wert nicht stillschweigend verschwindet.
using KonstantenMeta = std::vector<AchsenEintrag>;

/// Sysinfo der MESSENDEN Maschine fuers INFO-Blatt (Dossier 4.2 Punkt 7). "n/a statt Null"-Doktrin: ein
/// leeres Feld bedeutet "nicht ermittelt", niemals eine erfundene Angabe. Bewusst PLAIN DATA (keine
/// Abhaengigkeit auf machine_identity.hpp/operating_system_probe.hpp in dieser Schnittstelle) -- der
/// Aufrufer befuellt sie aus der bestehenden Erhebungs-Kette (live_hostname(), identify_machine(),
/// OperatingSystemProbe<...>::collect()); dieser Header bleibt frei von der Wahl DER Erhebungs-Strecke.
struct MaschinenSysinfo {
    std::string hostname;         ///< live_hostname() -- NUR Lookup-Hinweis, NIE Teil der Identitaet
    std::string machine_id;       ///< DeclaredMachine::machine_id (leer, wenn UnbekannteMaschine)
    std::string cpu_fabrication;  ///< deklarierte cpu_fabrication (leer, wenn unbekannt)
    std::string ram_pair;         ///< deklariertes ram_pair (leer, wenn unbekannt)
    std::string os_version;       ///< OperatingSystemInstance::os_version
    std::string kernel;           ///< OperatingSystemInstance::kernel
    std::string build;            ///< OperatingSystemInstance::build
    std::string identity_verdict; ///< machine_identity_verdict_label(...) (match/unbekannte_maschine/...)
};

/// "n/a statt Null"-Renderer fuer leere Meta-Felder (INFO-Blatt-Zellen). Geteilt zwischen CSV- und
/// xlsx-Backend, damit beide dieselbe Ehrlichkeits-Konvention rendern.
[[nodiscard]] inline std::string_view n_a_wenn_leer(std::string_view s) noexcept {
    return s.empty() ? std::string_view{"n/a"} : s;
}

// =================================================================================================
// 2B. FASSUNG 3 (Owner-Entscheid 08.08., additiv): compare/Macro/Micro-Blattsorte
// =================================================================================================
// ZWEITE Blattsorte in DERSELBEN Mappe -- gleiche Factory, gleiche Sheet-Namens-Wache, gleiches
// INFO-Sheet (Owner woertlich: "Wenn du die Mappe schon zusammen hast, ist das ein Zusatz, kein
// Umbau"). Drei Ebenen mit UNTERSCHIEDLICHER Sheet-Zaehlung:
//   compare -- 1 Sheet je Last-Messungs-Rekombination (Owner-Tabelle) bzw. GENAU 1 im einfachen Fall
//              (Owner-Formel "Blattzahl = 1 + |Funktionen| + |Achsen|"). Diese Fassung LOEST die
//              Spannung zwischen beiden Owner-Aussagen NICHT durch eine Wahl, sondern durch
//              Allgemeinheit: die Zahl der compare-Sheets folgt der Zahl der verschiedenen
//              Rekombinations-Bezeichner, die der Aufrufer tatsaechlich vorlegt (>=1) -- reduziert
//              sich im einfachen Fall automatisch auf 1 (deckt die Formel), erlaubt aber mehrere,
//              falls eine Mappe mehrere Rekombinationen vergleicht (deckt die Tabelle).
//   Macro   -- 1 Sheet je FUNKTION (Interface-Funktion der Tier-Binary, am Pruefdock aufgerufen).
//   Micro   -- 1 Sheet je ACHSE (der 18 Organ-Achsen, kCompositionAxisNames).
// Blattzahl waechst NICHT mit der Lauflaenge (Owner-Doktrin woertlich): AUFRUFE werden zu ZEILEN im
// EINEN Sheet ihrer Funktion/Achse, nie zu neuen Sheets.
//
// AUFRUFER = (PROZESS, THREAD) -- NACHTRAG woertlich: "Der Aufrufer ist also nicht uniform, sondern
// ein Prozess UND einer von dessen Threads." Ein Thread ist systemisch ein GoF-Visitor ueber die
// Mess-Ebenen (bewegt sich von hoeher nach tiefer und auf dem Stack sauber zurueck). JE AUFRUF ZWEI
// ZEILEN -- Checkpoint::In (Interface-enter) und Checkpoint::Out (return) -- weil eine Zeile je
// ABGESCHLOSSENEM Aufruf einen nie zurueckkehrenden Aufruf gar nicht schreiben wuerde: genau die
// Regression (Invariante "wenn ein Thread ein Interface betritt, aber es nicht wieder verlaesst"),
// die erkannt werden soll, bliebe unsichtbar. Mehrere Threads schreiben EINGEDAMPFT ins selbe Blatt
// in ANKUNFTSFOLGE (kein Blatt je Thread) -- Thread-Kennung UND Zeitpunkt stehen deshalb in JEDER
// Zeile, nicht nur in der ersten (bei Nebenlaeufigkeit "ergibt sich das NICHT aus der vorigen Zeile").
//
// SCOPE-GRENZE (bewusst, s. Bericht an den Auftraggeber): dieser Writer PRUEFT die IN/OUT-Balance
// NICHT. "Der Writer schreibt, er bewertet nicht" -- die Balance-Invariante ist eine AUSWERTUNG ueber
// das FERTIGE Blatt (Lese-Zeit), keine Schreib-Zeit-Pflicht dieser Klasse. Sie ist ein eigenes Paket.
//
// BEFUND (Recherche vor dem Bau, s. Bericht): Prozess+Thread-Identitaet erreicht HEUTE an KEINER
// Stelle der bestehenden Mess-Pipeline (abi_adapter.hpp run_workload*, IPruefDock::measure,
// perm_runner.hpp) einen Punkt, an dem dieser Writer sie abgreifen koennte -- der golden-Messpfad ist
// nachweislich 1-Thread (run_methodology_registry.hpp), und selbst der Debug-Parallel-Pool taggt
// keine Zeile mit Thread-/Prozess-Identitaet. Prozess/Thread/Aufrufer/Zeitpunkt sind deshalb reine
// PARAMETER, die der AUFRUFER dieses Writers liefern MUSS -- dieser Header erfindet KEINE Quelle
// dafuer und keinen Platzhalter-Wert.
//
// INTERNE HYPERLINKS (compare -> Funktion -> Achse): der vendorierte libxlsxwriter-Stand (Tag v1.2.4)
// unterstuetzt `worksheet_write_url(ws, row, col, "internal:'Blatt'!Zelle", NULL)` unveraendert
// (ext/io/libxlsxwriter/include/xlsxwriter/worksheet.h:2836-2877, src/worksheet.c:3393/:8420, keine
// #ifdef-Ausblendung, im CMakeLists-Glob enthalten) -- zeile_mit_verweis() unten macht das nutzbar.

/// Die drei Fassung-3-Ebenen.
enum class MessEbene : std::uint8_t { Compare, Macro, Micro };

[[nodiscard]] constexpr std::string_view mess_ebene_label(MessEbene e) noexcept {
    switch (e) {
        case MessEbene::Compare: return "compare";
        case MessEbene::Macro: return "macro";
        case MessEbene::Micro: return "micro";
    }
    return "mess_ebene_unbekannt";
}

/// IN (Interface-enter) oder OUT (return) -- je Aufruf ZWEI Zeilen, s. Kopf.
enum class Checkpoint : std::uint8_t { In, Out };

[[nodiscard]] constexpr std::string_view checkpoint_label(Checkpoint c) noexcept {
    switch (c) {
        case Checkpoint::In: return "in";
        case Checkpoint::Out: return "out";
    }
    return "checkpoint_unbekannt";
}

/// Schluessel EINES compare/Macro/Micro-Sheets. bezeichner ist je Ebene: Compare = Rekombinations-
/// Bezeichner, Macro = Funktionsname, Micro = Achsenname. Reihenfolge der Vorlage bei
/// IErgebnisMappe::mess_ebene_blatt() bestimmt wie bei SheetSchluessel die Anlage-Reihenfolge.
struct MessEbenenSchluessel {
    MessEbene   ebene;
    std::string bezeichner;

    [[nodiscard]] bool operator==(MessEbenenSchluessel const&) const = default;
};

// =================================================================================================
// 3. ErgebnisSchreibFehler -- honest-Fehler statt stillem Truncate (axis_error.hpp Domaene LagerAblage)
// =================================================================================================

/// Wird geworfen, wenn die Ablage NICHT still truncaten/kuerzen/ueberschreiben darf: Zeilenlimit
/// erreicht, Sheet-/Datei-Name verletzt die Wache, oder das finale Schreiben/Umbenennen scheitert. Die
/// Fehlerklasse ist die axis_error.hpp-Taxonomie (measurement::LagerAblageFehlerKlasse) -- KEIN
/// Insel-Enum (Dossier 4.2 Punkt 4).
class ErgebnisSchreibFehler final : public std::exception {
public:
    ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse klasse, std::string kontext)
        : klasse_(klasse), kontext_(std::move(kontext)),
          nachricht_(std::string{ms::lager_ablage_fehler_label(klasse_)} + ": " + kontext_) {}

    [[nodiscard]] ms::LagerAblageFehlerKlasse klasse() const noexcept { return klasse_; }
    [[nodiscard]] std::string const&          kontext() const noexcept { return kontext_; }
    [[nodiscard]] char const*                 what() const noexcept override { return nachricht_.c_str(); }

private:
    ms::LagerAblageFehlerKlasse klasse_;
    std::string                 kontext_;
    std::string                 nachricht_;
};

// =================================================================================================
// 4. DIE SHEET-NAMENS-WACHE (Dossier 4.2 Punkt 3, V-A9-6 Option A: Default)
// =================================================================================================

/// xlsx-Sheet-Namens-Limit (OOXML-Spezifikation, von libxlsxwriter nicht selbst durchgesetzt --
/// workbook_add_worksheet() truncatet stillschweigend, genau das honest-Doktrin verbietet).
inline constexpr std::size_t kXlsxSheetNameMaxLen = 31;

/// Ist EIN vorgeschlagener Sheet-Name zulaessig? <=31 Zeichen, keine der sechs xlsx-verbotenen
/// Zeichen [ ] : * ? / \, nicht leer. Reine CT-Praedikat-Wache (wie ist_wert_zeichen in
/// lager_pfad_grammatik.hpp) -- direkt gegen adversariale Eingaben testbar, unabhaengig davon, dass der
/// deterministische S001..Snnn-Generator unten sie immer erfuellt (Option B/Klartext-Namen, V-A9-6,
/// bleibt eine spaetere Geschmacksfrage und braucht DIESELBE Wache).
[[nodiscard]] constexpr bool xlsx_sheetname_zulaessig(std::string_view name) noexcept {
    if (name.empty() || name.size() > kXlsxSheetNameMaxLen) return false;
    for (char c : name) {
        if (c == '[' || c == ']' || c == ':' || c == '*' || c == '?' || c == '/' || c == '\\') return false;
    }
    return true;
}
static_assert(xlsx_sheetname_zulaessig("S001"));
static_assert(xlsx_sheetname_zulaessig("INFO"));
static_assert(xlsx_sheetname_zulaessig(std::string_view{"1234567890123456789012345678901"}));   // genau 31
static_assert(!xlsx_sheetname_zulaessig(std::string_view{"12345678901234567890123456789012"})); // 32 -> ROT
static_assert(!xlsx_sheetname_zulaessig(""));
static_assert(!xlsx_sheetname_zulaessig("a[b"));
static_assert(!xlsx_sheetname_zulaessig("a]b"));
static_assert(!xlsx_sheetname_zulaessig("a:b"));
static_assert(!xlsx_sheetname_zulaessig("a*b"));
static_assert(!xlsx_sheetname_zulaessig("a?b"));
static_assert(!xlsx_sheetname_zulaessig("a/b"));
static_assert(!xlsx_sheetname_zulaessig("a\\b"));

/// Deterministischer Sheet-Kurzname S001..Snnn -- Reihenfolge = die Reihenfolge, in der der Aufrufer
/// SheetSchluessel bei blatt() vorlegt (bindende Sortierung ist Aufrufer-Pflicht, s. SheetSchluessel).
/// Mindestens 3 Ziffern, zero-padded; waechst additiv jenseits von 999 (S1000, ...) statt umzubrechen.
/// NICHT constexpr (std::to_string) -- die Wache oben ist es, und genau die traegt die Bissproben.
[[nodiscard]] inline std::string sheet_label_fuer_index(std::size_t index_1based) {
    std::string digits = std::to_string(index_1based);
    if (digits.size() < 3) digits.insert(std::size_t{0}, 3 - digits.size(), '0');
    std::string label = "S" + digits;
    if (!xlsx_sheetname_zulaessig(label))
        throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::Namenslimit,
                                    "generierter Sheet-Name '" + label + "' verletzt die Wache");
    return label;
}

/// Der Sheet-Name des INFO-Blatts -- eigene Konstante statt Literal-Wiederholung an mehreren Stellen.
inline constexpr std::string_view kInfoBlattName = "INFO";
static_assert(xlsx_sheetname_zulaessig(kInfoBlattName));

/// Fassung-3-Sheet-Name: bevorzugt LESBAR (compare/Macro/Micro brauchen VORHERSAGBARE Hyperlink-
/// Ziele, z.B. "M_insert" statt eines opaken S00N-Codes) -- faellt auf sheet_label_fuer_index()
/// zurueck, wenn der bevorzugte Name die Wache verletzt (Kollisionsfreiheit ueber DENSELBEN
/// Mechanismus, keine zweite Wache). Das Ebenen-Praefix macht Compare-/Macro-/Micro-Namen
/// kollisionsfrei gegeneinander und gegen die Fassung-1/2-Sheets (die tragen nie C_/M_/X_).
/// bestandslog::sanitisiere_wert (lager_pfad_grammatik.hpp) ist die Single-Source der
/// Zeichenklassen-Bereinigung -- kein zweiter Sanitisierer.
[[nodiscard]] inline std::string mess_ebene_sheetname(MessEbene ebene, std::string_view bezeichner,
                                                      std::size_t fallback_index_1based) {
    std::string_view praefix;
    switch (ebene) {
        case MessEbene::Compare: praefix = "C_"; break;
        case MessEbene::Macro: praefix = "M_"; break;
        case MessEbene::Micro: praefix = "X_"; break;
    }
    std::string kandidat{praefix};
    kandidat += bestandslog::sanitisiere_wert(bezeichner);
    if (xlsx_sheetname_zulaessig(kandidat)) return kandidat;
    return sheet_label_fuer_index(fallback_index_1based);
}

// =================================================================================================
// 5. DIE ZEILENLIMIT-WACHE (Dossier 4.2 Punkt 3)
// =================================================================================================

/// xlsx-Zeilenlimit je Sheet (OOXML/Excel-Spezifikation, LXW_ROW_MAX in libxlsxwriter/worksheet.h).
/// Reihen sind 0-indiziert (Header = Zeile 0) -- gueltige Zeilen-Indices sind [0, kXlsxZeilenlimit).
inline constexpr std::uint32_t kXlsxZeilenlimit = 1'048'576;

/// Reine Grenz-Praedikat-Wache, VOM realen Limit ENTKOPPELT durch den optionalen zweiten Parameter --
/// so bleibt die Grenz-Logik selbst bei JEDEM Limit direkt testbar (rot/gruen an der Grenze), ohne dass
/// ein Test 1.048.576 echte Zeilen schreiben muss ("der Koeder muss erst beissen: pruefe ihn direkt
/// gegen die Wache"). Der echte Writer ruft sie IMMER mit dem Default-Limit auf.
[[nodiscard]] constexpr bool xlsx_zeile_erlaubt(std::uint32_t naechste_zeile,
                                                std::uint32_t limit = kXlsxZeilenlimit) noexcept {
    return naechste_zeile < limit;
}
// Bissprobe an der REALEN Grenze (compile-time, honest-Beleg statt einer Behauptung im Kommentar).
static_assert(xlsx_zeile_erlaubt(kXlsxZeilenlimit - 1)); // letzte gueltige Zeile -> GRUEN
static_assert(!xlsx_zeile_erlaubt(kXlsxZeilenlimit));    // eine zu viel -> ROT
// Dieselbe Grenzlogik an einem winzigen, injizierten Limit -- belegt, dass die Wache bei JEDER Groesse
// bissfest ist, nicht nur zufaellig bei der einen realen Zahl.
static_assert(xlsx_zeile_erlaubt(4, 5));
static_assert(!xlsx_zeile_erlaubt(5, 5));
static_assert(!xlsx_zeile_erlaubt(6, 5));

// =================================================================================================
// 6. DIE INTERFACES (Dossier 4.2)
// =================================================================================================

/// EIN Sheet (xlsx) bzw. EINE Datei (csv-Fallback). kopf()/zeile() nehmen bereits gesplittete
/// Spalten/Felder -- aus lazy_csv_header()/format_csv_row() (Semikolon-Split beim Aufrufer). Token-Treue
/// (failed/gesperrt/nicht_gebaut/n-a) ist Aufrufer- UND Implementierungs-Pflicht: KEINE Implementierung
/// darf ein Feld uminterpretieren.
class IErgebnisBlatt {
public:
    virtual ~IErgebnisBlatt() = default;

    /// Schreibt die Kopfzeile (Spaltennamen). Wird je Sheet GENAU EINMAL vor der ersten zeile() erwartet.
    virtual void kopf(std::span<std::string const> spalten) = 0;

    /// Schreibt EINE Datenzeile. Kann ErgebnisSchreibFehler{Zeilenlimit} werfen (xlsx-Backend) statt
    /// still zu truncaten.
    virtual void zeile(std::span<std::string const> felder) = 0;

    /// Fassung 3: schreibt EINE Zeile, in der GENAU EINE Spalte ein interner Hyperlink ist (compare ->
    /// Funktion -> Achse). felder_vor_verweis + [Verweis-Zelle] + felder_nach_verweis ergeben zusammen
    /// die Zeile. verweis_text ist der SICHTBARE Zellinhalt; ziel_sheet/ziel_zelle sind das
    /// Hyperlink-Ziel ("M_insert"/"A1"). CSV-Fallback DEGRADIERT ehrlich: CSV kennt keine Hyperlinks,
    /// die Zelle traegt dort nur verweis_text als Text (dokumentiertes Verhalten, kein stiller
    /// Funktionsverlust). Traegt dieselbe Zeilenlimit-Wache wie zeile().
    virtual void zeile_mit_verweis(std::span<std::string const> felder_vor_verweis, std::string const& verweis_text,
                                   std::string const& ziel_sheet, std::string const& ziel_zelle,
                                   std::span<std::string const> felder_nach_verweis) = 0;
};

/// EINE xlsx-Datei bzw. EIN CSV-Dateisatz am selben Baum-Blatt.
class IErgebnisMappe {
public:
    virtual ~IErgebnisMappe() = default;

    /// Registriert die INFO-Blatt-Inhalte (Sysinfo, verwendete Haupt-Achsen, konstante Meta-Achsen). Die
    /// tatsaechliche Sheet-Legende (welcher Sheet-Name zu welchem SheetSchluessel gehoert) wird bei
    /// schliessen() ergaenzt -- info_blatt() darf VOR, NACH oder ZWISCHEN blatt()-Aufrufen kommen; die
    /// Reihenfolge ist bewusst NICHT vorgeschrieben.
    virtual void info_blatt(MaschinenSysinfo const& sysinfo, HauptAchsenBelegung const& haupt,
                            KonstantenMeta const& konstanten) = 0;

    /// Liefert das Sheet fuer diese Unter-Achsen-Permutation -- legt es bei erstem Aufruf an
    /// (deterministischer Sheet-Name, Reihenfolge = Aufrufreihenfolge). Wiederholter Aufruf mit
    /// demselben Schluessel liefert DASSELBE Sheet (Idempotenz, kein Doppel-Anlegen).
    virtual IErgebnisBlatt& blatt(SheetSchluessel const& schluessel) = 0;

    /// Fassung 3: liefert das Sheet fuer diese compare/Macro/Micro-Ebene -- EIGENE Sheet-Zaehlung,
    /// getrennt von blatt()/SheetSchluessel, aber DIESELBE Mappe/Factory/Namens-Wache/INFO-Sheet.
    /// Idempotent wie blatt(): derselbe Schluessel liefert dasselbe Sheet.
    virtual IErgebnisBlatt& mess_ebene_blatt(MessEbenenSchluessel const& schluessel) = 0;

    /// Schliesst die Mappe ATOMAR: tmp-Datei(en) + rename auf den finalen Namen. Baut das INFO-Blatt
    /// (inkl. Sheet-Legende ueber ALLE inzwischen angelegten Sheets) und finalisiert. Wirft
    /// ErgebnisSchreibFehler bei jedem Fehl-Ausgang -- NIE eine stille Teil-Datei sichtbar machen.
    virtual void schliessen() = 0;
};

// =================================================================================================
// 7. CsvErgebnisMappe -- der Fallback DERSELBEN Factory (Dossier 4.2 Punkt 5)
// =================================================================================================
// Je Sheet EINE Datei <stamm>__S001.csv + <stamm>__INFO.csv (Owner-KERN 26.07. Section 6: "CSVs werden
// im FACTORY PATTERN je Sheet einzeln gebaut"). Semikolon-Konvention wie lazy_csv_header(). Header-only
// (Dossier 4.1): keine Vendor-Abhaengigkeit, reines std::ofstream.

namespace detail {

/// EIN CSV-Blatt: schreibt SOFORT (gestreamt) in eine tmp-Datei; das Rename auf den finalen Namen liegt
/// bei CsvErgebnisMappe::schliessen() (Atomaritaet auf Mappen-Ebene, nicht auf Blatt-Ebene).
class CsvErgebnisBlatt final : public IErgebnisBlatt {
public:
    explicit CsvErgebnisBlatt(std::filesystem::path tmp_pfad)
        : tmp_pfad_(std::move(tmp_pfad)), strom_(tmp_pfad_, std::ios::binary | std::ios::trunc) {
        if (!strom_.is_open())
            throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler,
                                        tmp_pfad_.string() + ": Datei nicht oeffenbar");
    }

    void kopf(std::span<std::string const> spalten) override { schreibe_zeile(spalten); }
    void zeile(std::span<std::string const> felder) override { schreibe_zeile(felder); }

    /// CSV-Fallback-Degradation: keine Hyperlinks im Textformat -- verweis_text wird als GEWOEHNLICHE
    /// Zelle geschrieben (dokumentiertes Verhalten, s. IErgebnisBlatt::zeile_mit_verweis).
    void zeile_mit_verweis(std::span<std::string const> felder_vor_verweis, std::string const& verweis_text,
                           std::string const& /*ziel_sheet*/, std::string const& /*ziel_zelle*/,
                           std::span<std::string const> felder_nach_verweis) override {
        zeile_hat_felder_ = false;
        for (auto const& f : felder_vor_verweis) schreibe_feld(f);
        schreibe_feld(verweis_text);
        for (auto const& f : felder_nach_verweis) schreibe_feld(f);
        strom_ << '\n';
        zeile_hat_felder_ = false;
    }

    /// Schliesst den Strom explizit (vor dem Rename) und meldet einen Schreibfehler ehrlich statt still.
    void strom_schliessen() {
        strom_.flush();
        bool const fehlgeschlagen = strom_.fail();
        strom_.close();
        if (fehlgeschlagen)
            throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler,
                                        tmp_pfad_.string() + ": Schreiben fehlgeschlagen");
    }

private:
    /// Schreibt EIN Feld inkl. des Trenners VOR sich, ausser es ist das allererste Feld der Zeile.
    void schreibe_feld(std::string_view feld) {
        if (!zeile_hat_felder_) {
            zeile_hat_felder_ = true;
        } else {
            strom_ << ';';
        }
        strom_ << feld;
    }

    void schreibe_zeile(std::span<std::string const> felder) {
        zeile_hat_felder_ = false;
        for (auto const& f : felder) schreibe_feld(f);
        strom_ << '\n';
        zeile_hat_felder_ = false;
    }

    std::filesystem::path tmp_pfad_;
    std::ofstream         strom_;
    bool                  zeile_hat_felder_ = false; // Trenner-Zustand ueber schreibe_feld()-Aufrufe hinweg
};

} // namespace detail

class CsvErgebnisMappe final : public IErgebnisMappe {
public:
    CsvErgebnisMappe(std::filesystem::path verzeichnis, std::string dateiname_stamm)
        : verzeichnis_(std::move(verzeichnis)), stamm_(std::move(dateiname_stamm)) {}

    void info_blatt(MaschinenSysinfo const& sysinfo, HauptAchsenBelegung const& haupt,
                    KonstantenMeta const& konstanten) override {
        sysinfo_    = sysinfo;
        haupt_      = haupt;
        konstanten_ = konstanten;
    }

    IErgebnisBlatt& blatt(SheetSchluessel const& schluessel) override {
        for (std::size_t i = 0; i < schluessel_.size(); ++i)
            if (schluessel_[i] == schluessel) return *blaetter_[i];

        std::string const label    = sheet_label_fuer_index(++naechster_sheet_index_);
        auto const        tmp_pfad = verzeichnis_ / (stamm_ + "__" + label + ".csv.tmp");
        schluessel_.push_back(schluessel);
        labels_.push_back(label);
        tmp_pfade_.push_back(tmp_pfad);
        blaetter_.push_back(std::make_unique<detail::CsvErgebnisBlatt>(tmp_pfad));
        return *blaetter_.back();
    }

    IErgebnisBlatt& mess_ebene_blatt(MessEbenenSchluessel const& schluessel) override {
        for (std::size_t i = 0; i < mess_ebenen_schluessel_.size(); ++i)
            if (mess_ebenen_schluessel_[i] == schluessel) return *mess_ebenen_blaetter_[i];

        // "CSV bleibt flach: eine Datei je Sheet, mit der EBENE ANFUEHREND im Namen" (Owner woertlich)
        // -- der Ebenen-Token fuehrt den Datei-Suffix an, danach der (sanitisierte) Bezeichner.
        std::string const dateisuffix = std::string{mess_ebene_label(schluessel.ebene)} + "_" +
                                        bestandslog::sanitisiere_wert(schluessel.bezeichner);
        auto const        tmp_pfad    = verzeichnis_ / (stamm_ + "__" + dateisuffix + ".csv.tmp");
        mess_ebenen_schluessel_.push_back(schluessel);
        mess_ebenen_dateisuffix_.push_back(dateisuffix);
        mess_ebenen_tmp_pfade_.push_back(tmp_pfad);
        ++naechster_sheet_index_; // gemeinsamer Zaehler mit blatt() -- s. Kopf-Kommentar der Klasse
        mess_ebenen_blaetter_.push_back(std::make_unique<detail::CsvErgebnisBlatt>(tmp_pfad));
        return *mess_ebenen_blaetter_.back();
    }

    void schliessen() override {
        // 1) INFO-CSV bauen (tmp), inkl. Sheet-Legende ueber ALLE bis hierher angelegten Sheets.
        auto const  info_tmp = verzeichnis_ / (stamm_ + "__" + std::string{kInfoBlattName} + ".csv.tmp");
        std::string zeile;
        {
            std::ofstream info(info_tmp, std::ios::binary | std::ios::trunc);
            if (!info.is_open())
                throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler,
                                            info_tmp.string() + ": Datei nicht oeffenbar");
            info << "kategorie;schluessel;wert\n";
            auto const feld = [](std::string_view s) { return n_a_wenn_leer(s); };
            info << "sysinfo;hostname;" << feld(sysinfo_.hostname) << '\n';
            info << "sysinfo;machine_id;" << feld(sysinfo_.machine_id) << '\n';
            info << "sysinfo;cpu_fabrication;" << feld(sysinfo_.cpu_fabrication) << '\n';
            info << "sysinfo;ram_pair;" << feld(sysinfo_.ram_pair) << '\n';
            info << "sysinfo;os_version;" << feld(sysinfo_.os_version) << '\n';
            info << "sysinfo;kernel;" << feld(sysinfo_.kernel) << '\n';
            info << "sysinfo;build;" << feld(sysinfo_.build) << '\n';
            info << "sysinfo;identity_verdict;" << feld(sysinfo_.identity_verdict) << '\n';
            for (auto const& e : haupt_) info << "hauptachse;" << e.achse << ';' << e.wert << '\n';
            for (auto const& e : konstanten_) info << "konstante;" << e.achse << ';' << e.wert << '\n';
            for (std::size_t i = 0; i < labels_.size(); ++i)
                info << "sheet_legende;" << labels_[i] << ';' << schluessel_[i].mess_unter << '|'
                     << schluessel_[i].system_unter << '|' << schluessel_[i].organ_unter << '\n';
            // Fassung 3: die compare/Macro/Micro-Legende -- Dateiname statt Sheet-Name (CSV hat keinen
            // Sheet-Namen), aber dieselbe Kategorie-Spalte "mess_ebene_legende" wie im xlsx-Backend.
            for (std::size_t i = 0; i < mess_ebenen_schluessel_.size(); ++i)
                info << "mess_ebene_legende;" << mess_ebenen_dateisuffix_[i] << ';'
                     << mess_ebene_label(mess_ebenen_schluessel_[i].ebene) << '|'
                     << mess_ebenen_schluessel_[i].bezeichner << '\n';
            info.flush();
            if (info.fail())
                throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler,
                                            info_tmp.string() + ": Schreiben fehlgeschlagen");
        }

        // 2) alle Sheet-Streams sauber schliessen, BEVOR irgendetwas umbenannt wird (atomar auf
        //    Mappen-Ebene: erst wenn ALLES geschrieben ist, wird SICHTBAR gemacht).
        for (auto& blatt : blaetter_) blatt->strom_schliessen();
        for (auto& blatt : mess_ebenen_blaetter_) blatt->strom_schliessen();

        // 3) rename tmp -> final, je Datei; jeder Fehl-Ausgang wirft (kein stilles Teil-Ergebnis).
        for (std::size_t i = 0; i < blaetter_.size(); ++i) {
            auto const      final_pfad = verzeichnis_ / (stamm_ + "__" + labels_[i] + ".csv");
            std::error_code ec;
            std::filesystem::rename(tmp_pfade_[i], final_pfad, ec);
            if (ec)
                throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler, tmp_pfade_[i].string() + " -> " +
                                                                                        final_pfad.string() + ": " +
                                                                                        ec.message());
        }
        for (std::size_t i = 0; i < mess_ebenen_blaetter_.size(); ++i) {
            auto const      final_pfad = verzeichnis_ / (stamm_ + "__" + mess_ebenen_dateisuffix_[i] + ".csv");
            std::error_code ec;
            std::filesystem::rename(mess_ebenen_tmp_pfade_[i], final_pfad, ec);
            if (ec)
                throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler, mess_ebenen_tmp_pfade_[i].string() +
                                                                                        " -> " + final_pfad.string() +
                                                                                        ": " + ec.message());
        }
        auto const      info_final = verzeichnis_ / (stamm_ + "__" + std::string{kInfoBlattName} + ".csv");
        std::error_code ec;
        std::filesystem::rename(info_tmp, info_final, ec);
        if (ec)
            throw ErgebnisSchreibFehler(ms::LagerAblageFehlerKlasse::ZipFehler,
                                        info_tmp.string() + " -> " + info_final.string() + ": " + ec.message());
    }

private:
    std::filesystem::path              verzeichnis_;
    std::string                        stamm_;
    MaschinenSysinfo                   sysinfo_{};
    HauptAchsenBelegung                haupt_{};
    KonstantenMeta                     konstanten_{};
    std::size_t                        naechster_sheet_index_ = 0; // geteilt: blatt()+mess_ebene_blatt()
    std::vector<SheetSchluessel>       schluessel_;
    std::vector<std::string>           labels_;
    std::vector<std::filesystem::path> tmp_pfade_;
    std::vector<std::unique_ptr<detail::CsvErgebnisBlatt>> blaetter_;
    // Fassung 3 (parallele Buchhaltung, EIGENE Sheet-Zaehlung -- s. Kopf-Kommentar Abschnitt 2B).
    std::vector<MessEbenenSchluessel>                      mess_ebenen_schluessel_;
    std::vector<std::string>                               mess_ebenen_dateisuffix_;
    std::vector<std::filesystem::path>                     mess_ebenen_tmp_pfade_;
    std::vector<std::unique_ptr<detail::CsvErgebnisBlatt>> mess_ebenen_blaetter_;
};

// =================================================================================================
// 8. DIE FACTORY (Dossier 4.2) -- Runtime-Draht xlsx|csv, KEIN std::variant
// =================================================================================================

namespace detail {
/// TU-Grenze: NUR diese eine Funktion kennt <xlsxwriter.h> (Definition in xlsx_ergebnis_writer.cpp,
/// Target comdare_lager_ablage). Ein Aufrufer, der NIE ErgebnisFormat::xlsx waehlt, referenziert diese
/// Funktion trotzdem (ODR-Use durch den Dispatch unten) und muss deshalb gegen comdare::lager_ablage
/// linken -- Nicht-Nutzer der Lager-Ablage-Strecke linken das Target ueberhaupt nicht (Consumer-only).
[[nodiscard]] std::unique_ptr<IErgebnisMappe> erzeuge_xlsx_ergebnis_mappe(std::filesystem::path const& verzeichnis,
                                                                          std::string const&           dateiname_stamm);
} // namespace detail

/// Abstract Factory (GoF): Default xlsx, csv einstellbar. EINE aktive Strategie je Aufruf.
class ErgebnisMappenFactory {
public:
    [[nodiscard]] static std::unique_ptr<IErgebnisMappe> oeffne(std::filesystem::path const& blatt_verzeichnis,
                                                                std::string const&           dateiname_stamm,
                                                                ErgebnisFormat               f = ErgebnisFormat::xlsx);
};

[[nodiscard]] inline std::unique_ptr<IErgebnisMappe>
ErgebnisMappenFactory::oeffne(std::filesystem::path const& blatt_verzeichnis, std::string const& dateiname_stamm,
                              ErgebnisFormat f) {
    if (f == ErgebnisFormat::csv) return std::make_unique<CsvErgebnisMappe>(blatt_verzeichnis, dateiname_stamm);
    return detail::erzeuge_xlsx_ergebnis_mappe(blatt_verzeichnis, dateiname_stamm);
}

} // namespace comdare::cache_engine::builder::lager_ablage
