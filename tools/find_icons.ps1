param([string]$ImgPath)
Add-Type -AssemblyName System.Drawing
$src = [System.Drawing.Image]::FromFile($ImgPath)
$bmp = New-Object System.Drawing.Bitmap($src)
$cols = @{}
for ($x = 300; $x -lt $bmp.Width; $x++) {
    $dark = 0
    for ($y = 3; $y -lt 29; $y++) {
        $c = $bmp.GetPixel($x, $y)
        if ($c.R -lt 160 -and $c.G -lt 160 -and $c.B -lt 160) { $dark++ }
    }
    if ($dark -gt 0) { $cols[$x] = $dark }
}
$src.Dispose(); $bmp.Dispose()
$runs = New-Object System.Collections.ArrayList
$start = $null; $prev = $null
foreach ($k in ($cols.Keys | Sort-Object)) {
    if ($null -eq $start) { $start = $k }
    elseif ($k -ne ($prev + 1)) {
        [void]$runs.Add(@($start, $prev)); $start = $k
    }
    $prev = $k
}
if ($null -ne $start) { [void]$runs.Add(@($start, $prev)) }
# merge runs separated by small gaps (<8px)
$merged = New-Object System.Collections.ArrayList
if ($runs.Count -gt 0) {
    $cur = $runs[0]
    for ($i = 1; $i -lt $runs.Count; $i++) {
        if ($runs[$i][0] - $cur[1] -le 8) { $cur = @($cur[0], $runs[$i][1]) }
        else { [void]$merged.Add($cur); $cur = $runs[$i] }
    }
    [void]$merged.Add($cur)
}
foreach ($r in $merged) {
    Write-Output ("icon run: x {0} - {1}, center {2}" -f $r[0], $r[1], [int](($r[0] + $r[1]) / 2))
}
