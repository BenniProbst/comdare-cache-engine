# L-LAZY-E2E (gate-frei, lokal) — Harness für den LAZY Ende-zu-Ende-Treiber (run_lazy_static_then_dynamic):
#   erst STATISCHE Kompilierung der Tier-Binary-DLLs (PROFIL-getrieben = 320 reale SA-Kompositionen ≥150), dann je DLL
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
    # GOAL-M1.3 (Audit: Harness-Re-Entry-Drift): Defaults = VOLL-LAUF-Konfiguration, damit ein argloser
    # Resume-Relaunch nach Reboot NIE eine falsche Matrix misst/überschreibt.
    [int]$NOps = 10000,
    [int]$NRepeats = 3,                       # (D, KF-10): Wiederholungen je (Binary×Setting), Default 3
    [string]$BuildVersion = "cowmem-v1",      # = aktueller DLL-Stand auf Disk (Resume-kompatibel)
    # GOAL-M1.3: das Harness PINNT die Workload-Achse selbst (statt volatiler Session-ENV — Audit-Blocker):
    [string]$LoadProfileDir = "",             # leer → <repo>\libs\cache_engine\algorithm_profiles\load_profiles
    [int]$WorkloadRecords = 10000,
    # M3v2-SELEKTION (Task #156, Design-Spec §3d, P-MD7): Working-Set-Sweep. N (Record-Zahl) ist die Mess-Dimension,
    # die festlegt, wie viele Sätze VOR der gemessenen Run-Phase befüllt werden (= Key-Range [1,N]). Default-Set
    # {2^14, 2^17, 2^20, 2^23} = {16384, 131072, 1048576, 8388608} (Infra-Agent passt an die reale LLC an). Je N-Wert
    # ein eigener Treiber-Lauf (eigener COMDARE_WORKLOAD_RECORDS) → die CSV-Spalte working_set_n trennt sie. Leer →
    # einmaliger Lauf mit -WorkloadRecords (rückwärtskompatibel).
    [int[]]$WorkingSetN = @(),
    # M3v2-Tags (Task #156): platform/build_version reisen als CSV-Tag-Spalten (Auswertungs-Trennung der Plattform-
    # /Build-Reihen). Default win-x86_64 / m3v2; der Infra-Agent setzt -Platform je ZIH-Plattform-Reihe.
    [string]$Platform = "win-x86_64",
    [string]$M3BuildVersionTag = "m3v2",
    # STRANG A Inc4/S5 (2026-06-18): PROFIL-GETRIEBEN. Der Treiber baut Baum + Selektion AUS DEM PROFIL
    # (parse_thesis_profile -> build_axis_levels -> profile_select). -Profile = der Profil-Pfad; -SweepAxis = ein im
    # Profil als <axis_sweep> DEKLARIERTER Per-Achsen-Sweep (leer = Basis-Selektion). argv[10] = "profile:<pfad>[@<achse>]".
    [string]$Profile = "",                    # leer -> <repo>\libs\cache_engine\algorithm_profiles\thesis_profiles\m3v2_study.profile.xml
    [string]$SweepAxis = "",                  # leer = Basis-Selektion; sonst search_algo|node_type|memory_layout|prefetch|path_compression
    # Mess-RESUME (#139, Default AN): Binaries mit vollständiger+konfigurations-aktueller result.csv (Stamp-Match:
    # BuildVersion/n_ops/Workload-Set/dyn-Dims) überspringen + ihre Zeilen in die globale CSV übernehmen.
    # -Resume:$false → alles neu messen (Stamps werden überschrieben).
    [bool]$Resume = $true,
    [switch]$RunTest,
    [switch]$RebuildHost,
    # #169(A) Nutzerfreundlichkeit (User 2026-06-19): REIN-LESENDES Validat. -Validate prueft das -Profile gegen die
    # AxisRegistry/EnabledStrategies (run_lazy_150 --validate) und BRICHT VOR dem teuren DLL-Bau/der Messung ab. Exit
    # 0 + Zusammenfassung bei OK; Exit != 0 + klare Meldung (Achse + ungueltiger Wert) bei einem getippten Profil.
    [switch]$Validate
)
$ErrorActionPreference = "Stop"
$repo   = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path  # repo-relativ (Script: tests\unit\thesis_tiere -> comdare-cache-engine) statt hartkodiertem Absolutpfad; macht den manuellen Lauf portabel
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

# GOAL-M1.3 (Audit: Re-Entry-Drift + unvalidiertes env): Workload-Achse HIER pinnen + validieren.
if ([string]::IsNullOrEmpty($LoadProfileDir)) {
    $LoadProfileDir = Join-Path $repo "libs\cache_engine\algorithm_profiles\load_profiles"
}
if (Test-Path $LoadProfileDir) { $LoadProfileDir = (Resolve-Path -LiteralPath $LoadProfileDir).Path }   # ABSOLUT: wird als env COMDARE_LOAD_PROFILE_DIR an run_lazy_150 (anderer CWD) gereicht
$lpCount = (Get-ChildItem $LoadProfileDir -Filter "*.xml" -File -EA SilentlyContinue | Measure-Object).Count
if ($lpCount -lt 1) { Write-Output "ABBRUCH: Lastprofil-Verzeichnis leer/fehlt: $LoadProfileDir (Achse 2 wuerde still entfallen)"; exit 3 }
$env:COMDARE_LOAD_PROFILE_DIR = $LoadProfileDir
$env:COMDARE_WORKLOAD_RECORDS = "$WorkloadRecords"
Remove-Item Env:COMDARE_WORKLOADS -EA SilentlyContinue   # XML-Discovery ist die Quelle, kein env-String-Fallback
Write-Host ("Workload-Achse gepinnt: {0} Profile aus {1}, records={2}" -f $lpCount, $LoadProfileDir, $WorkloadRecords)

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path $vswhere)) { Write-Output "vswhere fehlt"; exit 3 }
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

# Include-Satz = statische Dirs + ALLE generated-Leaf-Dirs (rekursiv) + Boost.MP11 (identisch thesis_tiere).
$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common",
               "libs\common\serialization",
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
# STRANG A Inc4/S5: der Treiber ist profil-getrieben → parse_thesis_profile (NICHT header-only) mitlinken.
$xmlCpp = Join-Path $repo "libs\common\serialization\xml_config_parser\xml_config_parser.cpp"

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
    # (C1128 beobachtet 2026-06-03 beim Voll-Katalog). /Fo: weggelassen (2 Quellen) → cl legt .obj im work-CWD ab.
    $rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
             "/Fe:`"$exe`"") + $allInc + @("`"$Src`"", "`"$loaderCpp`"", "`"$wgCpp`"", "`"$xmlCpp`"")
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

# M3v2-Tags (Task #156): platform + build_version reisen via env in den Treiber (→ CSV-Tag-Spalten).
$env:COMDARE_PLATFORM      = $Platform
$env:COMDARE_BUILD_VERSION = $M3BuildVersionTag

# M3v2-SELEKTION (Task #156): Working-Set-Sweep. Ist -WorkingSetN gesetzt, läuft der Treiber je N-Wert (eigenes
# COMDARE_WORKLOAD_RECORDS); die per-N-CSVs werden zur Gesamt-CSV zusammengeführt (Header einmal, Tag-Spalte
# working_set_n trennt). Leer → ein einziger Lauf mit -WorkloadRecords (rückwärtskompatibel).
$nSet = if ($WorkingSetN.Count -gt 0) { $WorkingSetN } else { @($WorkloadRecords) }

# STRANG A Inc4/S5: das PROFIL ist die Selektions-/Achsen-Quelle (kein Code-Selektor mehr). argv[10] = profile:<pfad>[@<achse>].
if ([string]::IsNullOrEmpty($Profile)) {
    $Profile = Join-Path $repo "libs\cache_engine\algorithm_profiles\thesis_profiles\m3v2_study.profile.xml"
}
if (!(Test-Path $Profile)) { Write-Output "ABBRUCH: Profil fehlt: $Profile"; exit 3 }
$Profile = (Resolve-Path -LiteralPath $Profile).Path   # auf ABSOLUT aufloesen: run_lazy_150 laeuft mit anderem CWD (%TEMP%) -> Relativpfade (.\libs\...) brechen sonst (parse_thesis_profile=nullopt)
$profileArg = "profile:" + $Profile + $(if ($SweepAxis) { "@" + $SweepAxis } else { "" })

# #169(A) — REIN-LESENDES Validat VOR dem Bau. -Validate ruft run_lazy_150 --validate <profil> (kein DLL-Bau, keine
# Messung) und beendet das Harness mit dem Validat-Exit-Code (0 = OK, != 0 = getipptes/ungueltiges Profil). So faellt
# ein <value>node_4</value>-Tippfehler SOFORT auf, statt nach langer Bau-/Mess-Wartezeit eine falsche Matrix.
if ($Validate) {
    Write-Host "=== --validate (rein-lesend; KEIN DLL-Bau, KEINE Messung): $Profile ==="
    # Run-InVcvars streamt die cmd-Ausgabe UND haengt $LASTEXITCODE an → das letzte Element ist der numerische Exit.
    $vOut  = @(Run-InVcvars $exe @("--validate", $Profile))
    $vOut | ForEach-Object { Write-Host $_ }
    $vCode = [int]$vOut[-1]
    Write-Host ("=== Validat-Exit = {0} ({1}) ===" -f $vCode, $(if ($vCode -eq 0) { "OK" } else { "FEHLER — Profil NICHT baubar" }))
    exit $vCode
}
Write-Host ("=== Lazy-E2E (m3v2, PROFIL-getrieben): {0} DLLs bauen+messen | profil={1} | sweep={2} | platform={3} | build_tag={4} | working_set_n=[{5}] | resume={6} ===" -f $MaxBinaries, $Profile, $(if ($SweepAxis) { $SweepAxis } else { "basis" }), $Platform, $M3BuildVersionTag, ($nSet -join ","), $Resume)

$lastCode = 0
$partCsvs = @()
foreach ($n in $nSet) {
    $env:COMDARE_WORKLOAD_RECORDS = "$n"
    $partCsv = Join-Path $out ("tier150_measurements_N{0}.csv" -f $n)
    Write-Host ("--- Working-Set-N = {0} ---" -f $n)
    $lastCode = Run-InVcvars $exe @($partCsv, $MaxBinaries, $NOps, $BuildVersion, $permSrc, $permDll, $MinFreeGB, 4, $NRepeats, $profileArg, $(if ($Resume) { "1" } else { "0" }))
    if (Test-Path $partCsv) { $partCsvs += $partCsv }
}

# Gesamt-CSV aus den per-N-Teilen zusammenführen (Header genau einmal aus dem ersten Teil).
if ($partCsvs.Count -gt 0) {
    $first = $true
    $allLines = New-Object System.Collections.Generic.List[string]
    foreach ($p in $partCsvs) {
        $lines = Get-Content $p
        if ($first) { $allLines.AddRange([string[]]$lines); $first = $false }
        else        { if ($lines.Count -gt 1) { $allLines.AddRange([string[]]($lines | Select-Object -Skip 1)) } }
    }
    Set-Content -Path $csv -Value $allLines -Encoding UTF8
    Copy-Item $csv (Join-Path $srcDir "tier150_measurements.csv") -Force   # committe-bare Snapshot-Kopie
    Write-Host ("CSV (m3v2, alle N zusammengeführt): {0}  ({1} Daten-Zeilen)" -f $csv, ($allLines.Count - 1))
    Write-Host "--- Header + erste 6 Zeilen ---"
    Get-Content $csv -TotalCount 7 | ForEach-Object { Write-Host "  $_" }
}
exit $lastCode
