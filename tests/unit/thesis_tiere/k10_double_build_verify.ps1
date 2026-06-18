# K10-PMAJOR-04 Doppel-Build-Verifikation (2026-06-18) — terminalisiert befund (2):
#   Die Observer-feeding Auto-Kopplungen in abi_adapter.hpp::tier_insert/tier_lookup sind jetzt unter
#   #if COMDARE_MEASUREMENT_ON geklammert. Diese Skript verifiziert LITERAL:
#     (A) MEASUREMENT_ON  → die 4 Mess-Pfad-/Vertiefungs-Tests bauen + laufen grün (Mess-Build UNVERAENDERT).
#     (B) MEASUREMENT_OFF → eine funktional-only-TU baut + die Kopplungs-Symbole sind im Objekt NICHT mehr
#                            referenziert (Kopplungen compile-time entfernt).
# Modell: build_node_delegation_proof.ps1 (gleicher Include-Satz, ad-hoc cl.exe).
param([double]$MinFreeGB = 1.5)
$ErrorActionPreference = "Stop"
$repo = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$gen  = Join-Path $repo "build\msvc-release\generated"
$out  = Join-Path $repo "build\thesis_tiere\k10"
$tmp  = Join-Path $env:TEMP "comdare_k10"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
if (!(Test-Path $gen)) { Write-Output "generated/ fehlt (CMake configure): $gen"; exit 3 }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

$staticInc = @("libs\cache_engine", "libs\cache_engine\include", "libs\cache_engine\src", "libs\common", ".") |
    ForEach-Object { Join-Path $repo $_ }
$genInc = @($gen) + (Get-ChildItem -Path $gen -Recurse -Directory | Select-Object -ExpandProperty FullName)
$boostI = Join-Path $repo "cmake\third_party\boost_mp11\include"
$incList = ($staticInc + $genInc + $boostI)
$inc = $incList | ForEach-Object { "/I `"$_`"" }

$commonDefs = @("/DCOMDARE_OS_WINDOWS=1", "/DCOMDARE_ARCH_X86_64=1", "/DCOMDARE_CACHE_LINE_SIZE=64")

function Invoke-Cl([string]$tag, [string[]]$extraArgs, [string]$srcOrNone, [string]$exeOrObj, [bool]$compileOnly) {
    $rsp = @("/nologo", "/std:c++latest", "/EHsc") + $extraArgs + $commonDefs
    if ($compileOnly) { $rsp += @("/c", "/Fo:`"$exeOrObj`"") }
    else              { $rsp += @("/Fe:`"$exeOrObj`"", "/Fo:`"$out\$tag.obj`"") }
    $rsp += $inc + @("`"$srcOrNone`"")
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
    return @{ log = $log; ok = (Test-Path $exeOrObj) }
}

Write-Host "================= (A) MEASUREMENT_ON: 4 Tests bauen + laufen ================="
$onDefs = @("/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1")
$tests = @(
    @{ tag = "test_pathb_segment_timer"; src = "tests\unit\test_pathb_segment_timer.cpp" },
    @{ tag = "test_obs_phaseA";          src = "tests\unit\test_obs_phaseA.cpp" },
    @{ tag = "test_migration_two_tier";  src = "tests\unit\test_migration_two_tier.cpp" },
    @{ tag = "test_prefetch_real";       src = "tests\unit\test_prefetch_real.cpp" }
)
$allGreen = $true
foreach ($t in $tests) {
    $src = Join-Path $repo $t.src
    $exe = Join-Path $out ($t.tag + ".exe")
    if (Test-Path $exe) { Remove-Item $exe -Force }
    Write-Host "--- compile $($t.tag) ---"
    $r = Invoke-Cl $t.tag $onDefs $src $exe $false
    if (!$r.ok) { Write-Host "BUILD FEHLER $($t.tag)"; Get-Content $r.log -Tail 25 -EA SilentlyContinue | ForEach-Object { Write-Host $_ }; $allGreen = $false; continue }
    Write-Host "--- run $($t.tag) ---"
    & $exe
    $rc = $LASTEXITCODE
    Write-Host ("{0} exit={1}" -f $t.tag, $rc)
    if ($rc -ne 0) { $allGreen = $false }
}
Write-Host ("MEASUREMENT_ON allGreen={0}" -f $allGreen)

Write-Host "================= (B) MEASUREMENT_OFF: funktional-only-TU + Kopplungs-Symbol-Check ================="
# Eine kleine TU, die den ABI-Adapter ueber eine reale Komposition instanziiert + tier_insert/tier_lookup ruft.
$probe = Join-Path $tmp "k10_release_probe.cpp"
Set-Content -Path $probe -Encoding ASCII -Value @'
// K10-PMAJOR-04 funktional-only-Probe: instanziiert den ABI-Adapter OHNE COMDARE_MEASUREMENT_ON.
// Muss kompilieren; die Observer-feeding Kopplungen (register_entry/register_slot/observe_critical_section/
// observe_prefetch_descent/compress/record_node_touch) duerfen NICHT mehr referenziert sein.
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
$relDefs = @("/DCOMDARE_RELEASE_BUILD_PROBE=1")   # KEIN COMDARE_MEASUREMENT_ON, KEIN STATISTICS
$r = Invoke-Cl "k10_release_probe" $relDefs $probe $obj $true
if (!$r.ok) { Write-Host "FUNKTIONAL-ONLY BUILD FEHLER"; Get-Content $r.log -Tail 30 -EA SilentlyContinue | ForEach-Object { Write-Host $_ }; exit 1 }
Write-Host "funktional-only TU kompiliert: OK"

# Symbol-Check: keine der Observer-feeding Kopplungs-Methoden darf im Objekt auftauchen.
$vswhereDump = Join-Path (& $vswhere -latest -property installationPath) "VC\Tools\MSVC"
$dumpbinDir = Get-ChildItem $vswhereDump -Directory | Sort-Object Name -Descending | Select-Object -First 1
$dumpbin = Join-Path $dumpbinDir.FullName "bin\Hostx64\x64\dumpbin.exe"
$coupSyms = @("register_entry", "register_slot", "observe_critical_section", "observe_prefetch_descent", "record_node_touch")
$batD = Join-Path $tmp "dump.bat"
$dumpOut = Join-Path $out "k10_release_probe.symbols.txt"
Set-Content -Path $batD -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "`"$dumpbin`" /SYMBOLS `"$obj`"") -Encoding ASCII
Start-Process cmd.exe -ArgumentList "/c", "`"$batD`"" -NoNewWindow -Wait -RedirectStandardOutput $dumpOut -RedirectStandardError "$dumpOut.err"
$symText = Get-Content $dumpOut -Raw
$leaked = @()
foreach ($s in $coupSyms) { if ($symText -match [regex]::Escape($s)) { $leaked += $s } }
if ($leaked.Count -gt 0) { Write-Host ("KOPPLUNGS-SYMBOLE LECKEN (Release): {0}" -f ($leaked -join ", ")); exit 2 }
Write-Host "Kopplungs-Symbole im funktional-only-Objekt: KEINE (compile-time entfernt) — OK"
Write-Host ("K10_DOUBLE_BUILD allGreen_ON={0} release_clean=TRUE" -f $allGreen)
if (-not $allGreen) { exit 1 }
exit 0
