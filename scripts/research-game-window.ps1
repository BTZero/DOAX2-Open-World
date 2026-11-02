# Targeted keyboard/screenshot helper for isolated local research builds only.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][ValidateSet('Capture', 'Key', 'Close', 'Status')][string]$Action,
    [ValidateSet('Return','Space','Backspace','Tab','Escape','Up','Down','Left','Right',
                 'W','A','S','D','I','J','K','L','Q','E','R','F')][string]$Key = 'Return',
    [ValidateRange(40,2000)][int]$HoldMilliseconds = 120,
    [switch]$SnapshotWhileHeld
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$outputRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'out')) + [IO.Path]::DirectorySeparatorChar
$gameProcess = Get-Process -Id $ProcessId -ErrorAction Stop
if ($gameProcess.ProcessName -ne 'doax2' -or
    -not $gameProcess.Path.StartsWith($outputRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Only a doax2.exe research process inside this workspace/out is permitted.'
}
$gameProcess.Refresh()
$window = $gameProcess.MainWindowHandle
if ($Action -eq 'Status') {
    $gameProcess | Select-Object Id, ProcessName, MainWindowHandle, MainWindowTitle, Responding
    return
}
if ($window -eq [IntPtr]::Zero) { throw 'The game has no main window yet.' }

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ResearchGameWindow {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT {
        public ushort wVk, wScan; public uint dwFlags, time; public UIntPtr dwExtraInfo;
    }
    [StructLayout(LayoutKind.Explicit, Size=40)] public struct INPUT {
        [FieldOffset(0)] public uint type;
        [FieldOffset(8)] public KEYBDINPUT ki;
    }
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int command);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll", SetLastError=true)] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint key, uint mode);
    public static bool Key(ushort key, bool up) {
        uint flags = 8u | (up ? 2u : 0u); // SDL consumes physical scancodes.
        if (key >= 37 && key <= 40) flags |= 1u; // Extended arrow keys, not keypad keys.
        var input = new INPUT { type = 1, ki = new KEYBDINPUT { wScan = (ushort)MapVirtualKey(key, 0), dwFlags = flags } };
        return SendInput(1, new[] { input }, Marshal.SizeOf(typeof(INPUT))) == 1;
    }
}
'@
if ($Action -eq 'Close') {
    if (-not [ResearchGameWindow]::PostMessage($window, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)) {
        throw 'Unable to request normal game-window shutdown.'
    }
    if (-not $gameProcess.WaitForExit(10000)) {
        throw 'Normal shutdown is still pending; no process was forcibly terminated.'
    }
    return
}
[ResearchGameWindow]::ShowWindow($window, 9) | Out-Null
$shellActivation = New-Object -ComObject WScript.Shell
$shellActivation.AppActivate($ProcessId) | Out-Null
[ResearchGameWindow]::SetForegroundWindow($window) | Out-Null
Start-Sleep -Milliseconds 200
if ([ResearchGameWindow]::GetForegroundWindow() -ne $window) {
    $foreground = [ResearchGameWindow]::GetForegroundWindow()
    $foregroundPid = [uint32]0
    $foregroundThread = [ResearchGameWindow]::GetWindowThreadProcessId($foreground, [ref]$foregroundPid)
    $callerThread = [ResearchGameWindow]::GetCurrentThreadId()
    $attached = [ResearchGameWindow]::AttachThreadInput($callerThread, $foregroundThread, $true)
    try {
        [ResearchGameWindow]::BringWindowToTop($window) | Out-Null
        [ResearchGameWindow]::SetForegroundWindow($window) | Out-Null
    } finally {
        if ($attached) { [ResearchGameWindow]::AttachThreadInput($callerThread, $foregroundThread, $false) | Out-Null }
    }
    Start-Sleep -Milliseconds 200
    if ([ResearchGameWindow]::GetForegroundWindow() -ne $window) {
        throw 'Game window is not foreground; refusing keyboard input or screen capture.'
    }
}
if ($Action -eq 'Key') {
    $keys = @{Return=13; Space=32; Backspace=8; Tab=9; Escape=27; Up=38; Down=40; Left=37; Right=39}
    $virtualKey = if ($keys.ContainsKey($Key)) { $keys[$Key] } else { [int][char]$Key }
    if (-not [ResearchGameWindow]::Key([uint16]$virtualKey, $false)) { throw 'Key-down injection failed.' }
    try {
        Start-Sleep -Milliseconds $HoldMilliseconds
        if ($SnapshotWhileHeld) { & "$PSScriptRoot/snapshot-research-image.ps1" -ProcessId $ProcessId }
    }
    finally {
        if (-not [ResearchGameWindow]::Key([uint16]$virtualKey, $true)) { throw 'Key-up injection failed.' }
    }
    return
}
Add-Type -AssemblyName System.Drawing
$rect = New-Object ResearchGameWindow+RECT
$origin = New-Object ResearchGameWindow+POINT
if (-not [ResearchGameWindow]::GetClientRect($window, [ref]$rect) -or
    -not [ResearchGameWindow]::ClientToScreen($window, [ref]$origin)) { throw 'Cannot locate game client area.' }
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if ($width -le 0 -or $height -le 0 -or $width -gt 7680 -or $height -gt 4320) {
    throw 'Invalid game-window capture dimensions.'
}
$captureRoot = Join-Path $projectRoot 'out/open-world-research/captures'
New-Item -ItemType Directory -Force -Path $captureRoot | Out-Null
$capturePath = Join-Path $captureRoot ('game-{0}-{1}.png' -f $ProcessId,[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
$bitmap = New-Object Drawing.Bitmap $width,$height
$graphics = [Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen($origin.X, $origin.Y, 0, 0, $bitmap.Size)
    $bitmap.Save($capturePath, [Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}
$capturePath
