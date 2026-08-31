# 用 UIA 在原生菜单(#32768)中选择指定名称的菜单项
# 用法: pwsh -NoProfile -File tools/uia_menu.ps1 -ProcId <pid> -ItemName "设置"
# 背景: 菜单键盘导航的 Down 计数在自动化里不稳（首键偶发被吞），
#       SelectionItem.Select() 是确定性路径。
param(
    [int]$ProcId = 0,
    [string]$ItemName = ""
)
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ClassNameProperty, "#32768")
$menus = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)

$found = 0
foreach ($m in $menus) {
    if ($ProcId -ne 0 -and $m.Current.ProcessId -ne $ProcId) { continue }
    $found++
    $items = $m.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)))
    Write-Output ("MENU hwnd pid={0} items={1}" -f $m.Current.ProcessId, $items.Count)
    foreach ($it in $items) {
        $n = $it.Current.Name
        if ($ItemName -ne "" -and $n -eq $ItemName) {
            Write-Output "  TARGET: '$n'"
            $done = $false
            $sel = $null
            if ($it.TryGetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern, [ref]$sel)) {
                $sel.Select(); $done = $true; Write-Output "  SELECTED(SelectionItem): '$n'"
            }
            $inv = $null
            if (-not $done -and $it.TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$inv)) {
                $inv.Invoke(); $done = $true; Write-Output "  INVOKED(Invoke): '$n'"
            }
            if (-not $done) {
                # 回退：取项矩形中心，SendInput 单击
                $r = $it.Current.BoundingRectangle
                if ($r.Width -gt 0) {
                    $cx = [int]($r.X + $r.Width / 2); $cy = [int]($r.Y + $r.Height / 2)
                    Add-Type @"
using System.Runtime.InteropServices;
public class UClick {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, System.UIntPtr e);
}
"@
                    [UClick]::SetCursorPos($cx, $cy) | Out-Null
                    Start-Sleep -Milliseconds 60
                    [UClick]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
                    Start-Sleep -Milliseconds 30
                    [UClick]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
                    Write-Output "  CLICKED(rect $cx,$cy): '$n'"
                } else { Write-Output "  (empty rect on '$n')" }
            }
        } else {
            Write-Output "  ITEM: '$n'"
        }
    }
}
if ($found -eq 0) { Write-Output "NO_MENU_WINDOW" }
