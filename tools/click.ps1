param(
    [int]$X,
    [int]$Y,
    [int]$DelayMs = 80
)
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class MLaunchClick {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
}
'@
[MLaunchClick]::SetCursorPos($X, $Y) | Out-Null
Start-Sleep -Milliseconds $DelayMs
[MLaunchClick]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)  # LEFTDOWN
Start-Sleep -Milliseconds 40
[MLaunchClick]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)  # LEFTUP
Write-Output "clicked at $X,$Y"

