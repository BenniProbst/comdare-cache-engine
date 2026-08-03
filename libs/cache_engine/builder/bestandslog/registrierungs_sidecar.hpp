#pragma once
// registrierungs_sidecar.hpp -- N8-VARIANTE B (A1-Lager-Rest-Welle, Herkunft N7-D4 / Chunk-31
// 26.07.): die per-Owner-SIDECAR-Registrierung.
//
// DER DEFEKT, den sie behebt: eine Reservierung einzutragen bedeutet heute einen VOLL-DOKUMENT-RMW
// (store_reservation_locked -> store_document_merged -> fetch des GANZEN Bestandslogs, mergen,
// zurueckschreiben). Bei den erwarteten Bestands-Groessen sind das 39-65 MB je Registrierung -- der
// Ledger sagt aber ausdruecklich, die Eintragung der Reservierung dauere "NUR MILLISEKUNDEN"
// (LED:3323). Zwei parallel registrierende Owner sind damit zusaetzlich ein Lost-Update-Kandidat.
//
// VARIANTE B (gewaehlt, Lage-Dossier GATE 6 "N8-Variante-B (per-Owner-Sidecar) vor der Messung"):
//   * REGISTRIEREN  = ein reiner KLEIN-STORE unter einem Registrierungs-Praefix des doc_key:
//                     <doc_key>.reg/<owner_uuid>.xml -- KEIN fetch des Hauptdokuments, kein Merge.
//                     Je Owner ein eigenes Objekt, also per Konstruktion kein Lost-Update.
//   * LESEN         = Hauptdokument + Sidecars VEREINIGT (Record-Union ueber merge_documents, die
//                     bestehende deterministische Aufloesung; doc_revision-Monotonie unangetastet).
//   * KONSOLIDIEREN = die Sidecars wandern ins Hauptdokument NUR im SCHLUSS-ZUSTAND (Build-Ende
//                     "Lager inventarisieren"), unter Lock -- dieselbe Typ-Zustands-Maschine wie der
//                     Truncate (knoten_heuristik_log.hpp: SchlussGrund + AlleinschreiberNachweis).
//                     Ausserhalb ist die Konsolidierung NICHT AUSDRUECKBAR (compile-hart).
//   Die ETA-Berechnung bleibt der einzige lange Exklusiv-Fall (ABNAHME-1) -- sie ist hier unberuehrt.
//
// DEKLARIERTE LUECKE (L14-Muster, KEINE stille): das Auflisten der Sidecars braucht ein
// LIST-Verb auf dem Objekt-Store. BestandTransport bekommt es hier als OPTIONALE Injektion
// (BestandTransport::list); der reale Binder in artifact_cache_transport.hpp bleibt UNGEBUNDEN,
// weil ArtifactCache heute kein Listing-Verb hat (verifiziert: object_fetch/store/remove/stat, kein
// object_list). Fehlt das Verb, meldet die Konsolidierung die Fehlerklasse
// auflistung_nicht_verfuegbar -- sie tut NICHT still nichts. Der mc-Listing-Binder ist ein eigenes,
// benanntes Folge-Increment.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + die B1-B4-Bestandslog-Header.

#include "bestandslog_document.hpp"
#include "bestandslog_lock.hpp"
#include "builder_registration.hpp" // with_document_lock_retry / utc_iso_from_epoch
#include "knoten_heuristik_log.hpp" // AlleinschreiberNachweis + SchlussGrund/BuildEnde (EINE Zustands-Doktrin)

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

/// Der Registrierungs-Praefix zu einem Dokument. EINE Ableitung -- Schreiber, Leser und
/// Konsolidierer duerfen nie ueber zwei Praefixe reden.
inline constexpr std::string_view kRegistrierungsPraefixSuffix = ".reg";

[[nodiscard]] inline std::string registrierungs_praefix(std::string_view doc_key) {
    std::string p{doc_key};
    p += kRegistrierungsPraefixSuffix;
    return p;
}

/// Der Sidecar-Schluessel EINES Owners. Der owner_uuid ist ein kontrolliertes Token (uuid) --
/// derselbe, der auch die Reservierungs-id traegt (owner_uuid/seq).
[[nodiscard]] inline std::string sidecar_key_for(std::string_view doc_key, std::string_view owner_uuid) {
    std::string k = registrierungs_praefix(doc_key);
    k += '/';
    k.append(owner_uuid);
    k += ".xml";
    return k;
}

// ---------------------------------------------------------------------------
// Fehlerklassen (Pflicht).
// ---------------------------------------------------------------------------
enum class SidecarFehler : int {
    ok = 0,
    owner_leer,                  // ohne Owner-Token gibt es kein per-Owner-Objekt
    doc_key_leer,                //
    kein_transport,              // fetch/store nicht gebunden
    store_fehlgeschlagen,        //
    auflistung_nicht_verfuegbar, // BestandTransport::list ungebunden (deklarierte Luecke, s. Kopf)
    kein_alleinschreiber_lock,   // Nachweis fehlt/gehoert einem anderen Dokument
    lock_nicht_bekommen,         // der gelockte Zyklus lief nicht durch
    hauptdokument_unlesbar,      // vorhanden, aber nicht parsbar/grammatik-fremd -> Haende weg
};

[[nodiscard]] constexpr std::string_view to_string(SidecarFehler f) noexcept {
    switch (f) {
        case SidecarFehler::ok: return "ok";
        case SidecarFehler::owner_leer: return "owner_leer";
        case SidecarFehler::doc_key_leer: return "doc_key_leer";
        case SidecarFehler::kein_transport: return "kein_transport";
        case SidecarFehler::store_fehlgeschlagen: return "store_fehlgeschlagen";
        case SidecarFehler::auflistung_nicht_verfuegbar: return "auflistung_nicht_verfuegbar";
        case SidecarFehler::kein_alleinschreiber_lock: return "kein_alleinschreiber_lock";
        case SidecarFehler::lock_nicht_bekommen: return "lock_nicht_bekommen";
        case SidecarFehler::hauptdokument_unlesbar: return "hauptdokument_unlesbar";
    }
    return "ok";
}

// ---------------------------------------------------------------------------
// REGISTRIEREN -- der reine Klein-Store. Der ganze Zweck von Variante B: KEIN fetch, KEIN Merge,
// KEIN Lock auf dem Hauptdokument. Ein Test zaehlt genau das (FakeStore zaehlt die Verben).
// ---------------------------------------------------------------------------
[[nodiscard]] inline SidecarFehler store_reservation_sidecar(BestandTransport const& t, std::string const& doc_key,
                                                             std::string const& owner_uuid, Genus genus,
                                                             std::vector<BatchReservierung> const& reservierungen,
                                                             std::string const&                    created_utc) {
    if (doc_key.empty()) return SidecarFehler::doc_key_leer;
    if (owner_uuid.empty()) return SidecarFehler::owner_leer; // fail-closed wie der Lock selbst
    if (!t.store) return SidecarFehler::kein_transport;
    BestandslogDocument sidecar;
    sidecar.genus          = genus;
    sidecar.created_utc    = created_utc;
    sidecar.doc_revision   = 0; // ein Sidecar fuehrt KEINE eigene Revision -- die Monotonie gehoert dem Hauptdokument
    sidecar.reservierungen = reservierungen;
    if (!t.store(sidecar_key_for(doc_key, owner_uuid), emit_document(sidecar)))
        return SidecarFehler::store_fehlgeschlagen;
    return SidecarFehler::ok;
}

// ---------------------------------------------------------------------------
// LESEN -- Hauptdokument + Sidecars vereinigt. Der Claim-Check und die Presence-Naht sehen damit
// dieselbe Wahrheit wie nach einer Konsolidierung, nur ohne sie abzuwarten.
//
// Die Vereinigung laeuft ueber merge_documents (die EINE deterministische Aufloesung): kein zweiter
// Merge-Algorithmus, keine zweite Konflikt-Regel. doc_revision bleibt monoton, weil merge_documents
// max(a,b)+1 rechnet und die Sidecars mit doc_revision 0 nichts nach unten ziehen koennen.
// ---------------------------------------------------------------------------
struct VereinigteLadung {
    std::optional<BestandslogDocument> doc;
    SidecarFehler                      fehler           = SidecarFehler::ok;
    std::size_t                        sidecars_gesehen = 0;

    [[nodiscard]] bool ok() const noexcept { return fehler == SidecarFehler::ok; }
};

[[nodiscard]] inline VereinigteLadung lade_mit_sidecars(BestandTransport const& t, std::string const& doc_key) {
    VereinigteLadung erg;
    if (doc_key.empty()) {
        erg.fehler = SidecarFehler::doc_key_leer;
        return erg;
    }
    if (!t.fetch) {
        erg.fehler = SidecarFehler::kein_transport;
        return erg;
    }
    BestandslogDocument basis;
    if (auto const raw = t.fetch(doc_key)) {
        auto geparst = parse_bestandslog(*raw);
        if (!geparst || !document_syntax_supported(*geparst)) {
            // Haende weg (dieselbe Doktrin wie die Voll-Wipe-Wache): ein unlesbares Hauptdokument
            // darf nicht durch eine Sidecar-Vereinigung "ersetzt" werden.
            erg.fehler = SidecarFehler::hauptdokument_unlesbar;
            return erg;
        }
        basis = std::move(*geparst);
    }
    if (!t.list) {
        // DEKLARIERTE Luecke (s. Kopf): ohne Listing sieht der Leser NUR das Hauptdokument. Das wird
        // gemeldet, nicht verschwiegen -- der Aufrufer weiss dann, dass seine Sicht unvollstaendig ist.
        erg.doc    = std::move(basis);
        erg.fehler = SidecarFehler::auflistung_nicht_verfuegbar;
        return erg;
    }
    auto schluessel = t.list(registrierungs_praefix(doc_key));
    std::sort(schluessel.begin(), schluessel.end()); // deterministische Vereinigungs-Reihenfolge
    for (auto const& k : schluessel) {
        auto const raw = t.fetch(k);
        if (!raw) continue;
        auto geparst = parse_bestandslog(*raw);
        if (!geparst || !document_syntax_supported(*geparst)) continue; // ein kaputtes Sidecar wird uebergangen
        ++erg.sidecars_gesehen;
        basis = merge_documents(basis, *geparst);
    }
    erg.doc = std::move(basis);
    return erg;
}

// ---------------------------------------------------------------------------
// Der DOKUMENT-Alleinschreiber-Zyklus: er stellt denselben AlleinschreiberNachweis aus wie
// mit_knoten_lock (knoten_heuristik_log.hpp), nur auf einen doc_key statt auf einen Baum-Knoten.
// EINE Token-Klasse, EINE Lock-Mechanik -- kein drittes Konstrukt.
// ---------------------------------------------------------------------------
template <class Fn>
[[nodiscard]] inline LockOutcome mit_dokument_alleinschreiber(BestandTransport const& t, std::string const& doc_key,
                                                              LockOwner const& me, NowFn const& now_fn, Fn&& fn,
                                                              int ttl_s            = kLockTtlSeconds,
                                                              int section_budget_s = kSectionBudgetSeconds) {
    static_assert(std::is_invocable_r_v<bool, Fn&, AlleinschreiberNachweis const&>,
                  "fn(nachweis) muss bool liefern: true == Arbeit gelungen");
    Fn& work = fn;
    return with_document_lock(
        t, doc_key, me, ttl_s, now_fn,
        [&]() {
            AlleinschreiberNachweis const nachweis = detail_nachweis_fabrik::stelle_aus(doc_key, me.owner_uuid);
            return work(nachweis);
        },
        section_budget_s);
}

// ---------------------------------------------------------------------------
// DIE ZUSTANDSMASCHINE der Konsolidierung -- dieselben Typ-Zustaende wie beim Truncate (S3).
// ---------------------------------------------------------------------------
struct KonsolidierungsErgebnis {
    SidecarFehler fehler            = SidecarFehler::ok;
    std::size_t   sidecars_vereint  = 0;
    std::size_t   sidecars_entfernt = 0;
    std::uint64_t doc_revision      = 0; // die Revision NACH der Konsolidierung (monoton)

    [[nodiscard]] bool ok() const noexcept { return fehler == SidecarFehler::ok; }
};

template <SchlussGrund G>
class RegistrierungSchluss;

/// Der ARBEITS-Zustand: registrieren ja (Klein-Store), konsolidieren NEIN -- die Methode existiert
/// hier nicht.
class RegistrierungOffen {
public:
    explicit RegistrierungOffen(std::string doc_key) : doc_key_{std::move(doc_key)} {}

    [[nodiscard]] std::string const& doc_key() const noexcept { return doc_key_; }

    /// Registrieren: reiner Klein-Store, ohne Lock und ohne Hauptdokument-Beruehrung (N8-B).
    [[nodiscard]] SidecarFehler registriere(BestandTransport const& t, std::string const& owner_uuid, Genus genus,
                                            std::vector<BatchReservierung> const& res,
                                            std::string const&                    created_utc) const {
        return store_reservation_sidecar(t, doc_key_, owner_uuid, genus, res, created_utc);
    }

    /// UEBERGANG: Build-Ende ("Lager inventarisieren"). Nur mit einem Nachweis, der GENAU dieses
    /// Dokument deckt -- sonst konsolidierte ein beliebiger Lock ein beliebiges Dokument.
    [[nodiscard]] std::optional<RegistrierungSchluss<BuildEnde>>
    schluss_build_ende(AlleinschreiberNachweis const& n) const;

private:
    std::string doc_key_;
};

template <SchlussGrund G>
class RegistrierungSchluss {
public:
    [[nodiscard]] static constexpr std::string_view grund() noexcept { return G::name(); }
    [[nodiscard]] std::string const&                doc_key() const noexcept { return doc_key_; }

    /// Die Sidecars ins Hauptdokument vereinigen und danach ENTFERNEN. Reihenfolge ist bewusst:
    /// erst der gelungene Store des vereinigten Dokuments, DANN das Loeschen -- ein Abriss dazwischen
    /// laesst nur doppelte Records zurueck, die der Record-Union idempotent wieder aufloest (Records
    /// gehen nie verloren, nur die Aufraeum-Arbeit wiederholt sich).
    [[nodiscard]] KonsolidierungsErgebnis konsolidiere(BestandTransport const& t, AlleinschreiberNachweis const& n,
                                                       std::string const& created_utc) const {
        KonsolidierungsErgebnis e;
        if (!n.deckt(doc_key_)) {
            e.fehler = SidecarFehler::kein_alleinschreiber_lock;
            return e;
        }
        if (!t.fetch || !t.store) {
            e.fehler = SidecarFehler::kein_transport;
            return e;
        }
        if (!t.list) {
            e.fehler = SidecarFehler::auflistung_nicht_verfuegbar; // deklarierte Luecke, nie still
            return e;
        }
        auto const ladung = lade_mit_sidecars(t, doc_key_);
        if (!ladung.doc) {
            e.fehler = ladung.fehler == SidecarFehler::ok ? SidecarFehler::hauptdokument_unlesbar : ladung.fehler;
            return e;
        }
        BestandslogDocument vereint = *ladung.doc;
        vereint.created_utc         = created_utc;
        if (!t.store(doc_key_, emit_document(vereint))) {
            e.fehler = SidecarFehler::store_fehlgeschlagen;
            return e;
        }
        e.sidecars_vereint = ladung.sidecars_gesehen;
        e.doc_revision     = vereint.doc_revision;
        if (t.remove)
            for (auto const& k : t.list(registrierungs_praefix(doc_key_)))
                if (t.remove(k)) ++e.sidecars_entfernt;
        return e;
    }

private:
    friend class RegistrierungOffen;
    explicit RegistrierungSchluss(std::string doc_key) : doc_key_{std::move(doc_key)} {}

    std::string doc_key_;
};

inline std::optional<RegistrierungSchluss<BuildEnde>>
RegistrierungOffen::schluss_build_ende(AlleinschreiberNachweis const& n) const {
    if (!n.deckt(doc_key_)) return std::nullopt;
    return RegistrierungSchluss<BuildEnde>{doc_key_};
}

} // namespace comdare::cache_engine::builder::bestandslog
