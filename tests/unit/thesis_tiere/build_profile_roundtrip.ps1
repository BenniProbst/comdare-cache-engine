# STRANG A KORRIGIERT — Increment 4 (S4a/S4b/S5). Baut+laeuft (1) den Golden-Generator (nur EINMAL, wenn die golden-
# Datei fehlt) und (2) test_profile_roundtrip (Round-Trip-Gate gegen die golden-Liste). Include-Satz identisch zur
# 150er-Harness. KEIN 320-Voll-Bau (nur String-/Index-Rekombination ueber den StaticBinaryView).
param(
    [switch]$RegenGolden   # erzwingt Neu-Generierung der golden-Datei (normalerweise NICHT — sie ist eingefroren)
)
$ErrorActionPreference = "Stop"
$repo   = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen    = Join-Path $repo "build\msvc-release\generated"
$srcDir = Join-Path $repo "tests\unit\thesis_tiere"
$out    = Join-Path $repo "build\thesis_tiere"
$work   = Join-Path $env:TEMP "comdare_profile_rt"
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

# parse_thesis_profile ist NICHT header-only → xml_config_parser.cpp mitkompilieren.
$xmlCpp = Join-Path $repo "libs\common\serialization\xml_config_parser\xml_config_parser.cpp"

function Compile([string]$Tag, [string[]]$Sources) {
    $exe = Join-Path $work "$Tag.exe"
    $rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj", "/DWIN32", "/D_WINDOWS",
             "/Fe:`"$exe`"") + $allInc + ($Sources | ForEach-Object { "`"$_`"" })
    $rspF = Join-Path $work "$Tag.rsp"; $bat = Join-Path $work "$Tag.bat"; $log = Join-Path $out "$Tag.rt.log"
    Set-Content -Path $rspF -Value $rsp -Encoding ASCII
    Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
    Write-Host "=== Baue $Tag ==="
    cmd /c "`"$bat`""
    if (!(Test-Path $exe)) { Write-Host "BUILD-FEHLER ($Tag), Log-Tail:"; if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Host "    $_" } }; return $null }
    Write-Host "gebaut: $exe"
    return $exe
}

$golden = Join-Path $srcDir "golden_fullpilot_320_binary_ids.txt"
if ($RegenGolden -or !(Test-Path $golden)) {
    Write-Host "=== (S4a) Golden-Generator: golden_fullpilot_320_binary_ids.txt erzeugen ==="
    $genExe = Compile "gen_golden_fullpilot" @((Join-Path $srcDir "gen_golden_fullpilot.cpp"))
    if (-not $genExe) { exit 1 }
    $bat = Join-Path $work "run_gen.bat"
    Set-Content -Path $bat -Encoding ASCII -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "`"$genExe`" `"$golden`"")
    cmd /c "`"$bat`""
    if (!(Test-Path $golden)) { Write-Host "GOLDEN-GEN FEHLER"; exit 1 }
}
Write-Host ("golden-Datei: {0}  ({1} Zeilen ohne Kommentar)" -f $golden, ((Get-Content $golden | Where-Object { $_ -notmatch '^#' }).Count))

Write-Host "=== test_profile_roundtrip (gegen golden) ==="
$rtExe = Compile "test_profile_roundtrip" @((Join-Path $srcDir "test_profile_roundtrip.cpp"), $xmlCpp)
if (-not $rtExe) { exit 1 }
$bat = Join-Path $work "run_rt.bat"
Set-Content -Path $bat -Encoding ASCII -Value @("@echo off", "cd /d `"$repo`"", "call `"$vcvars`" >nul 2>&1", "`"$rtExe`"")
cmd /c "`"$bat`""
exit $LASTEXITCODE
