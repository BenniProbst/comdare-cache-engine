#pragma once
// bestandslog_factory.hpp -- G3 / #46b Lagerhaltung, Scheibe B3 (Ledger Section 62-B, B17).
//
// Factory Pattern (benannt): make_binary_bestand() / make_messwert_bestand() liefern je-Genus-
// Bestand-Clients mit einer CT-Key-Policy als Template-Parameter. Der Concept-Guard BestandKeyPolicy
// ersetzt eine vtable (CRTP+Concept-Doktrin, statischer Dispatch, kein Runtime-Switch, kein Bloat).
//
// Die ZWEI Genera (Section 62-B, Nachtrag-4 Schluessel-Trennung):
//   * BinaryKeyPolicy   -- Key == sha512(concat(organ+system+measurement+merge)) == anatomy_fingerprint
//                          (Tier-Binary-Replay ueber [d,e,f]; die EINE SHA512-Wahrheit).
//   * MesswertKeyPolicy -- Key ueber die voll-permutativen Mess-Achsen-Zeilen + Hardware-Identitaet
//                          ([d,e,f]+[g,h,i]+HW gemeinsam; getrennter Schluesselteil, keine Fusion).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib. Statischer Dispatch.

#include "bestandslog/bestandslog_document.hpp"
#include "bestandslog/bestandslog_index.hpp"

#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::bestandslog {

// CT-Vertrag einer Key-Policy: nennt ihr Genus und leitet einen Sha512Key aus Key-Komponenten ab.
template <typename P>
concept BestandKeyPolicy = requires(std::span<std::string_view const> comps) {
    { P::genus() } -> std::same_as<Genus>;
    { P::derive_key(comps) } -> std::same_as<Sha512Key>;
};

// Binary-Genus: die EINE SHA512-Wahrheit (identisch zu abi::anatomy_fingerprint_hex).
struct BinaryKeyPolicy {
    [[nodiscard]] static constexpr Genus genus() noexcept { return Genus::binary; }
    [[nodiscard]] static Sha512Key       derive_key(std::span<std::string_view const> stamp_lines) {
        return derive_key_from_lines(stamp_lines);
    }
};

// Messwert-Genus: gleiche Konkatenations-Mechanik, aber der Aufrufer reicht die voll-permutativen
// Mess-Zeilen + Hardware-Identitaet (fixe Reihenfolge) -- ein EIGENER Schluessel, keine Binary-Fusion.
struct MesswertKeyPolicy {
    [[nodiscard]] static constexpr Genus genus() noexcept { return Genus::measurement; }
    [[nodiscard]] static Sha512Key       derive_key(std::span<std::string_view const> components) {
        return derive_key_from_lines(components);
    }
};

static_assert(BestandKeyPolicy<BinaryKeyPolicy>, "BinaryKeyPolicy muss den CT-Vertrag erfuellen");
static_assert(BestandKeyPolicy<MesswertKeyPolicy>, "MesswertKeyPolicy muss den CT-Vertrag erfuellen");

// ---------------------------------------------------------------------------
// Bestand<Policy> -- ein Bestands-Client (ein Genus). Haelt den Sha512-Index; add/find/contains
// laufen ueber die CT-Key-Policy (statischer Dispatch, kein vtable).
// ---------------------------------------------------------------------------
template <BestandKeyPolicy Policy>
class Bestand {
public:
    [[nodiscard]] static constexpr Genus genus() noexcept { return Policy::genus(); }

    // Key aus Komponenten ableiten (Policy-spezifisch).
    [[nodiscard]] static Sha512Key key_of(std::span<std::string_view const> components) {
        return Policy::derive_key(components);
    }

    // Eintrag unter seinem abgeleiteten Key eintragen; setzt key_sha512 (hex) konsistent im Eintrag.
    void add(std::span<std::string_view const> components, BestandEintrag e) {
        Sha512Key const k = Policy::derive_key(components);
        e.key_sha512      = to_hex(k);
        index_[k]         = std::move(e);
    }

    // Eintrag mit bereits bekanntem Key eintragen (z.B. aus einem geladenen Dokument).
    void add_keyed(Sha512Key const& k, BestandEintrag e) { index_[k] = std::move(e); }

    [[nodiscard]] bool contains(Sha512Key const& k) const { return index_.find(k) != index_.end(); }

    [[nodiscard]] BestandEintrag const* find(Sha512Key const& k) const {
        auto it = index_.find(k);
        return (it == index_.end()) ? nullptr : &it->second;
    }

    [[nodiscard]] std::size_t        size() const noexcept { return index_.size(); }
    [[nodiscard]] Sha512Index const& index() const noexcept { return index_; }

    // Index aus einem geladenen Bestandslog-Dokument aufbauen (key_sha512-Hex -> Sha512Key). Nur
    // Eintraege mit gueltigem 128-hex; das Genus sollte zum Client passen (nicht erzwungen).
    void load_from_document(BestandslogDocument const& doc) {
        for (auto const& e : doc.bestand)
            if (auto k = key_from_hex(e.key_sha512)) index_[*k] = e;
    }

private:
    Sha512Index index_;
};

// Factory Pattern (benannt): die zwei Genera als Client-Instanzen.
[[nodiscard]] inline Bestand<BinaryKeyPolicy>   make_binary_bestand() { return Bestand<BinaryKeyPolicy>{}; }
[[nodiscard]] inline Bestand<MesswertKeyPolicy> make_messwert_bestand() { return Bestand<MesswertKeyPolicy>{}; }

} // namespace comdare::cache_engine::builder::bestandslog
