# K10-PMAJOR-04 — NUR Teil (B): funktional-only-TU (OHNE COMDARE_MEASUREMENT_ON) baut + Kopplungs-Symbole weg.
param([double]$MinFreeGB = 1.5)
$ErrorActionPreference = "Stop"
$repo = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen  = Join-Path $repo "build\msvc-release\generated"
$out  = Join-Path $repo "build\thesis_tiere\k10"
$tmp  = Join-Path $env:TEMP "comdare_k10"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common", ".") |
    ForEach-Object { Join-Path $repo $_ }
$genInc = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI = Join-Path $repo "cmake\third_party\boost_mp11\include"
$inc = ($staticInc + $genInc + $boostI) | ForEach-Object { "/I `"$_`"" }
$commonDefs = @("/DCOMDARE_OS_WINDOWS=1", "/DCOMDARE_ARCH_X86_64=1", "/DCOMDARE_CACHE_LINE_SIZE=64")

$probe = Join-Path $tmp "k10_release_probe.cpp"
Set-Content -Path $probe -Encoding ASCII -Value @'
#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <compositions/art_reference.hpp>
#include <cstdint>
namespace an = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
extern "C" int k10_probe_main() {
    using Anatomy = an::SearchAlgorithmAnatomy<comp::ArtComposition>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    bool ok = tier.tier_insert(42u, 4242u);
    std::uint64_t v = 0;
    ok = tier.tier_lookup(42u, &v) && ok;
    tier.tier_clear();
    return ok ? static_cast<int>(v & 1u) : 7;
}
'@
$obj = Join-Path $out "k10_release_probe.obj"
if (Test-Path $obj) { Remove-Item $obj -Force }
$rsp = @("/nologo", "/std:c++latest", "/EHsc", "/c", "/Fo:`"$obj`"") + $commonDefs + $inc + @("`"$probe`"")
$rspFile = Join-Path $tmp "probe.rsp"; Set-Content -Path $rspFile -Value $rsp -Encoding ASCII
$bat = Join-Path $tmp "probe.bat"
Set-Content -Path $bat -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "cl @`"$rspFile`"") -Encoding ASCII
$log = Join-Path $out "k10_release_probe.cl.log"
$p = Start-Process cmd.exe -ArgumentList "/c", "`"$bat`"" -PassThru -NoNewWindow -RedirectStandardOutput $log -RedirectStandardError "$log.err"
while (!$p.HasExited) {
    Start-Sleep -Milliseconds 500
    $f = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1048576.0
    if ($f -lt $MinFreeGB) { Get-Process cl -EA SilentlyContinue | Stop-Process -Force; try { Stop-Process -Id $p.Id -Force } catch {}; break }
}
if (!$p.HasExited) { $p.WaitForExit() }
if (!(Test-Path $obj)) { Write-Host "FUNKTIONAL-ONLY BUILD FEHLER"; Get-Content $log -Tail 30 | ForEach-Object { Write-Host $_ }; exit 1 }
Write-Host "funktional-only TU (OHNE COMDARE_MEASUREMENT_ON) kompiliert: OK"

$msvcRoot = Join-Path (& $vswhere -latest -property installationPath) "VC\Tools\MSVC"
$dumpbinDir = Get-ChildItem $msvcRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
$dumpbin = Join-Path $dumpbinDir.FullName "bin\Hostx64\x64\dumpbin.exe"
$coupSyms = @("register_entry", "register_slot", "observe_critical_section", "observe_prefetch_descent", "record_node_touch")
$dumpOut = Join-Path $out "k10_release_probe.symbols.txt"
$batD = Join-Path $tmp "dump.bat"
Set-Content -Path $batD -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "`"$dumpbin`" /SYMBOLS `"$obj`"") -Encoding ASCII
Start-Process cmd.exe -ArgumentList "/c", "`"$batD`"" -NoNewWindow -Wait -RedirectStandardOutput $dumpOut -RedirectStandardError "$dumpOut.err"
$symText = Get-Content $dumpOut -Raw
$leaked = @()
foreach ($s in $coupSyms) { if ($symText -match [regex]::Escape($s)) { $leaked += $s } }
if ($leaked.Count -gt 0) { Write-Host ("KOPPLUNGS-SYMBOLE LECKEN (Release): {0}" -f ($leaked -join ", ")); exit 2 }
Write-Host "Kopplungs-Symbole im funktional-only-Objekt: KEINE (compile-time entfernt) — OK"
Write-Host "K10_RELEASE_PROBE clean=TRUE"
exit 0
