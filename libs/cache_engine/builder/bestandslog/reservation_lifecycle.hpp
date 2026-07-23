#pragma once
// reservation_lifecycle.hpp -- G3 / #46b Lagerhaltung, Scheibe B4 (Ledger §62-B P1).
//
// Der LEBENSZYKLUS einer batch-Reservierung: pro-forma-Registrierung (30min, ohne ETA) -> ETA-
// Kalibrierung (eta_estimator, avg_size als zweiter Wert) -> Done | Released. Dazu das
// +50%-TAKEOVER-Praedikat (eine offene Reservierung gilt als frei, wenn ihre ETA um 50% ohne
// Update ueberschritten ist -> tote Pipeline uebernehmbar) und der RAII-PROMISEGUARD (best-effort
// Release der Reservierung bei Abbruch; commit() bei erfolgreicher Fertigstellung).
//
// Diese Scheibe fasst das in B2 eingefrorene bestandslog_lock.hpp NICHT an -- der Lifecycle ist eine
// eigene Verantwortung (Zustandsmaschine ueber BatchReservierung); die Verdrahtung an Store/Merge/
// Lock macht der Aufrufer (I1). Die Funktionen sind ZEIT-frei parametrisiert (Epoch-Sekunden als
// Argumente) -> literal testbar, keine Wall-Clock-Abhaengigkeit.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + bestandslog_document.hpp +
// eta_estimator.hpp. Kein Runtime-Switch, keine vtable im Hot-Path (der PromiseGuard-dtor ist I/O,
// kein Hot-Path -> std::function-Callback zulaessig, Muster wie AsyncPushPump-Injektion).

#include "bestandslog/bestandslog_document.hpp"
#include "bestandslog/eta_estimator.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::bestandslog {

// pro-forma-Frist: 30min ab Reservierung (Design B7 / §62-B). Takeover-Faktor: ETA + 50%.
inline constexpr int    kProFormaMinutes = 30;
inline constexpr double kTakeoverFactor  = 1.5;

// ─────────────────────────────────────────────────────────────────────────────
// Zahl <-> Wire-String (eta_s ist im Bestandslog ein String; hier die deterministische Bruecke).
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] inline std::string format_seconds(double s) {
    std::array<char, 32> buf{};
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), s, std::chars_format::fixed, 3);
    if (ec != std::errc{}) return "0.000";
    return std::string(buf.data(), ptr);
}

[[nodiscard]] inline double parse_seconds(std::string_view s) noexcept {
    double v = 0.0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v; // leer/nicht-numerisch -> 0.0
}

// ─────────────────────────────────────────────────────────────────────────────
// pro-forma-Registrierung: erste Reservierung, status=offen, OHNE ETA (die kommt mit der
// Kalibrierung). Der Aufrufer liefert die ISO-Zeitstempel (Zeit-Formatierung ist nicht Sache dieser
// Zustandsmaschine); pro_forma_deadline_epoch_s hilft beim Berechnen der 30min-Frist in Epoch.
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] inline std::int64_t pro_forma_deadline_epoch_s(std::int64_t reserviert_epoch_s,
                                                             int          minutes = kProFormaMinutes) noexcept {
    return reserviert_epoch_s + static_cast<std::int64_t>(minutes) * 60;
}

[[nodiscard]] inline BatchReservierung make_pro_forma_reservation(std::string id, BatchTyp typ, std::string maschine,
                                                                  unsigned threads, std::uint64_t slice_begin,
                                                                  std::uint64_t slice_count, std::string reserviert_utc,
                                                                  std::string pro_forma_bis_utc) {
    BatchReservierung r;
    r.id                = std::move(id);
    r.typ               = typ;
    r.slice_begin       = slice_begin;
    r.slice_count       = slice_count;
    r.maschine          = std::move(maschine);
    r.threads           = threads;
    r.reserviert_utc    = std::move(reserviert_utc);
    r.pro_forma_bis_utc = std::move(pro_forma_bis_utc);
    r.eta_s             = ""; // noch nicht kalibriert
    r.avg_size_bytes    = "";
    r.status            = BatchStatus::offen;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Zustandsuebergaenge auf einer Reservierung.
// ─────────────────────────────────────────────────────────────────────────────

// Kalibrierung eintragen: eta_s + avg_size_bytes aus dem EtaResult (Mini-Batch, §62-B-NACHTRAG-2).
inline void apply_calibration(BatchReservierung& r, EtaResult const& e) {
    r.eta_s          = format_seconds(e.eta_s);
    r.avg_size_bytes = std::to_string(e.avg_size_bytes);
}

inline void mark_done(BatchReservierung& r) noexcept { r.status = BatchStatus::done; }
inline void mark_released(BatchReservierung& r) noexcept { r.status = BatchStatus::released; }

// ─────────────────────────────────────────────────────────────────────────────
// Takeover-Praedikate (tote Pipeline uebernehmen).
// ─────────────────────────────────────────────────────────────────────────────

// Vor der ETA-Kalibrierung greift die pro-forma-Frist: offen und ueber die 30min hinaus -> frei.
[[nodiscard]] inline bool is_pro_forma_expired(std::int64_t pro_forma_bis_epoch_s, std::int64_t now_epoch_s) noexcept {
    return now_epoch_s > pro_forma_bis_epoch_s;
}

// Nach der Kalibrierung: eine offene Reservierung ist uebernehmbar, wenn seit ihrem letzten Update
// mehr als factor*eta_s vergangen ist (Default 1.5 = ETA um 50% ohne Update ueberschritten, B6).
[[nodiscard]] inline bool is_takeable_by_eta(double eta_s, std::int64_t last_update_epoch_s, std::int64_t now_epoch_s,
                                             double factor = kTakeoverFactor) noexcept {
    double const elapsed = static_cast<double>(now_epoch_s - last_update_epoch_s);
    return elapsed > factor * eta_s;
}

// Gesamt-Praedikat auf einer Reservierung: offen? dann -- mit ETA das +50%-Praedikat, ohne ETA die
// pro-forma-Frist. done/released sind nie uebernehmbar.
[[nodiscard]] inline bool is_reservation_takeable(BatchReservierung const& r, std::int64_t last_update_epoch_s,
                                                  std::int64_t pro_forma_bis_epoch_s,
                                                  std::int64_t now_epoch_s) noexcept {
    if (r.status != BatchStatus::offen) return false;
    if (r.eta_s.empty()) return is_pro_forma_expired(pro_forma_bis_epoch_s, now_epoch_s);
    return is_takeable_by_eta(parse_seconds(r.eta_s), last_update_epoch_s, now_epoch_s);
}

// ─────────────────────────────────────────────────────────────────────────────
// PromiseGuard -- RAII: bei Abbruch (Destruktor ohne vorheriges commit()) feuert der best-effort
// Release-Pfad (z.B. Reservierung auf released + store_document_merged). commit() bei erfolgreicher
// Fertigstellung unterdrueckt den Release. Nicht kopierbar; move gibt das Versprechen weiter.
// ─────────────────────────────────────────────────────────────────────────────
class PromiseGuard {
public:
    explicit PromiseGuard(std::function<void()> on_abort) : on_abort_{std::move(on_abort)} {}

    PromiseGuard(PromiseGuard const&)            = delete;
    PromiseGuard& operator=(PromiseGuard const&) = delete;
    PromiseGuard(PromiseGuard&& o) noexcept : on_abort_{std::move(o.on_abort_)}, committed_{o.committed_} {
        o.committed_ = true; // die verschobene Huelle feuert nicht mehr
    }
    PromiseGuard& operator=(PromiseGuard&&) = delete;

    ~PromiseGuard() {
        if (!committed_ && on_abort_) {
            try {
                on_abort_();
            } catch (...) {
                // best-effort: der Release darf den Abbau nie zum Absturz bringen (dtor wirft nie)
            }
        }
    }

    // Erfolgreiche Fertigstellung -> kein Abort-Release beim Zerstoeren.
    void commit() noexcept { committed_ = true; }

    [[nodiscard]] bool committed() const noexcept { return committed_; }

private:
    std::function<void()> on_abort_;
    bool                  committed_ = false;
};

} // namespace comdare::cache_engine::builder::bestandslog
