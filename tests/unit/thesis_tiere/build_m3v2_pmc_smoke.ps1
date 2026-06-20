# build_m3v2_pmc_smoke.ps1 — reproduzierbarer Build+Lauf des #156-De-Risk-Smokes m3v2_pmc_smoke.cpp.
#
# Beweist die GESCHLOSSENE PMC-Naht im LAZY WIDE-Mess-Pfad LITERAL (P5, 2026-06-20): make_pmc_source()/IPmcSource
# ist in perm_runner verdrahtet + die 7 PMC-Spalten haengen additiv am Ende von lazy_csv_header(). NullPmcSource-Pfad
# (COMDARE_ENABLE_PMC NICHT definiert) -> available=0, alle PMC-Counter 0 (ehrliche Null). Mit Intel-PCM/Linux+PMC=ON
# tragen dieselben Spalten reale Cache-Misses ohne Code-Aenderung (Drop-in).
#
# Versioniert (statt gitignored build/-Scratch), damit der Smoke aus jedem Clone reproduzierbar ist — analog zur
# #155-Lektion (Verifikations-Tests brauchen einen getrackten Build-Mechanismus). Repo-Pfad via $PSScriptRoot.
#
# VORAUSSETZUNG: ein vorheriges `cmake`-Configure (build/msvc-release) fuer die generated/-Flags-Header
# (Standard-Build-Pipeline-Schritt 1). MSVC/vcvars64 (Windows). boost_mp11 ist vendored (cmake/third_party).
#
# AUFRUF:  pwsh -File tests/unit/thesis_tiere/build_m3v2_pmc_smoke.ps1
# ERWARTUNG: cl_exit=0 + RUN -> 'SMOKE_OK' + CSV-Kopfzeile endet auf ...;quality_flag;pmc_cache_misses_l1;...;pmc_available
param([double]$MinFreeGB = 1.0)
$ErrorActionPreference = "Stop"
# Repo-Wurzel = 3 Ebenen ueber diesem Skript (tests/unit/thesis_tiere/ -> repo).
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$src  = Join-Path $repo "tests\unit\thesis_tiere\m3v2_pmc_smoke.cpp"
$out  = Join-Path $repo "build"
$gen  = Join-Path $repo "build\msvc-release\generated"
$tmp  = Join-Path $env:TEMP "comdare_pmc_smoke"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
if (!(Test-Path $src)) { Write-Output "SRC FEHLT: $src"; exit 2 }
if (!(Test-Path $gen)) { Write-Output "GENERATED FEHLT: $gen — bitte zuerst `cmake` configure (build/msvc-release) ausfuehren."; exit 3 }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src",
               "libs\cache_engine\builder", "libs\common") | ForEach-Object { Join-Path $repo $_ }
$genInc = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI = Join-Path $repo "cmake\third_party\boost_mp11\include"
$allInc = ($staticInc + $genInc + $boostI) | ForEach-Object { "/I `"$_`"" }
$defs = @("/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1", "/DCOMDARE_EXPERIMENT_MODE_ON=1",
          "/DCOMDARE_OS_WINDOWS=1", "/DCOMDARE_ARCH_X86_64=1", "/DCOMDARE_CACHE_LINE_SIZE=64", "/DWIN32", "/D_WINDOWS")
$exe = Join-Path $out "m3v2_pmc_smoke.exe"
$rsp = Join-Path $tmp "smoke.rsp"; $bat = Join-Path $tmp "smoke.bat"; $log = Join-Path $out "m3v2_pmc_smoke.cl.log"
$rspLines = @("/nologo", "/std:c++latest", "/EHsc", "/bigobj", "/Fe:`"$exe`"", "/Fo:`"$out\m3v2_pmc_smoke.obj`"") + $defs + $allInc + @("`"$src`"")
Set-Content -Path $rsp -Value $rspLines -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "cl @`"$rsp`"") -Encoding ASCII
Write-Output "=== m3v2_pmc_smoke : COMPILE+LINK (NullPmcSource, COMDARE_ENABLE_PMC=OFF) ==="
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
Write-Output ("min_free_GB={0:N2} killed={1} cl_exit={2}" -f $minFree, $killed, $code)
if ($code -ne 0) { Write-Output "--- cl-Log (letzte 60 Zeilen) ---"; if (Test-Path $log) { Get-Content $log -Tail 60 }; if (Test-Path "$log.err") { Get-Content "$log.err" -Tail 20 }; exit $code }
Write-Output "=== RUN ==="
& $exe
Write-Output ("run_exit=" + $LASTEXITCODE)
exit $LASTEXITCODE
