param(
    [int]$X1, [int]$Y1,
    [int]$X2, [int]$Y2,
    [int]$Steps = 10
)
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class MDrag {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
}
'@
[MDrag]::SetCursorPos($X1, $Y1) | Out-Null
Start-Sleep -Milliseconds 120
[MDrag]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)  # LEFTDOWN
Start-Sleep -Milliseconds 100
for ($i = 1; $i -le $Steps; $i++) {
    $x = $X1 + [int](($X2 - $X1) * $i / $Steps)
    $y = $Y1 + [int](($Y2 - $Y1) * $i / $Steps)
    [MDrag]::SetCursorPos($x, $y) | Out-Null
    Start-Sleep -Milliseconds 30
}
Start-Sleep -Milliseconds 150
[MDrag]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)  # LEFTUP
Write-Output "dragged ($X1,$Y1) -> ($X2,$Y2)"
