# L-74a Build-Varianten-DLL-Round-Trip (COMMITTET, repo-reproduzierbar). Baut 2 Build-Varianten-DLLs DERSELBEN
# page/hw (Avx512 vs Avx2) aus den committeten Quellen tests/unit/genus_buildvariant_{avx512,avx2}.cpp (cl /LD),
# baut den Loader-Test (test_buildvariant_dll.cpp) und zieht aus JEDER DLL via GetProcAddress das Inspection-
# Symbol comdare_build_variant_inspect → 2 Build-Varianten host-seitig literal unterscheidbar (512 vs 256) über die
# echte .dll-Grenze. Beweist L-74a (test_d7a beweist denselben ABI-Pull nur in-process). RAM-Watchdog je Compile.
#
# Aufruf:  pwsh tests/unit/buildvariant_dll_roundtrip.ps1   (Exit 0 = ALLE OK)
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

# Phase 1: 2 Build-Varianten-DLLs aus den committeten Quellen bauen.
$mods = @(
    @{ Tag = "perm_buildvariant_avx512"; Src = "genus_buildvariant_avx512.cpp" },
    @{ Tag = "perm_buildvariant_avx2";   Src = "genus_buildvariant_avx2.cpp" })
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

# Phase 2: Loader-Test bauen (reines win32 LoadLibrary/GetProcAddress, kein Loader.cpp nötig).
$testExe = Join-Path $out "test_buildvariant_dll.exe"
$foDir   = Join-Path $out "test_buildvariant_obj"; New-Item -ItemType Directory -Force $foDir | Out-Null
Write-Output "=== Loader-Test bauen ==="
$rsp = @("/nologo","/std:c++latest","/EHsc","/Fe:`"$testExe`"", ('/Fo:"' + $foDir + '\\"')) + $incTest +
       @("`"" + (Join-Path $unit 'test_buildvariant_dll.cpp') + "`"")
if ((Invoke-ClWatched "test_buildvariant_dll" $rsp) -ne 0) { Write-Output "Test-Build FEHLER"; exit 1 }

# Phase 3: Round-Trip (avx512.dll avx2.dll als Argumente).
Write-Output "=== Round-Trip ausfuehren ==="
& $testExe $dlls[0] $dlls[1]
$rc = $LASTEXITCODE
Write-Output ("run_exit={0}" -f $rc)
exit $rc
