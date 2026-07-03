# analyze_f15_holm.ps1 — F15 Per-Achsen-Isolation (Pfad A der Hybrid-Methode, Doku 24 §8)
# -----------------------------------------------------------------------------------------
# Konsumiert die Mess-CSV (pilot_verify_measurements.csv / tier150_measurements.csv) und
# berechnet fuer JEDE variierte STATISCHE Achse die mittlere total_ns-Differenz ueber
# Binary-PAARE, die sich in GENAU dieser einen Achse unterscheiden (alle anderen statischen
# Achsen + die Laufzeit-Settings gleich).
#
# Methode (Doku 24 §8, Pfad A — isolierte Achsen-Algorithmen gegeneinander):
#   - Gleiche std::map-Interfaces, ein einzelnes Organ (Achse) variiert -> der gemessene
#     total_ns-Unterschied ist diesem Organ zurechenbar.
#   - Mittlung ueber die Wiederholungen je Konfiguration ist auf der ANALYSE-Schicht erlaubt
#     (NICHT in der Speicherung). repetition.repetition_index wird daher aus dem Setting
#     herausgekuerzt; die uebrigen Laufzeit-Settings (thread_count, prefetch_distance) bleiben
#     als Kontroll-Dimension fixiert.
#
# Aufruf:  pwsh -File analyze_f15_holm.ps1 [-Csv <pfad>]
# NICHT committen (Analyse-Hilfsskript).

[CmdletBinding()]
param(
    [string]$Csv
)

$ErrorActionPreference = 'Stop'

# --- CSV-Quelle bestimmen: bevorzugt 150er, sonst Pilot -----------------------------------
if (-not $Csv) {
    $build = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path 'build/thesis_tiere'
    $tests = $PSScriptRoot
    $candidates = @(
        (Join-Path $build 'tier150_measurements.csv'),
        (Join-Path $tests 'tier150_measurements.csv'),
        (Join-Path $build 'pilot_verify_measurements.csv')
    )
    $Csv = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Csv -or -not (Test-Path $Csv)) { throw "Keine Mess-CSV gefunden (zuletzt geprueft: $Csv)" }

Write-Host "F15 Per-Achsen-Isolation (Pfad A, Doku 24 ss8)" -ForegroundColor Cyan
Write-Host "CSV: $Csv"
Write-Host ""

# --- Einlesen (Semikolon-separiert) --------------------------------------------------------
$rows = Import-Csv -Path $Csv -Delimiter ';'
Write-Host ("Datenzeilen: {0}" -f $rows.Count)

# --- Helfer: 'a=1/b=2/...' -> Hashtable ----------------------------------------------------
function ConvertTo-AxisMap([string]$s) {
    $h = [ordered]@{}
    foreach ($kv in $s -split '/') {
        if ($kv -match '^([^=]+)=(.*)$') { $h[$Matches[1]] = $Matches[2] }
    }
    return $h
}

# --- Pro (binary_id, control-setting) ueber Wiederholungen mitteln -------------------------
# control-setting = Laufzeit-Setting OHNE repetition.repetition_index
$agg = @{}   # key "binary||control" -> @{ binary; control; vals=List[double] }
foreach ($r in $rows) {
    $control = ($r.setting -split '/' | Where-Object { $_ -notmatch '^repetition\.repetition_index=' }) -join '/'
    $key = "$($r.binary_id)||$control"
    if (-not $agg.ContainsKey($key)) {
        $agg[$key] = [pscustomobject]@{ binary = $r.binary_id; control = $control; vals = [System.Collections.Generic.List[double]]::new() }
    }
    $agg[$key].vals.Add([double]$r.total_ns)
}
$cells = foreach ($e in $agg.Values) {
    $mean = ($e.vals | Measure-Object -Average).Average
    [pscustomobject]@{ binary = $e.binary; control = $e.control; mean_total_ns = $mean; reps = $e.vals.Count }
}
Write-Host ("Konfig-Zellen (binary x control, ueber Wiederholungen gemittelt): {0}" -f @($cells).Count)

# --- Welche statischen Achsen variieren ueberhaupt? ---------------------------------------
$binaries = $rows | Select-Object -ExpandProperty binary_id -Unique
$maps = @{}
foreach ($b in $binaries) { $maps[$b] = ConvertTo-AxisMap $b }
$allAxes = (ConvertTo-AxisMap $binaries[0]).Keys
$varyingAxes = foreach ($ax in $allAxes) {
    $vals = $binaries | ForEach-Object { $maps[$_][$ax] } | Select-Object -Unique
    if (@($vals).Count -gt 1) { $ax }
}
Write-Host ("Distinct Binaries: {0}" -f @($binaries).Count)
Write-Host ("Variierte statische Achsen: {0}" -f ($(if (@($varyingAxes).Count) { $varyingAxes -join ', ' } else { '(keine)' })))
Write-Host ""

# --- Pro variierte Achse: Paare bilden, die sich NUR in dieser Achse unterscheiden --------
# (zusaetzlich identisches control-Setting). Delta = mean_total_ns(B) - mean_total_ns(A),
# Achsen-Effekt = Mittelwert |Delta| und signed-Delta ueber alle solchen Paare.
$cellLookup = @{}
foreach ($c in $cells) { $cellLookup["$($c.binary)||$($c.control)"] = $c.mean_total_ns }

$results = foreach ($ax in $varyingAxes) {
    $axVals = ($binaries | ForEach-Object { $maps[$_][$ax] } | Select-Object -Unique | Sort-Object)
    $pairDeltas    = [System.Collections.Generic.List[double]]::new()
    $pairAbsDeltas = [System.Collections.Generic.List[double]]::new()
    $details       = [System.Collections.Generic.List[object]]::new()

    # Identitaet "alle anderen Achsen" = binary_id mit dieser Achse herausgekuerzt
    $byOther = @{}
    foreach ($b in $binaries) {
        $other = (($b -split '/') | Where-Object { $_ -notmatch "^$ax=" }) -join '/'
        if (-not $byOther.ContainsKey($other)) { $byOther[$other] = [System.Collections.Generic.List[string]]::new() }
        $byOther[$other].Add($b)
    }

    foreach ($controlGrp in ($cells | Select-Object -ExpandProperty control -Unique)) {
        foreach ($other in $byOther.Keys) {
            $grpBins = $byOther[$other]
            if ($grpBins.Count -lt 2) { continue }
            # alle ungeordneten Paare innerhalb dieser Gruppe, beide Zellen muessen existieren
            for ($i = 0; $i -lt $grpBins.Count; $i++) {
                for ($j = $i + 1; $j -lt $grpBins.Count; $j++) {
                    $ba = $grpBins[$i]; $bb = $grpBins[$j]
                    $va = $cellLookup["$ba||$controlGrp"]
                    $vb = $cellLookup["$bb||$controlGrp"]
                    if ($null -eq $va -or $null -eq $vb) { continue }
                    # stabile Richtung: nach Achsen-Wert sortieren -> (kleinerer Wert) als A
                    $valA = $maps[$ba][$ax]; $valB = $maps[$bb][$ax]
                    if ($valA -gt $valB) { $t=$ba;$ba=$bb;$bb=$t; $t=$va;$va=$vb;$vb=$t; $t=$valA;$valA=$valB;$valB=$t }
                    $delta = $vb - $va
                    $pairDeltas.Add($delta)
                    $pairAbsDeltas.Add([math]::Abs($delta))
                    $details.Add([pscustomobject]@{ from=$valA; to=$valB; control=$controlGrp; delta_ns=$delta })
                }
            }
        }
    }

    [pscustomobject]@{
        axis        = $ax
        values      = ($axVals -join ', ')
        n_pairs     = $pairDeltas.Count
        mean_delta_ns     = if ($pairDeltas.Count)    { [math]::Round((($pairDeltas    | Measure-Object -Average).Average),1) } else { $null }
        mean_abs_delta_ns = if ($pairAbsDeltas.Count) { [math]::Round((($pairAbsDeltas | Measure-Object -Average).Average),1) } else { $null }
        details     = $details
    }
}

# --- Ausgabe: per-Achsen-Zeit-Deltas literal ----------------------------------------------
Write-Host "=== Per-Achsen-Zeit-Deltas (F15-Hybrid, Pfad A) ===" -ForegroundColor Green
$results |
    Sort-Object { if ($_.mean_abs_delta_ns) { -$_.mean_abs_delta_ns } else { 0 } } |
    Format-Table axis, values, n_pairs, mean_delta_ns, mean_abs_delta_ns -AutoSize | Out-String -Width 200 | Write-Host

# --- Richtungs-Aufschluesselung pro Achsen-Wert-Uebergang ----------------------------------
Write-Host "=== Aufschluesselung pro Wert-Uebergang (signed mean_delta_ns, gemittelt ueber Controls) ===" -ForegroundColor Green
foreach ($res in $results) {
    if (-not $res.details -or $res.details.Count -eq 0) { continue }
    Write-Host ("--- {0} (n_pairs={1}) ---" -f $res.axis, $res.n_pairs)
    $res.details |
        Group-Object { "$($_.from) -> $($_.to)" } |
        ForEach-Object {
            $m = ($_.Group.delta_ns | Measure-Object -Average).Average
            [pscustomobject]@{
                transition       = $_.Name
                n                = $_.Count
                mean_delta_ns    = [math]::Round($m,1)
                interpretation   = if ($m -gt 0) { "zweiter Wert langsamer" } elseif ($m -lt 0) { "zweiter Wert schneller" } else { "gleich" }
            }
        } | Format-Table -AutoSize | Out-String -Width 200 | Write-Host
}

# --- Hinweis bei zu wenigen Paaren ---------------------------------------------------------
$totalPairs = ($results | Measure-Object -Property n_pairs -Sum).Sum
if ($totalPairs -lt 1) {
    Write-Host "HINWEIS: Keine isolierenden Binary-Paare in dieser CSV (jede Achse konstant). Methode nicht anwendbar." -ForegroundColor Yellow
} elseif (@($binaries).Count -le 8) {
    Write-Host ("HINWEIS: Kleiner Pilot ({0} Binaries, nur {1} variierte Achse(n)). Die Methode ist an den vorhandenen Paaren demonstriert; fuer die uebrigen 17 statischen Achsen liegen mangels Variation KEINE Paare vor." -f @($binaries).Count, @($varyingAxes).Count) -ForegroundColor Yellow
}
