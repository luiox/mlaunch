param([string]$Image)
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Bitmap]::FromFile($Image)
Write-Host ("size: " + $b.Width + "x" + $b.Height)
$rows = @{}
for ($y = 0; $y -lt $b.Height; $y++) {
    $count = 0
    for ($x = 0; $x -lt $b.Width; $x++) {
        $c = $b.GetPixel($x, $y)
        if ($c.R -eq 0xE6 -and $c.G -eq 0xE6 -and $c.B -eq 0xE6) { $count++ }
    }
    if ($count -gt 20) { $rows[$y] = $count }
}
$keys = $rows.Keys | Sort-Object
$prev = -10; $start = -1
foreach ($y in $keys) {
    if ($y -ne $prev + 1) {
        if ($start -ge 0) { Write-Host ("E6E6E6 band y=" + $start + ".." + $prev) }
        $start = $y
    }
    $prev = $y
}
if ($start -ge 0) { Write-Host ("E6E6E6 band y=" + $start + ".." + $prev) }
$b.Dispose()
