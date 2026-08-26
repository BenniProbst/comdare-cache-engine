#pragma once
// builder/overlay_source_set.hpp -- E-E (07.08.2026): DIE EINE QUELLE der Dateimenge, ueber die das
// Overlay-Glied [7] des Tier-Fingerprints rechnet (abi/anatomy_fingerprint.hpp,
// kAnatomyFingerprintOverlayGlied).
//
// -- DIE OWNER-QUELLE (26.07.2026, verbatim) ------------------------------------------------------------
//   "Nun alle Stempel-Hauptachsen werden als string in der binary mit deren Version und HASH ALLER SOURCE
//    CODE DATEIEN IM OVERLAY als string gehasht und liefern einen zusaetzlichen SHA512 fingerprint ...
//    dann muss er nicht zur Laufzeit berechnet werden und DIE BINARY IST EINDEUTIG."
//
// -- DAS PROBLEM, DAS DIESE DATEI LOEST ----------------------------------------------------------------
// Bis hierher trug das Overlay-Glied nur seinen Separator (COMDARE_OVERLAY_SOURCE_HASH == ""). Damit galt,
// am Code benannt (build_orchestrator.hpp, L14): "geeicht wurde MIT LEEREM Overlay-Glied ... Der SHA512
// deckt damit reine Quell-Code-Aenderungen NOCH NICHT." Konkret: zwei Baue mit identischen Achsen-Strings,
// Versionen, Toolchain und bvset, aber GEAENDERTEM Implementierungs-Quelltext, trugen denselben
// Fingerprint. dll_is_current uebersprang den Neubau, das Lager lieferte die alte Binary aus. Genau diese
// Luecke schliesst der Schnitt unten.
//
// -- DIE DREI OWNER-ENTSCHEIDE, DIE HIER MATERIALISIEREN ------------------------------------------------
//   (1) KONKATENATION, nicht Hash je Datei: die Bytes aller Dateien werden in der Ordnung unten
//       aneinandergehaengt, und EINMAL SHA-512 darueber gerechnet. Der Rechner ist
//       tools/overlay_source_hash_gen (Pre-Build-Codegen, C++; keine Laufzeit-Berechnung -- ein
//       consteval-Hash kann keine Dateien lesen, und eine Laufzeit-Variante braeche die Doktrin
//       "compile-time only" UND die Owner-Zusage "muss er nicht zur Laufzeit berechnet werden").
//   (2) FESTE STATISCHE ORDNUNG JE ACHSEN-KATEGORIE. Sie wird hier NICHT ERFUNDEN, sondern aus den
//       bestehenden kanonischen Ordnungen GELESEN -- und die Wachen unten beweisen das compile-time:
//         Organ:  experiment_tree/axis_path_serialization.hpp, kCompositionAxisNames (18 Achsen)
//         System: cache_engine/abi/system_axis_order.hpp,      kSystemAxisOrder      (3 Achsen)
//         Mess:   strukturell EINE Haupt-Achse (measurement_tooling); pro Binary wird genau eine
//                 gewaehlt -- dort gibt es nichts zu sortieren.
//   (3) DER VERZEICHNIS-SCHNITT: ueber die drei kanonischen Achsen-Ordnungen -- je Achse die Dateimenge
//       IHRER EIGENEN Implementierung -- PLUS anatomy/ als gemeinsame Tier-Substanz.
//
// -- WARUM anatomy/ UNBEDINGT DAZUGEHOERT --------------------------------------------------------------
// Ein erster Zuschnitt lautete "nur topics/*/axis_*". Er war am Objekt widerlegbar, und die Gruende stehen
// hier, damit sie nicht wiederkehren:
//   * anatomy/ FEHLTE. Dort liegen observable_tier.hpp, abi_adapter.hpp, composition_factory.hpp. Aendert
//     man eine davon, aendert sich die Binary -- der Fingerprint bliebe gleich. Exakt der Fehlermodus,
//     den dieses Glied beseitigen soll.
//   * DER GLOB TRIFFT DIE FALSCHE MENGE: topics/*/axis_* liefert 28 Verzeichnisse, kCompositionAxisNames
//     zaehlt 18. Darunter Achsen, die die Komposition VERLASSEN haben (telemetry -> CEB-System-Achse,
//     isa -> target_isa) und drei BUILD-ONLY-Achsen (page_type, simd_extension, general_hardware) -- die
//     sind bereits vom bvset-Glied [6] abgedeckt und gehoeren NICHT ins Overlay.
//   * operating_system und external_utils haben unter topics/ GAR KEIN Verzeichnis. Deshalb ist die
//     ORDNUNG der Einstiegspunkt und nicht der Pfad-Glob.
//
// -- WO DIE ORGAN-ACHSEN WIRKLICH WOHNEN (am Objekt erhoben, nicht angenommen) -------------------------
// 16 der 18 Organ-Achsen haben unter topics/ nur eine WEITERLEITUNGS-HUELLE (4-5 Zeilen: #include
// <organ_axes/...> + using namespace); ihre eigene Implementierung liegt unter libs/cache_engine/organ_axes/.
// Beleg-Beispiel: topics/allocator/axis_06_allocator/axis_06_allocator_registry.hpp ist vier Zeilen lang
// und leitet nach organ_axes/alloc/ weiter. Die Bindung Achsen-NAME -> Typ steht in
// experiment_tree/registry_to_axis_levels.hpp:51-81 (axes26::T*) und :127-141/:166-168
// (push_static_axis<...>(lv, "<name>")).
// DESHALB TRAEGT EINE ORGAN-ACHSE HIER ZWEI PFADE: das Owner-Verzeichnis unter organ_axes/ UND das
// Andock-Verzeichnis unter topics/. Beide sind Quelltext dieser Achse -- eine Aenderung an der Huelle
// aendert, was die Binary einbindet, also muss sie in den Hash. queuing_q1/queuing_q2 sind die zwei
// Ausnahmen: ihre Implementierung wohnt seit dem #72-Umzug (KON72-05, 15.08.2026) im ORGAN-Home
// organ_axes/axis_q{1,2}_queuing/ und traegt nur EINEN Pfad; die Topic-Huelle (2 Dateien unter
// topics/queuing/) bleibt BEWUSST ausserhalb des Schnitts (keine Achsen-Implementierung, s. Lock-Kopf D1).
//
// -- DIE ABGRENZUNG ZU DEN NACHBAR-GLIEDERN (am Code erhoben) ------------------------------------------
//   Glied [5] Toolchain traegt BAU-SCHALTER (Dialekt, Compiler-Version, opt-Flags, atomic128, ext, bt,
//             gate, ceb) -- KEINEN Quelltext-Hash.
//   Glied [6] bvset traegt die Signatur der ENABLED-LISTEN der Build-Achsen.
//   Glieder [1]/[2]/[3] tragen Achsen-NAMEN und VERSIONEN -- Behauptungen UEBER den Code, nicht seinen
//             Inhalt.
//   Die Luecke, die keines von ihnen deckt, ist DER INHALT UNSERER QUELLDATEIEN. Das ist die Rolle von [7].
//
// -- WAS BEWUSST DRAUSSEN BLEIBT -----------------------------------------------------------------------
//   * ext/ (vendorierte Fremd-Quellen): nicht "unsere Implementierung". Ihre Auswahl/Aktivierung reist
//     ueber das bvset-Glied [6] und die Toolchain-Flags; ihr INHALT ist Fremd-Eigentum und wandert nicht
//     mit unserem Achsen-Quelltext. Die Gegenprobe der Verifikation prueft genau das.
//   * die drei BUILD-ONLY-Achsen (page_type, simd_extension, general_hardware): bereits Glied [6].
//   * telemetry und isa als KOMPOSITIONS-Achsen: haben die Komposition verlassen (Bau-INC-2c/2d).
//   * Doku (.md) und die GENERIERTEN Registry-XML: sie aendern kein Kompilat. Die *_flags.hpp.in-VORLAGEN
//     dagegen SIND Quelltext (handgepflegt, sie bestimmen den Inhalt der generierten Flags-Header) und
//     stehen deshalb IM Schnitt; ihre CMake-Variablen-Belegung deckt Glied [5]/[6].
//
// ASCII-only (Umlaute als ae/oe/ue/ss). Header-only, constexpr -- der Codegen liest diese eine Quelle.

#include "experiment_tree/axis_path_serialization.hpp" // kCompositionAxisNames (Organ-Ordnung)

#include <cache_engine/abi/system_axis_order.hpp> // kSystemAxisOrder (System-Ordnung)

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::overlay {

/// Die Achsen-KATEGORIE eines Schnitt-Eintrags. Sie ist kein Schmuck: die Ordnungs-Wachen unten pruefen
/// je Kategorie gegen eine ANDERE kanonische Quelle, und der Codegen gibt sie im Manifest aus, damit ein
/// Leser den Schnitt gegen die Owner-Entscheidung halten kann, ohne den Code zu lesen.
enum class Kategorie : unsigned char {
    organ, ///< die 18 Komposition-Haupt-Achsen (kCompositionAxisNames)
    /// E-10/ORG-19 (26.08.2026): die Organ-Meta-Meta-Achsen (Klammer-Anhang der Organ-Zeile, Glied [3];
    /// Heimat organ_axes/organ_meta_meta/). EIGENE Kategorie ZWISCHEN organ und system, weil der Block
    /// hinter den 18 Organ-Achsen steht und kategorien_stehen_als_bloecke() die ENUM-WERT-Monotonie
    /// prueft. 18+1-Schnitt: kCompositionAxisNames bleibt 18 -- eine Meta-Meta permutiert NIE die
    /// binary_id. Nachzug-Pflicht: beide kategorie_text()-Vokabulare (overlay_source_hash_gen +
    /// axis_version_lock) tragen den neuen Fall im SELBEN Zug (FIX-2/F-1).
    organ_meta_meta,
    system,        ///< die 3 System-Haupt-Achsen (kSystemAxisOrder)
    mess,          ///< die EINE Mess-Haupt-Achse (measurement_tooling)
    tier_substanz, ///< anatomy/ -- kein Achsen-Eigentum, sondern der gemeinsame Traeger aller Tiere
};

/// Die FORM eines Pfad-Eintrags. Zwei Formen, weil die Achsen-Welten verschieden gebaut sind:
///   verzeichnis  -- die Organ-Achsen und anatomy/ besitzen je einen eigenen Ordner (REKURSIV gelesen).
///   datei_praefix-- die System-/Mess-Achsen liegen FLACH in ihren Kategorie-Homes system_axes/ bzw.
///                   mess_axes/ (S-18/#16, KON27-01) nebeneinander; sie besitzen keinen Achsen-Ordner,
///                   sondern eine Namens-Familie. Der Eintrag nennt dann das Verzeichnis und ein
///                   Datei-Namens-PRAEFIX (nicht rekursiv).
/// WARUM PRAEFIX UND KEINE DATEI-LISTE: eine Liste waere beim naechsten neuen Unter-Achsen-Header still
/// unvollstaendig -- die neue Datei fiele aus dem Schnitt, und genau das ist der Fehlermodus, gegen den
/// dieses Glied gebaut ist. Ein Praefix nimmt sie automatisch mit, sobald sie der Namens-Konvention folgt.
enum class Form : unsigned char { verzeichnis, datei_praefix };

/// EIN Eintrag des Schnitts. Eine Achse darf MEHRERE Eintraege haben (sie stehen dann unmittelbar
/// hintereinander); die Ordnungs-Wachen unten pruefen die Achsen in der Reihenfolge ihres ERSTEN
/// Auftretens gegen die kanonische Ordnung.
struct Eintrag {
    Kategorie        kategorie;
    std::string_view achse;   ///< Achsen-Name EXAKT wie in der kanonischen Ordnung
    Form             form;    ///< wie `pfad` zu lesen ist
    std::string_view pfad;    ///< relativ zu libs/cache_engine/
    std::string_view praefix; ///< nur bei Form::datei_praefix -- das Datei-Namens-Praefix, sonst ""
};

/// DER SCHNITT. Reihenfolge = Preimage-Reihenfolge: Organ -> System -> Mess -> Tier-Substanz, und
/// innerhalb jeder Kategorie exakt die kanonische Achsen-Ordnung. Wer hier umsortiert, aendert die
/// Identitaet JEDER Tier-Binary -- die Wachen unten machen ein Auseinanderlaufen mit der kanonischen
/// Ordnung compile-hart.
/// std::to_array statt std::array<Eintrag, N>: die Laenge wird DEDUZIERT. Eine handgezaehlte Zahl waere
/// hier die klassische Falle -- sie faellt beim naechsten Nachtrag entweder als Uebersetzungsfehler auf
/// (gut) oder, wenn jemand sie mitzieht ohne zu zaehlen, gar nicht.
inline constexpr auto kOverlaySourceSet = std::to_array<Eintrag>({
    // -- ORGAN (18 Achsen, Ordnung == kCompositionAxisNames) ------------------------------------------
    {Kategorie::organ, "search_algo", Form::verzeichnis, "organ_axes/lookup", ""},
    {Kategorie::organ, "search_algo", Form::verzeichnis, "topics/traversal/axis_03a_search_algo", ""},
    {Kategorie::organ, "cache_traversal", Form::verzeichnis, "organ_axes/cache_traversal", ""},
    {Kategorie::organ, "cache_traversal", Form::verzeichnis, "topics/traversal/axis_03b_cache_traversal", ""},
    {Kategorie::organ, "mapping", Form::verzeichnis, "organ_axes/mapping", ""},
    {Kategorie::organ, "mapping", Form::verzeichnis, "topics/traversal/axis_03m_mapping", ""},
    {Kategorie::organ, "path_compression", Form::verzeichnis, "organ_axes/path_compression", ""},
    {Kategorie::organ, "path_compression", Form::verzeichnis, "topics/nodes/axis_02_path_compression", ""},
    {Kategorie::organ, "node_type", Form::verzeichnis, "organ_axes/node", ""},
    {Kategorie::organ, "node_type", Form::verzeichnis, "topics/nodes/axis_04_node_type", ""},
    {Kategorie::organ, "memory_layout", Form::verzeichnis, "organ_axes/layout", ""},
    {Kategorie::organ, "memory_layout", Form::verzeichnis, "topics/memory_layout/axis_05_memory_layout", ""},
    {Kategorie::organ, "allocator", Form::verzeichnis, "organ_axes/alloc", ""},
    {Kategorie::organ, "allocator", Form::verzeichnis, "topics/allocator/axis_06_allocator", ""},
    {Kategorie::organ, "prefetch", Form::verzeichnis, "organ_axes/prefetch_axis", ""},
    {Kategorie::organ, "prefetch", Form::verzeichnis, "topics/prefetch/axis_07_prefetch", ""},
    {Kategorie::organ, "concurrency", Form::verzeichnis, "organ_axes/concurrency_axis", ""},
    {Kategorie::organ, "concurrency", Form::verzeichnis, "topics/concurrency/axis_08_concurrency", ""},
    {Kategorie::organ, "serialization", Form::verzeichnis, "organ_axes/serialization_axis", ""},
    {Kategorie::organ, "serialization", Form::verzeichnis, "topics/serialization/axis_10_serialization", ""},
    {Kategorie::organ, "value_handle", Form::verzeichnis, "organ_axes/value_handle_axis", ""},
    {Kategorie::organ, "value_handle", Form::verzeichnis, "topics/value_handle/axis_14_value_handle", ""},
    {Kategorie::organ, "index_organization", Form::verzeichnis, "organ_axes/index_organization", ""},
    {Kategorie::organ, "index_organization", Form::verzeichnis, "topics/search_engine/axis_01_index_organization", ""},
    {Kategorie::organ, "io_dispatch", Form::verzeichnis, "organ_axes/io_dispatch", ""},
    {Kategorie::organ, "io_dispatch", Form::verzeichnis, "topics/io/axis_io", ""},
    {Kategorie::organ, "migration_policy", Form::verzeichnis, "organ_axes/migration_policy", ""},
    {Kategorie::organ, "migration_policy", Form::verzeichnis, "topics/migration/axis_migration", ""},
    {Kategorie::organ, "filter", Form::verzeichnis, "organ_axes/filter_axis", ""},
    {Kategorie::organ, "filter", Form::verzeichnis, "topics/filter/axis_filter", ""},
    // queuing_q1/q2: seit #72 (KON72-05) im ORGAN-Home -- der fruehere NATIV-unter-topics/-Zustand
    // (42 Lock-Records unter topics/queuing/) war das benannte Aufraeumproblem aus KON27-01.
    {Kategorie::organ, "queuing_q1", Form::verzeichnis, "organ_axes/axis_q1_queuing", ""},
    {Kategorie::organ, "queuing_q2", Form::verzeichnis, "organ_axes/axis_q2_queuing", ""},
    {Kategorie::organ, "persistence_target", Form::verzeichnis, "organ_axes/persistence_target", ""},
    {Kategorie::organ, "persistence_target", Form::verzeichnis, "topics/io/axis_persistence_target", ""},

    // -- ORGAN-META-META (E-10/ORG-19, 26.08.2026): 18 Komposition + 1 Meta-Meta ----------------------
    // Aufnahme-Entscheid D-9/KN-2 (Owner-GO 26.08.): Glied [7] deckt "ALLE SOURCE-CODE-DATEIEN IM
    // OVERLAY"; eine STEMPELNDE Meta-Meta ausserhalb des Schnitts koennte ihre Implementierung still
    // aendern -- exakt die Klasse, die das Glied beseitigt. Heute existiert kein gebauter Binary-
    // Bestand -> die Aufnahme ist kostenlos; nach dem Trigger waere sie ein Voll-Neubau (a.5 B-1).
    {Kategorie::organ_meta_meta, "disk_io", Form::verzeichnis, "organ_axes/organ_meta_meta", ""},

    // -- SYSTEM (3 Achsen, Ordnung == kSystemAxisOrder) -----------------------------------------------
    // S-18/#16 (KON27-01 Home-Prinzip): die drei Achsen wohnen im SYSTEM-Kategorie-Home system_axes/
    // (flach, Praefix-Familien; bis 15.08.2026 lagen sie flach in include/cache_engine/measurement/).
    // Die Zuordnung Datei -> Achse folgt der ORDNUNGS-QUELLE SELBST (system_axis_order.hpp:15-22), nicht
    // dem Dateinamen allein:
    //   * scheduling gehoert zu target_isa   -- ":21 scheduling wird Unter-Achse (sub_axis) des
    //                                          target_isa-Komplex-Wrappers"
    //   * compiler   gehoert zu external_utils -- ":20 compiler wird Unter-Achsen-GRUPPE der aeusseren
    //                                          System-Komplex-Achse"; external_utils ist ab 2026 der
    //                                          KOPF/HUB (external_utils_family_axis.hpp:1-6)
    //   * extension_hardware ist der VOR-RENAME-Name derselben Achse (A2-Rename, O-8 Schritt 3) und
    //     bleibt im Schnitt, obwohl deprecated: solange die Datei uebersetzt werden KANN, waere ihr
    //     Weglassen genau die stille Luecke, gegen die dieses Glied gebaut ist.
    {Kategorie::system, "target_isa", Form::datei_praefix, "system_axes", "target_isa"},
    {Kategorie::system, "target_isa", Form::datei_praefix, "system_axes", "scheduling_system_axis"},
    {Kategorie::system, "operating_system", Form::datei_praefix, "system_axes", "operating_system"},
    {Kategorie::system, "external_utils", Form::datei_praefix, "system_axes", "external_utils"},
    {Kategorie::system, "external_utils", Form::datei_praefix, "system_axes", "extension_hardware"},
    {Kategorie::system, "external_utils", Form::datei_praefix, "system_axes", "compiler_"},
    {Kategorie::system, "external_utils", Form::datei_praefix, "system_axes", "simd_sub_axis"},
    {Kategorie::system, "external_utils", Form::datei_praefix, "system_axes", "optimization_level_sub_axis"},

    // -- MESS (1 Achse) -------------------------------------------------------------------------------
    // measurement_tooling ist strukturell die EINE Mess-Haupt-Achse; pro Binary wird genau eine
    // Auspraegung gewaehlt (measurement_tooling_registry.hpp:2-3) -- es gibt hier nichts zu sortieren.
    // S-18/#16: Home = mess_axes/ (KON27-01; bis 15.08.2026 include/cache_engine/measurement/).
    {Kategorie::mess, "measurement_tooling", Form::datei_praefix, "mess_axes", "measurement_tooling"},

    // -- TIER-SUBSTANZ --------------------------------------------------------------------------------
    // anatomy/ gehoert KEINER Achse, sondern allen: observable_tier.hpp, abi_adapter.hpp,
    // composition_factory.hpp & Co. sind der Traeger, in den jede Komposition eingesetzt wird. Steht
    // deshalb am ENDE -- nach allen Achsen, als ihr gemeinsamer Grund.
    {Kategorie::tier_substanz, "anatomy", Form::verzeichnis, "anatomy", ""},
});

/// Die Datei-Endungen, die in den Hash EINGEHEN. Alles, was ein Kompilat veraendern kann.
/// (.in == die handgepflegten *_flags.hpp.in-Vorlagen, s. Kopf.)
inline constexpr std::array<std::string_view, 8> kQuellEndungen{
    {".hpp", ".h", ".cpp", ".cc", ".cxx", ".ipp", ".inl", ".in"}};

/// Die Endungen, die BEWUSST DRAUSSEN bleiben -- Doku und generierte Registry-Spiegel. Sie stehen
/// EXPLIZIT da und nicht als "alles andere faellt raus": der Codegen bricht FAIL-LOUD bei einer
/// Endung, die in KEINER der beiden Listen steht. Eine neue Endung erzwingt damit eine Entscheidung,
/// statt still in den Hash zu rutschen oder still herauszufallen.
inline constexpr std::array<std::string_view, 5> kNichtQuellEndungen{{".md", ".xml", ".txt", ".json", ".yml"}};

// -- DIE ORDNUNGS-WACHEN ------------------------------------------------------------------------------
// Sie sind der eigentliche Zweck dieses Headers. Der Owner-Entscheid (2) lautet "FESTE statische Ordnung
// je Achsen-Kategorie" -- und eine Ordnung, die nur als Kommentar behauptet wird, laeuft irgendwann
// gegen ihre Quelle. Ab hier bricht das COMPILE-HART.

namespace detail {

/// Zaehlt die Eintraege einer Kategorie.
[[nodiscard]] consteval std::size_t anzahl_der_kategorie(Kategorie k) {
    std::size_t n = 0;
    for (auto const& e : kOverlaySourceSet)
        if (e.kategorie == k) ++n;
    return n;
}

/// Die Achsen einer Kategorie in der Reihenfolge ihres ERSTEN Auftretens, ohne Wiederholungen.
template <std::size_t N>
[[nodiscard]] consteval std::array<std::string_view, N> achsen_in_reihenfolge(Kategorie k) {
    std::array<std::string_view, N> aus{};
    std::size_t                     n = 0;
    for (auto const& e : kOverlaySourceSet) {
        if (e.kategorie != k) continue;
        if (n > 0 && aus[n - 1] == e.achse) continue; // Folge-Eintrag derselben Achse
        if (n < N) aus[n] = e.achse;
        ++n;
    }
    return aus;
}

/// Die Zahl der VERSCHIEDENEN Achsen einer Kategorie (Bloecke, nicht Eintraege).
[[nodiscard]] consteval std::size_t achsen_bloecke(Kategorie k) {
    std::size_t      n   = 0;
    std::string_view vor = {};
    bool             an  = false;
    for (auto const& e : kOverlaySourceSet) {
        if (e.kategorie != k) continue;
        if (an && vor == e.achse) continue;
        vor = e.achse;
        an  = true;
        ++n;
    }
    return n;
}

/// Die Kategorien muessen als BLOECKE stehen (organ, dann system, dann mess, dann tier_substanz).
/// Ohne diese Wache koennte ein neuer Eintrag mitten in eine fremde Kategorie geraten -- das Preimage
/// wuerde sich verschieben, ohne dass eine Achsen-Ordnung verletzt waere.
[[nodiscard]] consteval bool kategorien_stehen_als_bloecke() {
    unsigned char zuletzt = 0;
    for (auto const& e : kOverlaySourceSet) {
        auto const jetzt = static_cast<unsigned char>(e.kategorie);
        if (jetzt < zuletzt) return false;
        zuletzt = jetzt;
    }
    return true;
}

/// Jeder Eintrag muss wohlgeformt sein: nicht-leerer Pfad, Praefix genau dann, wenn die Form es verlangt,
/// und ein Pfad ohne fuehrenden '/' bzw. ohne ".." (er wird an die ce-Wurzel gehaengt).
[[nodiscard]] consteval bool eintraege_sind_wohlgeformt() {
    for (auto const& e : kOverlaySourceSet) {
        if (e.achse.empty() || e.pfad.empty()) return false;
        if (e.pfad.front() == '/') return false;
        if (e.pfad.find("..") != std::string_view::npos) return false;
        if (e.form == Form::datei_praefix && e.praefix.empty()) return false;
        if (e.form == Form::verzeichnis && !e.praefix.empty()) return false;
    }
    return true;
}

/// KEIN Pfad-Eintrag darf doppelt vorkommen -- sonst gingen dieselben Bytes zweimal ins Preimage, und
/// eine spaetere Entdopplung waere ein stiller Fingerprint-Bruch.
[[nodiscard]] consteval bool pfade_sind_paarweise_verschieden() {
    for (std::size_t i = 0; i < kOverlaySourceSet.size(); ++i) {
        for (std::size_t j = i + 1; j < kOverlaySourceSet.size(); ++j) {
            if (kOverlaySourceSet[i].pfad == kOverlaySourceSet[j].pfad &&
                kOverlaySourceSet[i].praefix == kOverlaySourceSet[j].praefix)
                return false;
        }
    }
    return true;
}

/// Quell- und Nicht-Quell-Endungen duerfen sich nicht ueberschneiden (sonst waere die Fail-loud-Regel
/// des Codegens mehrdeutig).
[[nodiscard]] consteval bool endungen_sind_disjunkt() {
    for (auto const& q : kQuellEndungen)
        for (auto const& n : kNichtQuellEndungen)
            if (q == n) return false;
    return true;
}

inline constexpr std::size_t kOrganBloecke  = achsen_bloecke(Kategorie::organ);
inline constexpr std::size_t kSystemBloecke = achsen_bloecke(Kategorie::system);
inline constexpr std::size_t kMessBloecke   = achsen_bloecke(Kategorie::mess);
// E-10/ORG-19: eigener Zaehl-Anker der neuen Kategorie (Wachen unten).
inline constexpr std::size_t kOrganMetaMetaBloecke = achsen_bloecke(Kategorie::organ_meta_meta);

inline constexpr auto kOrganAchsen  = achsen_in_reihenfolge<kOrganBloecke>(Kategorie::organ);
inline constexpr auto kSystemAchsen = achsen_in_reihenfolge<kSystemBloecke>(Kategorie::system);

} // namespace detail

static_assert(detail::eintraege_sind_wohlgeformt(),
              "E-E: ein Schnitt-Eintrag ist nicht wohlgeformt (leerer Name/Pfad, absoluter Pfad, '..', oder "
              "Form und praefix passen nicht zusammen).");
static_assert(detail::pfade_sind_paarweise_verschieden(),
              "E-E: ein Pfad steht zweimal im Schnitt -- dieselben Bytes gingen doppelt ins Preimage.");
static_assert(detail::endungen_sind_disjunkt(), "E-E: kQuellEndungen und kNichtQuellEndungen ueberschneiden sich.");
static_assert(detail::kategorien_stehen_als_bloecke(),
              "E-E: die Kategorien stehen nicht mehr als Bloecke (organ -> organ_meta_meta -> system -> mess "
              "-> tier_substanz). "
              "Die Preimage-Ordnung haengt daran.");

// (2) ORGAN: die Achsen-Ordnung des Schnitts IST kCompositionAxisNames -- Name fuer Name, Position fuer
// Position. Wer eine Organ-Achse umbenennt, hinzufuegt oder umsortiert, ohne den Schnitt nachzuziehen,
// bricht hier. Das ist die Wache, die "die Ordnung muss nicht erfunden werden, sie existiert" mechanisch
// macht statt sie zu behaupten.
static_assert(detail::kOrganBloecke == experiment::kCompositionAxisNames.size(),
              "E-E: der Schnitt deckt nicht GENAU die 18 Organ-Haupt-Achsen aus kCompositionAxisNames.");
static_assert(
    [] {
        for (std::size_t i = 0; i < detail::kOrganAchsen.size(); ++i)
            if (detail::kOrganAchsen[i] != experiment::kCompositionAxisNames[i]) return false;
        return true;
    }(),
    "E-E: die Organ-Ordnung des Schnitts ist AUS DER REIHE zu kCompositionAxisNames gelaufen "
    "(axis_path_serialization.hpp). Beide tragen dieselbe kanonische Ordnung; eine Umsortierung "
    "muss in BEIDEN erfolgen -- und sie aendert die Identitaet jeder Tier-Binary.");

// (2) ORGAN-META-META (E-10/ORG-19, 26.08.2026): 18 Komposition + 1 Meta-Meta -- der 18+1-Schnitt nach
// H-23 C.1. Die Organ-Wache darueber bleibt auf kCompositionAxisNames (18); die Meta-Meta ist ADDITIV
// und permutiert NIE die binary_id.
static_assert(detail::kOrganMetaMetaBloecke == 1,
              "E-10/ORG-19: 18 Komposition + 1 Meta-Meta -- die Organ-Meta-Meta-Kategorie traegt heute "
              "GENAU disk_io. Wer ein Glied ergaenzt, zieht Registry-Gen, Lock-Regen und die beiden "
              "kategorie_text()-Vokabulare (overlay_source_hash_gen/axis_version_lock) im SELBEN Zug nach.");
static_assert(detail::achsen_in_reihenfolge<1>(Kategorie::organ_meta_meta)[0] == "disk_io",
              "E-10/ORG-19: die EINE Organ-Meta-Meta-Achse heisst disk_io.");

// (2) SYSTEM: dieselbe Wache gegen kSystemAxisOrder (abi/system_axis_order.hpp), die ihrerseits schon
// eine Drift-Wache gegen kSystemAxisCodeVersions traegt. Die Kette ist damit geschlossen.
static_assert(detail::kSystemBloecke == abi::kSystemAxisOrderCount,
              "E-E: der Schnitt deckt nicht GENAU die 3 System-Haupt-Achsen aus kSystemAxisOrder.");
static_assert(
    [] {
        for (std::size_t i = 0; i < detail::kSystemAchsen.size(); ++i)
            if (detail::kSystemAchsen[i] != abi::kSystemAxisOrder[i]) return false;
        return true;
    }(),
    "E-E: die System-Ordnung des Schnitts ist AUS DER REIHE zu kSystemAxisOrder gelaufen.");

// (2) MESS: strukturell GENAU EINE Haupt-Achse. Kommt je eine zweite dazu, ist das eine
// Ordnungs-Entscheidung und keine Nebensache -- sie faellt hier auf.
static_assert(detail::kMessBloecke == 1,
              "E-E: die Mess-Kategorie traegt nicht mehr genau EINE Haupt-Achse. Sobald es mehr als eine "
              "gibt, braucht sie eine kanonische Ordnung wie Organ und System -- und diese Wache muss "
              "gegen sie zeigen, statt auf 1 zu bestehen.");
static_assert(detail::achsen_in_reihenfolge<1>(Kategorie::mess)[0] == "measurement_tooling",
              "E-E: die EINE Mess-Haupt-Achse heisst measurement_tooling.");

// TIER-SUBSTANZ: genau ein Eintrag, und er steht am Ende.
static_assert(detail::anzahl_der_kategorie(Kategorie::tier_substanz) == 1,
              "E-E: die Tier-Substanz ist genau EIN Eintrag (anatomy/).");
static_assert(kOverlaySourceSet.back().kategorie == Kategorie::tier_substanz &&
                  kOverlaySourceSet.back().pfad == "anatomy",
              "E-E: anatomy/ ist der LETZTE Eintrag des Schnitts -- der gemeinsame Grund hinter allen Achsen.");

// -- DIE HOME-PIN-WACHEN (S-18/#16, KON27-01: je Achsen-Kategorie EIN Verzeichnis-Home) ---------------
// Der Schnitt darf eine Kategorie nicht still aus ihrem Home fuehren: ein system-Eintrag ausserhalb von
// system_axes/ (bzw. mess ausserhalb von mess_axes/, organ ausserhalb von organ_axes/ oder topics/) waere ein
// Home-Bruch, den erst der Waechter-Lauf saehe. Hier bricht er COMPILE-HART, mit Datei und Regel.
namespace detail {
[[nodiscard]] consteval bool pfad_liegt_unter(std::string_view pfad, std::string_view wurzel) {
    if (pfad == wurzel) return true;
    return pfad.size() > wurzel.size() && pfad.substr(0, wurzel.size()) == wurzel && pfad[wurzel.size()] == '/';
}
[[nodiscard]] consteval bool kategorie_haelt_ihr_home() {
    for (auto const& e : kOverlaySourceSet) {
        switch (e.kategorie) {
            case Kategorie::organ:
                if (!pfad_liegt_unter(e.pfad, "organ_axes") && !pfad_liegt_unter(e.pfad, "topics")) return false;
                break;
            case Kategorie::organ_meta_meta:
                if (!pfad_liegt_unter(e.pfad, "organ_axes/organ_meta_meta")) return false;
                break;
            case Kategorie::system:
                if (e.pfad != "system_axes") return false;
                break;
            case Kategorie::mess:
                if (e.pfad != "mess_axes") return false;
                break;
            case Kategorie::tier_substanz:
                if (e.pfad != "anatomy") return false;
                break;
            default:
                // fail-closed: eine kuenftige Kategorie OHNE eigene Home-Regel faellt hier
                // durch und macht den static_assert unten sofort rot -- nie still gruen.
                return false;
        }
    }
    return true;
}
} // namespace detail
static_assert(detail::kategorie_haelt_ihr_home(),
              "S-18/#16 HOME-PIN (KON27-01): ein Schnitt-Eintrag liegt ausserhalb des Homes seiner "
              "Kategorie (organ: organ_axes/ oder topics/ -- organ_meta_meta: organ_axes/organ_meta_meta -- "
              "system: system_axes -- mess: mess_axes -- "
              "tier_substanz: anatomy). Wer ein Home verschiebt, zieht Schnitt, Waechter-Nenner, "
              "Tripwire-Anker und den Lock-Regen im SELBEN bewussten Change nach.");

} // namespace comdare::cache_engine::builder::overlay
