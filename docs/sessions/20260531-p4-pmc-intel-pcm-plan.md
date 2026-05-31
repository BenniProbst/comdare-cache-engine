# P4-PMC — Intel PCM auf i7-1270P: Geräte-Status + Beschaffungs-/Setup-Plan

**Stand:** 2026-05-31 · **Recherche:** Workflow `wh17bc1dm` (4 Web-Dimensionen + Synthese, 5 Agenten).
**Kontext:** /goal-Strang P4-PMC (echte HW-Counter für cache_misses_l1/l2/l3, dtlb_misses,
coherence_invalidations, energy_micro_joules). Ergänzt `architektur-ziele-offene-punkte-ledger.md` §b R5.D.

---

## 0. Geräte-Status (live verifiziert 2026-05-31, read-only PowerShell)

| Faktor | Wert | Konsequenz |
|--------|------|------------|
| Secure Boot | **AUS** (`Confirm-SecureBootUEFI=False`) | KEIN BIOS-Eingriff nötig; `bcdedit testsigning` greift direkt |
| BitLocker C: | **AUS** (FullyDecrypted) | kein Recovery-Key-Risiko |
| HVCI / Memory Integrity | **AKTIV+erzwungen** (VBS=2, CodeIntegrity=2, Reg Enabled=1, Services 2,3,4,5,7) | **Blocker** — muss temporär aus, um test-signierten MSR.sys zu laden |
| CPU | i7-1270P, 12 Cores / 16 Threads (4P Golden Cove + 8E Gracemont) | Hybrid → Core-Pinning + P/E-getrennte Event-Codes nötig |
| Toolchain | VS2022 Community + signtool vorhanden | fehlt noch: Spectre-libs + WDK |

**Netto:** Die zwei riskantesten Manöver (Secure Boot, BitLocker) entfallen. Einziger sicherheitsrelevanter
Schritt = Memory Integrity temporär deaktivieren (reversibel, Windows-Sicherheit-Toggle + Reboot).

---

## 1. Empfohlener Weg

Intel PCM (BSD-3, github.com/intel/pcm) aus Quellen bauen + eigenen `MSR.sys` test-signieren. Nur PCM liefert
`energy_micro_joules` (RAPL) UND `cache_misses_l2/l3` direkt und ist vendor-fest statisch linkbar.
Treiberloser ETW-Weg (`wpr`/`xperf -pmc`) = Fallback, deckt aber KEINE Energie ab (siehe §5).

> ⚠️ BSOD-Warnung (intel/pcm Issue #566): auf Win11+Alder-Lake-Hybrid ist ein SYSTEM_SERVICE_EXCEPTION (wrmsr)
> dokumentiert. Erst-Test auf einer Maschine, deren Reboot vertretbar ist — NICHT während ZIH/Cluster-Arbeit.

## 2. User-Schritte (bereinigt nach Geräte-Status)

**EINMALIG:**
1. VS2022: Workloads „Desktop development with C++" + „.NET desktop development" + Komponente
   „MSVC v143 Spectre-mitigated libs (Latest)" (ohne → Treiber-Build MSB4044) + **WDK** (mit VS-Extension).
2. ~~Secure Boot / BitLocker prüfen~~ → bereits geklärt (beide aus).
3. **Memory Integrity AUS:** Einstellungen → Datenschutz & Sicherheit → Windows-Sicherheit → Gerätesicherheit
   → Kernisolierung → **Speicherintegrität = Aus** → **Reboot**. (Sicherheits-Implikation: §3.)
4. PCM-Quellen beschaffen (FortiGate: SourceForge-Mirror `intel-pcm.mirror` `202604.tar.gz` ODER
   GitHub-zip mit FortiGate-CA im Trust-Store). Entpacken → Pfad mir nennen → ich vendore nach `ext/pcm/`.
5. PCM bauen: `cmake -B build && cmake --build build --config Release --parallel` → `pcm.exe`.
6. MSR-Treiber bauen (Dev-Prompt VS2022, `src\WinMSRDriver`):
   `MSBuild MSR.vcxproj -p:Configuration=Release -p:Platform=x64` → `MSR.sys`.
7. Treiber test-signieren (Admin-PowerShell): `New-SelfSignedCertificate -Type CodeSigning` →
   `Export-PfxCertificate` → `signtool sign /fd SHA256 /f TestCert.pfx /p <pw> /t http://timestamp.digicert.com MSR.sys`
   → Zertifikat in Trusted-Root-CA importieren (certlm.msc).
8. `bcdedit /set testsigning on` (Admin) + Reboot. (Secure Boot ist aus → greift direkt.)
9. `MSR.sys` nach System32, `pcm.exe`+DLLs nach `C:\Program Files\PCM`.
10. Funktionstest (Admin-CMD): `pcm.exe` → IPC/L2/L3/Energie-Tabelle. Erste Ausgabe mir schicken.
    Falls „Hybrid processors are not supported" → Version zu alt (`202604` vermeidet das).

**PRO MESSUNG:** Admin-Prompt; kein anderer PMU-Konsument (VTune); optional BIOS Turbo aus + Warm-up
verwerfen. **Nach Messreihe:** `bcdedit /set testsigning off` + Memory Integrity wieder AN + Reboot.

## 3. Entscheidungspunkte (Risiko)

- **Memory Integrity aus** (Pflicht für diesen Weg): senkt Kernel-Schutz, solange aus. Auf einer Maschine mit
  Cluster-Credentials = bewusst temporär + danach reaktivieren. Weniger drastisch als Secure-Boot-Aus.
- **WinRing0**: NICHT verwenden (CVE-2020-14979, von Defender geblockt).
- **Drittanbieter-PCM-Binaries** (larsnm): spart Bauen, nicht Intel-auditiert → nur Test/Thesis tolerierbar.
- **Energie = Package** (i7-1270P hat KEINE DRAM-RAPL-Domain). `energy_micro_joules` = PKG.
- **PCM-Version**: Vorschlag `202604` (zu bestätigen).

## 4. Was ICH danach übernehme

1. Vendoring `ext/pcm/` (BSD-3-Lizenz mitführen) + CMake-Wrapper-Target (`PCM_NO_STATIC_MSVC_RUNTIME_LIBRARY`
   passend zu `/MT` vs `/MD`, sonst LNK2038).
2. `IPmcSource`-ABI analog `IObservableTier`: `init()`→`PCM::getInstance()+program()`; `read_delta()` über
   `getSystemCounterState`/`getCoreCounterState`.
3. Spalten: L2/L3 → `getL2/L3CacheMisses`; Energie → `getConsumedJoules*1e6` (RAPL-Unit aus `MSR_RAPL_POWER_UNIT`
   0x606, nicht 61µJ hart); L1/DTLB/coherence → zweiter Lauf `CUSTOM_CORE_EVENTS` mit P/E-getrennten Codes.
4. Core-Pinning (`SetThreadAffinityMask`) + Core-Typ via CPUID-Leaf 0x1A; Codes aus `intel/perfmon`
   (`alderlake_goldencove_*`/`gracemont_*`).
5. F15 neu mit realen Countern; Multiplexing vermeiden; Run-A/B-Varianz dokumentieren.

## 5. Ehrliche Einschränkungen

- **L2/L3-Miss + Package-Energie: zuverlässig** (PCM-Default-Funktionen).
- **L1-Miss: nur Näherung** — `MEM_LOAD_RETIRED.L1_MISS` zählt nur retired Loads, unterzählt (LFB-Coalescing).
  Keine PCM-Default-Funktion → programmierbares Event, zweiter Lauf.
- **DTLB-Miss: Definitionsabhängig** — mehrere Sub-Events (causes_a_walk/walk_completed/…); Store-Walks/I-TLB
  fehlen. Thesis muss Semantik festlegen.
- **coherence_invalidations: am unsichersten** — kein Standard-Event; i7-1270P (Client, kein Server-Uncore)
  nur über Proxys (`MEM_LOAD_L3_HIT_RETIRED.XSNP_HITM` / OFFCORE_RESPONSE-Snoop-Bits), E-Core-Abdeckung
  experimentell zu kalibrieren. Semantik muss definiert werden.
- **Hybrid-Verschärfung:** ohne Thread-Pinning → 0/falsche Werte ohne PCM-Warnung. Exakte Gracemont-Codes +
  GP-Counter-Zahl am Gerät gegen `intel/perfmon`-JSON verifizieren.
- **DRAM-Energie: nicht verfügbar** (keine DRAM-RAPL auf Client).
- **RAPL-Genauigkeit:** <1ms unzuverlässig → über Sekunden integrieren, Warm-up verwerfen.
- **Fallback (falls Treiber-Weg scheitert / HVCI-Policy / BSOD #566):** treiberloser ETW-Weg
  (`wpr`/`xperf -pmc`, BenchmarkDotNet) — kein Treiber, kein testsigning, **aber KEINE Energie-Spalte**,
  nur ~3-4 PMC gleichzeitig.

## 6. Offene Entscheidungen (User, Thesis-Methodik)
1. P+E getrennt auswerten (Pinning+Ext-Events) ODER P-only (E-Cores im BIOS abschalten = Vereinfachung)?
2. `energy_micro_joules` für F15 ZWINGEND (→ PCM-Treiber-Pflicht) oder verzichtbar (→ treiberloser ETW)?
3. Kanonische Semantik für `cache_misses_l1`, `dtlb_misses`, `coherence_invalidations`.
4. PCM-Version `202604` bestätigen.
