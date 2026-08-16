#pragma once
// axis_10_serialization_primitives.hpp — VarLen-Encoding + Signaling-Stream (BASIS).
//
// V41.F.6.1 F.6 Migration (Doku 19, Phase A): aus prt-art (comdare::prt_art::serialization)
// nach cache-engine migriert — VarLen (Protobuf-style 7-Bit-Continue) + Signaling-Stream
// (1 Signal-Byte + varlen-Length + Payload) sind cross-cutting Serialisierungs-BASIS, die
// die axis_10-Wrapper (VarLenSerialization/RawBinary/...) konsumieren. Der Pruefling prt-art
// erbt/konsumiert diese Primitive (Richtung prt-art -> cache-engine).
//
// Layout pro Eintrag: [signal_bit:1 Byte] [serialized_length:1-9 varlen] [payload:length].
//
// A8-S5 Familie 02a (2026-08-04) -- SCHNITT-REGEL (Dossier 20260803-a8_f2 Abschn. 3.4: "Speicher NUR ueber
// das Allokator-Achsen-Interface"): der Byte-Puffer des SignalingStream lief bis hierher ueber den
// Default-Allokator. Er ist der EINZIGE besitzende Container der serialization-Achse (Familien-grep) und
// laeuft seit dem Schnitt ueber das Achsen-Interface. Das WIRE-FORMAT ist davon nicht beruehrt: Signal-Byte,
// varlen-Laenge und Payload-Bytes sind unveraendert, raw()/decode_one liefern dieselben Bytes -- der Schnitt
// wechselt die Speicher-QUELLE, nicht die Kodierung (G8: kein ABI, kein Wire, kein Fingerprint).

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp> // A8-S5-02a: Versorger-Achse des Stream-Puffers (s.u.)

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::serialization_axis {

/// A8-S5 Familie 02a: die EINE Stelle, die den Versorger des Stream-Puffers benennt (kein zweiter Name kann
/// driften). Begruendung der Wahl identisch zur path_compression-Seite dieser Scheibe und zu den Scheiben
/// 03/01d: derselbe Achsen-Default, den die bereits konformen Pool-Stores fuehren
/// (btree_node_pool_store.hpp:56); der Stream ist single-threaded append-only, die Exgen-Sub-Achse AA4
/// (Single-Threaded Specialized) passt; bei abgeschaltetem Vendor-Flag derselbe libc-Heap wie vorher, aber
/// ueber das Achsen-Interface. Kein Kompositions-Allokator: SignalingStream ist kein Template ueber A und
/// kennt die Komposition nicht (Abgrenzung zum HERZ-Fall der node-Achse, wo der Store Template ueber A IST).
/// ABHAENGIGKEITSRICHTUNG (Dossier 3.3): serialization -> alloc, die unterste Versorger-Achse; organ_axes/alloc/
/// zieht keinen serialization-Header -> kein Zyklus.
using serialization_stream_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

enum class SignalKind : std::uint8_t {
    Normal  = 0, // signal = 0
    Special = 1, // signal = 1 (tombstone, layout-switch, layout-marker)
};

/// VarLenEncoder — Protobuf-style 7-Bit-Continue VarInt (1..9 Bytes).
class VarLenEncoder {
public:
    /// Encode value in 1..9 Bytes; gibt Anzahl geschriebener Bytes zurueck.
    [[nodiscard]] static std::size_t encode(std::uint64_t value, std::byte* out, std::size_t cap) noexcept {
        std::size_t written = 0;
        while (value >= 0x80 && written < cap) {
            out[written++] = static_cast<std::byte>((value & 0x7F) | 0x80);
            value >>= 7;
        }
        if (written < cap) out[written++] = static_cast<std::byte>(value & 0x7F);
        return written;
    }

    struct Decoded {
        std::uint64_t value          = 0;
        std::size_t   consumed_bytes = 0; // 0 = Overflow/Fehler
    };

    [[nodiscard]] static Decoded decode(std::span<std::byte const> in) noexcept {
        Decoded       result{};
        std::uint64_t shift = 0;
        for (std::byte b : in) {
            ++result.consumed_bytes;
            result.value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(b) & 0x7F) << shift;
            if ((static_cast<std::uint8_t>(b) & 0x80) == 0) break;
            shift += 7;
            if (shift >= 64) { // Overflow
                result.consumed_bytes = 0;
                result.value          = 0;
                break;
            }
        }
        return result;
    }
};

/// SignalingStream — Append-only Stream aus (Signal, Payload)-Eintraegen mit varlen-Length.
class SignalingStream {
public:
    /// A8-S5-02a Form-B-Ausweis: der Speicher dieses Organs laeuft ueber die Allokator-Achse -- real
    /// verdrahtet (buffer_ traegt den StdAllocatorAdapter dieses allocator_, s. Member unten +
    /// stream_allocator_statistics()).
    using allocator_type = serialization_stream_allocator_t;

private:
    using byte_alloc = typename allocator_type::template StdAllocatorAdapter<std::byte>;

public:
    // -- A8-S5-02a Lebensdauer-Vertrag der Achsen-Verdrahtung (Muster btree_node_pool_store.hpp:19/:84-105) --
    //   (a) allocator_ MUSS vor buffer_ deklariert sein (Member-Reihenfolge unten),
    //   (b) buffer_ wird IMMER mit allocator_.as_std_allocator<std::byte>() konstruiert (der Adapter ist
    //       nicht default-konstruierbar -> kein stilles Zurueckfallen auf einen Default-Allokator),
    //   (c) die Kopie REBINDET auf das EIGENE allocator_ (sonst zeigte der Adapter der Kopie auf die Quelle;
    //       raw() gibt eine span AUF diesen Puffer heraus -- eine geteilte Speicherquelle waere hier
    //       besonders heimtueckisch) + verwirft die transiente Kopier-Pollution per restore_statistics,
    //   (d) Move bewusst nicht deklariert -> degradiert zur korrekt rebindenden Kopie.
    SignalingStream() : buffer_(allocator_.template as_std_allocator<std::byte>()) {}
    SignalingStream(SignalingStream const& o)
        : allocator_(o.allocator_), buffer_(o.buffer_, allocator_.template as_std_allocator<std::byte>()),
          entry_count_(o.entry_count_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    SignalingStream& operator=(SignalingStream const& o) {
        if (this != &o) {
            buffer_      = o.buffer_; // Allokator propagiert NICHT (POCCA=false) -> eigener Adapter bleibt
            entry_count_ = o.entry_count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~SignalingStream() = default;

    /// SONDERFALL [[allocation-failure-exception]] (A8-S5-02a, Auflage 11 -- Fehlerklassen): das Puffer-
    /// Wachstum laeuft seit dem Schnitt ueber die Allokator-ACHSE statt ueber operator new. Die Strategie
    /// meldet OOM per nullptr; der StdAllocatorAdapter uebersetzt das seit Posten 64 an EINER Stelle in
    /// std::bad_alloc (axis_06_allocator_strategy_base.hpp) -- der Wurf bleibt, nur der Traeger wechselt.
    /// Fehlerklasse unveraendert der FK-5-Boden der Allokator-Achse (kOrganAxisErrorFloor).
    void append(SignalKind signal, std::span<std::byte const> payload) {
        std::byte   length_buf[9];
        std::size_t length_bytes =
            VarLenEncoder::encode(static_cast<std::uint64_t>(payload.size()), length_buf, sizeof(length_buf));
        buffer_.push_back(static_cast<std::byte>(static_cast<std::uint8_t>(signal)));
        buffer_.insert(buffer_.end(), length_buf, length_buf + length_bytes);
        buffer_.insert(buffer_.end(), payload.begin(), payload.end());
        ++entry_count_;
    }

    [[nodiscard]] std::span<std::byte const> raw() const noexcept {
        return std::span<std::byte const>(buffer_.data(), buffer_.size());
    }
    [[nodiscard]] std::size_t entry_count() const noexcept { return entry_count_; }
    [[nodiscard]] std::size_t byte_size() const noexcept { return buffer_.size(); }

    void clear() noexcept {
        buffer_.clear();
        entry_count_ = 0;
    }

    struct Entry {
        SignalKind                 signal = SignalKind::Normal;
        std::span<std::byte const> payload{};
        std::size_t                next_offset = 0;
    };

    [[nodiscard]] static Entry decode_one(std::span<std::byte const> stream, std::size_t offset) noexcept {
        Entry e{};
        if (offset + 1 > stream.size()) return e;
        e.signal = static_cast<SignalKind>(static_cast<std::uint8_t>(stream[offset]));
        auto dec = VarLenEncoder::decode(stream.subspan(offset + 1));
        if (dec.consumed_bytes == 0) return e;
        std::size_t payload_start = offset + 1 + dec.consumed_bytes;
        std::size_t payload_end   = payload_start + dec.value;
        if (payload_end > stream.size()) return e;
        e.payload     = stream.subspan(payload_start, dec.value);
        e.next_offset = payload_end;
        return e;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// A8-S5-02a VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128): die
    /// Statistik der Versorger-Strategie DIESES Organs -- macht die Form-B-Aussage der S5-Gate-Wache am
    /// Objekt pruefbar (ein bloss deklarierter allocator_type bliebe hier auf 0 stehen). NICHT im T9-Mess-
    /// Pfad (T9 misst serialize/deserialize) und NICHT im T6-Pfad -- keine Doppelzaehlung.
    [[nodiscard]] typename allocator_type::snapshot_t stream_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // A8-S5-02a: allocator_ VOR buffer_ (Lebensdauer des Zeigers im StdAllocatorAdapter, s. Ctor-Kommentar).
    allocator_type                     allocator_{};
    std::vector<std::byte, byte_alloc> buffer_;
    std::size_t                        entry_count_ = 0;
};

} // namespace comdare::cache_engine::serialization_axis
