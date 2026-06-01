#pragma once
// V41.F.6.1.C Stufe 1 Vendor-Header-Shim: PMR-Resource (W6-Pattern)
//
// @vendor A22 std::pmr::memory_resource (Halpern N3916 C++17)
//
// **Anmerkung:** std::pmr::memory_resource ist Teil der C++17 Standard-Bibliothek
// und IMMER verfuegbar — KEINE Forward-Stubs noetig. Shim existiert nur fuer
// Naming-Konsistenz mit anderen Vendor (Mimalloc/Snmalloc).

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <memory_resource>
