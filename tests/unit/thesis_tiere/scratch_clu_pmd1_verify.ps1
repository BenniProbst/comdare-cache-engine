# P-MD1 CLU-Fix Scratch-Verifikation (2026-06-18): baut + laeuft test_clu_per_layout (neu) +
# test_pathb_segment_timer + test_obs_phaseA (kein Regress). Modell: k10_double_build_verify.ps1 (ad-hoc cl.exe).
param([double]$MinFreeGB = 1.2)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gen  = Join-Path $repo "build\msvc-release\generated"
$out  = Join-Path $repo "build\thesis_tiere\pmd1"
$tmp  = Join-Path $env:TEMP "comdare_pmd1"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
if (!(Test-Path $gen)) { Write-Output "generated/ fehlt (CMake configure): $gen"; exit 3 }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common", ".") |
    ForEach-Object { Join-Path $repo $_ }
$genInc = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI = Join-Path $repo "cmake\third_party\boost_mp11\include"
$inc = ($staticInc + $genInc + $boostI) | ForEach-Object { "/I `"$_`"" }
$commonDefs = @("/DCOMDARE_OS_WINDOWS=1", "/DCOMDARE_ARCH_X86_64=1", "/DCOMDARE_CACHE_LINE_SIZE=64")
$onDefs = @("/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1")

function Invoke-Cl([string]$tag, [string]$src, [string]$exe) {
    $rsp = @("/nologo", "/std:c++latest", "/EHsc") + $onDefs + $commonDefs + @("/Fe:`"$exe`"", "/Fo:`"$out\$tag.obj`"") + $inc + @("`"$src`"")
    $rspFile = Join-Path $tmp "$tag.rsp"; Set-Content -Path $rspFile -Value $rsp -Encoding ASCII
    $bat = Join-Path $tmp "$tag.bat"
    Set-Content -Path $bat -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspFile`"") -Encoding ASCII
    $log = Join-Path $out "$tag.cl.log"
    $p = Start-Process cmd.exe -ArgumentList "/c", "`"$bat`"" -PassThru -NoNewWindow -RedirectStandardOutput $log -RedirectStandardError "$log.err"
    while (!$p.HasExited) {
        Start-Sleep -Milliseconds 500
        $f = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1048576.0
        if ($f -lt $MinFreeGB) { Get-Process cl -EA SilentlyContinue | Stop-Process -Force; try { Stop-Process -Id $p.Id -Force } catch {}; break }
    }
    if (!$p.HasExited) { $p.WaitForExit() }
    return @{ log = $log; ok = (Test-Path $exe) }
}

$tests = @(
    @{ tag = "test_clu_per_layout";      src = "tests\unit\test_clu_per_layout.cpp" },
    @{ tag = "test_pathb_segment_timer"; src = "tests\unit\test_pathb_segment_timer.cpp" },
    @{ tag = "test_obs_phaseA";          src = "tests\unit\test_obs_phaseA.cpp" },
    @{ tag = "test_migration_two_tier";  src = "tests\unit\test_migration_two_tier.cpp" }
)
$allGreen = $true
foreach ($t in $tests) {
    $src = Join-Path $repo $t.src
    $exe = Join-Path $out ($t.tag + ".exe")
    if (Test-Path $exe) { Remove-Item $exe -Force }
    Write-Host "--- compile $($t.tag) ---"
    $r = Invoke-Cl $t.tag $src $exe
    if (!$r.ok) { Write-Host "BUILD FEHLER $($t.tag)"; Get-Content $r.log -Tail 40 -EA SilentlyContinue | ForEach-Object { Write-Host $_ }; $allGreen = $false; continue }
    Write-Host "--- run $($t.tag) ---"
    & $exe
    $rc = $LASTEXITCODE
    Write-Host ("{0} exit={1}" -f $t.tag, $rc)
    if ($rc -ne 0) { $allGreen = $false }
}
Write-Host ("PMD1 allGreen={0}" -f $allGreen)
if (-not $allGreen) { exit 1 }
exit 0
