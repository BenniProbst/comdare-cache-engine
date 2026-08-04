#pragma once
// V41.F.6.1.F.6 axis_07_prefetch PathOrientedImpl (2026-05-29)
//
// @topic prefetch @achse 07 @impl path_oriented
//
// **Herkunft:** F.6-Migration aus prt-art `prefetch/path_oriented_prefetch.hpp`
// (PathOrientedPrefetch, REV 6 §5.17 + REV 7.6 V11.1 Hot-Path-Hints) — der
// Algorithmus-Kern der Diplomarbeit (pfad-orientiertes Prefetching). Der
// metadaten-only-Wrapper PathOrientedPrefetch (PF3 granularity) bekommt hierueber
// echte Logik: verfolgt den aktiven Suchpfad, extrapoliert die naechste erwartete
// Adresse und integriert Hot-Path-Hints aus rohen Schluessel-Bytes.
//
// HotPathMixin-Aspekt (V11.1): note_hot_path_bytes() interpretiert die ersten
// 8 Byte eines Schluessels (z.B. binary_key_t aus dem SearchEngine-ABI) als
// uint64-Adresse und speist sie in die Pfad-Trajektorie ein.
//
// SPEICHER (A8-S5, Familie 04_execution, 2026-08-04 -- F2-Schnitt-Regel, Dossier Abschn. 3.4):
// Die Pfad-Trajektorie ist ein rein LOKAL-BOUNDED Arbeitspuffer mit COMPILE-TIME-Kappe
// (kMaxTrackedSlots). Sie liegt deshalb als INLINE-Array im Organ selbst -- KEIN Heap, kein
// Default-Allokator, damit auch kein generischer OS-/libc-Allokations-Call am
// Allokator-Achsen-Interface vorbei. Der frueher noetige Weg ueber die Allokator-Achse
// (StdAllocatorAdapter, vgl. btree_node_pool_store.hpp) entfaellt hier ersatzlos: wo die
// Kappe zur Compile-Zeit feststeht, ist die staerkere Aussage "gar keine Allokation".
// FOLGE (Fehlerklassen-Auflage 11): der bisherige Fehlerpfad [[allocation-failure-exception]]
// (push_back -> std::bad_alloc) EXISTIERT NICHT MEHR. Es entsteht KEIN neuer Mess-Fehlerpfad,
// also auch keine neue A15-Fehlerklasse -- ein Pfad faellt weg, keiner kommt hinzu.
// FOLGE (Vertrag): enqueue()/note_hot_path_bytes() sind jetzt ECHT noexcept. Der Wrapper
// axis_07_prefetch_path_oriented.hpp deklarierte note_hot_path_bytes() schon bisher noexcept
// und rief damit eine potenziell werfende Operation -- das war ein latenter std::terminate-Pfad,
// der mit dem Heap verschwindet (F57/Muster B ist fuer dieses Organ damit gegenstandslos).
// SEMANTIK UNVERAENDERT: FIFO, bounded, aeltestes Element faellt bei Ueberlauf heraus -- exakt
// das Verhalten des vorherigen push_back + erase(begin)-Standes.

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace comdare::cache_engine::prefetch_axis::impl {

/// Pfad-orientierter Prefetch-Tracker (prt-art REV 6 §5.17, V11.1 Hot-Path).
class PathOrientedImpl {
public:
    static constexpr std::size_t kMaxTrackedSlots = 16;

    PathOrientedImpl() = default;

    /// Reiht die naechste erwartete Adresse in die Pfad-Trajektorie ein (FIFO, bounded).
    /// Bei voller Spur wandert das aelteste Element heraus (Schiebe-Schritt ueber die CT-Kappe,
    /// kMaxTrackedSlots-1 Kopien) -- identische Semantik zum frueheren erase(begin), ohne Heap.
    void enqueue(std::uint64_t addr) noexcept {
        if (size_ == kMaxTrackedSlots) {
            for (std::size_t i = 1; i < kMaxTrackedSlots; ++i) recent_path_[i - 1] = recent_path_[i];
            recent_path_[kMaxTrackedSlots - 1] = addr;
        } else {
            recent_path_[size_] = addr;
            ++size_;
        }
        ++total_enqueued_;
    }

    [[nodiscard]] std::uint64_t total_enqueued() const noexcept { return total_enqueued_; }
    [[nodiscard]] std::size_t   queue_depth() const noexcept { return size_; }
    /// Sicht auf die belegte Trajektorie (aeltestes zuerst). std::span statt vector-Referenz:
    /// der Puffer ist inline, es gibt keinen Container-Typ mehr, den man nach aussen reichen koennte;
    /// die Sicht bleibt eine nicht-besitzende const-Sequenz mit size()/back()/Index wie zuvor.
    [[nodiscard]] std::span<std::uint64_t const> path() const noexcept {
        return std::span<std::uint64_t const>{recent_path_.data(), size_};
    }

    /// Empfehlung fuer die naechste Prefetch-Adresse via linearer Schritt-Extrapolation.
    [[nodiscard]] std::uint64_t suggest_next() const noexcept {
        if (size_ < 2) return size_ == 0 ? 0u : recent_path_[size_ - 1];
        std::uint64_t a    = recent_path_[size_ - 2];
        std::uint64_t b    = recent_path_[size_ - 1];
        std::uint64_t step = (b > a) ? (b - a) : 0u;
        return b + step;
    }

    void reset() noexcept {
        size_           = 0;
        total_enqueued_ = 0;
    }

    /// V11.1 Hot-Path-Hint: erste min(8, bytes) Byte als uint64 interpretieren + einreihen.
    /// noexcept ist seit dem A8-S5-Scrub ECHT (enqueue alloziert nicht mehr).
    void note_hot_path_bytes(std::byte const* data, std::size_t bytes) noexcept {
        if (data == nullptr || bytes == 0) return;
        std::uint64_t addr = 0;
        std::size_t   copy = bytes < sizeof(addr) ? bytes : sizeof(addr);
        std::memcpy(&addr, data, copy);
        enqueue(addr);
        ++total_hot_path_hints_;
    }
    [[nodiscard]] std::uint64_t total_hot_path_hints() const noexcept { return total_hot_path_hints_; }

private:
    // Inline-Puffer mit CT-Kappe: kein Heap, kein Default-Allokator (A8-S5 Schnitt-Regel).
    std::array<std::uint64_t, kMaxTrackedSlots> recent_path_{};
    std::size_t                                 size_                 = 0;
    std::uint64_t                               total_enqueued_       = 0;
    std::uint64_t                               total_hot_path_hints_ = 0; // V11.1
};

} // namespace comdare::cache_engine::prefetch_axis::impl
