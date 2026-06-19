# #169(A) Nutzerfreundlichkeit (User 2026-06-19). Baut+laeuft test_validate_profile (HARTE GATE des REIN-LESENDEN
# --validate: (a) m3v2 passt, (b) getipptes Profil gefangen mit klarer Meldung, (c) KEIN DLL-Bau). Include-Satz wie
# die thesis_tiere-Harness. KEIN Tier-Bau — nur parse_thesis_profile + validate_profile (read-only) + die leichte
# m3v2-Achsen-Registry-Reflektion (KEIN 22-Achsen-Umbrella).
$ErrorActionPreference = "Stop"
$repo   = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$out    = Join-Path $repo "build\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_validate_profile"
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

# parse_thesis_profile ist NICHT header-only → xml_config_parser.cpp mitkompilieren. /Od + /bigobj: die m3v2-Achsen-
# ConfigSets sind mp11-schwer (aber NICHT der Voll-22-Umbrella — bewusst leicht gehalten).
$xmlCpp = Join-Path $repo "libs\common\serialization\xml_config_parser\xml_config_parser.cpp"
$exe = Join-Path $work "test_validate_profile.exe"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
         "/Fe:`"$exe`"") + $allInc + @("`"$(Join-Path $srcDir 'test_validate_profile.cpp')`"", "`"$xmlCpp`"")
$rspF = Join-Path $work "test_validate_profile.rsp"; $bat = Join-Path $work "build_test.bat"; $log = Join-Path $out "test_validate_profile.log"
Set-Content -Path $rspF -Value $rsp -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
Write-Host "=== Baue test_validate_profile (cl /Od /bigobj) ==="
cmd /c "`"$bat`""
if (!(Test-Path $exe)) { Write-Host "BUILD-FEHLER, Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Host "    $_" } }; exit 1 }
Write-Host "gebaut: $exe"

Write-Host "=== Laeuft test_validate_profile (rein-lesend, KEIN Tier-Bau) ==="
$runBat = Join-Path $work "run_test.bat"
Set-Content -Path $runBat -Encoding ASCII -Value @("@echo off", "cd /d `"$repo`"", "call `"$vcvars`" >nul 2>&1", "`"$exe`" `"$repo`"")
cmd /c "`"$runBat`""
exit $LASTEXITCODE
