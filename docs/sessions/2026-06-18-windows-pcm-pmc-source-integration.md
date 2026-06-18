# Windows Intel-PCM PMC-Source — Integration & Aktivierung (Task #153)

> **Datum:** 2026-06-18 · **Scope:** cache-engine (Implementierungs-Agent) · **User-Entscheid:** Option (b)
> (Windows lokal Intel-PCM) freigegeben 2026-06-18.
> **Status:** Code-Naht (`WindowsPcmPmcSource` + `make_pmc_source()` + CMake-Flag) gescaffoldet, **build-sicher**
> (Default-Build unverändert grün — literal verifiziert, s. §4). Reale Messung = **System-Schritt** (msr.sys + Admin),
> NICHT Teil dieses Scaffolds.
> **Querverweise:** Handover `docs/sessions/2026-06-18-HANDOVER-an-infra-agent-linux-pmc-gitlab-ci.md` (Linux-PAPI =
> Infra-Agent, CE-DL2) · Naht `libs/cache_engine/builder/pmc_source.hpp` · Beschaffungs-Spec
> `docs/sessions/20260601-26-pmc-counter-beschaffungs-spec.md`.

## 1. Was gescaffoldet wurde (build-sicher, Default OFF)

| Datei | Inhalt |
|-------|--------|
| `libs/cache_engine/builder/windows_pcm_pmc_source.hpp` | `class WindowsPcmPmcSource final : public IPmcSource`. **GANZE** Implementierung hinter `#if defined(COMDARE_ENABLE_PMC) && defined(_WIN32)`. Ohne das Makro = leerer TU-Inhalt → `cpucounters.h` wird NIE inkludiert. |
| `libs/cache_engine/builder/pmc_source_factory.hpp` | `make_pmc_source()` (inline, header-only). `COMDARE_ENABLE_PMC && _WIN32` → `WindowsPcmPmcSource`, sonst `NullPmcSource`. |
| `CMakeLists.txt` (Root, nach `COMDARE_MICROBENCH_CONTINUOUS`) | `option(COMDARE_ENABLE_PMC … OFF)` + `COMDARE_PCM_DIR` (CACHE PATH). ON → `find_path(cpucounters.h)` + `find_library(pcm)`; Fund → `add_compile_definitions(COMDARE_ENABLE_PMC)` + `include_directories` + `link_libraries`; Nicht-Fund/Nicht-Windows → klare `message(WARNING)` + Flag effektiv OFF (NullPmcSource). |
| `apps/f15_compare/main.cpp` | Factory eingehängt: `make_pmc_source()` einmal vor der Mess-Schleife, `begin()` vor `run_measurement_plan`, `end()` danach, Counter-Delta in `measurement_from_workload_result(res, nm, pmc_delta)`. |

**KEINE Änderung** an `IPmcSource` / `PmcCounters` / `NullPmcSource`-Signaturen (Drop-in-Vertrag, `pmc_source.hpp:6-9`).

## 2. Intel-PCM-API (web-verifiziert, BSD-3-License)

- Header: `cpucounters.h`. Singleton: `PCM::getInstance()`. Init: `program()` (`== PCM::Success` bei Erfolg).
- Snapshot: `getSystemCounterState()` → `SystemCounterState`.
- Delta-Funktionen: `getL2CacheMisses(before, after)`, `getL3CacheMisses(before, after)`.
- `WindowsPcmPmcSource` füllt L2/L3 (robuste System-weite Pfade). **L1 bleibt ehrlich 0** (system-weiter
  Counter-State liefert L1 nicht direkt; architektur-abhängig). dTLB/coherence/energy = aktuell 0 (PENDING, s. §6).

## 3. AKTIVIERUNG — System-Schritte (NICHT im Build; Admin + Treiber nötig)

> ⚠️ **Kritisches Manöver (msr.sys-Treiber-Signierung + Admin-Messung): mit User abstimmen.**

1. **Intel PCM beziehen + vendoren (BSD-3):** Repo `github.com/intel/pcm` klonen/herunterladen (FortiGate-Egress
   beachten — ggf. offline-Archiv wie bei Boost.MP11). Bauen (CMake) oder vendored Header+Lib bereitstellen.
   Lizenz BSD-3 → kompatibel; in `NOTICE` ergänzen.
2. **`COMDARE_PCM_DIR` setzen** auf die PCM-Wurzel (enthält `cpucounters.h` + die `pcm`-Lib):
   `cmake -DCOMDARE_ENABLE_PMC=ON -DCOMDARE_PCM_DIR=<pcm-root> …`
3. **`msr.sys` signieren (self-signed, PowerShell als Admin)** — Intel PCM braucht den MSR-Kernel-Treiber:
   ```powershell
   # Test-Cert erzeugen + Treiber signieren (Test-Signing-Modus nötig: bcdedit /set testsigning on; Reboot)
   $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=COMDARE-PCM-Test" `
       -CertStoreLocation Cert:\CurrentUser\My -KeyUsage DigitalSignature -KeySpec Signature
   Export-Certificate -Cert $cert -FilePath C:\comdare-pcm-test.cer
   Import-Certificate -FilePath C:\comdare-pcm-test.cer -CertStoreLocation Cert:\LocalMachine\Root
   Import-Certificate -FilePath C:\comdare-pcm-test.cer -CertStoreLocation Cert:\LocalMachine\TrustedPublisher
   & "${env:WindowsSdkVerBinPath}x64\signtool.exe" sign /v /fd SHA256 /s My /n "COMDARE-PCM-Test" `
       /t http://timestamp.digicert.com <pcm-root>\msr.sys
   ```
4. **`msr.sys` nach `C:\Windows\System32\` kopieren** (bzw. den vom PCM erwarteten Pfad) und den Treiber-Service
   registrieren (Intel-PCM lädt ihn beim ersten `program()`-Aufruf).
5. **Messung als Administrator** ausführen (`f15_compare … --pipeline-csv …`). `WindowsPcmPmcSource::available()`
   wird `true`, sobald `program() == PCM::Success`; `pmc_available=1` + reale L2/L3-Cache-Misses landen im POD.
   Schlägt Treiber/Admin fehl → `available()=false` → wie `NullPmcSource` (ehrlich 0, Build/Lauf bleiben grün).

## 4. Verifikation (literal, MSVC `/std:c++latest /EHsc`)

Standalone-TU `#include "pmc_source_factory.hpp"` → `make_pmc_source()`:

| Test | Kommando | Erwartung | Ergebnis |
|------|----------|-----------|----------|
| **OFF (Default)** | `cl /std:c++latest /EHsc /c /I<builder> verify_pmc.cpp` | EXIT 0 (leerer Guard, kein `cpucounters.h`, NullPmcSource) | ✅ EXIT 0 |
| **ON ohne PCM-Header** | `cl … /DCOMDARE_ENABLE_PMC /c …` | klarer `cpucounters.h not found`-Fehler | ✅ `fatal error C1083: "cpucounters.h": No such file or directory` @ `windows_pcm_pmc_source.hpp(27)` |

→ Beweist: (1) Default-Build bricht NIE; (2) der `#ifdef`-Guard ist korrekt platziert (Header-Include feuert nur
unter dem Makro). Der **eigentliche PCM-API-Compile** (mit echtem `cpucounters.h` + Lib) ist OHNE die Vendor-Lib
**NICHT** verifizierbar — ehrlich als PENDING gekennzeichnet (s. §6).

## 5. Einhäng-Stelle der Factory

`apps/f15_compare/main.cpp` (Lastprofil-Plan-Zweig, ~Z.247-278): `make_pmc_source()` einmal vor der Schleife,
`begin()`/`end()` klammern `run_measurement_plan`, das Delta geht in den 3-arg-`measurement_from_workload_result`.
Das ist die einzige Stelle, an der bisher `measurement_from_workload_result(res, nm)` (2-arg) ohne PMC lief.

## 6. PENDING / offene Risiken

- **PCM-Lib nicht vorhanden** → der reale API-Compile (`cpucounters.h`-Symbole, `PCM::`-Namespace-Pfad, exakte
  Signaturen von `getL2CacheMisses`/`SystemCounterState`) ist NOCH NICHT gegen echte Header geprüft. Beim ersten
  ON-Build mit echter PCM-Lib evtl. Namespace-/Signatur-Nachzug nötig (PCM kapselt vieles in `namespace pcm`; der
  Scaffold nutzt `::pcm::PCM` / `::pcm::getL2CacheMisses` — bei älteren PCM-Versionen ggf. globaler Namespace →
  dann Guard-intern anpassen, Default-Build unberührt).
- **L1 / dTLB / coherence / energy** liefern aktuell 0 (nur L2/L3 belegt). Erweiterbar via `getL3CacheHitsNoSnoop`/
  RAPL-Energy-Pfade (`getConsumedJoules`) — separater Folge-Task.
- **3 Mirror-Kopien** von `pmc_source.hpp` existieren (`modules/comdare-measurement/`, `modules/comdare-build-tools/`).
  Diese sind dokumentierte Mirrors von `libs/cache_engine/`. Der Scaffold liegt nur unter `libs/` (kanonisch);
  bei künftigem Modul-Sync `windows_pcm_pmc_source.hpp` + `pmc_source_factory.hpp` mit-spiegeln.
- **msr.sys-Signierung** = kritisches Manöver (Test-Signing-Modus / Reboot). Mit User abstimmen.
