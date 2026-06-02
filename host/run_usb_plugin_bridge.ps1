# USB serial -> localhost TCP :5000 for Open Ephys Ephys Socket (or TCP test clients).
# Usage (from PowerShell, NOT python):
#   cd host
#   .\run_usb_plugin_bridge.ps1 COM3
#
# Requires: pip install pyserial

param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$ComPort,

    [int]$Baud = 115200,
    [string]$Bind = "127.0.0.1",
    [int]$TcpPort = 5000
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$bridge = Join-Path $here "serial_tcp_bridge.py"

if (-not (Test-Path $bridge)) {
    Write-Error "Missing $bridge"
}

Write-Host "Bridge: $ComPort @ $Baud -> ${Bind}:${TcpPort}"
Write-Host "Close Arduino Serial Monitor first. Open Ephys Ephys Socket -> Connect on port $TcpPort"
Write-Host ""

python $bridge $ComPort --baud $Baud --bind $Bind --tcp-port $TcpPort
