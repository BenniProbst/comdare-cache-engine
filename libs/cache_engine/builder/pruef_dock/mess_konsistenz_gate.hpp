#pragma once
// mess_konsistenz_gate.hpp -- M-1/D-2 (06.08.2026): DER VERTRAG CEB <-> TIER-BINARY AM PRUEFDOCK.
//
// SIBLING von conformance_gate.hpp und mess_interface_testate.hpp, NICHT deren Erweiterung. Die drei
// Gatter am Dock pruefen drei verschiedene Dinge, und die Trennung ist tragend:
//   conformance_gate      -- KANN die Huelle, was eine std::map kann?            (FUNKTION)
//   mess_interface_testate-- TRAEGT die Huelle die Mess-Interfaces, und WIRKEN sie? (AUSSTATTUNG, IST)
//   DIESE DATEI           -- ist die Ausstattung DIE, die die CEB einkompiliert hat? (IDENTITAET, SOLL/IST)
//
// ------------------------------------------------------------------------------------------------
// DER BEFUND, DEN DIESE DATEI HEILT (D-2, am Objekt gemessen, Stand b241a272 / 62a5b6f7)
// ------------------------------------------------------------------------------------------------
// Vollstaendiger Zensus ueber libs/ + apps/ + tests/ nach measurement_line|measurement_entries|
// measurement_entry_count: 20 Treffer in 6 Dateien -- 6 in der ABI-Deklaration selbst
// (anatomy_module_abi_v1_decl.hpp), 4 in der Stempel-Erzeugung (anatomy_version_stamp.hpp,
// anatomy_module_abi_v1.hpp, meta_meta_stamp_suffix.hpp), 10 in zwei Unit-Tests. PRODUKTIVE LESER: NULL.
// Das Tier DEKLARIERTE seine Mess-Ausstattung, und niemand las die Deklaration je.
//
// NENNER + GEGENPROBE (eine nackte Null ist kein Befund): dasselbe Verfahren, auf die ZWILLINGS-Felder
// organ_line|system_line|organ_entries|system_entries angesetzt, findet 31 Treffer -- darunter ZWEI in
// bestandslog_factory.hpp, also produktiven Code. Das Verfahren findet produktive Leser dort, wo es
// welche gibt; bei der Mess-Zeile gab es keine.
//
// EINORDNUNG, ehrlich: D-2 ist damit KEIN Regressionsschaden, sondern eine NIE GEBAUTE BRUECKE. Der
// Vertrag aus LEDGER:3319 ("welche Pruef-Tools in die CEB EINKOMPILIERT sind [...] bestimmt, was auch das
// Tier-Binary beinhalten MUSS") war nie Code. Diese Datei baut ihn.
//
// ------------------------------------------------------------------------------------------------
// WAS GENAU VERGLICHEN WIRD -- UND WARUM DIE ZEILE UND NICHT EINE MENGE
// ------------------------------------------------------------------------------------------------
// Verglichen wird die MESS-STEMPEL-ZEILE: BYTE-GLEICHHEIT zwischen
//   SOLL == die Zeile, die die CEB in DIESEM Lauf in die Tier-Quelle stempelt
//           (profile_facade/lazy_adhoc_source_gen.hpp measurement_stamp_from_env(), gespeist aus
//            der EINEN Aufloesung resolve_live_measurement_combo_legend -- der M-1/D-1-Naht), und
//   IST  == AnatomyVersionLines::measurement_line des GELADENEN Moduls, also das, was das Modul
//           per COMDARE_ANATOMY_VERSION_STAMP_M wirklich einkompiliert bekommen hat.
//
// WARUM STRING-GLEICHHEIT UND KEIN MENGEN-VERGLEICH: die Zeile IST die kanonische Form der Mess-Achsen-
// Wahl (measurement_stamp_line, Section 43/47) und traegt mehr als die Tooling-MENGE -- naemlich die
// Code-VERSION je Tooling ("@1.0.0c") und den Meta-Meta-Klammer-Anhang ([load_framework=...]). Ein
// Mengen-Vergleich wuerde einen Versions-Sprung der Mess-Achse ("neues Messsystem", Owner-KERN F2)
// GENAU NICHT sehen, obwohl F2 fuer diesen Fall den Neubau aller Binaries verlangt. Ein Zweit-Parser
// waere ausserdem die Drift-Klasse, gegen die schon die D-1-Naht gebaut ist. Die Zeile ist der Vertrag.
//
// ------------------------------------------------------------------------------------------------
// FAIL-CLOSED -- JEDER UNKLARE ZUSTAND IST EIN FEHLER, KEIN FREIFAHRTSCHEIN
// ------------------------------------------------------------------------------------------------
// Sechs Ablehnungsklassen, und keine davon hat einen stillen Durchlass:
//   (1) erwartung_leer      Die CEB kann nicht sagen, was sie einkompiliert hat. Dann ist NICHTS
//                           pruefbar -- und eine unpruefbare Messkette ist kein bestandener Vertrag.
//   (2) stempel_fehlt       Das Modul exportiert das Stempel-Symbol nicht (2-arg-Emission / Fremd-.so).
//                           Es DEKLARIERT nichts, also kann nichts uebereinstimmen.
//   (3) layout_fremd        stamp_layout_version != 6. Ein fremdes POD-Layout haette ANDERE Offsets
//                           (A13-M3/K-4 hat sie um -16 verschoben) -- weiterlesen hiesse Zeiger-Muell
//                           interpretieren. Gleichheits-Wache, nie Ordnung.
//   (4) deklaration_leer    measurement_len == 0 bzw. measurement_entry_count == 0: "kein Mess-Tooling
//                           einkompiliert". Das ist eine EHRLICHE Aussage des Moduls -- und exakt der
//                           Zustand, in dem es nicht messen darf.
//   (5) zeile_abweichung    Die Kern-Abweisung: SOLL != IST.
//   (6) entries_widerspruch Die Zeile stimmt, aber das aus DERSELBEN Zeile geparste Entry-Array traegt
//                           eine andere Zahl von Mess-HAUPT-Eintraegen als die SOLL-Zeile. Das kann nur
//                           entstehen, wenn Zeile und Array im Modul auseinandergelaufen sind (zwei
//                           Ableitungswege) -- die O-8-Schritt-12-Fehlerklasse. Billig zu pruefen,
//                           deshalb geprueft.
//
// WARUM (6) NICHT REDUNDANT IST, obwohl (5) schon byte-gleich verglichen hat: (5) vergleicht die SOLL-
// Zeile gegen das FELD measurement_line. (6) vergleicht dieselbe SOLL-Zeile gegen das FELD
// measurement_entries. Beide Felder entstehen im Makro aus demselben Literal kM -- aber ueber ZWEI
// verschiedene consteval-Wege (sizeof(kM)-1 vs. parse_stamp_entries<count_stamp_entries(kM)>). Genau so
// eine Doppel-Ableitung war der O-8-Befund. (6) ist die Wache darauf.
//
// ------------------------------------------------------------------------------------------------
// WAS DIESES GATE BEWUSST NICHT TUT
// ------------------------------------------------------------------------------------------------
// Es prueft NICHT, ob die Mess-INTERFACES real da sind und wirken -- das ist die Aufgabe der Testate
// (mess_interface_testate.hpp), und die kann es besser, weil sie treibt statt liest. Dieses Gate prueft
// die DEKLARATION gegen die ERWARTUNG. Erst beide zusammen schliessen den Kreis:
//   Testat  : "die Binary traegt IObservableTier und die Zaehler bewegen sich"      (IST wirkt)
//   Gate    : "die Binary sagt [macro], und die CEB hat [macro] einkompiliert"      (IST == SOLL)
// Ein Testat allein liesse eine [micro]-Binary durch, die als [wallclock] deklariert ist; ein Gate
// allein liesse eine Binary durch, deren Stempel stimmt und deren Observer tot sind.
//
// DOKTRIN: header-only C++23, ASCII-only, nur stdlib + die ABI-/Loader-Header. KEINE Beruehrung von
// abi_adapter/PODs/Wire -- diese Datei LIEST die ABI, sie erweitert sie nicht (kein ABI-Schritt: das POD
// traegt measurement_line/measurement_entries seit Layout 6, siehe anatomy_module_abi_v1_decl.hpp:193/221).

#include "conformance_gate.hpp" // ConformanceResult (die EINE Quoten-Form des Docks)

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp> // AnatomyModuleHandle::version_lines()
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>         // AnatomyVersionLines / stamp_pod_has_entries
#include <cache_engine/abi/anatomy_stamp_entries.hpp>              // stamp_entry_meta_level (Ebene 0 == Haupt-Achse)

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::pruef_dock {

// ---------------------------------------------------------------------------------------------------------------
// Status-Taxonomie -- errno-Stil wie der Loader (anatomy_module_loader.hpp:35-58), damit der Aufrufer
// klassifizieren kann statt nur "rot" zu sehen.
// ---------------------------------------------------------------------------------------------------------------

enum class MessKonsistenzStatus : int {
    ok = 0,               ///< SOLL == IST, Deklaration vollstaendig
    erwartung_leer,       ///< (1) die CEB reicht keine SOLL-Zeile -- unpruefbar == Fehler
    stempel_symbol_fehlt, ///< (2) das Modul exportiert comdare_anatomy_version_lines nicht
    stempel_layout_fremd, ///< (3) stamp_layout_version != kAnatomyVersionLinesLayout
    deklaration_leer,     ///< (4) das Modul deklariert KEIN Mess-Tooling
    zeile_abweichung,     ///< (5) SOLL != IST
    entries_widerspruch,  ///< (6) Zeile und Entry-Array des Moduls widersprechen sich
};

[[nodiscard]] constexpr char const* mess_konsistenz_status_name(MessKonsistenzStatus s) noexcept {
    switch (s) {
        case MessKonsistenzStatus::ok: return "ok";
        case MessKonsistenzStatus::erwartung_leer: return "erwartung_leer";
        case MessKonsistenzStatus::stempel_symbol_fehlt: return "stempel_symbol_fehlt";
        case MessKonsistenzStatus::stempel_layout_fremd: return "stempel_layout_fremd";
        case MessKonsistenzStatus::deklaration_leer: return "deklaration_leer";
        case MessKonsistenzStatus::zeile_abweichung: return "zeile_abweichung";
        case MessKonsistenzStatus::entries_widerspruch: return "entries_widerspruch";
    }
    return "unbekannt";
}

/// Ergebnis EINER Konsistenz-Pruefung. Traegt neben dem Status BEIDE Zeilen, damit der Aufrufer eine
/// ehrliche Fehlermeldung schreiben kann ("erwartet X, gefunden Y") statt eines nackten Codes -- die
/// Fehlerklassen-Doktrin verlangt die AUSSAGE, nicht nur die Ablehnung.
struct MessKonsistenzErgebnis {
    MessKonsistenzStatus status = MessKonsistenzStatus::erwartung_leer; ///< fail-closed per DEFAULT-Wert
    std::string          soll;                                          ///< die von der CEB erwartete Mess-Zeile
    std::string          ist;                  ///< die vom Modul deklarierte Mess-Zeile ("" wenn keine)
    std::uint64_t        ist_entry_count  = 0; ///< measurement_entry_count des Moduls (ALLE Ebenen)
    std::uint64_t        ist_haupt_count  = 0; ///< davon Ebene 0 (Mess-HAUPT-Achsen) -- der zweite Ableitungsweg
    std::uint64_t        soll_haupt_count = 0; ///< Zahl der Mess-HAUPT-Segmente in der SOLL-Zeile
    std::uint32_t        ist_stamp_layout = 0; ///< stamp_layout_version des Moduls (0 == kein Stempel)
    [[nodiscard]] bool   passed() const noexcept { return status == MessKonsistenzStatus::ok; }
};

namespace mess_konsistenz_detail {

/// Zahl der Mess-HAUPT-Achsen-Segmente einer Stempel-Zeile (Ebene 0 der Klammer-Grammatik).
///
/// FORM DER ZEILE (anatomy_version_stamp.hpp::measurement_stamp_line, A13-M2/Owner-E2):
///     "measurement_tooling=wallclock@1.0.0c;measurement_tooling=macro@1.0.0c;[load_framework=ycsb@1.0.0c]"
/// Die HAUPT-Achsen stehen als ';'-getrennte Segmente auf Ebene 0; der Meta-Meta-Anhang steht GEKLAMMERT
/// am ENDE und ist Ebene 1, also KEINE Haupt-Achse. Genau dafuer ist er geklammert -- ohne die Klammer
/// koennte ein Konsument load_framework nicht von einer Mess-HAUPT-Achse unterscheiden
/// (anatomy_version_stamp.hpp, Absatz "EINE Regel fuer alle Realms").
///
/// WARUM HIER EIN EIGENER ZAEHLER UND NICHT count_stamp_entries: dieser Zaehler laeuft ueber eine
/// LAUFZEIT-Zeile (die SOLL-Zeile kommt als std::string herein), waehrend count_stamp_entries/
/// parse_stamp_entries consteval sind und ein LITERAL brauchen -- sie sind fuer eine zur Laufzeit
/// gerenderte Zeile schlicht nicht aufrufbar.
///
/// UND WARUM DAS TROTZDEM KEINE ZWEITE GRAMMATIK IST: gezaehlt wird ausschliesslich die EBENE-0-Menge,
/// und das Ergebnis wird unten NICHT gegen measurement_entry_count gehalten (das zaehlt ALLE Ebenen),
/// sondern gegen die Zahl der Eintraege mit stamp_entry_meta_level(e) == 0. Beide Seiten sprechen damit
/// vom selben Begriff, und die Ebenen-Zuordnung selbst kommt aus der EINEN consteval-Grammatik im Modul
/// (reserved-Bits 3-5) -- dieser Zaehler bildet sie nicht nach, er zaehlt nur die Trennzeichen der
/// obersten Ebene. Klammer-Inhalte werden dabei UEBERSPRUNGEN, damit ein ';' INNERHALB einer Gruppe
/// (mehrere Meta-Metas) nicht faelschlich ein Haupt-Segment abtrennt.
[[nodiscard]] inline std::uint64_t haupt_segment_count(std::string_view zeile) noexcept {
    std::uint64_t n     = 0;
    std::size_t   start = 0;
    std::size_t   tiefe = 0;
    for (std::size_t i = 0; i <= zeile.size(); ++i) {
        bool const ende = (i == zeile.size());
        char const c    = ende ? ';' : zeile[i];
        if (!ende && c == '[') {
            ++tiefe;
            continue;
        }
        if (!ende && c == ']') {
            if (tiefe > 0) --tiefe;
            continue;
        }
        if (c != ';' || tiefe != 0) continue;
        std::string_view const seg = zeile.substr(start, i - start);
        if (!seg.empty() && seg.front() != '[') ++n;
        start = i + 1;
    }
    return n;
}

/// Zahl der EBENE-0-Eintraege im Mess-Array eines geladenen Moduls. Liest die Ebene aus den reserved-Bits,
/// die die consteval-Grammatik des Moduls dort hinterlegt hat -- KEINE erneute Tokenisierung der Zeile
/// (genau dafuer ist stamp_entry_meta_level da, anatomy_stamp_entries.hpp).
[[nodiscard]] inline std::uint64_t haupt_entry_count(::comdare::cache_engine::abi::AnatomyStampEntryV1 const* entries,
                                                     std::uint64_t count) noexcept {
    if (entries == nullptr) return 0;
    std::uint64_t n = 0;
    for (std::uint64_t i = 0; i < count; ++i)
        if (::comdare::cache_engine::abi::stamp_entry_meta_level(entries[i]) == 0u) ++n;
    return n;
}

} // namespace mess_konsistenz_detail

/// pruefe_mess_konsistenz(lines, soll) -- DAS GATE. Rein lesend, ohne Loader-Wissen, damit der Biss ohne
/// DLL-Bau gefuehrt werden kann (dieselbe Schnitt-Begruendung wie bei den Mess-Testaten: Interface-/POD-
/// Referenz rein, Ergebnis raus).
///
/// `lines` == nullptr ist der Fall "Modul ohne Stempel-Symbol" und wird als solcher klassifiziert -- NICHT
/// als Absturz und NICHT als Durchlass.
///
/// REIHENFOLGE DER PRUEFUNGEN ist tragend und NICHT beliebig:
///   erwartung -> symbol -> layout -> deklaration -> zeile -> entries.
/// Jede spaetere Pruefung setzt die frueheren voraus. Insbesondere darf KEIN Feld des POD gelesen werden,
/// bevor das Layout bestaetigt ist: bei fremdem Layout laegen die Offsets um zwei Felder daneben
/// (A13-M3/K-4), und ein Vergleich haette dann Zeiger-Muell gegen die SOLL-Zeile gehalten -- also ein
/// Ergebnis, das schlimmer waere als gar keines.
[[nodiscard]] inline MessKonsistenzErgebnis
pruefe_mess_konsistenz(::comdare::cache_engine::abi::AnatomyVersionLines const* lines, std::string_view soll) noexcept {
    MessKonsistenzErgebnis e{};
    e.soll             = std::string{soll};
    e.soll_haupt_count = mess_konsistenz_detail::haupt_segment_count(soll);

    // (1) ERWARTUNG. Fail-closed: ohne SOLL ist nichts pruefbar. Das ist KEIN Sonderfall, den man
    //     durchwinken darf -- eine CEB, die ihre eigene einkompilierte Mess-Achse nicht benennen kann,
    //     hat die Stufe-2-Haertung der Stufen-Doktrin nicht vollzogen.
    if (soll.empty()) {
        e.status = MessKonsistenzStatus::erwartung_leer;
        return e;
    }

    // (2) SYMBOL.
    if (lines == nullptr) {
        e.status = MessKonsistenzStatus::stempel_symbol_fehlt;
        return e;
    }

    // (3) LAYOUT -- VOR jedem anderen Feldzugriff (K-4-Gleichheits-Wache, keine Ordnung).
    e.ist_stamp_layout = lines->stamp_layout_version;
    if (!::comdare::cache_engine::abi::stamp_pod_has_entries(*lines)) {
        e.status = MessKonsistenzStatus::stempel_layout_fremd;
        return e;
    }

    // Ab hier sind die Offsets bestaetigt. measurement_line ist per Makro-Vertrag NIE nullptr ("" -Doktrin),
    // der Zweig deckt eine von Hand gebaute POD-Instanz ab.
    e.ist_entry_count = lines->measurement_entry_count;
    e.ist_haupt_count = mess_konsistenz_detail::haupt_entry_count(lines->measurement_entries, e.ist_entry_count);
    if (lines->measurement_line != nullptr && lines->measurement_len > 0)
        e.ist = std::string{lines->measurement_line, static_cast<std::size_t>(lines->measurement_len)};

    // (4) DEKLARATION. Beide Leer-Formen sind derselbe Fall und werden beide gefangen: eine leere Zeile
    //     OHNE leeres Array (oder umgekehrt) waere schon fuer sich ein Widerspruch.
    if (e.ist.empty() || e.ist_entry_count == 0) {
        e.status = MessKonsistenzStatus::deklaration_leer;
        return e;
    }

    // (5) DIE KERN-ABWEISUNG.
    if (e.ist != e.soll) {
        e.status = MessKonsistenzStatus::zeile_abweichung;
        return e;
    }

    // (6) ZWEITER ABLEITUNGSWEG. Die Zeile stimmt -- traegt das ARRAY dieselbe Zahl von HAUPT-Eintraegen
    //     (Ebene 0), wie die SOLL-Zeile Haupt-Segmente hat? Verglichen wird bewusst Ebene-0 gegen Ebene-0
    //     und NICHT gegen measurement_entry_count: das zaehlt ALLE Ebenen und traegt in jeder heute
    //     gerenderten Mess-Zeile den [load_framework=...]-Anhang mit (Vollmenge: 3 Haupt + 1 Meta-Meta = 4).
    //     Ein Vergleich gegen die Gesamtzahl waere also von vornherein schief -- und haette das Gate an
    //     JEDER korrekten Binary zum Anschlagen gebracht.
    if (e.ist_haupt_count != e.soll_haupt_count) {
        e.status = MessKonsistenzStatus::entries_widerspruch;
        return e;
    }

    e.status = MessKonsistenzStatus::ok;
    return e;
}

/// Bequemform am geladenen Modul. Zieht die Deklaration aus der Handle (die der Loader beim dlopen aus dem
/// optionalen Probe-Symbol gezogen hat) und faehrt dasselbe Gate.
[[nodiscard]] inline MessKonsistenzErgebnis pruefe_mess_konsistenz(anatomy_loader::AnatomyModuleHandle const& h,
                                                                   std::string_view soll) noexcept {
    return pruefe_mess_konsistenz(h.version_lines(), soll);
}

/// mess_konsistenz_meldung(e) -- die EINE ehrliche Fehlerzeile zu einem Ergebnis. Sie steht hier und nicht
/// beim Aufrufer, damit alle Aufrufer dieselbe Sprache sprechen und der Text nicht an drei Stellen driftet.
/// Nennt IMMER beide Seiten -- eine Abweisung ohne "erwartet X, gefunden Y" zwingt den Bediener, den Grund
/// selbst zu suchen, und das ist genau die Sorte Log, aus der Gedaechtnisluecken entstehen.
[[nodiscard]] inline std::string mess_konsistenz_meldung(MessKonsistenzErgebnis const& e) {
    std::string m{"fehlerklasse=mess_konsistenz status="};
    m += mess_konsistenz_status_name(e.status);
    m += " erwartet='";
    m += e.soll;
    m += "' gefunden='";
    m += e.ist;
    m += "' (stamp_layout=";
    m += std::to_string(static_cast<unsigned long long>(e.ist_stamp_layout));
    m += ", entries_ist=";
    m += std::to_string(static_cast<unsigned long long>(e.ist_entry_count));
    m += ", haupt_ist=";
    m += std::to_string(static_cast<unsigned long long>(e.ist_haupt_count));
    m += ", haupt_soll=";
    m += std::to_string(static_cast<unsigned long long>(e.soll_haupt_count));
    m += ")";
    return m;
}

// ---------------------------------------------------------------------------------------------------------------
// TESTAT-FORM -- damit das Gate im Quoten-Batch des Docks mitlaeuft
// ---------------------------------------------------------------------------------------------------------------
//
// Der Pruefstand aggregiert ConformanceResult-Quoten (conformance_gate + die sieben Mess-Testate). Dieses
// Testat reiht sich EIN, statt eine zweite Ergebnis-Waehrung einzufuehren. Es prueft die Konsistenz in
// FUENF benannten Einzel-Zusicherungen statt in einer, damit die Quote sagt, WORAN es lag.
//
// ANTI-LEERLAUF wie ueberall am Dock: cases_total > 0 ist Pflicht (ConformanceResult::passed fordert es),
// und die erste Zusicherung ist die Existenz einer SOLL-Zeile -- ein Aufruf ohne Erwartung erzeugt also
// eine FEHLGESCHLAGENE Quote, nicht eine leere.
[[nodiscard]] inline ConformanceResult
testat_mess_deklarations_konsistenz(::comdare::cache_engine::abi::AnatomyVersionLines const* lines,
                                    std::string_view soll, std::FILE* report = nullptr) noexcept {
    ConformanceResult            r{};
    MessKonsistenzErgebnis const e = pruefe_mess_konsistenz(lines, soll);

    auto check = [&](bool ok, char const* what) noexcept {
        ++r.cases_total;
        if (ok) {
            ++r.cases_passed;
        } else if (r.first_fail == 0) {
            r.first_fail = r.cases_total;
        }
        if (report != nullptr) std::fprintf(report, "    %s: %s\n", what, ok ? "OK" : "FAIL");
    };

    // Die fuenf Zusicherungen spiegeln die Ablehnungsklassen; jede spaetere ist nur dann WAHR, wenn die
    // frueheren es sind (das Ergebnis traegt genau einen Status). Deshalb wird gegen `>=`-Schwellen der
    // Taxonomie geprueft und nicht gegen sechs unabhaengige Bedingungen.
    check(e.status != MessKonsistenzStatus::erwartung_leer, "die CEB benennt ihre einkompilierte Mess-Achse (SOLL)");
    check(e.status != MessKonsistenzStatus::erwartung_leer && e.status != MessKonsistenzStatus::stempel_symbol_fehlt,
          "die Tier-Binary exportiert den Versionierungs-Stempel (comdare_anatomy_version_lines)");
    check(e.status != MessKonsistenzStatus::erwartung_leer && e.status != MessKonsistenzStatus::stempel_symbol_fehlt &&
              e.status != MessKonsistenzStatus::stempel_layout_fremd,
          "der Stempel-POD traegt das EIGENE Layout (stamp_pod_has_entries)");
    check(e.status == MessKonsistenzStatus::zeile_abweichung || e.status == MessKonsistenzStatus::entries_widerspruch ||
              e.status == MessKonsistenzStatus::ok,
          "die Tier-Binary DEKLARIERT ueberhaupt ein Mess-Tooling (nicht-leer)");
    check(e.status == MessKonsistenzStatus::ok, "DEKLARATION == ERWARTUNG (Vertrag CEB <-> Tier-Binary)");

    if (report != nullptr) std::fprintf(report, "    (info) %s\n", mess_konsistenz_meldung(e).c_str());
    return r;
}

[[nodiscard]] inline ConformanceResult testat_mess_deklarations_konsistenz(anatomy_loader::AnatomyModuleHandle const& h,
                                                                           std::string_view soll,
                                                                           std::FILE*       report = nullptr) noexcept {
    return testat_mess_deklarations_konsistenz(h.version_lines(), soll, report);
}

} // namespace comdare::cache_engine::builder::pruef_dock
