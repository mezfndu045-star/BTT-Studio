param([int]$StartIndex = 0, [int]$MaxShots = 100000)

# Drives the BTT-Studio photo tour: the mod writes sd:/BTT-Studio/tour_shot.txt when a picture can be taken, this script
# presses Ctrl+P in yuzu (Capture Screenshot), moves the new screenshot to tour_shots\<shot>.png and answers with
# tour_ack.txt so the mod warps to the next shot.
$ErrorActionPreference = 'Stop'
$sd = Join-Path $env:APPDATA 'yuzu\sdmc\BTT-Studio'
$shots = Join-Path $env:APPDATA 'yuzu\screenshots'
$out = Join-Path $PSScriptRoot 'tour_shots'
$log = Join-Path $PSScriptRoot 'tour_capture.log'
New-Item -ItemType Directory -Force $out | Out-Null
New-Item -ItemType Directory -Force $shots | Out-Null

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Win {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
}
'@

function Log($m) { $line = "$(Get-Date -Format HH:mm:ss) $m"; Add-Content -Path $log -Value $line; Write-Output $line }

foreach ($f in 'tour_shot.txt', 'tour_ack.txt', 'tour_done.txt', 'tour_stop.txt') {
    $p = Join-Path $sd $f; if (Test-Path $p) { Remove-Item $p -Force }
}
[IO.File]::WriteAllText((Join-Path $sd 'tour_start.txt'), "$StartIndex")
Log "start at $StartIndex"

$count = 0
$idle = 0
while ($true) {
    if (Test-Path (Join-Path $sd 'tour_done.txt')) { Log 'tour done'; break }
    $shotFile = Join-Path $sd 'tour_shot.txt'
    if (-not (Test-Path $shotFile)) {
        Start-Sleep -Milliseconds 200
        $idle++
        if ($idle -gt 5 * 60 * 3) { Log 'no shot for 3 minutes, stopping'; break }
        continue
    }
    $idle = 0
    Start-Sleep -Milliseconds 100
    $name = ([IO.File]::ReadAllText($shotFile)).Trim()
    Remove-Item $shotFile -Force

    $yuzu = Get-Process yuzu -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if (-not $yuzu) { Log 'yuzu not found'; break }
    [Win]::ShowWindow($yuzu.MainWindowHandle, 3) | Out-Null
    [Win]::SetForegroundWindow($yuzu.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 400
    # Picture of the yuzu window as shown on screen (yuzu's own Ctrl+P screenshots are written minutes later)
    $r = New-Object Win+RECT
    [Win]::GetWindowRect($yuzu.MainWindowHandle, [ref]$r) | Out-Null
    $scr = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $x = [Math]::Max($r.Left, $scr.Left); $y = [Math]::Max($r.Top, $scr.Top)
    $w = [Math]::Min($r.Right, $scr.Right) - $x; $h = [Math]::Min($r.Bottom, $scr.Bottom) - $y
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size $w, $h))
    $bmp.Save((Join-Path $out "$name.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Log "ok $name"
    [IO.File]::WriteAllText((Join-Path $sd 'tour_ack.txt'), 'ok')
    $count++
    if ($count -ge $MaxShots) {
        [IO.File]::WriteAllText((Join-Path $sd 'tour_stop.txt'), 'stop')
        Log "stopped after $count shots"
        break
    }
}
