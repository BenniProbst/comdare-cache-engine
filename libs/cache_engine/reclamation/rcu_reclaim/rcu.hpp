// SPDX-License-Identifier: LicenseRef-Comdare-Research-1.0
// Copyright (c) 2026 BEP Venture UG (haftungsbeschraenkt), Marke Comdare
//
// comdare::rcu — eigene Read-Copy-Update-Implementation (Aufgabe #104)
// REV 7 §6 (PRT-ART konsumiert CacheEngine) + REV 6 §5.17 (Concurrency-Pool)
//
// Ersetzt liburcu (P29 ext/) durch eine C++23-Header-only-Implementation
// mit den Kern-Primitiven:
//
//   RcuDomain         - global verwaltete Grace-Period-Sequenz
//   RcuReadGuard      - RAII fuer rcu_read_lock/unlock
//   RcuDeferred       - call-rcu-Pattern (deferred reclamation)
//
// Diese Implementation ist KONZEPT-konform zu McKenney et al. RCU 2001 OLS
// (P29 in der Bausteine-Matrix), aber bewusst SIMPLIFIZIERT:
//   - keine per-CPU-Zaehler (epoch-based statt quiescent-state-based)
//   - keine kernel-bypass-Optimierungen
//
// Garantien (siehe RCU-Paper §4.2 "Read-Side Constraints"):
// 1. Reader sehen entweder die alte ODER die neue Version, NIE eine Mischung
// 2. Reader-Pfad ist wait-free (nur ein atomic_load + Memory-Barrier)
// 3. Writer warten via synchronize() bis alle aktiven Reader fertig sind
//
// EIGENTUMSMODELL (Feature #25, honest-100 %) — Ownership-Inversion + Generational Handle:
//   Die DOMAIN besitzt die Reader-States (Object-Pool in einem adress-stabilen std::deque). Der
//   Thread haelt nur einen generation-gesicherten Cache-Zeiger je (Domain,Thread). McKenney-faithful
//   (P29 urcu.c:121/479/493): die Registry ueberlebt jeden Reader-Thread; Registrierung ist explizit
//   und aus dem wait-free Read-Hot-Path herausgehoben (rcu_register_thread-Analogon). Damit ist die
//   frueher moegliche Dangling-Klasse STRUKTURELL ausgeschlossen — an BEIDEN Enden:
//     - Thread-Ende: der Slot lebt Domain-owned weiter (inert, active=false). Der thread_local Cache
//       haelt nur Skalare/Pointer → sein Teardown dereferenziert NIE die Domain (kein Shutdown-UAF).
//     - Domain-Ende (auch Stack-Domains): der deque wird zerstoert; ein toter (serial,ptr)-Cache-Eintrag
//       wird nie wieder gematcht (Serials sind monoton, nie wiederverwendet) und nie dereferenziert.
//     - Adress-Wiederverwendung (sequenzielle Stack-Domains an gleicher Adresse): neuer serial_ →
//       Cache-Miss → frischer Slot. Kein Stale-Deref.

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// A2.5/G4 M-2 (2026-08-19): TSan-Erkennung fuer die Fence-Naht unten. GCC setzt
// __SANITIZE_THREAD__, Clang meldet thread_sanitizer ueber __has_feature -- beide Wege werden
// gefragt, damit die Bruecke nicht vom Compiler abhaengt.
#if defined(__SANITIZE_THREAD__)
#define COMDARE_RCU_TSAN_BRUECKE 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define COMDARE_RCU_TSAN_BRUECKE 1
#endif
#endif
#ifndef COMDARE_RCU_TSAN_BRUECKE
#define COMDARE_RCU_TSAN_BRUECKE 0
#endif

namespace comdare::rcu {

class RcuDomain;

// ============================================================================================
// DIE FENCE-NAHT (A2.5/G4 M-2) -- EINE Funktion, ZWEI Traeger, und warum es sie gibt
// ============================================================================================
// Das SB/Dekker-Muster dieser Domain haengt an ZWEI seq_cst-Fences: read_lock (active=true MUSS
// global sichtbar sein, BEVOR die geschuetzten Loads folgen) und synchronize (Epoch-Bump MUSS
// sichtbar sein, BEVOR die active-Loads folgen). Verboten ist, dass BEIDE Seiten "das Alte"
// sehen -- das waere die verfruehte Grace-Period-Beendigung (UAF).
//
// DIE WERKZEUG-GRENZE, GEMESSEN (g4_m2_vorher_wtsan_warnung.log, GCC 15.3): -fsanitize=thread
// modelliert atomic_thread_fence NICHT -- "atomic_thread_fence is not supported with
// -fsanitize=thread [-Wtsan]", inlined an genau den beiden Naht-Punkten (vormals rcu.hpp:105
// und :125). Ein TSan-Lauf prueft die Fence-Ordnung also nicht, er wirft sie weg; gruene
// TSan-Laeufe deckten die Naht strukturell nie (Grenz-Doku test_rcu_concurrency.cpp, 17.08.).
//
// DIE BRUECKE: unter TSan faehrt dieselbe Naht als seq_cst-RMW auf EINER geteilten Atomic.
// Das ist REAL mindestens fence-stark (RMWs auf derselben Variable bilden eine Totalordnung;
// der spaetere RMW synchronisiert mit dem frueheren -- Lehrbuch-Alternative des Dekker-Musters)
// UND fuer TSan voll modellierbar: die happens-before-Kante existiert in seinem Modell, die
// Naht wird geprueft statt ignoriert. Ausserhalb von TSan bleibt die Fence unveraendert --
// der wait-free Read-Hot-Path traegt im Normal-Build KEINE geteilte RMW-Last.
//
// EINE Funktion statt zwei Inline-Stellen: die Struktur-Wache im Test
// (test_rcu_concurrency.cpp, FenceNahtStruktur*) liest diese Datei und haelt fest, dass BEIDE
// Naht-Punkte DIESE Funktion rufen und dass beide Zweige seq_cst tragen. Wer die Naht schwaecht
// oder einen Punkt am Ruf vorbeifuehrt, bricht die Wache deterministisch -- kein Timing-Flake.
#if COMDARE_RCU_TSAN_BRUECKE
// Die geteilte Brueckens-Atomic. inline (C++17): EINE Instanz ueber alle TUs.
inline std::atomic<std::uint64_t> g_tsan_fence_bridge{0};
#endif

inline void store_load_fence_seq_cst() noexcept {
#if COMDARE_RCU_TSAN_BRUECKE
    (void)g_tsan_fence_bridge.fetch_add(1, std::memory_order_seq_cst);
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

// Per-Thread-Reader-State (epoch-based)
struct alignas(64) RcuReaderState {
    std::atomic<std::uint64_t> local_epoch{0}; // 0 = nicht im Read-Block
    std::atomic<bool>          active{false};
    // Nesting-Zaehler fuer rekursive/geschachtelte read-side critical sections (McKenney-RCU nestet).
    // Nur der EIGENE Thread greift darauf zu (per-(Domain,Thread)-State) → nicht-atomar ausreichend,
    // kein Data-Race (Writer liest ausschliesslich local_epoch/active). Verhindert, dass ein innerer
    // Guard den Reader fuer den GANZEN Thread als inaktiv markiert, waehrend ein aeusserer Guard noch
    // im kritischen Abschnitt ist.
    std::uint32_t nesting{0};
};

// Monotone, global eindeutige Domain-Seriennummer (Generational-Handle-Muster gegen Adress-
// Wiederverwendung): jede RcuDomain bekommt genau EINE, nie wiederverwendete serial_. Ein
// thread_local Cache-Eintrag einer laengst zerstoerten Stack-Domain kann so NIE faelschlich auf
// eine neue Domain an derselben Adresse matchen. Startet bei 1 (0 = "keine Domain").
inline std::atomic<std::uint64_t> g_next_rcu_serial{1};

// RcuDomain: globaler Grace-Period-Counter + Reader-Registry (Domain-owned Object-Pool)
class RcuDomain {
public:
    // Explizite Registrierung (McKenney rcu_register_thread-faithful, P29 urcu.c:479). DARF allozieren
    // /werfen — bewusst NICHT noexcept: der First-Touch je (Domain,Thread) legt den Slot an (ehrlich).
    // Idempotent: liefert stets denselben, adress-stabilen Slot dieses Threads in DIESER Domain.
    [[nodiscard]] RcuReaderState* register_thread() {
        // Steady-State: allokationsfreier Scan des thread_local Caches (serial_-gekeyt).
        for (auto const& [serial, slot] : tls_cache()) {
            if (serial == serial_) { return slot; }
        }
        // First-Touch: Slot Domain-owned im deque anlegen (deque: emplace_back invalidiert KEINE
        // Adressen bestehender Slots → der gecachte Zeiger bleibt lebenslang gueltig).
        std::lock_guard lock{registry_mutex_};
        RcuReaderState* slot = &readers_.emplace_back();
        tls_cache().emplace_back(serial_, slot);
        return slot;
    }

    // Ergonomischer Lazy-Fallback (identische Semantik zu register_thread()). Beibehalten fuer die
    // deterministische Test-Introspektion (test_rcu.cpp) und API-Kompatibilitaet.
    [[nodiscard]] RcuReaderState* current_thread_state() { return register_thread(); }

    // Wait-free Read-Hot-Path (Garantie 2): reiner atomic-Store + Memory-Barrier, allokationsfrei,
    // noexcept. Der Slot MUSS zuvor via register_thread()/current_thread_state() beschafft sein
    // (RcuReadGuard hebt das aus dem Hot-Path heraus). Nur der AEUSSERSTE Guard (nesting 0→1)
    // publiziert local_epoch/active; geschachtelte Guards erhoehen nur den Zaehler.
    void read_lock(RcuReaderState* s) noexcept {
        if (s->nesting++ == 0) {
            s->local_epoch.store(global_epoch_.load(std::memory_order_acquire), std::memory_order_relaxed);
            s->active.store(true, std::memory_order_release);
            // Store-Load-Fence (SB/Dekker-Muster): das active=true MUSS global sichtbar sein, BEVOR die
            // geschuetzten Loads (alter Pointer) folgen. Zusammen mit dem seq_cst-Fence in synchronize()
            // ist ausgeschlossen, dass der Writer den Reader als inaktiv uebersieht, waehrend dieser noch
            // den alten Pointer liest → verhindert die verfruehte Grace-Period-Beendigung (UAF).
            // M-2: die EINE Naht-Funktion (Fence; unter TSan die modellierbare Bruecke, s. Kopf).
            store_load_fence_seq_cst();
        }
    }

    void read_unlock(RcuReaderState* s) noexcept {
        // Nur der AEUSSERSTE Guard (nesting 1→0) beendet den kritischen Abschnitt. Release-Store stellt
        // sicher, dass alle geschuetzten Loads vor active=false abgeschlossen sind.
        if (s->nesting != 0 && --s->nesting == 0) { s->active.store(false, std::memory_order_release); }
    }

    // Lazy-Fallback-Overloads (registrieren beim First-Touch → koennen allozieren, daher NICHT noexcept).
    void read_lock() { read_lock(register_thread()); }
    void read_unlock() { read_unlock(register_thread()); }

    // Writer-Pfad: erhoeht global_epoch und wartet, bis alle Reader
    // den Block verlassen haben (busy-wait mit yield).
    void synchronize() noexcept {
        std::uint64_t const target_epoch = global_epoch_.fetch_add(1, std::memory_order_seq_cst) + 1;
        // seq_cst-Fence (Gegenstueck zum read_lock-Fence): trennt Epoch-Bump/Pointer-Publikation von den
        // folgenden active-Loads → SB-Muster geschlossen (siehe read_lock).
        // M-2: die EINE Naht-Funktion (Fence; unter TSan die modellierbare Bruecke, s. Kopf).
        store_load_fence_seq_cst();

        std::lock_guard lock{registry_mutex_};
        // Iteriert die Domain-owned Slots direkt (keine rohen Zeiger auf fremd-owned Speicher mehr →
        // das Dangling-Finding kann per Konstruktion nicht wiederkehren). Slots ausgeschiedener Threads
        // sind inert (active=false) und werden korrekt uebersprungen.
        for (auto& r : readers_) {
            // Warte, bis Reader entweder inaktiv ist oder die neue Epoch sieht.
            while (true) {
                if (!r.active.load(std::memory_order_acquire)) { break; }
                if (r.local_epoch.load(std::memory_order_acquire) >= target_epoch) { break; }
                std::this_thread::yield();
            }
        }
    }

    [[nodiscard]] std::uint64_t current_epoch() const noexcept { return global_epoch_.load(std::memory_order_acquire); }

    [[nodiscard]] std::size_t reader_count() noexcept {
        std::lock_guard lock{registry_mutex_};
        return readers_.size();
    }

    // Singleton-Default-Domain
    [[nodiscard]] static RcuDomain& instance() noexcept {
        static RcuDomain d;
        return d;
    }

private:
    // (serial, slot)-Cache je Thread. Funktions-lokal thread_local (EINE Instanz je Thread, ueber alle
    // Domains geteilt), aber per serial_ dis-ambiguiert → jeder (Domain,Thread) bekommt einen EIGENEN
    // Slot. Der Cache haelt ausschliesslich Skalare + rohe Zeiger; sein Teardown bei Thread-Ende
    // dereferenziert NIE eine Domain (kein Shutdown-UAF). Tote Eintraege sind inert (nie gematcht/deref).
    static std::vector<std::pair<std::uint64_t, RcuReaderState*>>& tls_cache() noexcept {
        thread_local std::vector<std::pair<std::uint64_t, RcuReaderState*>> cache;
        return cache;
    }

    // Monoton-eindeutige Identitaet dieser Domain (Generational-Handle-Schluessel).
    std::uint64_t const serial_ = g_next_rcu_serial.fetch_add(1, std::memory_order_relaxed);

    std::atomic<std::uint64_t> global_epoch_{1};
    std::mutex                 registry_mutex_;
    // std::deque (NICHT std::vector): emplace_back invalidiert KEINE Adressen bestehender Elemente →
    // die im tls_cache gehaltenen Slot-Zeiger bleiben lebenslang stabil. Die Domain BESITZT die States.
    std::deque<RcuReaderState> readers_{};
};

// RAII Reader-Guard
class [[nodiscard]] RcuReadGuard {
public:
    // Ctor registriert einmalig (register_thread() — darf allozieren/werfen, aus dem wait-free Pfad
    // herausgehoben) und betritt dann den kritischen Abschnitt ueber den allokationsfreien noexcept
    // Fast-Path. Bewusst NICHT noexcept: der First-Touch je (Domain,Thread) alloziert (ehrlich).
    explicit RcuReadGuard(RcuDomain& d = RcuDomain::instance()) : domain_{&d}, state_{d.register_thread()} {
        domain_->read_lock(state_);
    }

    ~RcuReadGuard() {
        if (domain_) { domain_->read_unlock(state_); }
    }

    RcuReadGuard(RcuReadGuard const&)            = delete;
    RcuReadGuard& operator=(RcuReadGuard const&) = delete;
    RcuReadGuard(RcuReadGuard&& o) noexcept : domain_{o.domain_}, state_{o.state_} { o.domain_ = nullptr; }
    RcuReadGuard& operator=(RcuReadGuard&&) = delete;

private:
    RcuDomain*      domain_;
    RcuReaderState* state_;
};

// Deferred-Reclamation (call_rcu-Pattern): registriert Callbacks, die nach
// der naechsten synchronize() aufgerufen werden.
class RcuDeferred {
public:
    void defer(std::function<void()> fn) {
        std::lock_guard lock{mutex_};
        pending_.push_back(std::move(fn));
    }

    // Sync alle deferred callbacks: erst synchronize() der Domain,
    // dann alle Callbacks ausfuehren + leeren.
    void flush(RcuDomain& d = RcuDomain::instance()) {
        std::vector<std::function<void()>> snapshot;
        {
            std::lock_guard lock{mutex_};
            snapshot.swap(pending_);
        }
        d.synchronize();
        for (auto& fn : snapshot) fn();
    }

    [[nodiscard]] std::size_t pending_count() noexcept {
        std::lock_guard lock{mutex_};
        return pending_.size();
    }

private:
    std::mutex                         mutex_;
    std::vector<std::function<void()>> pending_;
};

// Helper: atomic-swap eines Pointers + defer der alten Version
template <typename T>
inline void rcu_replace(std::atomic<T*>& slot, T* new_value, RcuDeferred& def) {
    T* old = slot.exchange(new_value, std::memory_order_acq_rel);
    if (old) {
        def.defer([old]() { delete old; });
    }
}

} // namespace comdare::rcu
