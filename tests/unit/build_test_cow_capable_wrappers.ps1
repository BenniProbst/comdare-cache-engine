# #142 Welle 2 / Audit-K3 — Standalone-Build der Capability-Verifikations-TU (test_cow_capable_wrappers.cpp).
# Spiegelt den Harness-Include-Satz (build_and_measure_150_tiere.ps1) + COMDARE_CE_ENABLE_STATISTICS.
# Ein static_assert-Fehler -> Compile-Fehler (Exit != 0). Sicher neben dem M2-Lauf (separater cl-Prozess).
$ErrorActionPreference = "Stop"
$repo = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen  = Join-Path $repo "build\msvc-release\generated"
$work = Join-Path $env:TEMP "comdare_cowcap_verify"
New-Item -ItemType Directory -Force $work | Out-Null
if (!(Test-Path $gen)) { Write-Output "generated/ fehlt (CMake configure noetig): $gen"; exit 3 }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common",
               "tests\unit", "tests\unit\thesis_tiere") | ForEach-Object { Join-Path $repo $_ }
$genInc = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI = Join-Path $repo "cmake\third_party\boost_mp11\include"
$allInc = (@($staticInc + $genInc + $boostI) | ForEach-Object { "/I `"$_`"" })

$src = Join-Path $repo "tests\unit\test_cow_capable_wrappers.cpp"
$exe = Join-Path $work "test_cow_capable_wrappers.exe"
$log = Join-Path $work "build.log"
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/Od", "/bigobj",
         "/DCOMDARE_CE_ENABLE_STATISTICS=1", "/Fe:`"$exe`"") + $allInc + @("`"$src`"")
$rspF = Join-Path $work "build.rsp"; $bat = Join-Path $work "build.bat"
Set-Content -Path $rspF -Value $rsp -Encoding ASCII
Set-Content -Path $bat -Value @("@echo off", "cd /d `"$work`"", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspF`" > `"$log`" 2>&1") -Encoding ASCII
Write-Output "=== Baue Verifikations-TU (cl, COMDARE_CE_ENABLE_STATISTICS=1) ==="
cmd /c "`"$bat`""
if (!(Test-Path $exe)) {
    Write-Output "BUILD-FEHLER (static_assert oder Compile) — Log-Tail:"
    if (Test-Path $log) { Get-Content $log -Tail 40 | ForEach-Object { Write-Output "    $_" } }
    exit 1
}
Write-Output "=== Build OK -> Lauf ==="
& $exe
exit $LASTEXITCODE
