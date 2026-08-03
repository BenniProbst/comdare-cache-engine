#pragma once
// lager_ziel_strategie.hpp -- G-E7 (A1-Lager-Rest-Welle): die DUAL-ccache-Konfigurationsnaht.
//
// IST-BEFUND, den diese Datei schliesst: der Bestandslog-Transport haengt heute NUR am minio-
// ArtifactCache (artifact_cache_transport.hpp ist die EINE Kante). Die Storage-Doktrin (Section 65,
// KONSOLIDIERT:16) verlangt aber ZWEI Ziele mit einer Default-MATRIX und beidseitiger Umschaltbarkeit:
//   binary      -> minio-Objekt-Store   (Default)
//   measurement -> NAS / measure-drop   (Default)
// und BEIDES gegenteilig konfigurierbar.
//
// BAUFORM: CT-STRATEGY im Muster des SpoolWriter<Backend> (portable | io_uring | winring) --
// das Backend ist ein TEMPLATE-PARAMETER mit Concept-Guard, kein Laufzeit-Schalter. Die
// Konfigurations-Auswertung passiert VOR dem Lauf (Env/XML), im Hot-Path wird nichts mehr gefragt.
// Das Enum LagerBackend ist ein NAME fuer Testate und Diagnose, KEIN Schalter: welches Backend
// gilt, entscheidet der Typ.
//
// mc-ALIAS-DOKTRIN (L1, Owner-Praezisierung): COMDARE_MINIO_ENDPOINT traegt einen mc-ALIAS, NIE eine
// S3-URL. Die Falle dahinter ist real und still: mc behandelt ein unbekanntes erstes Argument als
// LOKALEN PFAD -- ein Tippfehler im Alias erzeugt also klaglos ein Verzeichnis statt eines
// Objekt-Store-Zugriffs, und der Lauf meldet Erfolg. Deshalb ist der Endpoint-PREFLIGHT Pflicht und
// liefert eine FEHLERKLASSE, keinen Fallback. Bucket-Namen werden nie hartkodiert.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + die LB-0-Grammatik (objekt_key). Kein
// std::variant, kein Runtime-Switch, statischer Dispatch.

#include "bestandslog_document.hpp" // Genus (die zwei Bestands-Gattungen)
#include "lager_pfad_grammatik.hpp" // objekt_key -- Baum-Pfad == Objekt-Praefix (LB-0)

#include <concepts>
#include <functional>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::bestandslog {

/// Die zwei Ziel-ARTEN. Name fuer Testate/Diagnose -- die Wahl faellt im Typsystem (s. Kopf).
enum class LagerBackend { objekt_store, nas_ablage };

[[nodiscard]] constexpr std::string_view to_string(LagerBackend b) noexcept {
    switch (b) {
        case LagerBackend::objekt_store: return "objekt_store";
        case LagerBackend::nas_ablage: return "nas_ablage";
    }
    return "objekt_store";
}

// ---------------------------------------------------------------------------
// Fehlerklassen der Ziel-Aufloesung (Pflicht). Kein Ausgang ist ein stiller lokaler Pfad.
// ---------------------------------------------------------------------------
enum class ZielFehler : int {
    ok = 0,
    alias_leer,         // COMDARE_MINIO_ENDPOINT nicht gesetzt
    alias_ist_url,      // eine S3-/HTTP-URL statt eines mc-Alias (L1: verboten)
    alias_unbekannt,    // der Alias ist mc nicht bekannt -> mc naehme ihn als LOKALEN PFAD
    nas_wurzel_leer,    // kein measure-drop-Ziel konfiguriert
    nas_wurzel_relativ, // eine relative NAS-Wurzel haengt am CWD des Laufs -- nie zulaessig
    blatt_leer,         // ohne Blatt-Namen gibt es kein Ziel
};

[[nodiscard]] constexpr std::string_view to_string(ZielFehler f) noexcept {
    switch (f) {
        case ZielFehler::ok: return "ok";
        case ZielFehler::alias_leer: return "alias_leer";
        case ZielFehler::alias_ist_url: return "alias_ist_url";
        case ZielFehler::alias_unbekannt: return "alias_unbekannt";
        case ZielFehler::nas_wurzel_leer: return "nas_wurzel_leer";
        case ZielFehler::nas_wurzel_relativ: return "nas_wurzel_relativ";
        case ZielFehler::blatt_leer: return "blatt_leer";
    }
    return "ok";
}

// ---------------------------------------------------------------------------
// Konfiguration. Sie wird VOR dem Lauf aus Env/XML gefuellt und danach nur noch gelesen.
// ---------------------------------------------------------------------------
struct ZielKonfiguration {
    /// mc-ALIAS (COMDARE_MINIO_ENDPOINT). NIE eine S3-URL -- s. Kopf.
    std::string minio_alias;
    /// Objekt-Praefix im Store (Bucket/Unterpfad). Nie hartkodiert, immer konfiguriert.
    std::string minio_praefix;
    /// NAS-/measure-drop-Wurzel (Ebene C). ABSOLUT -- eine relative Wurzel haengt am CWD.
    std::string nas_wurzel;

    friend bool operator==(ZielKonfiguration const&, ZielKonfiguration const&) = default;
};

/// Ist der Alias mc bekannt? Injiziert (Muster BestandTransport): real fragt der Host `mc alias list`,
/// im Test antwortet eine Lambda. Diese Schicht spawnt NICHTS.
using AliasBekanntFn = std::function<bool(std::string const&)>;

/// Der ENDPOINT-PREFLIGHT (L1). Er ist die einzige Stelle, die ueber den Alias urteilt.
[[nodiscard]] inline ZielFehler minio_alias_preflight(std::string_view alias, AliasBekanntFn const& bekannt) {
    if (alias.empty()) return ZielFehler::alias_leer;
    // L1: eine URL ist kein Alias. Beide Schreibweisen abfangen, die real vorkommen.
    if (alias.find("://") != std::string_view::npos) return ZielFehler::alias_ist_url;
    if (alias.starts_with("http")) return ZielFehler::alias_ist_url;
    // Ohne Pruefer wird NICHT geraten: ein ungeprueftes Ziel ist genau die mc-Falle.
    if (!bekannt) return ZielFehler::alias_unbekannt;
    if (!bekannt(std::string{alias})) return ZielFehler::alias_unbekannt;
    return ZielFehler::ok;
}

// ---------------------------------------------------------------------------
// Die zwei BACKENDS als CT-Policies (Concept-Guard statt vtable).
// ---------------------------------------------------------------------------
struct ObjektStoreBackend {
    [[nodiscard]] static constexpr LagerBackend     art() noexcept { return LagerBackend::objekt_store; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "minio"; }

    [[nodiscard]] static ZielFehler pruefe(ZielKonfiguration const& k, AliasBekanntFn const& bekannt) {
        return minio_alias_preflight(k.minio_alias, bekannt);
    }

    /// Der Objekt-Schluessel: Praefix + Baum-Pfad + Blatt -- alles mit DERSELBEN LB-0-Grammatik
    /// (Baum-Pfad == Objekt-Praefix, keine zweite Uebersetzung).
    [[nodiscard]] static std::string ziel(ZielKonfiguration const& k, std::string_view baum_pfad,
                                          std::string_view blatt) {
        return objekt_key(objekt_key(k.minio_praefix, baum_pfad), blatt);
    }
};

struct NasAblageBackend {
    [[nodiscard]] static constexpr LagerBackend     art() noexcept { return LagerBackend::nas_ablage; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "nas"; }

    [[nodiscard]] static ZielFehler pruefe(ZielKonfiguration const& k, AliasBekanntFn const&) {
        if (k.nas_wurzel.empty()) return ZielFehler::nas_wurzel_leer;
        // Eine relative Wurzel laege je nach CWD des Laufs woanders -- im Zwei-Maschinen-Betrieb
        // genau die stille Divergenz, die das Lager ausschliessen soll.
        if (k.nas_wurzel.front() != '/' && !(k.nas_wurzel.size() > 1 && k.nas_wurzel[1] == ':'))
            return ZielFehler::nas_wurzel_relativ;
        return ZielFehler::ok;
    }

    [[nodiscard]] static std::string ziel(ZielKonfiguration const& k, std::string_view baum_pfad,
                                          std::string_view blatt) {
        return objekt_key(objekt_key(k.nas_wurzel, baum_pfad), blatt);
    }
};

template <typename B>
concept LagerZielBackend = requires(ZielKonfiguration const& k, AliasBekanntFn const& f, std::string_view s) {
    { B::art() } -> std::same_as<LagerBackend>;
    { B::name() } -> std::same_as<std::string_view>;
    { B::pruefe(k, f) } -> std::same_as<ZielFehler>;
    { B::ziel(k, s, s) } -> std::same_as<std::string>;
};

static_assert(LagerZielBackend<ObjektStoreBackend>);
static_assert(LagerZielBackend<NasAblageBackend>);

// ---------------------------------------------------------------------------
// Die DEFAULT-MATRIX als CT-Tabelle (Section 65 / KONSOLIDIERT:16). Sie ist eine Vorbelegung, keine
// Fessel: der Aufrufer setzt das Backend als Template-Argument und dreht die Matrix damit um.
// ---------------------------------------------------------------------------
template <Genus G>
struct StandardZiel;

template <>
struct StandardZiel<Genus::binary> {
    using type = ObjektStoreBackend; // Binaries -> minio (Default)
};

template <>
struct StandardZiel<Genus::measurement> {
    using type = NasAblageBackend; // Mess-CSV/xlsx -> NAS measure-drop (Default)
};

static_assert(std::same_as<StandardZiel<Genus::binary>::type, ObjektStoreBackend>);
static_assert(std::same_as<StandardZiel<Genus::measurement>::type, NasAblageBackend>);
static_assert(!std::same_as<StandardZiel<Genus::binary>::type, StandardZiel<Genus::measurement>::type>,
              "Die Default-MATRIX trennt die zwei Realms: Binaries nach minio, Messdaten aufs NAS. Faellt sie "
              "zusammen, ist die Dual-ccache-Zusage (Section 65) verloren.");

// ---------------------------------------------------------------------------
// Die Aufloesung EINES Blattes auf ein konkretes Ziel.
// ---------------------------------------------------------------------------
struct ZielAufloesung {
    ZielFehler   fehler  = ZielFehler::ok;
    LagerBackend backend = LagerBackend::objekt_store;
    std::string  ziel; // Objekt-Schluessel bzw. NAS-Pfad (leer, wenn fehler != ok)

    [[nodiscard]] bool ok() const noexcept { return fehler == ZielFehler::ok; }
};

/// LagerZiel<Genus, Backend> -- die CT-Strategy. Ohne zweites Argument gilt die Default-Matrix; mit
/// zweitem Argument ist sie gegenteilig konfiguriert. In BEIDEN Faellen steht die Wahl im TYP.
template <Genus G, LagerZielBackend B = typename StandardZiel<G>::type>
struct LagerZiel {
    using Backend = B;

    [[nodiscard]] static constexpr Genus            genus() noexcept { return G; }
    [[nodiscard]] static constexpr LagerBackend     backend() noexcept { return B::art(); }
    [[nodiscard]] static constexpr std::string_view backend_name() noexcept { return B::name(); }

    /// Konfiguration pruefen (VOR dem Lauf) und das Ziel bilden. Ein Fehler ist ein FEHLER --
    /// insbesondere faellt nichts still auf einen lokalen Pfad zurueck (die mc-Falle, L1).
    [[nodiscard]] static ZielAufloesung aufloesen(ZielKonfiguration const& k, std::string_view baum_pfad,
                                                  std::string_view blatt, AliasBekanntFn const& alias_bekannt = {}) {
        ZielAufloesung a;
        a.backend = B::art();
        if (blatt.empty()) {
            a.fehler = ZielFehler::blatt_leer;
            return a;
        }
        if (auto const f = B::pruefe(k, alias_bekannt); f != ZielFehler::ok) {
            a.fehler = f;
            return a;
        }
        a.ziel = B::ziel(k, baum_pfad, blatt);
        return a;
    }
};

// Die zwei DEFAULT-Ziele als benannte Aliase (Factory-Pattern-Aequivalent im Typsystem).
using BinaryStandardZiel   = LagerZiel<Genus::binary>;
using MesswertStandardZiel = LagerZiel<Genus::measurement>;
// ... und die GEGENTEILIGE Belegung, die die Doktrin ausdruecklich zulaesst.
using BinaryAufNasZiel     = LagerZiel<Genus::binary, NasAblageBackend>;
using MesswertAufMinioZiel = LagerZiel<Genus::measurement, ObjektStoreBackend>;

static_assert(BinaryStandardZiel::backend() == LagerBackend::objekt_store);
static_assert(MesswertStandardZiel::backend() == LagerBackend::nas_ablage);
static_assert(BinaryAufNasZiel::backend() == LagerBackend::nas_ablage);
static_assert(MesswertAufMinioZiel::backend() == LagerBackend::objekt_store);

} // namespace comdare::cache_engine::builder::bestandslog
