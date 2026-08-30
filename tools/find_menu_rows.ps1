param([string]$ImgPath)
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Bitmap]::FromFile($ImgPath)
$rows = @{}
for ($y = 0; $y -lt $b.Height; $y++) {
    $dark = 0
    for ($x = 0; $x -lt $b.Width; $x += 2) {
        $c = $b.GetPixel($x, $y)
        if ($c.R -lt 120 -and $c.G -lt 120 -and $c.B -lt 120) { $dark++ }
    }
    if ($dark -gt 2) { $rows[$y] = $dark }
}
$b.Dispose()
# merge into runs
$runs = New-Object System.Collections.ArrayList
$start = $null; $prev = $null
foreach ($k in ($rows.Keys | Sort-Object)) {
    if ($null -eq $start) { $start = $k }
    elseif ($k -ne ($prev + 1)) { [void]$runs.Add(@($start, $prev)); $start = $k }
    $prev = $k
}
if ($null -ne $start) { [void]$runs.Add(@($start, $prev)) }
# filter tiny runs (borders) and report
foreach ($r in $runs) {
    $h = $r[1] - $r[0] + 1
    if ($h -ge 8 -and $h -le 24) {
        Write-Output ("text row: y {0}-{1} (h={2}, center={3})" -f $r[0], $r[1], $h, [int](($r[0]+$r[1])/2))
    }
}
