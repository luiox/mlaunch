# mlaunch UI 截图工具
# 用法:
#   pwsh -File tools/shot.ps1                          # 截 mlaunch 主窗口 -> build/shot_latest.png
#   pwsh -File tools/shot.ps1 -ProcessName poner       # 指定进程名
#   pwsh -File tools/shot.ps1 -TitlePattern "Poner"    # 按窗口标题匹配
#   pwsh -File tools/shot.ps1 -OutPath shot1.png       # 指定输出
# 行为: 优先 PrintWindow(PW_RENDERFULLCONTENT)，全黑/全白时自动退回屏幕拷贝。
param(
    [string]$ProcessName = "mlaunch",
    [string]$TitlePattern = "",
    [string]$OutPath = "",
    [switch]$StartIfMissing,
    [string]$ExePath = ""
)

Add-Type -AssemblyName System.Drawing

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public class MLaunchShot {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
}
'@

if ($OutPath -eq "") {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $OutPath = Join-Path (Split-Path -Parent $scriptDir) "build\shot_latest.png"
}
$outDir = Split-Path -Parent $OutPath
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory $outDir -Force | Out-Null }

# 1) 找窗口: 枚举可见顶层窗口, 按进程名或标题匹配, 取面积最大者
$candidates = [System.Collections.ArrayList]::new()
$cb = [MLaunchShot+EnumWindowsProc]{
    param($hWnd, $lParam)
    if ([MLaunchShot]::IsWindowVisible($hWnd)) {
        $title = New-Object System.Text.StringBuilder 256
        [MLaunchShot]::GetWindowText($hWnd, $title, 256) | Out-Null
        $t = $title.ToString()
        if ($t.Length -eq 0) { return $true }
        $match = $false
        if ($TitlePattern -ne "" -and $t -match $TitlePattern) { $match = $true }
        if ($ProcessName -ne "" -and $TitlePattern -eq "") {
            $pid2 = 0
            [MLaunchShot]::GetWindowThreadProcessId($hWnd, [ref]$pid2) | Out-Null
            $proc = Get-Process -Id $pid2 -ErrorAction SilentlyContinue
            if ($proc -and $proc.ProcessName -eq $ProcessName) { $match = $true }
        }
        if ($match) {
            $r = New-Object MLaunchShot+RECT
            [MLaunchShot]::GetWindowRect($hWnd, [ref]$r) | Out-Null
            [void]$candidates.Add(@{Hwnd=$hWnd; Title=$t; W=$r.Right-$r.Left; H=$r.Bottom-$r.Top; L=$r.Left; T=$r.Top})
        }
    }
    return $true
}
[MLaunchShot]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

$target = $candidates | Sort-Object { $_.W * $_.H } -Descending | Select-Object -First 1
if (-not $target -and $StartIfMissing) {
    $exe = if ($ExePath -ne "") { $ExePath } else { Join-Path (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)) "build\windows\x64\release\$ProcessName.exe" }
    if (Test-Path $exe) {
        Start-Process -FilePath $exe | Out-Null
        Start-Sleep -Seconds 4
        & $MyInvocation.MyCommand.Path -ProcessName $ProcessName -TitlePattern $TitlePattern -OutPath $OutPath
        return
    }
}
if (-not $target) { Write-Output "NO_WINDOW matched (process=$ProcessName title=$TitlePattern)"; exit 1 }

$w = $target.W; $h = $target.H
if ($w -le 0 -or $h -le 0) { Write-Output "BAD_SIZE"; exit 1 }
Write-Output "WINDOW: '$($target.Title)' ${w}x${h} at $($target.L),$($target.T)"

# 2) PrintWindow 渲染
function Capture-PrintWindow([IntPtr]$hwnd, [int]$w, [int]$h) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    [MLaunchShot]::PrintWindow($hwnd, $hdc, 2) | Out-Null
    $g.ReleaseHdc($hdc); $g.Dispose()
    return $bmp
}
function Test-Blank([System.Drawing.Bitmap]$bmp) {
    # 采样判断全黑/全白
    $sum = 0.0; $n = 0; $step = [Math]::Max(1, [int](($bmp.Width * $bmp.Height) / 500))
    for ($i = 0; $i -lt $bmp.Width * $bmp.Height; $i += $step) {
        $x = $i % $bmp.Width; $y = [Math]::Min($bmp.Height - 1, [int]($i / $bmp.Width))
        $c = $bmp.GetPixel($x, $y); $sum += ($c.R + $c.G + $c.B) / 3.0; $n++
    }
    $avg = $sum / [Math]::Max(1, $n)
    return ($avg -lt 2 -or $avg -gt 254.5), $avg
}

$bmp = Capture-PrintWindow $target.Hwnd $w $h
$blank, $avg = Test-Blank $bmp
if ($blank) {
    Write-Output "PrintWindow blank (avg=$avg), fallback to screen copy"
    $bmp.Dispose()
    [MLaunchShot]::ShowWindow($target.Hwnd, 9) | Out-Null
    [MLaunchShot]::SetForegroundWindow($target.Hwnd) | Out-Null
    Start-Sleep -Milliseconds 600
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($target.L, $target.T, 0, 0, $bmp.Size)
    $g.Dispose()
}
$bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "SAVED: $OutPath (avg brightness: $([Math]::Round($avg, 1)))"
