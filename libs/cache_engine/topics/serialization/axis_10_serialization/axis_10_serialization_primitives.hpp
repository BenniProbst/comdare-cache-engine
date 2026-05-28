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

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::serialization::axis_10_serialization {

enum class SignalKind : std::uint8_t {
    Normal  = 0,    // signal = 0
    Special = 1,    // signal = 1 (tombstone, layout-switch, layout-marker)
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
        std::size_t   consumed_bytes = 0;  // 0 = Overflow/Fehler
    };

    [[nodiscard]] static Decoded decode(std::span<std::byte const> in) noexcept {
        Decoded result{};
        std::uint64_t shift = 0;
        for (std::byte b : in) {
            ++result.consumed_bytes;
            result.value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(b) & 0x7F) << shift;
            if ((static_cast<std::uint8_t>(b) & 0x80) == 0) break;
            shift += 7;
            if (shift >= 64) {                 // Overflow
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
    void append(SignalKind signal, std::span<std::byte const> payload) {
        std::byte length_buf[9];
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
    [[nodiscard]] std::size_t byte_size()   const noexcept { return buffer_.size(); }

    void clear() noexcept { buffer_.clear(); entry_count_ = 0; }

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

private:
    std::vector<std::byte> buffer_{};
    std::size_t            entry_count_ = 0;
};

}  // namespace comdare::cache_engine::serialization::axis_10_serialization
