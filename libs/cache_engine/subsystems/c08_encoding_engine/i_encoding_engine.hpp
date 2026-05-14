#pragma once
// C08 IEncodingEngine — Succinct + Compression Encodings (F2/F16 + P04/P09/P10/P15)
// Termin 7 / 11_md F2+F16

#include <cstdint>

namespace comdare::cache_engine::subsystems::encoding {

enum class EncodingKind : std::uint8_t {
    LoudsJacobson        = 0,   // P09
    LoudsDense           = 1,   // P10
    LoudsSparse          = 2,   // P10
    EliasFano            = 3,   // P04 CoCo Pool
    PackedArray          = 4,   // P04 CoCo Pool
    Bitvector            = 5,   // P04 CoCo Pool
    DenseEncoding        = 6,   // P04 CoCo Pool
    PointerElimination   = 7,   // P11 CSS
    PartialPointerElim   = 8,   // P12 CSB+
    OrderPreservingHuffman = 9, // P15 Survey
    PrefixTruncation     = 10,  // P15+P20
    KeyNormalization     = 11,  // P15+P01
};

class IEncodingEngine {
public:
    virtual ~IEncodingEngine() = default;

    [[nodiscard]] virtual EncodingKind kind() const noexcept = 0;

    // Encode: input → kompakte Repraesentation. Liefert geschriebene Bytes.
    [[nodiscard]] virtual std::size_t encode(void const* input,
                                             std::size_t input_bytes,
                                             void* output,
                                             std::size_t output_capacity) noexcept = 0;

    [[nodiscard]] virtual std::size_t decode(void const* encoded,
                                             std::size_t encoded_bytes,
                                             void* output,
                                             std::size_t output_capacity) noexcept = 0;

    [[nodiscard]] virtual double estimated_compression_ratio() const noexcept = 0;
};

}  // namespace comdare::cache_engine::subsystems::encoding
