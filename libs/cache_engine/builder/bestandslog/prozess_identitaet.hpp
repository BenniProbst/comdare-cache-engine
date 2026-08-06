#pragma once
// -----------------------------------------------------------------------------
// prozess_identitaet.hpp -- T2-A/F4-NB: der EINE Ort, an dem die Prozess-Id
// dieses Hauses erhoben wird.
//
// EXTRACT, keine Neuerung: current_pid() stand bis 2026-08-06 in
// builder_registration.hpp (dort Z.153-162) und ist von dort hierher gezogen
// worden -- unveraendert, gleicher Namespace, gleiche Semantik. Der Registrar
// inkludiert diesen Header und benutzt sie fortan von hier.
//
// WARUM EIGENER HEADER (Single Source ohne Gewicht -- Praezedenz
// fingerprint_sidecar.hpp, G4b-1/AUF-A2): die atomare Plan-Ablage
// (batch_planner.hpp::schreibe_atomar) braucht einen PROZESS-EINDEUTIGEN
// tmp-Namen und damit dieselbe Zahl. Die beiden Alternativen waeren beide
// falsch gewesen: die Plattform-Weiche im Planer zu wiederholen (zwei
// Wahrheiten ueber dieselbe Frage, und die zweite altert unbemerkt) oder
// builder_registration.hpp in den Planer zu ziehen (der bringt Dokument,
// Factory, Index und Lock samt ctsha512 in eine Datei, die eine Zahl braucht).
// Dieser Header kostet <process.h> bzw. <unistd.h> -- nichts weiter.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib +
// die eine POSIX-/Win32-Weiche.
// -----------------------------------------------------------------------------

#if defined(_WIN32)
#include <process.h> // ::_getpid (Muster test_parser_konsolidierung.cpp:16)
#else
#include <unistd.h> // ::getpid (Muster artifact_cache.hpp:662)
#endif

namespace comdare::cache_engine::builder::bestandslog {

// Prozess-Id dieses Prozesses. ZWEI Konsumenten, zwei Zwecke, EINE Quelle:
//   (1) das Lock-Objekt (make_lock_owner, builder_registration.hpp) -- REIN DIAGNOSTISCH: der Lock
//       vergleicht ausschliesslich owner_uuid; die pid steht im Objekt, damit eine haengende
//       Maschine im Log auffindbar bleibt.
//   (2) der tmp-Name der atomaren Ablage (schreibe_atomar, batch_planner.hpp) -- dort ist sie
//       WIRKSAM: sie trennt die Schreibvorgaenge paralleler Bau-Prozesse derselben Maschine.
[[nodiscard]] inline long current_pid() noexcept {
#if defined(_WIN32)
    return static_cast<long>(::_getpid());
#else
    return static_cast<long>(::getpid());
#endif
}

} // namespace comdare::cache_engine::builder::bestandslog
