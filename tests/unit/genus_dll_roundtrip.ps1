# L-76a-c Genus-DLL-Round-Trip (COMMITTET, repo-reproduzierbar). Baut die 3 Genus-Permutations-DLLs aus den
# COMMITTETEN Quellen tests/unit/genus_module_{set,sequence,view}.cpp (cl /LD), baut den gattungs-agnostischen
# Loader-Test (test_dgenus_dll.cpp + anatomy_module_loader.cpp) und fährt den Round-Trip: lädt jede .dll, erkennt
# die Gattung runtime über anatomy()->genus() und treibt das passende Sub-Interface (ISetTier/ISequenceTier/
# IViewTier) über die DLL-Grenze. Beweist "baubares Tier-Binary per Gattung" (Doc 24 §8.8). RAM-Watchdog je Compile.
#
# Vorher nur via git-ignorierte build/scratch_dgenus_dll.ps1 (autor-behauptet). Diese committete Variante macht den
# Beweis aus committeter Quelle reproduzierbar (Phase-E-Audit-Vorbehalt). Leichtgewichtig (interne Organe, kein Boost).
#
# Aufruf:  pwsh tests/unit/genus_dll_roundtrip.ps1   (Exit 0 = ALLE OK)
param([double]$MinFreeGB = 2.5)
$repo = "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
$out  = Join-Path $repo "build"
$tmp  = Join-Path $env:TEMP "comdare_build"
New-Item -ItemType Directory -Force $out, $tmp | Out-Null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars  = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

function Invoke-ClWatched([string]$Tag, [string[]]$RspLines) {
    $rsp = Join-Path $tmp "$Tag.rsp"; $bat = Join-Path $tmp "$Tag.bat"; $log = Join-Path $out "$Tag.cl.log"
    Set-Content -Path $rsp -Value $RspLines -Encoding ASCII
    Set-Content -Path $bat -Value @("@echo off", "call `"$vcvars`" >nul 2>&1", "cl @`"$rsp`"") -Encoding ASCII
    $p = Start-Process cmd.exe -ArgumentList "/c", "`"$bat`"" -PassThru -NoNewWindow -RedirectStandardOutput $log -RedirectStandardError "$log.err"
    $minFree = [double]::MaxValue; $killed = $false
    while (!$p.HasExited) {
        Start-Sleep -Milliseconds 400
        $f = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1048576.0
        if ($f -lt $minFree) { $minFree = $f }
        if ($f -lt $MinFreeGB) { Get-Process cl -EA SilentlyContinue | Stop-Process -Force; try { Stop-Process -Id $p.Id -Force } catch {}; $killed = $true; break }
    }
    if (!$p.HasExited) { $p.WaitForExit() }
    $code = if ($killed) { 999 } else { [int]$p.ExitCode }
    Write-Host ("  [$Tag] min_free_GB={0:N2} killed={1} exit={2}" -f $minFree, $killed, $code)
    if ($code -ne 0) { if (Test-Path $log) { Get-Content $log -Tail 25 | ForEach-Object { Write-Host $_ } }; if (Test-Path "$log.err") { Get-Content "$log.err" -Tail 25 | ForEach-Object { Write-Host $_ } } }
    return $code
}

$incDll  = @("libs\cache_engine","libs\cache_engine\include","libs\common") | ForEach-Object { "/I `"" + (Join-Path $repo $_) + "`"" }
$incTest = @("libs\cache_engine","libs\cache_engine\include") | ForEach-Object { "/I `"" + (Join-Path $repo $_) + "`"" }
$unit    = Join-Path $repo "tests\unit"

# Phase 1: 3 DLLs aus den COMMITTETEN Quellen bauen.
$mods = @(
    @{ Tag = "perm_set_d9";      Src = "genus_module_set.cpp" },
    @{ Tag = "perm_sequence_d10"; Src = "genus_module_sequence.cpp" },
    @{ Tag = "perm_view_d11";     Src = "genus_module_view.cpp" })
$dlls = @()
foreach ($m in $mods) {
    $dll = Join-Path $out ($m.Tag + ".dll")
    $src = Join-Path $unit $m.Src
    Write-Output ("=== DLL: " + $m.Tag + " (aus " + $m.Src + ") ===")
    $rsp = @("/nologo","/std:c++latest","/EHsc","/LD","/DCOMDARE_ANATOMY_MODULE_BUILD",
             "/Fe:`"$dll`"","/Fo:`"$out\$($m.Tag).obj`"") + $incDll + @("`"$src`"")
    if ((Invoke-ClWatched $m.Tag $rsp) -ne 0) { Write-Output ($m.Tag + " FEHLER"); exit 1 }
    $dlls += $dll
}

# Phase 2: Loader-Test bauen (Test + Loader.cpp).
$testExe   = Join-Path $out "test_dgenus_dll.exe"
$foDir     = Join-Path $out "test_dgenus_obj"; New-Item -ItemType Directory -Force $foDir | Out-Null
$loaderCpp = Join-Path $repo "libs\cache_engine\builder\anatomy_module_loader\anatomy_module_loader.cpp"
Write-Output "=== Loader-Test bauen ==="
$rsp = @("/nologo","/std:c++latest","/EHsc","/Fe:`"$testExe`"", ('/Fo:"' + $foDir + '\\"')) + $incTest +
       @("`"" + (Join-Path $unit 'test_dgenus_dll.cpp') + "`"", "`"$loaderCpp`"")
if ((Invoke-ClWatched "test_dgenus_dll" $rsp) -ne 0) { Write-Output "Test-Build FEHLER"; exit 1 }

# Phase 3: Round-Trip ausfuehren (Set + Sequence + View DLLs als Argumente).
Write-Output "=== Round-Trip ausfuehren ==="
& $testExe @dlls
$rc = $LASTEXITCODE
Write-Output ("run_exit={0}" -f $rc)
exit $rc
