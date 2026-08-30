param([int]$X, [int]$Y)
Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WFP2 {
    [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(int x, int y);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    public struct RECT { public int L,T,R,B; }
}
'@
$h = [WFP2]::WindowFromPoint($X, $Y)
$c = New-Object System.Text.StringBuilder 256; [WFP2]::GetClassName($h, $c, 256) | Out-Null
$t = New-Object System.Text.StringBuilder 256; [WFP2]::GetWindowText($h, $t, 256) | Out-Null
$pid2 = 0; [WFP2]::GetWindowThreadProcessId($h, [ref]$pid2) | Out-Null
$proc = (Get-Process -Id $pid2 -ErrorAction SilentlyContinue).ProcessName
$r = New-Object WFP2+RECT; [WFP2]::GetWindowRect($h, [ref]$r) | Out-Null
Write-Output ("physical($X,$Y) -> $proc [$($c.ToString())] title='$($t.ToString())' rect($($r.L),$($r.T),$($r.R),$($r.B))")
