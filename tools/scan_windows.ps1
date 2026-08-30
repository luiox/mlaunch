# 列出指定进程的全部顶层窗口：类名/可见性/扩展样式/矩形
# 用法: pwsh -NoProfile -File tools/scan_windows.ps1 [-ProcessName mlaunch]
param(
    [string]$ProcessName = "mlaunch"
)
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WScan {
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern int GetWindowLong(IntPtr h, int i);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public struct RECT { public int L,T,R,B; }
  static uint target;
  static bool Proc(IntPtr h, IntPtr l) {
    uint pid; GetWindowThreadProcessId(h, out pid);
    if(pid == target) {
      var sb = new StringBuilder(256); GetClassName(h, sb, 256);
      int ex = GetWindowLong(h, -20);
      var r = new RECT(); GetWindowRect(h, out r);
      Console.WriteLine(string.Format("hwnd=0x{0:X} class={1} visible={2} WS_EX_LAYERED={3} exstyle=0x{4:X} rect={5},{6} {7}x{8}",
        h.ToInt64(), sb, IsWindowVisible(h), (ex & 0x80000) != 0, ex, r.L, r.T, r.R-r.L, r.B-r.T));
    }
    return true;
  }
  public static void Run(uint pid) { target = pid; var d = new EnumWindowsProc(Proc); EnumWindows(d, IntPtr.Zero); }
}
"@
$p = Get-Process -Name $ProcessName -ErrorAction Stop | Select-Object -First 1
"WScan output for pid=$($p.Id):"
[WScan]::Run([uint32]$p.Id)
