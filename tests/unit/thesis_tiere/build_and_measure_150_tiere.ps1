# L-LAZY-E2E (gate-frei, lokal) — Harness für den LAZY Ende-zu-Ende-Treiber (run_lazy_static_then_dynamic):
#   erst STATISCHE Kompilierung der Tier-Binary-DLLs (FullPilot = 320 reale SA-Kompositionen ≥150), dann je DLL
#   geladen + DYNAMISCHE Variablen-Variation (thread_count × prefetch_distance) auf der geladenen Binary, messen
#   (Observer real), ingest in den Experiment-B+-Baum → eine Mess-Zeile je (Binary × dyn-Setting).
#
# Baut EINMAL das Host-Executable run_lazy_150.exe (der Treiber-Host), übergibt ihm den Include-Satz via
# COMDARE_PILOT_INCLUDES + die Mess-Defines (in run_lazy_150.cpp gebacken) und lässt ES die einzelnen perm-DLLs
# bauen+laden+messen. RESUMIERBAR: gleiche -BuildVersion überspringt versions-aktuelle DLLs (.version-Sidecar).
#
# Aufruf:
#   pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 [-MaxBinaries 150] [-MinFreeGB 2.0] [-NOps 2000]
#   pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 -RunTest      # KLEINER E2E-Test (4 Binaries)
# Voraussetzung: CMake EINMAL konfiguriert (build/msvc-release/generated/ da). Exit 0 = mind. 1 gemessen.
param(
    [int]$MaxBinaries = 150,
    [double]$MinFreeGB = 2.0,
    [int]$NOps = 2000,
    [int]$NRepeats = 3,                       # (D, KF-10): Wiederholungen je (Binary×Setting), Default 3
    [string]$BuildVersion = "tier150-v1",
    # Selektions-Modus (2026-06-04): "index" (Default, rückwärtskompatibel — erste N Blätter, search_algo konstant)
    # ODER "search_algo_grid" (F15-Grid — variiert NUR search_algo → die CSV zeigt die per-Achsen-Differenzierung).
    [ValidateSet("index","search_algo_grid")]
    [string]$SelectMode = "index",
    # Mess-RESUME (#139, Default AN): Binaries mit vollständiger+konfigurations-aktueller result.csv (Stamp-Match:
    # BuildVersion/n_ops/Workload-Set/dyn-Dims) überspringen + ihre Zeilen in die globale CSV übernehmen.
    # -Resume:$false → alles neu messen (Stamps werden überschrieben).
    [bool]$Resume = $true,
    [switch]$RunTest,
    [switch]$RebuildHost
)
$ErrorActionPreference = "Stop"
$repo   = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$out    = Join-Path $repo "build\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_lazy150"   # nur Host-Exe + .rsp/.bat-Hilfsdateien (NICHT die Tiere)
# (E) 2026-06-04: je Tier-Binary ein eigener Ordner UNTER dem Repo-Build-Baum (weg von flachem %TEMP%):
#     build/thesis_tiere/tiere/<stem>/perm_<stem>.{cpp,dll,dll.obj,dll.cl.log,dll.version} + result.csv.
# Der Host-Treiber (cfg.per_binary_subdirs=true) legt je Binary den Unterordner unter $permDll an; $permSrc
# wird im per_binary_subdirs-Pfad NICHT mehr benutzt (Source teilt den per-Binary-Ordner), bleibt aber als
# Fallback-Arg gesetzt (rückwärtskompatibel, falls per_binary_subdirs aus).
$permBase = Join-Path $out "tiere"
$permSrc = $permBase                          # (E): Source + DLL teilen den per-Binary-Ordner unter tiere/<stem>/
$permDll = $permBase
New-Item -ItemType Directory -Force $out, $work, $permBase | Out-Null
if (!(Test-Path $gen)) { Write-Output "generated/ fehlt (CMake configure): $gen"; exit 3 }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path $vswhere)) { Write-Output "vswhere fehlt"; exit 3 }
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

# Include-Satz = statische Dirs + ALLE generated-Leaf-Dirs (rekursiv) + Boost.MP11 (identisch thesis_tiere).
$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common",
               "tests\unit", "tests\unit\thesis_tiere") |
    ForEach-Object { Join-Path $repo $_ }
$genInc  = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI  = Join-Path $repo "cmake\third_party\boost_mp11\include"
$incList = @($staticInc + $genInc + $boostI)
$allInc  = $incList | ForEach-Object { "/I `"$_`"" }
# Diesen Include-Satz reicht der Host-Treiber an seine per-DLL-cl-Aufrufe weiter (COMDARE_PILOT_INCLUDES).
$env:COMDARE_PILOT_INCLUDES = ($incList -join ";")
Write-Host ("Include-Dirs: {0}" -f $allInc.Count)

# Der AnatomyModuleLoader ist NICHT header-only (load/unload in .cpp) → mitkompilieren+linken (wie perm_runner-Target).
$loaderCpp = Join-Path $repo "libs\cache_engine\builder\anatomy_module_loader\anatomy_module_loader.cpp"
# Achse 2 (INC-3): WorkloadGenerator (workload_driver::run_workload_profile-Interpreter) ist NICHT header-only
# (generate_all/Konstruktor in .cpp) → in den Host-Exe-Build mitkompilieren+linken (analog loaderCpp).
$wgCpp = Join-Path $repo "libs\cache_engine\builder\workload_driver\workload_generator.cpp"

function Build-HostExe([string]$Tag, [string]$Src) {
    $exe = Join-Path $work "$Tag.exe"
    if ((Test-Path $exe) -and !$RebuildHost -and ((Get-Item $exe).LastWriteTime -gt (Get-Item $Src).LastWriteTime) `
        -and ((Get-Item $exe).LastWriteTime -gt (Get-Item $loaderCpp).LastWriteTime)) {
        Write-Host "Host-Exe aktuell (reuse): $exe"; return $exe
    }
    # Host-Treiber selbst: KEINE Mess-Defines nötig (er ist NICHT die Tier-DLL); braucht aber windows.h (system_ram).
    # KRITISCH: /Od (KEIN /O2) — der Host-TU instanziiert die 320er Pilot-Typ-Map (mp11-schwer); /O2 hängt den
    # MSVC-Optimizer auf dieser Template-Tiefe (beobachtet 2026-06-03: cl wedged). Der Host braucht keine Perf;
    # nur die per-DLL-Tiere bauen mit /O2. /bigobj: 320 Permutationen × 19 type_name → >2^16 Sektionen
    # (C1128 beobachtet 2026-06-03 bei FullPilot). /Fo: weggelassen (2 Quellen) → cl legt .obj im work-CWD ab.
    $rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
             "/Fe:`"$exe`"") + $allInc + @("`"$Src`"", "`"$loaderCpp`"", "`"$wgCpp`"")
    $rspF = Join-Path $work "$Tag.rsp"; $bat = Join-Path $work "$Tag.bat"; $log = Join-Path $out "$Tag.host.log"
    Set-Content -Path $rspF -Value $rsp -Encoding ASCII
    Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
    Write-Host "=== Baue Host-Exe $Tag (cl /O2, kann etwas dauern — 320er Typ-Map) ==="
    cmd /c "`"$bat`""
    if (!(Test-Path $exe)) { Write-Host "HOST-BUILD-FEHLER ($Tag), Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 30 | ForEach-Object { Write-Host "    $_" } }; return $null }
    Write-Host "Host-Exe gebaut: $exe"
    return $exe
}

# KRITISCH: der Host-Treiber ruft je perm-DLL `cl` via std::system → cl MUSS im PATH sein. Daher die EXE INNERHALB
# der vcvars64-Umgebung starten (sonst „cl ist kein Befehl" → built=0). COMDARE_PILOT_INCLUDES wird vererbt.
function Run-InVcvars([string]$Exe, [string[]]$ExeArgs) {
    $bat = Join-Path $work "run_host.bat"
    $argLine = ($ExeArgs | ForEach-Object { "`"$_`"" }) -join " "
    Set-Content -Path $bat -Encoding ASCII -Value @(
        "@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "`"$Exe`" $argLine")
    cmd /c "`"$bat`""
    return $LASTEXITCODE
}

if ($RunTest) {
    # ── KLEINER E2E-Test (SmallPilot = 4 Binaries × 6 dyn-Settings) ──
    $exe = Build-HostExe "test_lazy_static_dynamic_driver" (Join-Path $repo "tests\unit\test_lazy_static_dynamic_driver.cpp")
    if (-not $exe) { exit 1 }
    Write-Host "=== Laufe E2E-Test (baut 4 perm-DLLs real, dyn-Loop, Messung) — innerhalb vcvars (cl im PATH) ==="
    $code = Run-InVcvars $exe @()
    exit $code
}

# ── ≥150-Lauf: Host-Treiber-Exe bauen + ausführen ──
$exe = Build-HostExe "run_lazy_150" (Join-Path $srcDir "run_lazy_150.cpp")
if (-not $exe) { exit 1 }

$csv = Join-Path $out "tier150_measurements.csv"
Write-Host ("=== Lazy-E2E: {0} DLLs bauen+messen (resumierbar, version={1}, n_repeats={2}, select_mode={3}, mess-resume={4}) — innerhalb vcvars ===" -f $MaxBinaries, $BuildVersion, $NRepeats, $SelectMode, $Resume)
$code = Run-InVcvars $exe @($csv, $MaxBinaries, $NOps, $BuildVersion, $permSrc, $permDll, $MinFreeGB, 4, $NRepeats, $SelectMode, $(if ($Resume) { "1" } else { "0" }))
if (Test-Path $csv) {
    Copy-Item $csv (Join-Path $srcDir "tier150_measurements.csv") -Force   # committe-bare Snapshot-Kopie
    Write-Host ("CSV: {0}  ({1} Daten-Zeilen)" -f $csv, ((Get-Content $csv).Count - 1))
    Write-Host "--- erste 8 Zeilen ---"
    Get-Content $csv -TotalCount 9 | ForEach-Object { Write-Host "  $_" }
}
exit $code
