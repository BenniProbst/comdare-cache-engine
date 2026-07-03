# STRANG A Inc7 / FF (#168). Baut+laeuft test_axis_sweep_pilot: die 4 vertieften Achsen sweep-faehig +
# >=2 REALE distinkte migration-DLLs via cl (HotCold + none, distinkte binary_id). Include-Satz identisch zur
# SOTA-Pilot-Harness. Der Test ruft cl SELBST (via COMDARE_PILOT_INCLUDES/COMDARE_SOTA_DEFS) fuer den DLL-Bau.
$ErrorActionPreference = "Stop"
$repo   = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_axis_sweep_pilot"
New-Item -ItemType Directory -Force $work | Out-Null
if (!(Test-Path $gen)) { Write-Output "generated/ fehlt (CMake configure): $gen"; exit 3 }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common",
               "libs\common\serialization", "tests\unit", "tests\unit\thesis_tiere") |
    ForEach-Object { Join-Path $repo $_ }
$genInc  = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI  = Join-Path $repo "cmake\third_party\boost_mp11\include"
$incList = @($staticInc + $genInc + $boostI)
$allInc  = $incList | ForEach-Object { "/I `"$_`"" }

# Der Test selbst (Host-Logik) — source_catalog zieht den all_axes_umbrella (320er-Typ-Map-Apparat) → /Od /bigobj.
$exe = Join-Path $work "test_axis_sweep_pilot.exe"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
         "/Fe:`"$exe`"") + $allInc + @("`"$(Join-Path $srcDir 'test_axis_sweep_pilot.cpp')`"")
$rspF = Join-Path $work "test_axis_sweep_pilot.rsp"; $bat = Join-Path $work "build_test.bat"; $log = Join-Path $work "test_build.log"
Set-Content -Path $rspF -Value $rsp -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
Write-Host "=== Baue test_axis_sweep_pilot (cl /Od /bigobj — Sweep-Typ-Maps, kann etwas dauern) ==="
cmd /c "`"$bat`""
if (!(Test-Path $exe)) { Write-Host "BUILD-FEHLER (Test), Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Host "    $_" } }; exit 1 }
Write-Host "gebaut: $exe"

# Den DLL-Bau-Include-Satz + die Mess-Defines, die der Test je Auspraegung an cl uebergibt (';'-getrennt).
$pilotIncludes = ($incList -join ";")
$sotaDefs = @("/DCOMDARE_ANATOMY_MODULE_BUILD=1","/DCOMDARE_MEASUREMENT_ON=1","/DCOMDARE_CE_ENABLE_STATISTICS=1",
              "/DCOMDARE_EXPERIMENT_MODE_ON=1","/DCOMDARE_OS_WINDOWS=1","/DCOMDARE_ARCH_X86_64=1",
              "/DCOMDARE_CACHE_LINE_SIZE=64","/DWIN32","/D_WINDOWS") -join ";"

$goldenAbs = Join-Path $repo "tests\unit\thesis_tiere\golden_fullpilot_320_binary_ids.txt"
$runBat = Join-Path $work "run_test.bat"
Set-Content -Path $runBat -Encoding ASCII -Value @(
    "@echo off",
    "cd /d `"$repo`"",
    "call `"$vcvars`" >nul 2>&1",
    "set COMDARE_PILOT_INCLUDES=$pilotIncludes",
    "set COMDARE_SOTA_DEFS=$sotaDefs",
    "`"$exe`" `"$goldenAbs`" `"$work`"")
Write-Host "=== Laeuft test_axis_sweep_pilot (baut >=2 distinkte migration-DLLs: HotCold + none) ==="
cmd /c "`"$runBat`""
exit $LASTEXITCODE
