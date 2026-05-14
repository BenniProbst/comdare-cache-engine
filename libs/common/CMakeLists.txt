# REV 7.7 V23.C — libs/common/ Cross-Cutting Aggregator
# ClickHouse-Stil: jede Subkomponente eigenes CMake-Target mit comdare::common::<name> Alias.
# Memory-Direktive Domain -> Common (NIE umgekehrt).
#
# Membership (V23.C laufend):
#   succinct/      — V23.C.1 (heute, 2026-05-14) — SDSL-Portierung
#   serialization/ — V23.C.2 (next) — xml_config_parser, JSON-Writer
#   platform/      — V23.C.3 (next) — CPU detect, ISA detect (heute hardware_isa/)
#   measurement/   — V23.C.4 (next) — HW counters, telemetry
#   io/            — V23.C.5 (next) — File, Stream, Buffer
#   logging/       — V23.C.6 (next) — std::cerr-Wrapper
#   error/         — V23.C.7 (next) — ErrorCodes, status_t

add_subdirectory(succinct)
