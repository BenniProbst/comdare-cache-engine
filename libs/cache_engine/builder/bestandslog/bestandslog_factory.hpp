#pragma once
// bestandslog_factory.hpp -- G3 / #46b Lagerhaltung, Scheibe B3 (Ledger Section 62-B, B17).
//
// Factory Pattern (benannt): make_binary_bestand() / make_messwert_bestand() liefern je-Genus-
// Bestand-Clients mit einer CT-Key-Policy als Template-Parameter. Der Concept-Guard BestandKeyPolicy
// ersetzt eine vtable (CRTP+Concept-Doktrin, statischer Dispatch, kein Runtime-Switch, kein Bloat).
//
// Die ZWEI Genera (Section 62-B, Nachtrag-4 Schluessel-Trennung):
//   * BinaryKeyPolicy   -- Digest == sha512 ueber die '\n'-getrennte Glied-Folge (abi::anatomy_fingerprint_
//                          glieder) == anatomy_fingerprint (Tier-Binary-Replay; die EINE SHA512-Wahrheit).
//   * MesswertKeyPolicy -- Digest ueber die voll-permutativen Mess-Achsen-Zeilen + Hardware-Identitaet
//                          ([d,e,f]+[g,h,i]+HW gemeinsam; getrennter Schluesselteil, keine Fusion).
//
// Section 62-NACHTRAG-4: der Digest ALLEIN ist kein vollstaendiger Lager-Schluessel -- er traegt die
// per-Zelle gewaehlte ISA/Optimierung nicht. Deshalb hat jede Policy ZWEI Ableitungen:
//   derive_key(comps)                -> Sha512Key : der reine Digest (unveraendert, A1-Testvektor-treu)
//   derive_lager_key(comps, zelle)   -> LagerKey  : das Tupel (Digest, Zell-Koordinaten)
// Der Digest-Weg bleibt erhalten, weil er der Anker zum einkompilierten .fingerprint-Sidecar ist; der
// Lager-Weg klammert die Zelle DANEBEN, nie hinein (keine Fusion, kein zweiter Preimage).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib. Statischer Dispatch.

#include "bestandslog_document.hpp"
#include "bestandslog_index.hpp"

#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::bestandslog {

// CT-Vertrag einer Key-Policy: nennt ihr Genus, leitet den Digest aus Key-Komponenten ab und klammert
// ihn mit den Zell-Koordinaten zum Lager-Schluessel. Der Concept-Guard ersetzt die vtable.
template <typename P>
concept BestandKeyPolicy = requires(std::span<std::string_view const> comps, ZellKoordinaten const& zelle) {
    { P::genus() } -> std::same_as<Genus>;
    { P::derive_key(comps) } -> std::same_as<Sha512Key>;
    { P::derive_lager_key(comps, zelle) } -> std::same_as<LagerKey>;
};

// Binary-Genus: die EINE SHA512-Wahrheit (identisch zu abi::anatomy_fingerprint_hex) + die Zell-Klammer.
struct BinaryKeyPolicy {
    [[nodiscard]] static constexpr Genus genus() noexcept { return Genus::binary; }
    [[nodiscard]] static Sha512Key       derive_key(std::span<std::string_view const> stamp_lines) {
        return derive_key_from_lines(stamp_lines);
    }
    [[nodiscard]] static LagerKey derive_lager_key(std::span<std::string_view const> stamp_lines,
                                                   ZellKoordinaten const&            zelle) {
        return LagerKey{derive_key_from_lines(stamp_lines), zelle};
    }
};

// Messwert-Genus: gleiche Konkatenations-Mechanik, aber der Aufrufer reicht die voll-permutativen
// Mess-Zeilen + Hardware-Identitaet (fixe Reihenfolge) -- ein EIGENER Schluessel, keine Binary-Fusion.
// Die Zell-Klammer gilt hier ebenso: die Mess-Zeilen decken [g,h,i]+HW, die Zelle bleibt [d,e,f]. Ein
// Aufrufer, der die Zelle bereits in den Mess-Zeilen fuehrt, uebergibt eine leere ZellKoordinaten.
struct MesswertKeyPolicy {
    [[nodiscard]] static constexpr Genus genus() noexcept { return Genus::measurement; }
    [[nodiscard]] static Sha512Key       derive_key(std::span<std::string_view const> components) {
        return derive_key_from_lines(components);
    }
    [[nodiscard]] static LagerKey derive_lager_key(std::span<std::string_view const> components,
                                                   ZellKoordinaten const&            zelle) {
        return LagerKey{derive_key_from_lines(components), zelle};
    }
};

static_assert(BestandKeyPolicy<BinaryKeyPolicy>, "BinaryKeyPolicy muss den CT-Vertrag erfuellen");
static_assert(BestandKeyPolicy<MesswertKeyPolicy>, "MesswertKeyPolicy muss den CT-Vertrag erfuellen");

// ---------------------------------------------------------------------------
// Bestand<Policy> -- ein Bestands-Client (ein Genus). Haelt den Lager-Index; add/find/contains
// laufen ueber die CT-Key-Policy (statischer Dispatch, kein vtable).
// ---------------------------------------------------------------------------
template <BestandKeyPolicy Policy>
class Bestand {
public:
    [[nodiscard]] static constexpr Genus genus() noexcept { return Policy::genus(); }

    // Digest aus Komponenten ableiten (Policy-spezifisch; der Fingerprint-Teil allein).
    [[nodiscard]] static Sha512Key key_of(std::span<std::string_view const> components) {
        return Policy::derive_key(components);
    }

    // Vollstaendigen Lager-Schluessel ableiten: Digest + Zell-Koordinaten (das Lookup-Tupel).
    [[nodiscard]] static LagerKey lager_key_of(std::span<std::string_view const> components,
                                               ZellKoordinaten const&            zelle) {
        return Policy::derive_lager_key(components, zelle);
    }

    // Eintrag unter seinem abgeleiteten Lager-Schluessel eintragen; setzt key_sha512 (hex) UND die
    // Zell-Felder konsistent im Eintrag -> Index-Schluessel und serialisierter Eintrag koennen nicht
    // auseinanderlaufen (der Merge in B2 vergleicht genau diese Felder).
    void add(std::span<std::string_view const> components, ZellKoordinaten const& zelle, BestandEintrag e) {
        LagerKey const k = Policy::derive_lager_key(components, zelle);
        e.key_sha512     = to_hex(k.sha);
        e.zelle          = zelle;
        index_[k]        = std::move(e);
    }

    // Eintrag mit bereits bekanntem Lager-Schluessel eintragen (z.B. aus einem geladenen Dokument).
    void add_keyed(LagerKey const& k, BestandEintrag e) { index_[k] = std::move(e); }

    [[nodiscard]] bool contains(LagerKey const& k) const { return index_.find(k) != index_.end(); }

    [[nodiscard]] BestandEintrag const* find(LagerKey const& k) const {
        auto it = index_.find(k);
        return (it == index_.end()) ? nullptr : &it->second;
    }

    [[nodiscard]] std::size_t       size() const noexcept { return index_.size(); }
    [[nodiscard]] LagerIndex const& index() const noexcept { return index_; }

    // Index aus einem geladenen Bestandslog-Dokument aufbauen ((key_sha512-Hex, Zelle) -> LagerKey).
    // Nur Eintraege mit gueltigem 128-hex; das Genus sollte zum Client passen (nicht erzwungen).
    void load_from_document(BestandslogDocument const& doc) {
        for (auto const& e : doc.bestand)
            if (auto k = lager_key_from_entry(e)) index_[*k] = e;
    }

private:
    LagerIndex index_;
};

// Factory Pattern (benannt): die zwei Genera als Client-Instanzen.
[[nodiscard]] inline Bestand<BinaryKeyPolicy>   make_binary_bestand() { return Bestand<BinaryKeyPolicy>{}; }
[[nodiscard]] inline Bestand<MesswertKeyPolicy> make_messwert_bestand() { return Bestand<MesswertKeyPolicy>{}; }

} // namespace comdare::cache_engine::builder::bestandslog
