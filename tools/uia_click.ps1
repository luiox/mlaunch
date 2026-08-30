param(
    [string]$WindowTitle = "",
    [int]$ProcId = 0,
    [string]$ButtonName = ""
)
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$windows = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)

foreach ($w in $windows) {
    $name = $w.Current.Name
    $wpid = $w.Current.ProcessId
    if ($WindowTitle -ne "" -and $name -notmatch $WindowTitle) { continue }
    if ($ProcId -ne 0 -and $wpid -ne $ProcId) { continue }
    Write-Output "WINDOW: '$name' pid=$wpid"
    $btnCond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Button)
    $btns = $w.FindAll([System.Windows.Automation.TreeScope]::Descendants, $btnCond)
    foreach ($b in $btns) {
        $bn = $b.Current.Name
        Write-Output "  BUTTON: '$bn'"
        if ($ButtonName -ne "" -and $bn -match $ButtonName) {
            $inv = $null
            if ($b.TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$inv)) {
                $inv.Invoke()
                Write-Output "  INVOKED: $bn"
            }
        }
    }
}
