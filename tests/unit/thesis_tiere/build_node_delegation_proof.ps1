# Audit-30 Fix Q2 / Beweis-Schnitt (2026-06-03) — baut + fährt den vertikalen Beleg, dass ein DELEGIERENDES
# Such-Organ (ComposedSearch<LinearScan, NodeChunkedStore<N,L,A>>) die node_type-Achse REAL konsumiert.
# Erwartete Ausgabe: slot_count/hits identisch (std::map-Äquivalenz), chunk_count = ceil(n/node_cap) je Node
# verschieden → node_type runtime-wirksam (PROOF_OK, exit 0). Gegenprobe zu den Monolith-Tieren (thesis_sa_*).
#
# Voraussetzung: CMake einmal konfiguriert (build/msvc-release/generated/ vorhanden).
# Aufruf:  pwsh tests/unit/thesis_tiere/build_node_delegation_proof.ps1
param([double]$MinFreeGB = 2.0)
$repo = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen  = Join-Path $repo "build\msvc-release\generated"
$out  = Join-Path $repo "build\thesis_tiere"
$tmp  = Join-Path $env:TEMP "comdare_proof"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
if (!(Test-Path $gen)) { Write-Output "generated/ fehlt (CMake configure): $gen"; exit 3 }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common") |
    ForEach-Object { Join-Path $repo $_ }
$genInc = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI = Join-Path $repo "cmake\third_party\boost_mp11\include"
$inc = ($staticInc + $genInc + $boostI) | ForEach-Object { "/I `"$_`"" }

$src = Join-Path $repo "tests\unit\thesis_tiere\test_node_delegation_proof.cpp"
$exe = Join-Path $out "test_node_delegation_proof.exe"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1",
         "/DCOMDARE_OS_WINDOWS=1", "/DCOMDARE_ARCH_X86_64=1", "/DCOMDARE_CACHE_LINE_SIZE=64",
         "/Fe:`"$exe`"", "/Fo:`"$out\proof.obj`"") + $inc + @("`"$src`"")
$rspFile = Join-Path $tmp "proof.rsp"; Set-Content -Path $rspFile -Value $rsp -Encoding ASCII
$bat = Join-Path $tmp "proof.bat"
Set-Content -Path $bat -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspFile`"") -Encoding ASCII
Write-Host "=== Compile (Include-Dirs: $($inc.Count)) ==="
$log = Join-Path $out "proof.cl.log"
$p = Start-Process cmd.exe -ArgumentList "/c", "`"$bat`"" -PassThru -NoNewWindow -RedirectStandardOutput $log -RedirectStandardError "$log.err"
while (!$p.HasExited) {
    Start-Sleep -Milliseconds 400
    $f = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1048576.0
    if ($f -lt $MinFreeGB) { Get-Process cl -EA SilentlyContinue | Stop-Process -Force; try { Stop-Process -Id $p.Id -Force } catch {}; break }
}
if (!$p.HasExited) { $p.WaitForExit() }
if (!(Test-Path $exe)) { Write-Output "BUILD FEHLER"; Get-Content $log -Tail 20 -EA SilentlyContinue | ForEach-Object { Write-Host $_ }; exit 1 }
Write-Host "=== Run ==="
& $exe
$rc = $LASTEXITCODE
Write-Host ("proof_exit={0}" -f $rc)
exit $rc
