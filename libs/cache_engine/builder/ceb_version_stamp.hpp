#pragma once
// builder/ceb_version_stamp.hpp -- G2-1/A5 (Lager-Gate, A8-Erfuellung fuer die CEB): der CEB-Selbst-Stempel. Die CEB
// (CacheEngineBuilder) hat KEINE Gesamt-Version (der Planer schon: planner_version.hpp). Ihre Provenienz ist ihre
// EINKOMPILIERTE Mess-WAHL: das Mess-Array [Xa.Ya.Za, ...] je Tooling, das diese CEB real traegt, X.Y.Z-gerendert
// -- DIESELBE Form wie die Tier-Binary-measurement_stamp_line (keine zweite Wahrheit / keine Drift).
//
// ------------------------------------------------------------------------------------------------
// M-1/D-4 (06.08.2026): DER SCHLUESSEL RENDERT DIE WAHL, NICHT MEHR DAS ANGEBOT
// ------------------------------------------------------------------------------------------------
// BIS HIERHER stand in genau diesem Kopf das Wort "Mess-ANGEBOT", und der Code hielt sich daran: die
// consteval-Schleife lief ueber die KOMPLETTE kMeasurementToolingRegistry, ohne jede Filterung. Damit hing
// kCebFingerprint an NICHTS, was COMDARE_MEASUREMENT_COMBO_CT kennt.
//
// AM OBJEKT GEMESSEN (Vorher-Stand b9fd81ff, Probe-TU gegen diesen Header, vier Uebersetzungen mit
// -DCOMDARE_MEASUREMENT_COMBO_CT="[wallclock]" / "[macro]" / "[micro]" / "[all]"):
//     [wallclock] -> sha512=004251f467c004a88f...f76c8d8
//     [macro]     -> sha512=004251f467c004a88f...f76c8d8
//     [micro]     -> sha512=004251f467c004a88f...f76c8d8
//     [all]       -> sha512=004251f467c004a88f...f76c8d8
// Vier verschieden EINKOMPILIERTE CEBs, EIN Schluessel. Das ist eine Injektivitaets-Verletzung und bricht
// Owner-KERN F6 ("die gleiche binary auf der selben Maschine mit den selben Messachsen liefert identische
// Ergebnisse") auf der CEB-Ebene: der Schluessel, der eine Bestandslog-Reservierung der emittierenden CEB
// zuordnet (ceb_key_sha512), konnte zwei messsystem-verschiedene CEBs nicht auseinanderhalten.
// Plan-Soll dagegen, Paragraf 58-V (LEDGER:3120): ein Mess-Array "je EINKOMPILIERTER Mess-Achse".
//
// AB HIER: die Zeile wird aus der EINKOMPILIERTEN Combo-Legende gerendert. Der no-define-Fall ist "[all]"
// (die CEB hat keine spezifische Mess-Achse einkompiliert, ihre Identitaet ist die Vollmenge) -- und damit
// BYTE-IDENTISCH zum Vor-D-4-Stand. Der gesamte Alt-Bestand behaelt seinen ceb_key_sha512; siehe die
// BESTANDSLOG-BILANZ weiter unten.
//
// ------------------------------------------------------------------------------------------------
// DIE ODR-FALLE, DIE D-4 ERST GEFAEHRLICH MACHT -- UND WIE SIE HIER GESCHLOSSEN IST
// ------------------------------------------------------------------------------------------------
// kCebMeasurementStampArray und kCebFingerprint sind `inline constexpr`, also Entitaeten mit EXTERNER
// Bindung. Sobald ihr Wert am Praeprozessor haengt, gilt: zwei TUs desselben Programms, die den Header mit
// UNTERSCHIEDLICHEM COMDARE_MEASUREMENT_COMBO_CT sehen, definieren dieselbe Entitaet verschieden -> ODR-
// Verstoss, IFNDR. Der Linker nimmt eine Fassung; der CEB-Log-Kopf koennte dann etwas anderes ausgeben als
// die Bestandslog-Zelle schreibt. Das waere GENAU die Krankheit, die D-1/D-4 heilen: ein Stempel, der etwas
// anderes behauptet als das, was wirkt -- nur diesmal still im Linker.
//
// GEMESSENE LAGE (Stand b9fd81ff): der Header hat GENAU DREI Uebersetzungseinheiten --
//     libs/cache_engine/profile_facade/profile_run_facade.cpp:35   (hatte das Define, PRIVATE am Target)
//     apps/cache_engine_builder/main.cpp:35                        (hatte es NICHT -- 0 MEASUREMENT-Treffer
//                                                                   in apps/cache_engine_builder/)
//     tests/unit/test_m_w12_stamp_bausteine.cpp:19                 (hatte es NICHT)
// Gegenprobe ueber den super-Baum (alles ausser dem ce-Submodul): 0 weitere Einbinder.
//
// GESCHLOSSEN MIT ZWEI MASSNAHMEN, die zusammen fail-CLOSED sind:
//   (1) EINE EMISSIONSSTELLE. Das Define entsteht nicht mehr an einem einzelnen Target, sondern an dem
//       INTERFACE-Ziel comdare_measurement_combo_ct (libs/cache_engine/profile_facade/CMakeLists.txt).
//       Wer den Header uebersetzt, LINKT dieses Ziel -- und bekommt damit denselben Wert wie jede andere TU
//       desselben Programms. Eine Usage-Requirement statt einer Abschrift.
//   (2) EINE VERDRAHTUNGS-WACHE. Dasselbe Ziel setzt IMMER (auch bei leerer Combo) das Marker-Makro
//       COMDARE_MEASUREMENT_COMBO_CT_WIRED. Der #error unten macht aus dem stillen ODR-Verstoss einen
//       lauten Uebersetzungsfehler: eine TU, die den Header ohne die Verdrahtung zieht, BAUT NICHT.
//       Ohne (2) waere (1) eine Absichtserklaerung -- ein viertes Target koennte den Header morgen wieder
//       ohne das Ziel einbinden, und niemand merkte es.
//
// ------------------------------------------------------------------------------------------------
// WARUM ES KEIN CEB-SEITIGES SKIP-GATE GIBT (Auftrag Punkt 3 -- begruendet, nicht verschwiegen)
// ------------------------------------------------------------------------------------------------
// kCebFingerprint ist Provenienz, KEIN Wiederverwendungs-Kriterium, und darf auch keines werden:
//   (a) ER IST DAFUER KONSTRUKTIV UNGEEIGNET -- und zwar ABSICHTLICH. Die Kommentar-Wachen unten
//       (W10-C3, O-2/C-2) halten fest, dass dieser Fingerprint die System-Zellwerte, das Toolchain-Glied
//       und das bvset-Glied BEWUSST NICHT traegt ("die CEB ist KEIN Tier-Binary"). Ein Skip-Gate auf einem
//       Fingerprint, der Toolchain und Zelle absichtlich weglaesst, waere fail-OPEN: derselbe Schluessel
//       bei anderem Compiler oder anderer ISA. Fail-open an einer Identitaets-Naht ist schlimmer als kein
//       Gate -- das ist die Lehre des D-1-Befunds in der Gegenrichtung.
//   (b) ES GIBT NICHTS ZU UEBERSPRINGEN. Alles, was die CEB PRODUZIERT, ist bereits gegatet, und zwar von
//       Gattern, die vollstaendig sind: die Tier-.so ueber das Preimage-Glied [3] (anatomy_fingerprint.hpp,
//       kAnatomyFingerprintGliedCount = 9 seit R-3) mit dem fail-closed dll_is_current (build_orchestrator.hpp),
//       der Objekt-Cache ueber "+mtool=" (artifact_transport/artifact_cache.hpp). Eine Mess-Achsen-
//       Aenderung erzwingt heute schon den Tier-Neubau. Ein zweites Gatter ueber dieselbe Information
//       waere kein Gewinn, sondern eine ZWEITE WAHRHEIT, die der ersten widersprechen kann.
//   (c) DIE CEB SELBST BAUT NINJA. Das Define steht auf der Compile-Kommandozeile; ein geaenderter
//       Cache-Wert fuehrt zu einer geaenderten Kommandozeile, und sowohl Ninja als auch ccache schluesseln
//       darauf. Ein Fehl-Treffer ist auf diesem Weg nicht konstruierbar -- der offizielle Weg genuegt.
// FOLGE, die daraus WIRKLICH faellt: die drei Konsumenten bleiben, was sie sind -- Log-Kopf
// (apps/cache_engine_builder/main.cpp), --version, und die Bestandslog-Zelle ceb_key_sha512. Was sie ab
// jetzt koennen und vorher nicht konnten: zwei messsystem-verschiedene CEBs auseinanderhalten.
// GEMESSEN dazu: ceb_key_sha512 hat heute NULL Lese-Stellen -- die Suche nach einem Vergleich auf dem Feld
// liefert 0 Treffer in libs/, apps/ und tests/ (Nenner-Gegenprobe: dieselbe Suche findet die Schreib-Stelle
// profile_run_facade.cpp und den Emitter/Parser in bestandslog_document.hpp). Das Feld ist Provenienz, wie
// dieser Kopf es beschreibt, und kein Schluessel, auf dem etwas nachschlaegt.
//
// ------------------------------------------------------------------------------------------------
// EHRLICHE GRENZE: PERMUTIERTE LEGENDEN UEBERUNTERSCHEIDEN
// ------------------------------------------------------------------------------------------------
// "[wallclock,micro]" und "[micro,wallclock]" liefern VERSCHIEDENE Schluessel (gemessen: 250be8b2... bzw.
// a30fe495...), obwohl die BAU-Seite fuer beide dasselbe Kompilat erzeugt: mess_achsen_defines()
// (profile_facade/mess_achsen_naht.hpp) bildet die Legende auf eine MENGE ab und emittiert in
// Registry-Reihenfolge. Zwei identisch gebaute CEBs bekaemen also zwei Schluessel.
// WARUM DAS TROTZDEM SO BLEIBT: die Ordnung kommt nicht von hier, sondern vom Runtime-Renderer
// abi::measurement_stamp_line(span), der die Eingabe-Reihenfolge haelt -- und derselbe Renderer stempelt
// das TIER-Preimage-Glied [3]. Wuerde dieser Zwilling hier kanonisch nach Registry-Reihenfolge sortieren,
// haetten CEB-Stempel und Tier-Stempel bei permutierter Legende verschiedene Zeilen: genau die Drift, die
// der O-8-Schritt-12-Guard verhindert. Die Ueberunterscheidung ist damit KEIN neuer Defekt dieser Scheibe,
// sondern eine Eigenschaft der gemeinsamen Legenden-Ordnung -- und sie ist fail-CLOSED (zwei Schluessel fuer
// eine Sache kostet nichts, solange niemand darauf nachschlaegt, s.o.; ein Schluessel fuer zwei Sachen
// dagegen war der Defekt D-4). Eine Kanonisierung muesste BEIDE Zwillinge zugleich erfassen und ist damit
// ein Byte-Ereignis am Tier-Preimage -- also eine eigene Scheibe mit Owner-Entscheid, kein Nebenprodukt.
//
// ------------------------------------------------------------------------------------------------
// R-3 (07.08.2026) -- FORMAT-BUMP 3 -> 4: kCebFingerprint BEWEGT SICH. DEKLARIERT, NICHT STILL.
// ------------------------------------------------------------------------------------------------
// kCebFingerprintArrayFor rechnet ueber abi::anatomy_fingerprint_hex und damit ueber das Format-Glied [0]
// und die Glied-ANZAHL. R-3 haengt ein NEUNTES Preimage-Glied an (abi/mess_gates_glied.hpp: der
// Praeprozessor-Zustand der Mess-Gates einer Uebersetzungseinheit) und bumpt fingerprint_format 3 -> 4.
// FOLGE, gemessen und im selben Commit als Test-Pin nachgezogen (test_d4_ceb_schluessel_wahl):
//     [all]  vor R-3 (Format 3):        004251f467c004a88f...f76c8d8
//     [all]  ab  R-3 (Format 4):        db7bac00b3de6eef05...aa832234
//     [all]  ab  FLAG-GRAMMATIK v2:     9f8802514f7a5e5ee4...6f5c4fe8
// DIESE CEB REICHT DAS NEUE GLIED BEWUSST NICHT (der Aufruf unten bleibt die 3-arg-Form, das Glied kommt
// als LEERER Default): sie ist KEIN Tier-Binary -- dieselbe Sache wie bei den Zellwerten und den
// Toolchain-/bvset-Gliedern. Ihre Identitaet ist ihre einkompilierte Mess-WAHL, nicht der Gate-Zustand
// ihrer eigenen Uebersetzung. Ein kMessGatesTuGlied hier haette ausserdem eine Entitaet mit EXTERNER
// Bindung (kCebFingerprint) an einen TU-abhaengigen Wert gehaengt -- exakt die ODR-Fehlerklasse, gegen die
// die Verdrahtungs-Wache weiter unten gebaut ist.
// WARUM TRAGBAR: ceb_key_sha512 hat GEMESSEN null Lese-Stellen (s. unten) -- das Feld ist Provenienz,
// kein Schluessel, auf dem etwas nachschlaegt. Alt-Zeilen behalten ihren alten Wert und bleiben
// auffindbar; niemand vergleicht sie gegen den neuen. Die BESTANDSLOG-BILANZ direkt darunter beschreibt
// den M-1/D-4-Stand und bleibt fuer den Alt-Bestand richtig -- sie gilt aber nicht mehr als Aussage
// "der Vollmengen-Schluessel ist unveraendert": das ist ab R-3 HISTORIE.
//
// ------------------------------------------------------------------------------------------------
// PMC ALS META-META-MESS-ACHSE (Owner 10.08.2026) -- DEKLARIERTES BYTE-EREIGNIS, NUR FUER PMC-BAUE
// ------------------------------------------------------------------------------------------------
// OWNER-WORTLAUT (verbatim): "Das PMC ist eine Meta-Meta-Mess-Achse und wird daher IN DER CEB UND DEREN
// FINGERPRINT VERBAUT, wenn damit gebaut wird. [...] Es entsteht hier WIEDER EINE PERMUTATION, weil ohne in
// die CEB eingebaute PMC, auch die Tier-Binaries keine PMC messen und sich daher der Binary Overhead
// eingebauter Messfuehler unterscheidet/reduziert, was die Ergebnisse wiederum NICHT VERGLEICHBAR macht.
// JEDE HARDWAREFORM EINER PMC AMD/INTEL IST ZU UNTERSCHEIDEN, ES SIND 2 VERSCHIEDENE HARDWARE KOMPONENTEN
// NICHT EIN PMC."
//
// WAS HIER ENTSTEHT: GENAU EIN Glied "pmc=<amd|intel>@X.Y.Z" im BESTEHENDEN Meta-Meta-Klammer-Anhang, als
// ';'-Geschwister HINTER load_framework:
//     ...;[load_framework=ycsb@1.0.0.c;pmc=amd@1.0.0.c]
// Keine neue Array-Zeile (RF-7: je Achsen-Typ EINE Zeile), keine neue Klammer-Ebene, kein Struktur-Bump --
// Owner-E2 (02.08.2026): eine Meta-Meta-Achse wird "dynamisch ans Ende der Kette in den bestehenden Zeilen
// angehaengt". Dieselbe Grammatik wie load_framework (<name>=<marker>@<version>), derselbe Renderer.
//
// NICHT Ja/Nein: ein blosses "pmc=on" kollabierte die ZWEI Hardware-Komponenten zu einer und widerspraeche
// dem Owner-Satz direkt. NICHT die Zaehler-Liste: ein Event-Set-Wechsel laeuft ueber die VERSION des
// jeweiligen Vendor-Eintrags (kPmcVendorRegistry) -- ein deklariertes Byte-Ereignis statt einer
// Permutations-Explosion ueber Event-Teilmengen.
//
// DIE GRENZE, DIE HIER NICHT UEBERSCHRITTEN WIRD (Owner 10.08.2026, verbatim): "binary_id identifiziert das
// TIER-Binary; die Varianten sind CEB-Binaries. Eine aus-/eingebaute Messeinrichtung erzeugt eine andere
// CEB, NICHT ein anderes Tier-Binary. Wer beides gleichsetzt, kommt auf einen ABI-Bump, der nicht anfaellt."
// Das Glied steht deshalb AUSSCHLIESSLICH hier und AUSDRUECKLICH NICHT in abi::measurement_stamp_line /
// abi::measurement_meta_meta_suffix. Die emittierten Tier-Quellen bleiben byte-identisch, Tier-SHA512,
// golden-CRC und binary_id unveraendert. Die Asymmetrie "CEB-Zeile != abi-Zeile" ist der KERN dieses
// Schnitts, nicht sein Schoenheitsfehler -- ein Glied in der gemeinsamen Suffix-Stelle haette 524.288
// Phantom-Binaries verdoppelt, ohne dass sich eine einzige .so aendert. Die Wache dagegen ist ein eigener
// Test (test_m_w12_stamp_bausteine, "TierSeiteFuehrtNiemalsEinPmcGlied").
//
// ADDITIVITAET, in drei Aussagen:
//   (a) OHNE die Vendor-Makros: dieser Header rendert BYTE-IDENTISCH zum Vor-10.08.-Stand. Der gesamte
//       Nicht-PMC-Bestand behaelt seinen ceb_key_sha512.
//   (b) Die Tier-Seite bewegt sich in KEINER Konfiguration (s.o.).
//   (c) DIE EINE VERSCHIEBUNG: eine CEB, die MIT PMC UND Vendor gebaut wird, stempelt kuenftig das Glied --
//       ihr kCebFingerprint weicht damit vom heutigen PMC=ON-Bestand ab. Das ist die in diesem Kopf schon
//       dreimal praezedierte Klasse "deklariertes Byte-Ereignis, golden-/binary_id-neutral" (A13-M2,
//       A13-M3/K-1, O-2/C-2, R-3). Tragbar aus demselben Grund wie dort: ceb_key_sha512 hat GEMESSEN null
//       Lese-Stellen, es ist Provenienz und kein Schluessel, auf dem etwas nachschlaegt. Alt-Records
//       behalten ihren alten Key und werden NIE umgeschrieben (BU additiv). Kein xlsx-Bestand wird
//       entwertet, keine Neumessung faellt an.
//
// UEBERGANGSZUSTAND, benannt statt verschwiegen: COMDARE_ENABLE_PMC OHNE Vendor-Makro rendert KEIN Glied.
// Das ist heute der Zustand der beiden super-Altaufrufer (.gitlab-ci.yml), die ON ohne Vendor setzen. Die
// Wurzel-CMakeLists warnt dort laut; der Flip auf FATAL gehoert in dasselbe Fenster wie der Super-Nachzug
// und ist dort als Folgepaket markiert. Solange er aussteht, ist "ON ohne Vendor" ein Bau, dessen Stempel
// die PMC-Ausstattung NICHT nennt -- fail-loud in der Konfiguration, nicht still im Byte.
//
// ------------------------------------------------------------------------------------------------
// BESTANDSLOG-BILANZ (Auftrag Punkt 4): DER ALT-BESTAND WIRD NICHT ENTWERTET
// ------------------------------------------------------------------------------------------------
// Jeder heute existierende Bestandslog-Eintrag stammt von einer CEB OHNE COMDARE_MEASUREMENT_COMBO_CT
// (die CMake-Cache-Variable COMDARE_MEASUREMENT_COMBO ist per Default leer, und der Emissionsweg loescht
// sie fuer [all]-Laeufe sogar aktiv per -U, s. experiment_plan_director.hpp F-B1). Der no-define-Zweig
// rendert weiterhin die Vollmengen-Zeile in Registry-Reihenfolge -- Byte fuer Byte dieselbe wie vorher,
// also derselbe SHA-512. GEMESSEN: 004251f4...f76c8d8 vor und nach dieser Aenderung.
// => KEINE Entwertung, KEINE Migration, KEIN Loeschen von Messdaten. Neue Schluessel entstehen
//    AUSSCHLIESSLICH fuer CEBs, die mit einer spezifischen Combo gebaut werden -- und von denen existiert
//    im Bestand kein einziger Eintrag. Die Aenderung ist damit rein ADDITIV im Sinne der Doktrin
//    "Messdaten nie loeschen": der Alt-Bestand behaelt seinen Schluessel und bleibt auffindbar.
//
// ------------------------------------------------------------------------------------------------
// HISTORIE (unveraendert gueltig)
// ------------------------------------------------------------------------------------------------
// O-8 Schritt 12 (Nachzug zu Schritt 9): die Zeile fuehrt jetzt ebenfalls das load_framework-Segment. Schritt 9
// hatte es nur in abi::measurement_stamp_line eingezogen und dabei DIESEN dritten Ableitungsweg uebersehen --
// die Folge war genau die Drift, die der Kopf hier ausschliesst, und der Drift-Guard in
// test_m_w12_stamp_bausteine (A5CebVersionStamp..., "EINE Wahrheit") wurde dadurch rot. Er ist die Wache, die
// den Fehler gefunden hat; abgeschwaecht wurde er nicht -- D-4 hat ihn im Gegenteil GESCHAERFT: er vergleicht
// nicht mehr gegen die Vollmenge, sondern gegen den Renderer AN DER EINKOMPILIERTEN LEGENDE, und zusaetzlich
// ueber ALLE sieben nicht-leeren Tooling-Teilmengen.
//
// A13-M2 (Owner-Entscheid E2 + Antwort Q1 vom 02.08.2026): der ZWILLING wird SYMMETRISCH nachgezogen --
// load_framework steht nicht mehr vorne, sondern als KLAMMER-ANHANG ANS ENDE der Mess-Zeile
// ("measurement_tooling=...;[load_framework=ycsb@1.0.0.c]"). Genau dieser Header ist der von O-8 Schritt 12
// dokumentierte DRITTE Ableitungsweg; wer ihn beim Umbau der Mess-Ordnung vergisst, bekommt exakt dieselbe
// Drift zurueck. Die Klammer-Zeichen kommen aus abi::kMetaMetaGroupOpen/Close (EINE Wahrheit ihrer
// Schreibweise), die Ordnung aus diesem Kommentar -- und der Drift-Guard beweist beides.
// Dazu eine eigene SHA-512-Provenienz-Zeile ueber diese Mess-Array-Zeile, via anatomy_fingerprint_hex mit leeren
// organ/system-Anteilen ("","",mess) -- die EINE K7b-Primitive wiederverwendet, keine zweite.
//
// Alles consteval (registry- und legenden-abgeleitet, Compile-Zeit-konstant); nur die Ausgabe-Formatierung
// (ceb_version_stamp()) ist Runtime. Ausgabe im CEB-Log-Kopf (apps/cache_engine_builder).

#include <cache_engine/abi/anatomy_fingerprint.hpp>    // anatomy_fingerprint_hex (consteval SHA-512)
#include <cache_engine/abi/anatomy_stamp_entries.hpp>  // S-1: der EINE Zeilen-Scanner (CebStempel-Mess-Pflicht)
#include <cache_engine/abi/meta_meta_stamp_suffix.hpp> // A13-M2: kMetaMetaGroupOpen/Close (EINE Klammer-Wahrheit)
#include <cache_engine/abi/stempel_basis.hpp>          // S-1: StempelBasis/StempelVertrag + ist_stempel_baustein
#include <cache_engine/measurement/algo_semver.hpp>    // parse_algo_semver + render_algo_semver (die EINE Grammatik)
#include <cache_engine/measurement/measurement_framework_registry.hpp> // O-8 Schritt 12: load_framework-Segment
#include <mess_axes/measurement_tooling_registry.hpp>   // kMeasurementToolingRegistry (Single-Source)
#include <cache_engine/measurement/pmc_vendor_registry.hpp>            // 10.08.2026: die ZWEI PMC-Hardware-Komponenten

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

// -- M-1/D-4 VERDRAHTUNGS-WACHE (Massnahme (2) des ODR-Abschnitts im Kopf) ------------------------------
// Sie ist KEIN Stil-Check. Ab D-4 haengt der Wert von kCebFingerprint -- einer Entitaet mit externer
// Bindung -- am Praeprozessor. Eine TU, die den Header ohne die eine Emissionsstelle uebersetzt, wuerde
// eine ABWEICHENDE Definition derselben Entitaet erzeugen; der Verstoss waere still (IFNDR) und wuerde
// sich erst als widerspruechlicher Log-Kopf vs. Bestandslog-Zelle zeigen. Deshalb bricht der Bau hier.
#ifndef COMDARE_MEASUREMENT_COMBO_CT_WIRED
#error "fehlerklasse=konfiguration_widerspruch: diese Uebersetzungseinheit zieht \
builder/ceb_version_stamp.hpp, ohne an die EINE Emissionsstelle der einkompilierten Mess-Combo \
verdrahtet zu sein. Das Ziel comdare_measurement_combo_ct \
(libs/cache_engine/profile_facade/CMakeLists.txt) traegt COMDARE_MEASUREMENT_COMBO_CT und diesen \
Marker; ohne es bekaeme kCebFingerprint in dieser TU einen ANDEREN Wert als in den uebrigen TUs \
desselben Programms (ODR-Verstoss, still). Abhilfe: \
target_link_libraries(<ziel> PRIVATE comdare_measurement_combo_ct)."
#endif

namespace comdare::cache_engine::builder {

/// CebComboLegend<N> -- der Traeger der Combo-Legende als NICHT-TYP-TEMPLATE-PARAMETER.
///
/// WOZU EIN TRAEGERTYP: bis D-4 war die Mess-Zeile eine argumentlose consteval-Funktion ueber die Registry.
/// Die Zeile haengt jetzt an einer Legende -- und diese Legende muss eine COMPILE-ZEIT-Groesse bleiben
/// (Stufen-Doktrin: die zweite Stufe ist Compile-Time-EINBAU, kein Laufzeit-Schalter). Ein string_view als
/// NTTP geht nicht (kein struktureller Typ mit Zeiger auf Nicht-statisches); ein char-Array in einem
/// strukturellen Aggregat geht.
///
/// DER EIGENTLICHE GEWINN IST DER BISS: weil die Zeile ueber DIESEN Parameter parametrisiert ist, kann EINE
/// Uebersetzungseinheit den Schluessel fuer BELIEBIG VIELE Combos ausrechnen und ihre Injektivitaet
/// nachweisen. Ohne den Traegertyp waere der Nachweis nur ueber N getrennte Baue fuehrbar -- also praktisch
/// gar nicht, und damit unbewacht.
template <std::size_t N>
struct CebComboLegend {
    char v[N]{}; // NOLINT(*-avoid-c-arrays) -- strukturelle NTTP-Form verlangt das rohe Array

    consteval CebComboLegend(char const (&s)[N]) noexcept { // NOLINT(*-explicit-constructor)
        for (std::size_t i = 0; i < N; ++i) v[i] = s[i];
    }

    /// Die Legende ohne das abschliessende '\0'.
    [[nodiscard]] constexpr std::string_view view() const noexcept { return std::string_view{v, N - 1}; }
};

namespace detail {

// -- PMC-META-META-ACHSE (10.08.2026): die EINE Lesung der Vendor-Makros auf der CEB-Seite ---------------
//
// DREI ZUSTAENDE, und nur drei: kein PMC / PMC+amd / PMC+intel. Ein VIERTER Zustand ("beide Vendor-Makros
// gesetzt") ist keine Hardware-Lage, sondern ein Konfigurations-Widerspruch -- eine CEB wird auf GENAU
// EINEM Host gebaut. Er bricht deshalb den Bau, statt still einen der beiden zu gewinnen.
#if defined(COMDARE_ENABLE_PMC) && defined(COMDARE_PMC_VENDOR_AMD) && defined(COMDARE_PMC_VENDOR_INTEL)
#error "fehlerklasse=konfiguration_widerspruch: COMDARE_PMC_VENDOR_AMD UND COMDARE_PMC_VENDOR_INTEL sind \
BEIDE gesetzt. Eine CEB wird auf GENAU EINEM Host gebaut und traegt daher GENAU EINE PMC-Hardwareform \
(Owner 10.08.2026: 'ES SIND 2 VERSCHIEDENE HARDWARE KOMPONENTEN NICHT EIN PMC'). Abhilfe: genau ein \
-DCOMDARE_PMC_VENDOR=<amd|intel> an der Wurzel-Konfiguration."
#endif

/// COMDARE_CEB_HAT_PMC_GLIED -- das EINE Praedikat, an dem Laengen-Rechnung und Renderer haengen. Zwei
/// getrennte #if-Bedingungen waeren die Falle, die dieser Header an anderer Stelle schon beschreibt: eine
/// zu grosse Puffer-Zusage bei zu kurzem Render (oder umgekehrt) ist ein abgeschnittener Stempel und damit
/// eine stille Fehl-Identitaet. Es gibt deshalb genau EINE Bedingung, und beide Seiten lesen sie.
#if defined(COMDARE_ENABLE_PMC) && (defined(COMDARE_PMC_VENDOR_AMD) || defined(COMDARE_PMC_VENDOR_INTEL))
#define COMDARE_CEB_HAT_PMC_GLIED 1
#else
#define COMDARE_CEB_HAT_PMC_GLIED 0
#endif

namespace detail_pmc {
#if COMDARE_CEB_HAT_PMC_GLIED
/// Die einkompilierte Hardwareform, aus der REGISTRY geholt -- nicht aus einem Literal an dieser Stelle.
[[nodiscard]] consteval ::comdare::cache_engine::measurement::PmcVendorInfo const& ceb_pmc_vendor() {
    using ::comdare::cache_engine::measurement::pmc_vendor_info;
    using ::comdare::cache_engine::measurement::PmcVendor;
#if defined(COMDARE_PMC_VENDOR_AMD)
    return pmc_vendor_info(PmcVendor::Amd);
#else
    return pmc_vendor_info(PmcVendor::Intel);
#endif
}
#endif
} // namespace detail_pmc

/// consteval: die gerenderte Laenge EINER Version -- ueber DENSELBEN Renderer, den auch der Laufzeit-Weg
/// benutzt (measurement::render_algo_semver).
///
/// HIER STAND VOR DER FLAG-GRAMMATIK v2 EIN ZWEITER RENDERER, und das war der Kern der Naht: eine
/// Laengen-Rechnung (ceb_digits + ceb_flag_len) und eine Zeichen-Ausgabe (put_num + Flag-Schwanz), beide
/// von Hand symmetrisch zu abi::measurement_stamp_line gehalten. Mit einem SKALAREN Flag ("c" bzw. "ce")
/// war das noch nachrechenbar -- eine Zahl zwischen 0 und 2. Mit einer Flag-LISTE, die Klammern,
/// Rekursion und mehrere Basen kennt, waere die Nachrechnung ein zweiter Parser gewesen, und ihre Drift
/// haette sich als FALSCHE PUFFERLAENGE gezeigt, also als abgeschnittener Stempel und damit als stille
/// Fehl-Identitaet. Deshalb rechnet und rendert ab jetzt beides derselbe Code.
[[nodiscard]] consteval std::size_t ceb_version_len(std::string_view rohe_version) noexcept {
    return ::comdare::cache_engine::measurement::render_algo_semver(
               ::comdare::cache_engine::measurement::parse_algo_semver(rohe_version))
        .len;
}
/// consteval: Laenge des load_framework-Segments "load_framework=<id>@<version>" (ohne Trenner/Klammern).
/// O-8 Schritt 12: dieselbe Quelle und dieselbe Form wie in abi::measurement_stamp_line (Schritt 9).
[[nodiscard]] consteval std::size_t ceb_load_framework_segment_len() noexcept {
    using ::comdare::cache_engine::measurement::kMeasurementFrameworkRegistry;
    auto const& fw = kMeasurementFrameworkRegistry[0];
    return std::string_view{"load_framework="}.size() + fw.id.size() + 1 // '@'
           + ceb_version_len(fw.version);
}

/// consteval: Laenge des PMC-Segments INKLUSIVE seines fuehrenden ';' -- also der GESAMTE Zuwachs, den das
/// Glied im Klammer-Anhang kostet. 0, wenn diese CEB ohne PMC-Vendor gebaut wird (dann ist der Anhang
/// byte-identisch zum Vor-10.08.-Stand). Der fuehrende Trenner steckt bewusst IN dieser Zahl: so gibt es
/// keine zweite Stelle, an der jemand ihn vergessen oder doppelt zaehlen kann.
[[nodiscard]] consteval std::size_t ceb_pmc_segment_len() noexcept {
#if COMDARE_CEB_HAT_PMC_GLIED
    auto const& v = detail_pmc::ceb_pmc_vendor();
    return 1                                                                // ';'
           + ::comdare::cache_engine::measurement::kPmcStampName.size() + 1 // '='
           + v.id.size() + 1                                                // '@'
           + ceb_version_len(v.version);
#else
    return 0;
#endif
}

/// Kapazitaets-Deckel der Tooling-Liste. Grosszuegig gegen die drei Registry-Eintraege, weil eine Legende
/// Tokens WIEDERHOLEN darf ("[macro,macro]") -- der Runtime-Renderer tut das ebenfalls, und der Zwilling
/// muss ihn spiegeln koennen, statt vorher stillschweigend zu kuerzen.
inline constexpr std::size_t kCebMaxToolingTokens = 16;

/// Die aus einer Legende abgeleitete Tooling-Folge -- IN DER REIHENFOLGE, IN DER GERENDERT WIRD.
struct CebToolingList {
    std::array<std::string_view, kCebMaxToolingTokens> ids{};
    std::size_t                                        count{0};
};

/// ceb_tooling_list(legend) -- DIE EINE ZERLEGUNG. Beide Verbraucher (Laengen-Rechnung und Renderer)
/// konsultieren sie; keiner parst selbst.
///
/// WARUM DAS DIE ZENTRALE FALLE DIESER SCHEIBE IST: der Vor-D-4-Stand berechnete die Laenge in
/// ceb_measurement_stamp_len() und rendert in ceb_measurement_stamp_array() -- in ZWEI getrennten
/// Schleifen ueber dieselbe Registry. Solange beide unbedingt ueber alles liefen, konnten sie nicht
/// auseinanderlaufen. Ein Filter in NUR der Render-Schleife haette ein zu grosses Array mit Fuell-Nullen
/// geliefert, und der Fingerprint haette ueber Nullbytes gehasht -- eine stille Fehl-Identitaet. Deshalb
/// gibt es ab hier genau EINE Zerlegung und zwei Verbraucher, nicht zwei Zerlegungen.
///
/// SPIEGEL-VERTRAG zu abi::measurement_stamp_line_from_combo_legend (der Runtime-Weg):
///   -- []-Klammern strippen, an ',' trennen, leere Tokens ueberspringen: identisch.
///   -- innen leer oder "all" -> die VOLLE Vollmenge in REGISTRY-Reihenfolge (Section 64-D1-B): identisch
///      zu measurement_stamp_line_full_set().
///   -- sonst: die Tokens in EINGABE-Reihenfolge (nicht Registry-Reihenfolge). Das ist die Ordnung, die
///      measurement_stamp_line(span) faehrt; eine Registry-Sortierung hier waere eine Drift, die erst bei
///      einer permutierten Legende ("[micro,wallclock]") sichtbar wuerde -- also spaet und leise.
///
/// EINE BEWUSSTE ABWEICHUNG vom Runtime-Renderer, benannt: der rendert eine UNBEKANNTE id mit dem
/// Sentinel @0.0.0 weiter. Hier wirft sie. Begruendung: dies ist der IDENTITAETS-Weg der CEB, und ein
/// Tippfehler in der Legende darf keine stille Ersatz-Identitaet erzeugen. Die Bau-Seite derselben
/// Legende wirft ohnehin (profile_facade/mess_achsen_naht.hpp mess_tooling_menge_from_legend); eine CEB
/// mit unbekanntem Tooling ist kein baubarer Zustand, sie soll nur FRUEHER und COMPILE-HART scheitern.
/// Der Wurf in einer consteval-Funktion ist die im Projekt etablierte Fail-Loud-Form (Praezedenz:
/// abi/anatomy_fingerprint.hpp require_injizierter_glied_wert) -- er macht den Ausdruck zu keinem
/// konstanten Ausdruck mehr und nennt sich dabei beim Namen.
[[nodiscard]] consteval CebToolingList ceb_tooling_list(std::string_view legend) {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    CebToolingList   l{};
    std::string_view inner = legend;
    if (inner.size() >= 2 && inner.front() == '[' && inner.back() == ']') inner = inner.substr(1, inner.size() - 2);
    // Section 64-D1-B: [all] / leer-innen == die VOLLE Vollmenge. Der no-define-Fall erreicht diesen Zweig
    // ueber kCebCtLegend == "[all]" -- deshalb ist der Bestand byte-stabil.
    if (inner.empty() || inner == "all") {
        for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) l.ids[l.count++] = kMeasurementToolingRegistry[i].id;
        return l;
    }
    for (std::size_t start = 0; start <= inner.size();) {
        std::size_t const comma = inner.find(',', start);
        std::size_t const end   = comma == std::string_view::npos ? inner.size() : comma;
        if (end > start) {
            std::string_view const tok   = inner.substr(start, end - start);
            bool                   known = false;
            for (std::size_t i = 0; i < kMeasurementToolingCount; ++i)
                if (kMeasurementToolingRegistry[i].id == tok) known = true;
            if (!known)
                throw std::invalid_argument(
                    std::string{"fehlerklasse=konfiguration_widerspruch: unbekanntes Mess-Tooling '"} +
                    std::string{tok} + "' in der einkompilierten Combo-Legende '" + std::string{legend} +
                    "' -- gueltig sind ausschliesslich die ids aus kMeasurementToolingRegistry");
            if (l.count == kCebMaxToolingTokens)
                throw std::length_error("fehlerklasse=konfiguration_widerspruch: die einkompilierte Combo-Legende "
                                        "fuehrt mehr Tooling-Tokens als kCebMaxToolingTokens -- der Deckel ist "
                                        "eine Kapazitaets-Zusage der CEB-Stempel-Zeile, kein Zufallswert");
            l.ids[l.count++] = tok;
        }
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    if (l.count == 0)
        throw std::invalid_argument(std::string{"fehlerklasse=konfiguration_widerspruch: die einkompilierte "
                                                "Combo-Legende '"} +
                                    std::string{legend} +
                                    "' waehlt kein einziges Tooling aus -- eine CEB ohne jedes Mess-Tooling ist "
                                    "kein baubarer Zustand (die Vollmenge heisst '[all]', nicht '')");
    return l;
}

/// consteval: Laenge der gerenderten Mess-Array-Zeile zu EINER bereits zerlegten Tooling-Folge
/// "measurement_tooling=<id>@X.Y.Z;...;[load_framework=<id>@X.Y.Z]" (ohne '\0').
/// A13-M2: der Klammer-Anhang kostet ';' + '[' + Segment + ']' == Segment + 3 Zeichen.
[[nodiscard]] consteval std::size_t ceb_measurement_stamp_len(CebToolingList const& l) noexcept {
    using ::comdare::cache_engine::measurement::tooling_version_for_id;
    std::size_t n = 0;
    for (std::size_t i = 0; i < l.count; ++i) {
        if (i != 0) ++n; // ';'
        n += std::string_view{"measurement_tooling="}.size();
        n += l.ids[i].size();
        ++n; // '@'
        n += ceb_version_len(tooling_version_for_id(l.ids[i]));
    }
    // Leer-Semantik wie in Schritt 9: der Meta-Meta-Anhang entsteht NUR, wenn ueberhaupt Tooling da ist
    // (eine sonst leere Mess-Zeile bleibt leer und wird nie zu einem einsamen Rahmen-Segment). Der
    // Wurf in ceb_tooling_list macht count==0 heute unerreichbar; die Bedingung bleibt trotzdem stehen,
    // damit die Leer-Semantik an der Zeile ablesbar ist und nicht nur an einer Fernwirkung.
    // 10.08.2026: das PMC-Glied ist ein ';'-GESCHWISTER von load_framework INNERHALB derselben Klammer --
    // es kostet daher nur sein eigenes Segment (der Trenner steckt schon in ceb_pmc_segment_len()) und
    // KEINE weiteren Rahmen-Zeichen. Ohne Vendor-Makro ist die Zahl 0 und diese Zeile ein No-Op.
    if (l.count != 0) n += ceb_load_framework_segment_len() + ceb_pmc_segment_len() + 3; // ';' + '[' + ']'
    return n;
}

} // namespace detail

/// kCebCtLegend -- DIE EINE LESUNG des einkompilierten Mess-Combo-Makros auf der CEB-Seite.
/// Bewusst NICHT in detail: sie ist der Template-Parameter, an dem die vier oeffentlichen Konstanten unten
/// spezialisiert sind, und der Biss (tests/unit/test_d4_ceb_schluessel_wahl.cpp) muss zeigen koennen, dass
/// genau diese Spezialisierung -- und keine zweite Ableitung -- den Log-Kopf und die Bestandslog-Zelle speist.
#ifdef COMDARE_MEASUREMENT_COMBO_CT
// cppcheck-ADJAZENZ-FALLE (Fallen-Kanon 05.08., lint:static 14673): das Makro darf NIE zwischen zwei
// String-Literalen stehen. Hier steht es allein in den Klammern eines Aggregat-Initialisierers.
inline constexpr auto kCebCtLegend = CebComboLegend{COMDARE_MEASUREMENT_COMBO_CT};
#else
/// KEIN Define == "diese CEB hat keine spezifische Mess-Achse einkompiliert" == ihre Identitaet IST das
/// volle Angebot. Genau dieser Zweig haelt den Alt-Bestand byte-stabil (s. BESTANDSLOG-BILANZ im Kopf).
inline constexpr auto kCebCtLegend = CebComboLegend{"[all]"};
#endif

/// ceb_measurement_stamp_array_for<L>() -- die gerenderte Mess-Array-Zeile ZU EINER LEGENDE, als fixed char
/// array (+ '\0'). X.Y.Z via parse_algo_semver ueber tooling_version_for_id -> keine Drift zur
/// Tier-Binary-measurement_stamp_line. Single-Source der Zerlegung: detail::ceb_tooling_list.
/// A13-M2: Reihenfolge == die der Legende (bzw. Registry-Reihenfolge fuer [all]), danach der geklammerte
/// load_framework-Anhang (Owner-E2/Q1).
template <CebComboLegend L>
[[nodiscard]] consteval auto ceb_measurement_stamp_array_for() {
    using ::comdare::cache_engine::measurement::tooling_version_for_id;
    constexpr auto        kList = detail::ceb_tooling_list(L.view());
    constexpr std::size_t kLen  = detail::ceb_measurement_stamp_len(kList);

    std::array<char, kLen + 1> out{};
    std::size_t                p   = 0;
    auto                       put = [&](std::string_view s) {
        for (char const c : s) out[p++] = c;
    };
    /// FLAG-GRAMMATIK v2: die Version kommt aus DEM EINEN Renderer. Der Puffer wird benannt, weil .view()
    /// in ihn hineinzeigt (dieselbe Lebenszeit-Falle wie in measurement::render_algo_semver).
    auto put_version = [&put](std::string_view rohe_version) {
        ::comdare::cache_engine::measurement::RenderedAlgoSemVer const r =
            ::comdare::cache_engine::measurement::render_algo_semver(
                ::comdare::cache_engine::measurement::parse_algo_semver(rohe_version));
        put(r.view());
    };
    for (std::size_t i = 0; i < kList.count; ++i) {
        if (i != 0) out[p++] = ';';
        put("measurement_tooling=");
        put(kList.ids[i]);
        out[p++] = '@';
        put_version(tooling_version_for_id(kList.ids[i]));
    }
    // A13-M2 (OP-3-Rueckbau, Owner-E2 "ans Ende der Kette" + Owner-Q1 Klammer-Form): das load_framework-
    // Segment steht als geklammerter Meta-Meta-Anhang AM ENDE -- dieselbe Ordnung wie in
    // abi::measurement_stamp_line. Es entsteht nur bei nicht-leerer Tooling-Menge, damit die Leer-Zeile
    // leer bleibt.
    if (kList.count != 0) {
        using ::comdare::cache_engine::measurement::kMeasurementFrameworkRegistry;
        auto const& fw = kMeasurementFrameworkRegistry[0];
        out[p++]       = ';';
        out[p++]       = ::comdare::cache_engine::abi::kMetaMetaGroupOpen;
        put("load_framework=");
        put(fw.id);
        out[p++] = '@';
        put_version(fw.version);
        // 10.08.2026 (Owner: PMC ist eine Meta-Meta-Mess-Achse): das PMC-Glied als ';'-Geschwister HINTER
        // load_framework, INNERHALB derselben Klammer -- "ans Ende der Kette" (Owner-E2), keine zweite
        // Klammer-Ebene und keine neue Zeile (RF-7). Ohne Vendor-Makro entsteht hier kein einziges Byte.
#if COMDARE_CEB_HAT_PMC_GLIED
        {
            auto const& pv = detail::detail_pmc::ceb_pmc_vendor();
            out[p++]       = ';';
            put(::comdare::cache_engine::measurement::kPmcStampName);
            out[p++] = '=';
            put(pv.id);
            out[p++] = '@';
            put_version(pv.version);
        }
#endif
        out[p++] = ::comdare::cache_engine::abi::kMetaMetaGroupClose;
    }
    out[p] = '\0';
    return out;
}

/// Die gerenderte Mess-Array-Zeile ZU EINER LEGENDE (constexpr storage) + ihre string_view (ohne '\0').
template <CebComboLegend L>
inline constexpr auto kCebMeasurementStampArrayFor = ceb_measurement_stamp_array_for<L>();
template <CebComboLegend L>
inline constexpr std::string_view kCebMeasurementStampFor{kCebMeasurementStampArrayFor<L>.data(),
                                                          kCebMeasurementStampArrayFor<L>.size() - 1};

/// consteval SHA-512-Provenienz ZU EINER LEGENDE (A8): anatomy_fingerprint_hex ueber
/// ("", "", Mess-Array-Zeile) -- die EINE K7b-Primitive wiederverwendet (leere organ/system). 128-hex.
///
/// W10-C3 -- KOMMENTAR-WACHE, DER CEB-SELBST-STEMPEL BLEIBT ZELLWERTFREI (Manager-Entscheid, Bauplan-Dossier
/// 20260803 Sektion 2/5.8): W10 vervollstaendigt die System-Zeile der TIER-Binaries um ihre System-Zellwerte
/// (OS-Familie / ISA / SIMD-Zelle). DIESER Aufruf bekommt sie AUSDRUECKLICH NICHT -- und zwar nicht aus
/// Vergesslichkeit, sondern aus der Sache heraus: die CEB ist KEIN Tier-Binary. Sie baut Tier-Binaries fuer
/// beliebige Zellen; ihre Identitaet ist ihre CODE-Identitaet (ab D-4: ihre einkompilierte Mess-WAHL), nicht
/// die Zelle eines einzelnen Bauauftrags. Ein Zellwert hier waere schlicht falsch -- er wuerde behaupten,
/// die CEB selbst sei fuer avx512 gebaut worden.
/// MECHANISCH GEDECKT: der system-Parameter dieses Aufrufs ist "" -- der Vervollstaendiger wird hier gar nicht
/// gerufen, es gibt also keinen Pfad, ueber den ein Zellwert versehentlich hereinkaeme. WER DAS AENDERN WILL,
/// aendert damit kCebFingerprint und den CEB-Log-Kopf und muss es als eigenes, deklariertes Byte-Ereignis
/// begruenden. Diese Datei ist der von O-8 Schritt 12 dokumentierte DRITTE Ableitungsweg; der Drift-Guard
/// A5CebVersionStamp in test_m_w12_stamp_bausteine haelt ihre Mess-Zeile symmetrisch zu
/// abi::measurement_stamp_line -- die System-Zeile ist hier gar nicht erst im Spiel.
/// M-1/D-4 SCHAERFT DIESE WACHE, statt sie zu lockern: gerade WEIL dieser Fingerprint Toolchain, bvset und
/// Zellwerte absichtlich weglaesst, ist er kein zulaessiges Skip-Kriterium (s. Kopf, Punkt (a)).
///
/// A13-M3/K-1: dieser Aufruf IST die im Bauplan benannte "ceb_version_stamp.hpp-Falle". Bis M2 stand hier ein
/// VIERTES Argument "" -- der merge-Slot. Waere die Signatur beim merge-Entfall naiv auf
/// (organ, system, measurement, overlay) verkuerzt worden, waere dieser Aufruf GUELTIG GEBLIEBEN und das ""
/// still vom merge- auf den Overlay-Slot gerutscht: kompiliert, Semantik verschoben, niemand merkt es. Der
/// benannte OverlayHash-Typ + die Sperr-Ueberladung machen genau das unmoeglich; der Aufruf ist auf die
/// 3-arg-Form gezogen.
///
/// O-2/C-2 (Format 2 -> 3, 05.08.2026) -- FORMAT-KONFORM UND BEWUSST LEER FUER DIE NEUEN GLIEDER. Der
/// Aufruf bleibt die 3-arg-Form; Toolchain- und bvset-Glied kommen damit als DEFAULT (leer) herein, und
/// zwar aus derselben Sache heraus wie die Zellwerte oben: die CEB ist KEIN Tier-Binary.
/// MECHANISCH GEDECKT: die 3-arg-Form kann die neuen Slots gar nicht belegen; wer sie belegen will, muss
/// die benannten Traeger-Typen explizit nennen und begruendet damit ein eigenes Byte-Ereignis.
///
/// -- E-E (07.08.2026): DIE ZUSAGE "MECHANISCH GEDECKT" WAERE HIER GERADE FALSCH GEWORDEN -------------
///
/// Der Satz oben stimmte, SOLANGE alle Schwanz-Glieder einen LEEREN Default hatten. Mit der
/// Scharfschaltung des Overlay-Glieds [7] hat sein Default einen WERT (abi::kOverlaySourceHash, der
/// Quell-Hash dieses Baums) -- die 3-arg-Form belegte den Slot ab da also SEHR WOHL, und zwar still. Die
/// Deckung war keine mehr; sie musste explizit werden. Genau deshalb steht der Traeger jetzt da.
///
/// UND WARUM ER LEER BLEIBT -- die eigentliche Sachfrage. Das Overlay-Glied traegt den Quelltext-Stand
/// des Achsen-Baums. Haenge kCebFingerprint daran, bewegte er sich bei JEDER Aenderung an irgendeiner
/// Achsen-Quelle. Das klaenge nach mehr Ehrlichkeit, zerstoerte aber genau die Zusage, fuer die dieser
/// Wert existiert: der Byte-Anker in test_d4_ceb_schluessel_wahl haelt fest, dass eine Bewegung von
/// ceb_key_sha512 "ein deklariertes Byte-Ereignis mit Owner-Entscheid" ist "und nie ein Nebenprodukt".
/// Ein Wert, der sich mit jedem Commit bewegt, KANN diese Wache nicht mehr tragen -- der Anker waere
/// dauerrot und wuerde als erstes weggeworfen. Die Entscheidung ist also nicht "weniger Information",
/// sondern "die Wache behalten": die CEB ist KEIN Tier-Binary, ihr Schluessel ist Provenienz und
/// ausdruecklich KEIN Wiederverwendungs-Kriterium (s. Kopf, Punkt (a)) -- er darf und soll ruhig liegen.
/// Die Quelltext-Identitaet, um die es dem Owner geht, sitzt dort, wo sie hingehoert: in der
/// TIER-Binary, ueber den Bau-Kanal (profile_facade/overlay_source_hash_naht.hpp).
/// FOLGE, ausdruecklich benannt: kCebFingerprint bewegt sich mit E-E NICHT. Der Aufruf unten ist
/// byte-identisch zum Vor-E-E-Stand, weil die drei genannten Traeger genau die Werte tragen, die vorher
/// die Defaults lieferten.
template <CebComboLegend L>
inline constexpr auto kCebFingerprintArrayFor = ::comdare::cache_engine::abi::anatomy_fingerprint_hex(
    "", "", kCebMeasurementStampFor<L>,
    ::comdare::cache_engine::abi::ToolchainGlied{::comdare::cache_engine::abi::kToolchainStampGlied},
    ::comdare::cache_engine::abi::BvsetGlied{::comdare::cache_engine::abi::kBuildVariantSetSignatureGlied},
    ::comdare::cache_engine::abi::OverlayHash{""});
template <CebComboLegend L>
inline constexpr std::string_view kCebFingerprintFor{kCebFingerprintArrayFor<L>.data(), 128};

/// -- DIE EINKOMPILIERTE WAHL DIESER CEB ---------------------------------------------------------------
/// Die vier Namen unten sind die Spezialisierung der Vorlagen oben an der EINEN Legende, die in DIESE CEB
/// einkompiliert ist. Sie sind Referenzen bzw. Kopien auf dieselben Entitaeten -- es gibt keinen zweiten
/// Ableitungsweg, an dem der Log-Kopf und die Bestandslog-Zelle auseinanderlaufen koennten.
inline constexpr std::string_view kCebCtComboLegend         = kCebCtLegend.view();
inline constexpr auto const&      kCebMeasurementStampArray = kCebMeasurementStampArrayFor<kCebCtLegend>;
inline constexpr std::string_view kCebMeasurementStamp      = kCebMeasurementStampFor<kCebCtLegend>;
inline constexpr auto const&      kCebFingerprintArray      = kCebFingerprintArrayFor<kCebCtLegend>;
inline constexpr std::string_view kCebFingerprint           = kCebFingerprintFor<kCebCtLegend>;

// -- S-1 (P5): DIE CEB-ERBIN DES STEMPEL-VERTRAGS ------------------------------------------------------

/// Die Teile des gesamt_stempel-Kompositums in ERBIN-genannter Reihenfolge (S-6-Sperre: die Basis kennt
/// keine Ordnung). BYTE-IDENTISCH zur bisherigen ceb_version_stamp()-Konkatenation.
[[nodiscard]] constexpr std::array<std::string_view, 4> ceb_gesamt_stempel_teile() noexcept {
    return {"ceb-measurement=", kCebMeasurementStamp, ";sha512=", kCebFingerprint};
}
inline constexpr auto kCebGesamtStempelKompositum =
    ::comdare::cache_engine::abi::stempel_kompositum<&ceb_gesamt_stempel_teile>();

/// CebStempel -- die Erbin des CRTP-Stempel-Vertrags fuer den Traeger CEB (S-1). Sie DELEGIERT auf die
/// einkompilierten Werte dieser Datei (S-1 aendert KEINEN Stempel-WERT, kein X.Y.Z-Bump).
struct CebStempel
    : ::comdare::cache_engine::abi::StempelBasis<CebStempel, ::comdare::cache_engine::abi::StempelTraeger::Ceb> {
    /// mess_zeile: die einkompilierte Mess-WAHL (M-1/D-4) -- die Identitaet der CEB.
    [[nodiscard]] static constexpr std::string_view mess_zeile() noexcept { return kCebMeasurementStamp; }

    /// system_zeile: DEKLARIERTE LUECKE -- die W10-C3-KOMMENTAR-Wache ("der CEB-Selbst-Stempel bleibt
    /// zellwertfrei", s. Fingerprint-Block oben) wird hier zur RIEGEL-KONSTANTE: die Leere ist benannt
    /// und begruendet statt still. Die FUELLUNG der System-Zeile ist der KON8-03-Bauauftrag, nicht S-1.
    [[nodiscard]] static constexpr std::string_view system_zeile() noexcept { return {}; }
    static constexpr bool                           kSystemZeileBewusstLeer = true;
    static constexpr std::string_view               kSystemZeileBewusstLeerGrund =
        "W10-C3: der CEB-Selbst-Stempel bleibt zellwertfrei -- die CEB ist KEIN Tier-Binary; die "
        "Fuellung der System-Zeile ist der KON8-03-Bauauftrag, nicht S-1";

    /// fingerprint_sha: die 128-hex-SHA-512-Provenienz der Mess-Wahl.
    [[nodiscard]] static constexpr std::string_view fingerprint_sha() noexcept { return kCebFingerprint; }

    /// gesamt_stempel: das consteval-Kompositum "ceb-measurement=<mess>;sha512=<sha>".
    [[nodiscard]] static constexpr std::string_view gesamt_stempel() noexcept {
        return kCebGesamtStempelKompositum.view();
    }

    // KEIN organ_zeile -- die ABSENZ ist der maschinelle Riegel "CEB traegt keine Organ-Achsen"
    // (KON7-07, Matrixzelle organ@CEB == VERBOTEN): wer hier eine organ_zeile() einbaut, macht den
    // static_assert direkt unter dieser Definition ROT (Absenz-Assert der Zulassungsmatrix).
};
/// DER ZWANG (S-1/P1.2): die Erbin schliesst direkt unter ihrer Definition mit dem Vertrag.
static_assert(
    ::comdare::cache_engine::abi::StempelVertrag<CebStempel, ::comdare::cache_engine::abi::StempelTraeger::Ceb>,
    "S-1: CebStempel verletzt den Stempel-Vertrag des Traegers CEB (Zulassungsmatrix "
    "kStempelZulassungsMatrix, stempel_basis.hpp) -- u.a. ist organ_zeile hier VERBOTEN (KON7-07)");

/// ceb_version_stamp() -- der CEB-Selbst-Stempel fuer den Log-Kopf/--version: die Mess-Array-Zeile + ihre SHA-512-
/// Provenienz. Runtime-String (nur Ausgabe-Formatierung); beide Bestandteile sind consteval (registry- und
/// legenden-abgeleitet). S-1 (P5): DELEGIERT byte-identisch auf das Kompositum der Erbin (dieselben zwei
/// consteval-Konstanten, derselbe Rahmen "ceb-measurement=...;sha512=...").
[[nodiscard]] inline std::string ceb_version_stamp() { return std::string{CebStempel::gesamt_stempel()}; }

} // namespace comdare::cache_engine::builder

// -- S-1 (P3): Baustein-Anbindung von Legende und Tooling-Liste UNTER ihren Definitionen (die
//    Spezialisierung gehoert in den abi-Namensraum, deshalb steht sie hier am Datei-Ende) -- OHNE
//    Basisklassen-Einbau: CebComboLegend ist ein struktureller NTTP-Traeger (eine Basis braeche die
//    NTTP-Faehigkeit), CebToolingList ein positional befuellbares Aggregat.
namespace comdare::cache_engine::abi {
template <std::size_t N>
struct ist_stempel_baustein<builder::CebComboLegend<N>> : StempelBausteinTag<StempelBausteinRolle::Legende> {};
static_assert(StempelBaustein<builder::CebComboLegend<6>>); // "[all]" traegt N == 6
template <>
struct ist_stempel_baustein<builder::detail::CebToolingList> : StempelBausteinTag<StempelBausteinRolle::ToolingListe> {
};
static_assert(StempelBaustein<builder::detail::CebToolingList>);
} // namespace comdare::cache_engine::abi
