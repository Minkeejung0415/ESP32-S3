# Plugin — 3 minimal edits (if auto-patcher fails)

Target file: `acqboard.ccp` (or your `AcqBoardRedPitaya` implementation).

## 1. Host `127.0.0.1`

```cpp
static const char* kRedPitayaHosts[] = {
    "127.0.0.1",        // ESP32 USB gateway (rp_compat_gateway.py)
    "rp-f0f85a.local",
    "rp-f0cd35.local",
};
```

**Or** skip this if you use [hosts.txt](hosts.txt).

## 2. TCP samples in `run()` (ESP32 mode)

Add member: `bool esp32TcpStream = false;`

In `performDetectionHandshake()`, after parsing `OK CHANNELS:N`:

```cpp
if (response.contains("CHANNELS:11"))
    esp32TcpStream = true;
```

At the top of `run()`:

```cpp
if (esp32TcpStream && commandSocket != nullptr)
{
    const int frameBytes = headerSize + numAdcChannels * 2;
    // Read frameBytes from commandSocket (same 22-byte header + int16 payload as UDP).
    // Copy payload into thisSample and call your existing broadcastSample() path.
    return;
}
// ... existing UDP 55001 loop ...
```

Use the same framing as `host/esp32_tcp_client.py`.

## 3. Skip `SENSORS:` in `startAcquisition()`

```cpp
if (esp32TcpStream)
{
    numAdcChannels = 11;
    return;   // STARTED already received; no SENSORS line from ESP32
}
// ... existing SENSORS: parse ...
```

## No-rebuild path

```powershell
# 1) hosts.txt line
# 2)
python host\rp_compat_gateway.py COM5
# 3) Open Ephys Plugin → connect to rp-f0f85a.local as before
```

Auto-apply: `python scripts/patch_plugin_esp32.py --plugin-dir C:\path\to\Plugin`
