# LoRa Gateway Sniffer - Troubleshooting Guide

## Current Issues and Solutions

### 1. LoRa Transmission Failures (-1108 Error)

**Problem:** Device shows as "Joined: YES" but all data transmissions fail with error -1108 (Unknown error).

**Root Cause:** DevNonce reuse violation. Your ChirpStack logs show "DevNonce has already been used" errors.

**Solutions:**

1. **Immediate Fix - Clear Persistence:**
   ```
   clear_persistence
   ```
   Or use the short command:
   ```
   cp
   ```

2. **Alternative - Reset DevNonce:**
   ```
   reset_devnonce
   ```
   Or use the short command:
   ```
   rd
   ```

3. **Force Rejoin:**
   ```
   rejoin
   ```
   Or use the short command:
   ```
   rj
   ```

**What's happening:** LoRaWAN requires unique DevNonce values for each join attempt. When the device restarts, it's reusing old DevNonce values that ChirpStack has already seen, causing security violations.

### 2. GPS Not Getting Satellite Data

**Problem:** GPS shows 0 satellites and no sentences processed.

**Symptoms:**
- Total sentences: 0
- Passed checksums: 0
- Failed checksums: 0
- Characters processed: 0

**Possible Causes:**

1. **Indoor Operation:** GPS needs clear sky view
   - **Solution:** Test outdoors with clear view of sky
   - **Note:** Initial GPS fix can take 5-15 minutes outdoors

2. **Pin Configuration Issues:**
   - Current pins: RX:34, TX:33
   - **Verify:** Check if these pins are correct for your Heltec Wireless Tracker v1.1

3. **GPS Module Power:**
   - GPS module may not be receiving power
   - **Check:** Verify 3.3V supply to GPS module

4. **Serial Communication:**
   - UART communication may not be working
   - **Debug:** Add GPS raw data logging

### 3. Device Status Summary

**Working:**
- ✅ Display initialization
- ✅ LoRa hardware initialization  
- ✅ LoRaWAN network join (shows as joined)
- ✅ Command interface via serial

**Not Working:**
- ❌ LoRa data transmission (DevNonce issue)
- ❌ GPS satellite acquisition
- ❌ GPS data processing

## ChirpStack Payload Decoder

I've created a payload decoder (`payload_decoder.js`) that handles:

- **Port 2:** GPS data - `{"lat":40.712776,"lon":-74.005974,"alt":10.5,"sats":8}`
- **Port 3:** Status data - `{"uptime":238,"heap":366736,"rssi":-51.0,"snr":8.0}`

The decoder provides:
- Data validation with warnings for unusual values
- Human-readable uptime formatting
- Error handling for malformed payloads
- Separate fields for different data types

## Next Steps

1. **Fix DevNonce Issue (PRIORITY 1):**
   - Run `clear_persistence` command
   - Device should successfully transmit data after next join

2. **Test GPS Outdoors:**
   - Take device outside with clear sky view
   - Wait 10-15 minutes for initial fix
   - Monitor serial output for satellite acquisition

3. **Upload Payload Decoder:**
   - Copy `payload_decoder.js` content to ChirpStack
   - Configure device profile to use the decoder

4. **Monitor Results:**
   - Check ChirpStack for successful uplinks
   - Verify decoded data appears correctly
   - Monitor GPS fix acquisition outdoors

## Serial Commands Reference

- `help` or `h` - Show available commands
- `status` or `s` - Show system status  
- `clear_persistence` or `cp` - Clear LoRaWAN session (fixes DevNonce)
- `reset_devnonce` or `rd` - Reset DevNonce and force fresh join
- `rejoin` or `rj` - Attempt to rejoin network
- `devnonce` or `dn` - Show DevNonce info 