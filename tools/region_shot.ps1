param(
    [int]$X,
    [int]$Y,
    [int]$W = 300,
    [int]$H = 320,
    [string]$OutPath = "C:/Users/Canrad/AppData/Local/Temp/region.png"
)
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap($W, $H)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($X, $Y, 0, 0, $bmp.Size)
$g.Dispose()
$bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "SAVED: $OutPath (region $W x $H at $X,$Y)"
