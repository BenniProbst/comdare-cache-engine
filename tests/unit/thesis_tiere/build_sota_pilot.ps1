# STRANG A KORRIGIERT — Increment 5 / S6 (Klein-Pilot). Baut+laeuft test_sota_series_pilot:
# Stufe1/2 als reale Smoke-DLLs plus Stufe3/Reihe B als 6 per-Host-DLLs. Include-Satz identisch zur
# 150er-Harness. Der Test ruft cl SELBST (via COMDARE_PILOT_INCLUDES/COMDARE_SOTA_DEFS) fuer den DLL-Bau.
$ErrorActionPreference = "Stop"
$repo   = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_sota_pilot"
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

# Der Test selbst (Host-Logik) — parse_thesis_profile ist NICHT header-only → xml_config_parser.cpp mit.
$xmlCpp = Join-Path $repo "libs\common\serialization\xml_config_parser\xml_config_parser.cpp"
$exe = Join-Path $work "test_sota_series_pilot.exe"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
         "/Fe:`"$exe`"") + $allInc + @("`"$(Join-Path $srcDir 'test_sota_series_pilot.cpp')`"", "`"$xmlCpp`"")
$rspF = Join-Path $work "test_sota_series_pilot.rsp"; $bat = Join-Path $work "build_test.bat"; $log = Join-Path $work "test_build.log"
Set-Content -Path $rspF -Value $rsp -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
Write-Host "=== Baue test_sota_series_pilot ==="
cmd /c "`"$bat`""
if (!(Test-Path $exe)) { Write-Host "BUILD-FEHLER (Test), Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Host "    $_" } }; exit 1 }
Write-Host "gebaut: $exe"

# Den DLL-Bau-Include-Satz + die Mess-Defines, die der Test je Stufe/Host an cl uebergibt (';'-getrennt).
$pilotIncludes = ($incList -join ";")
$sotaDefs = @("/DCOMDARE_ANATOMY_MODULE_BUILD=1","/DCOMDARE_MEASUREMENT_ON=1","/DCOMDARE_CE_ENABLE_STATISTICS=1",
              "/DCOMDARE_EXPERIMENT_MODE_ON=1","/DCOMDARE_OS_WINDOWS=1","/DCOMDARE_ARCH_X86_64=1",
              "/DCOMDARE_CACHE_LINE_SIZE=64","/DWIN32","/D_WINDOWS") -join ";"

$runBat = Join-Path $work "run_test.bat"
Set-Content -Path $runBat -Encoding ASCII -Value @(
    "@echo off",
    "cd /d `"$repo`"",
    "call `"$vcvars`" >nul 2>&1",
    "set COMDARE_PILOT_INCLUDES=$pilotIncludes",
    "set COMDARE_SOTA_DEFS=$sotaDefs",
    "`"$exe`" `"$(Join-Path $repo 'libs\cache_engine\algorithm_profiles\thesis_profiles\m3v2_study.profile.xml')`" `"$work`"")
Write-Host "=== Laeuft test_sota_series_pilot (Stufe1/2 + 6x Stufe3/B real) ==="
cmd /c "`"$runBat`""
exit $LASTEXITCODE
