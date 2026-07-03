# STRANG A KORRIGIERT — Increment 6 / S7. Baut+laeuft test_run_profile_union (HARTE GATE (1) der Union-
# Verdrahtung: Basis-320 ∪ SOTA-Reihen ueber EINE SourceGenFn + Multi-Pass + Golden-Stabilitaet). Include-Satz
# identisch zur 150er-Harness. KEIN 320-/SOTA-Bau-Sturm — nur String-/Index-Rekombination + Katalog-Materialisierung.
$ErrorActionPreference = "Stop"
$repo   = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$out    = Join-Path $repo "build\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_run_profile_union"
New-Item -ItemType Directory -Force $out, $work | Out-Null
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

# parse_thesis_profile ist NICHT header-only → xml_config_parser.cpp mitkompilieren. /Od + /bigobj: der Umbrella
# (source_catalog → all_axes_umbrella + 320er Typ-Map) ist mp11-/Sektions-schwer (analog Host-Exe-Bau).
$xmlCpp = Join-Path $repo "libs\common\serialization\xml_config_parser\xml_config_parser.cpp"
$exe = Join-Path $work "test_run_profile_union.exe"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
         "/Fe:`"$exe`"") + $allInc + @("`"$(Join-Path $srcDir 'test_run_profile_union.cpp')`"", "`"$xmlCpp`"")
$rspF = Join-Path $work "test_run_profile_union.rsp"; $bat = Join-Path $work "build_test.bat"; $log = Join-Path $out "test_run_profile_union.log"
Set-Content -Path $rspF -Value $rsp -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
Write-Host "=== Baue test_run_profile_union (cl /Od /bigobj — 320er Typ-Map, kann etwas dauern) ==="
cmd /c "`"$bat`""
if (!(Test-Path $exe)) { Write-Host "BUILD-FEHLER, Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Host "    $_" } }; exit 1 }
Write-Host "gebaut: $exe"

Write-Host "=== Laeuft test_run_profile_union (Union-Verdrahtung, KEIN cl-Tier-Bau) ==="
$runBat = Join-Path $work "run_test.bat"
Set-Content -Path $runBat -Encoding ASCII -Value @("@echo off", "cd /d `"$repo`"", "call `"$vcvars`" >nul 2>&1", "`"$exe`"")
cmd /c "`"$runBat`""
exit $LASTEXITCODE
