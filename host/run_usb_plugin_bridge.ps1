# USB serial → TCP bridge for Open Ephys Plugin Acq Board (Minkeejung0415/Plugin).
# Requires: pip install pyserial; board flashed with USB_OPEN_EPHYS_MODE true; close Serial Monitor.
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$ComPort
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

Write-Host "Bridge: $ComPort -> 127.0.0.1:5000 (--plugin). Open Ephys: Acq Board, Node IP 127.0.0.1, 100 Hz."
python host\serial_tcp_bridge.py $ComPort --plugin
