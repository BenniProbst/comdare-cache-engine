# L-MEAS-ADAPTER (gate-frei, lokal) — echter Build + Messung der CONTAINER-Gattung (Adapter-Tier-Unterklasse,
# Doku 14 §28) für den Diplomarbeit-Mess-Anhang. Die 3 std-Adapter-Disziplinen queue/stack/priority_queue werden
# über die 3 inner_container-Organe (DequeInner FIFO/LIFO, HeapInner Max-Heap) + die §26.4-Adapter-API getrieben.
#
# In-process über die AdapterAnatomy (= das per-Gattung-Prüf-Dock-Treiben, Doc 24 §8.8). Anders als die SA-Tiere
# (DLL + perm_runner über IObservableTier insert/lookup/erase) misst der Adapter hier in-process, weil die ABI
# IAdapterTier nur put/get(=pop_front, FIFO) hat — kein tier_pop_back, also kein LIFO über die DLL-Grenze. Der
# Dock-Weg deckt über die volle push/pop_front/pop_back-API ALLE 3 Disziplinen ab. (DLL+adapter_runner mit ABI-
# Erweiterung tier_pop_back = möglicher Folgeschritt.)
#
# Über alle 3 Tiere IDENTISCHER §28-Achsen-Satz (13); variiert NUR inner_container + Disziplin → der Unterschied ist
# der Achse/Disziplin zurechenbar. Erhoben: eingebauter Adapter-Observer + Wall-Clock (steady_clock), Median über 3 Reps.
#
# Aufruf:  pwsh tests/unit/thesis_tiere/build_and_measure_adapter_tiere.ps1
# Exit 0 = gebaut + gemessen; schreibt build/thesis_tiere/adapter_measurements.csv (+ Kopie ins Quellverzeichnis).
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$src  = Join-Path $repo "tests\unit\thesis_tiere\measure_adapter_tiere.cpp"
$inc  = Join-Path $repo "libs\cache_engine"
$out  = Join-Path $repo "build\thesis_tiere"
New-Item -ItemType Directory -Force $out | Out-Null
$exe  = Join-Path $out "measure_adapter_tiere.exe"
$obj  = Join-Path $out "measure_adapter_tiere.obj"
$csv  = Join-Path $out "adapter_measurements.csv"
$csvSrc = Join-Path $repo "tests\unit\thesis_tiere\adapter_measurements.csv"   # committete Snapshot-Kopie

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path $vswhere)) { Write-Output "vswhere fehlt"; exit 3 }
$vcvars = Join-Path (& $vswhere -latest -property installationPath) "VC\Auxiliary\Build\vcvars64.bat"
if (!(Test-Path $vcvars)) { Write-Output "vcvars64 fehlt"; exit 3 }

# Adapter ist self-contained (nur libs/cache_engine; kein generated/ + kein Boost nötig). /O2 für echte Perf-Messung.
$bat = Join-Path $env:TEMP "build_adapter_meas.bat"
Set-Content -Path $bat -Encoding ASCII -Value @(
    "@echo off", "call `"$vcvars`" >nul 2>&1",
    "cl /nologo /std:c++latest /O2 /EHsc /I `"$inc`" `"$src`" /Fe:`"$exe`" /Fo:`"$obj`"")
Write-Host "=== Build (cl /O2 /LD-frei, in-process) ==="
cmd /c "`"$bat`"" 2>&1 | Select-Object -Last 6
if (!(Test-Path $exe)) { Write-Output "BUILD-FEHLER: keine exe"; exit 1 }

Write-Host "=== Messung (queue/stack/priority_queue x 100k/500k/1M, 3 Reps Median) ==="
& $exe | Set-Content -Path $csv -Encoding ASCII
Copy-Item $csv $csvSrc -Force
Get-Content $csv
Write-Host ("CSV: {0}  (+ committete Kopie {1})" -f $csv, $csvSrc)
exit 0
