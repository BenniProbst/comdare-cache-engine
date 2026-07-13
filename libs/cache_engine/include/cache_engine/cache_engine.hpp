#pragma once
// ===== DEPRECATED (Muster-C Voll-Review 2026-07-12, verifiziert tot 2026-07-13) =====
// Public-Aggregations-Header ohne Includer; get_cache_engine() deklariert, nie definiert
// (src/facade/ existiert nicht). Kein Konsument (grep=0: kein #include von
// cache_engine/cache_engine.hpp repo-weit, nur Doku-Erwaehnung). Datei-Entfernung =
// separater je-GO (Doku-nie-loeschen-Doktrin G4).
// KEIN [[deprecated]]-Attribut (haelt -Werror fuer Restnutzer).
//
// cache_engine.hpp - Public Aggregation-Header
// Reicht aus, um cache_engine als Konsument einzubinden.

#include "cache_engine/concepts/cache_recommendation.hpp"
#include "cache_engine/concepts/i_cache_engine.hpp"
#include "cache_engine/concepts/i_sub_engine.hpp"
#include "cache_engine/concepts/platform_snapshot.hpp"
#include "cache_engine/concepts/pressure_state.hpp"
#include "cache_engine/concepts/request_context.hpp"
#include "cache_engine/platform_probe/cpuid_probe.hpp"
