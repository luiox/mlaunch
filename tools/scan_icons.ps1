param([string]$Image, [int]$X0 = 190, [int]$X1 = 287)
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Bitmap]::FromFile($Image)
$cols = @{}
for ($x = $X0; $x -le $X1; $x++) {
    $hit = $false
    for ($y = 2; $y -le 32; $y++) {
        $c = $b.GetPixel($x, $y)
        if ($c.R -lt 0xA0) { $hit = $true; break }
    }
    $cols[$x] = $hit
}
$s = -1
for ($x = $X0; $x -le $X1 + 1; $x++) {
    $h = if ($x -le $X1) { $cols[$x] } else { $false }
    if ($h -and $s -lt 0) { $s = $x }
    if (-not $h -and $s -ge 0) {
        $ymin = 9999; $ymax = -1
        for ($xx = $s; $xx -le $x - 1; $xx++) {
            for ($y = 2; $y -le 32; $y++) {
                $c = $b.GetPixel($xx, $y)
                if ($c.R -lt 0xA0) {
                    if ($y -lt $ymin) { $ymin = $y }
                    if ($y -gt $ymax) { $ymax = $y }
                }
            }
        }
        Write-Host ("icon x=" + $s + ".." + ($x - 1) + " y=" + $ymin + ".." + $ymax + " size=" + ($x - $s) + "x" + ($ymax - $ymin + 1))
        $s = -1
    }
}
$b.Dispose()
