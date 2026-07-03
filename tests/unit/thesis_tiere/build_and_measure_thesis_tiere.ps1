# L-MEAS-THESIS (gate-frei, lokal) — echter Ende-zu-Ende-Build + Messung der SearchAlgorithm-DLL-„Tiere" (Gattung
# Suchalgorithmen) für den Diplomarbeit-Mess-Anhang. Je thesis_sa_<organ>.cpp wird EINE echte perm-DLL gebaut
# (cl /LD, voller ADHOC-Include-Satz aus build/msvc-release/generated + Boost.MP11 vendored offline) und via
# perm_runner gemessen → result_ingest-Zeile (binary_id + 13 Observer-Felder). Über ALLE Tiere sind 16 Achsen
# IDENTISCH; variiert wird NUR axis_03a_search_algo → der gemessene Unterschied ist dem Such-Organ zurechenbar (F15).
#
# Voraussetzung: CMake muss EINMAL konfiguriert worden sein (build/msvc-release/generated/ + perm_runner.exe da).
# Aufruf:  pwsh tests/unit/thesis_tiere/build_and_measure_thesis_tiere.ps1
# Exit 0 = mind. 1 Tier gebaut + gemessen; schreibt build/thesis_tiere/thesis_measurements.csv.
param([double]$MinFreeGB = 2.0, [int[]]$NOps = @(1000, 2000, 4000))
$ErrorActionPreference = "Stop"
$repo   = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gen    = Join-Path $repo "build\msvc-release\generated"
$runner = Join-Path $repo "build\msvc-release\apps\perm_runner\Release\perm_runner.exe"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$out    = Join-Path $repo "build\thesis_tiere"
$tmp    = Join-Path $env:TEMP "comdare_thesis_build"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
if (!(Test-Path $runner)) { Write-Output "perm_runner fehlt (CMake bauen): $runner"; exit 3 }
if (!(Test-Path $gen))    { Write-Output "generated/ fehlt (CMake configure): $gen"; exit 3 }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

# Include-Satz = statische Dirs + ALLE generated-Leaf-Dirs (rekursiv, Superset ist unschädlich) + Boost.MP11.
$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common") |
    ForEach-Object { Join-Path $repo $_ }
$genInc  = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI  = Join-Path $repo "cmake\third_party\boost_mp11\include"
$allInc  = ($staticInc + $genInc + $boostI) | ForEach-Object { "/I `"$_`"" }
Write-Host ("Include-Dirs: {0}" -f $allInc.Count)

# KRITISCH: COMDARE_MEASUREMENT_ON=1 schaltet die IObservableTier-Vererbung im SearchAlgorithmAbiAdapter scharf
# (abi_adapter.hpp:88 `#if COMDARE_MEASUREMENT_ON`). OHNE diese Defines baut die DLL im „MESSUNG-AUS"-Modus (nur
# IDriveableTier) → perm_runner findet kein IObservableTier. Define-Satz gespiegelt vom CMake-Target perm_adhoc_buildvariant.
$defs = @("/DCOMDARE_ANATOMY_MODULE_BUILD=1", "/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1",
          "/DCOMDARE_EXPERIMENT_MODE_ON=1", "/DCOMDARE_OS_WINDOWS=1", "/DCOMDARE_ARCH_X86_64=1",
          "/DCOMDARE_CACHE_LINE_SIZE=64", "/DWIN32", "/D_WINDOWS")

function Invoke-ClWatched([string]$Tag, [string[]]$RspLines) {
    $rsp = Join-Path $tmp "$Tag.rsp"; $bat = Join-Path $tmp "$Tag.bat"; $log = Join-Path $out "$Tag.cl.log"
    Set-Content -Path $rsp -Value $RspLines -Encoding ASCII
    Set-Content -Path $bat -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "cl @`"$rsp`"") -Encoding ASCII
    $p = Start-Process cmd.exe -ArgumentList "/c", "`"$bat`"" -PassThru -NoNewWindow -RedirectStandardOutput $log -RedirectStandardError "$log.err"
    $minFree = [double]::MaxValue; $killed = $false
    while (!$p.HasExited) {
        Start-Sleep -Milliseconds 400
        $f = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1048576.0
        if ($f -lt $minFree) { $minFree = $f }
        if ($f -lt $MinFreeGB) { Get-Process cl -EA SilentlyContinue | Stop-Process -Force; try { Stop-Process -Id $p.Id -Force } catch {}; $killed = $true; break }
    }
    if (!$p.HasExited) { $p.WaitForExit() }
    $code = if ($killed) { 999 } else { [int]$p.ExitCode }
    Write-Host ("  [$Tag] min_free_GB={0:N2} killed={1} exit={2}" -f $minFree, $killed, $code)
    if ($code -ne 0 -and (Test-Path $log)) { Get-Content $log -Tail 20 | ForEach-Object { Write-Host "    $_" } }
    return $code
}

# Tiere = ALLE thesis_*.cpp im Verzeichnis (Auto-Discovery). thesis_sa_* variieren axis_03a (Such-Organ),
# thesis_nt_* variieren axis_04 (Node-Organ); der Organ-Label wird aus der variierten Achsen-Zeile geparst.
$tiere = Get-ChildItem -Path $srcDir -Filter "thesis_*.cpp" | Sort-Object Name | ForEach-Object {
    $txt = Get-Content $_.FullName -Raw
    $rx  = if ($_.BaseName -like "thesis_nt_*") { 'axis_04_node_type::(\w+)' } else { 'axis_03a_search_algo::(\w+)' }
    $m   = [regex]::Match($txt, $rx)
    @{ Tag = $_.BaseName; Organ = $(if ($m.Success) { $m.Groups[1].Value } else { $_.BaseName }) }
}
Write-Host ("Tiere entdeckt: {0}" -f $tiere.Count)

# ── Phase 1: jede Quelle → echte perm-DLL ──
Write-Host "=== Phase 1: Build (cl /LD) ==="
$built = @()
foreach ($t in $tiere) {
    $dll = Join-Path $out ($t.Tag + ".dll")
    $src = Join-Path $srcDir ($t.Tag + ".cpp")
    if (!(Test-Path $src)) { Write-Host "  FEHLT Quelle: $src"; continue }
    # Inkrementell: bereits gebaute, aktuelle DLL (neuer als Quelle) wiederverwenden statt neu kompilieren.
    if ((Test-Path $dll) -and ((Get-Item $dll).LastWriteTime -gt (Get-Item $src).LastWriteTime)) {
        Write-Host ("--- $($t.Tag)  (aktuell -> reuse) ---"); $built += @{ Tag = $t.Tag; Organ = $t.Organ; Dll = $dll }; continue
    }
    Write-Host ("--- $($t.Tag)  (Organ=$($t.Organ)) ---")
    $rsp = @("/nologo", "/std:c++latest", "/EHsc", "/LD") + $defs +
           @("/Fe:`"$dll`"", "/Fo:`"$out\$($t.Tag).obj`"") + $allInc + @("`"$src`"")
    if ((Invoke-ClWatched $t.Tag $rsp) -eq 0 -and (Test-Path $dll)) {
        $built += @{ Tag = $t.Tag; Organ = $t.Organ; Dll = $dll }
    } else { Write-Host "  -> uebersprungen (Build-Fehler)" }
}
Write-Host ("Gebaut: {0}/{1} Tiere" -f $built.Count, $tiere.Count)
if ($built.Count -eq 0) { Write-Output "KEIN Tier gebaut"; exit 1 }

# ── Phase 2: jede DLL via perm_runner messen (je n_ops) → result_ingest-Zeile ──
Write-Host "=== Phase 2: Messung (perm_runner) ==="
$csv = Join-Path $out "thesis_measurements.csv"
"organ;n_ops;search_lookup;hit;miss;insert;erase;peak;bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail;obs_axes;fill" |
    Set-Content -Path $csv -Encoding ASCII
foreach ($b in $built) {
    foreach ($n in $NOps) {
        $raw  = & $runner $b.Dll $b.Tag $n 2>&1
        # robuste Extraktion: die EINE wohlgeformte result_ingest-Zeile (binary_id + 13 ';'-Zahlen) herausfiltern.
        $line = $raw | ForEach-Object { "$_" } | Where-Object { $_ -match '^[^;]+(;[0-9]+){13}$' } | Select-Object -First 1
        if ($line) {
            $fields = $line.Split(';')                 # [0]=binary_id, [1..13]=Observer
            $row = @($b.Organ, $n) + $fields[1..13]
            ($row -join ';') | Add-Content -Path $csv -Encoding ASCII
            Write-Host ("  {0,-28} n={1,-5} -> {2}" -f $b.Organ, $n, $line)
        } else { Write-Host ("  {0} n={1}: MESS-FEHLER -> {2}" -f $b.Organ, $n, ($raw -join ' | ')) }
    }
}
Write-Host ("CSV: {0}" -f $csv)
exit 0
