# #171 (2026-06-20) — Baut+laeuft test_pruefling_type_pilot: LITERALER Beleg der additiven pruefling_type-Spalte
# (full/abstract/-) durch die REALE format_csv_row/lazy_csv_header/build_sota_passes-Kette. Include-Satz identisch
# zur SOTA-Pilot-Harness (build_sota_pilot.ps1). KEIN cl-DLL-Bau noetig (Tag reist durch die CSV-Mechanik).
$ErrorActionPreference = "Stop"
$repo   = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_pruefling_type_pilot"
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

$xmlCpp = Join-Path $repo "libs\common\serialization\xml_config_parser\xml_config_parser.cpp"
$exe = Join-Path $work "test_pruefling_type_pilot.exe"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
         "/Fe:`"$exe`"") + $allInc + @("`"$(Join-Path $srcDir 'test_pruefling_type_pilot.cpp')`"", "`"$xmlCpp`"")
$rspF = Join-Path $work "test_pruefling_type_pilot.rsp"; $bat = Join-Path $work "build_test.bat"; $log = Join-Path $work "test_build.log"
Set-Content -Path $rspF -Value $rsp -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
Write-Host "=== Baue test_pruefling_type_pilot ==="
cmd /c "`"$bat`""
if (!(Test-Path $exe)) { Write-Host "BUILD-FEHLER (Test), Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Host "    $_" } }; exit 1 }
Write-Host "gebaut: $exe"

$runBat = Join-Path $work "run_test.bat"
Set-Content -Path $runBat -Encoding ASCII -Value @(
    "@echo off",
    "cd /d `"$repo`"",
    "call `"$vcvars`" >nul 2>&1",
    "`"$exe`" `"$(Join-Path $repo 'libs\cache_engine\algorithm_profiles\thesis_profiles\m3v2_study.profile.xml')`" `"$work`"")
Write-Host "=== Laeuft test_pruefling_type_pilot ==="
cmd /c "`"$runBat`""
$rc = $LASTEXITCODE
Write-Host "=== Pilot-CSV (Daten-Zeilen) ==="
$csv = Join-Path $work "pruefling_type_pilot.csv"
if (Test-Path $csv) {
    $lines = Get-Content $csv
    Write-Host ("HEADER-Endung: ..." + (($lines[0] -split ';')[-3..-1] -join ';'))
    for ($i = 1; $i -lt $lines.Count; $i++) {
        $cols = $lines[$i] -split ';'
        Write-Host ("ZEILE " + $i + ": binary_id=" + $cols[0] + "  ... letzte 6 Spalten=" + (($cols[-6..-1]) -join ';'))
    }
}
exit $rc
