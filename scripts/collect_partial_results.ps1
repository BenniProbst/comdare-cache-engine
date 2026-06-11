# collect_partial_results.ps1 — GOAL-M2.2 (2026-06-12): Teilstand-Snapshot des laufenden Voll-Laufs.
#
# Sammelt die per-Binary result.csv ALLER fertig GESTEMPELTEN Tiere (result.csv.stamp vorhanden und auf
# die erwartete BuildVersion+n_ops passend) zu EINER Auswertungs-CSV (Header exakt einmal; Schema-Identität
# über Header-Vergleich erzwungen — abweichende/alte Dateien werden übersprungen und gemeldet). Der laufende
# Mess-Prozess wird NICHT gestört (reines Lesen). Grundlage für L6/L7 (Generalprobe + Professor-Paket).
#
# Aufruf: pwsh scripts/collect_partial_results.ps1 [-OutCsv <pfad>] [-BuildVersion cowmem-v1] [-NOps 10000]
param(
    [string]$OutCsv = "",
    [string]$BuildVersion = "cowmem-v1",
    [int]$NOps = 10000
)
$ErrorActionPreference = "Stop"
$repo  = Split-Path -Parent $PSScriptRoot
$tiere = Join-Path $repo "build\thesis_tiere\tiere"
if ([string]::IsNullOrEmpty($OutCsv)) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmm"
    $OutCsv = Join-Path $repo "build\thesis_tiere\partial_snapshot_$stamp.csv"
}

$dirs = Get-ChildItem $tiere -Directory
$header = $null; $rows = 0; $tiereOk = 0; $skipped = @()
$sb = [System.Text.StringBuilder]::new()
foreach ($d in $dirs) {
    $st = Join-Path $d.FullName "result.csv.stamp"
    $rc = Join-Path $d.FullName "result.csv"
    if (!(Test-Path $st) -or !(Test-Path $rc)) { continue }
    $stampTxt = (Get-Content $st -Raw)
    if ($stampTxt -notmatch [regex]::Escape("build=$BuildVersion|n_ops=$NOps")) { $skipped += "$($d.Name) [stamp: andere Config]"; continue }
    $lines = Get-Content $rc
    if ($lines.Count -lt 2) { $skipped += "$($d.Name) [leer]"; continue }
    if ($null -eq $header) { $header = $lines[0]; [void]$sb.AppendLine($header) }
    elseif ($lines[0] -ne $header) { $skipped += "$($d.Name) [Header-Drift]"; continue }
    for ($i = 1; $i -lt $lines.Count; $i++) { if ($lines[$i].Length -gt 0) { [void]$sb.AppendLine($lines[$i]); $rows++ } }
    $tiereOk++
}
if ($null -eq $header) { Write-Output "KEIN fertiges Tier mit passendem Stamp (build=$BuildVersion, n_ops=$NOps) gefunden."; exit 1 }
Set-Content -Path $OutCsv -Value $sb.ToString() -Encoding UTF8 -NoNewline
Write-Output ("Snapshot: {0} Tiere, {1} Zeilen -> {2}" -f $tiereOk, $rows, $OutCsv)
if ($skipped.Count -gt 0) { Write-Output ("Uebersprungen ({0}): " -f $skipped.Count); $skipped | Select-Object -First 8 | ForEach-Object { "  $_" } }
exit 0
