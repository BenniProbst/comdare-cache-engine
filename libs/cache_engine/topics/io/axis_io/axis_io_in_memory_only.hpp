#pragma once
// V41.F.2 Forwarding-Header (Stufe 2): Achse physisch nach axes/io_dispatch/ migriert.
// Haelt externe #include <topics/io/axis_io/...> + io::axis_io::-Nutzungen gueltig (Stufe 3 = Referenz-Migration).
#include <axes/io_dispatch/axis_io_in_memory_only.hpp>
namespace comdare::cache_engine::io::axis_io { using namespace comdare::cache_engine::io_dispatch; }
