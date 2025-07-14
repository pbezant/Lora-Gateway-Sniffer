/**
 * LoRa Gateway Sniffer - ChirpStack Payload Decoder
 * 
 * This decoder handles multiple payload types:
 * - Port 1: Plain text messages (e.g., "Online!" join confirmation)
 * - Port 2: GPS data as JSON (latitude, longitude, altitude, satellites)
 * - Port 3: Status data as JSON (uptime, heap, rssi, snr)
 * - Port 4: Gateway discovery data as JSON (GPS + signal strength when gateway detected)
 * 
 * The decoder automatically detects JSON vs plain text payloads
 */

function decodeUplink(input) {
    try {
        const bytes = input.bytes;
        const result = {};
        
        if (bytes.length < 11) {
            throw new Error("Payload too short for binary format");
        }
        
        let offset = 0;
        
        // Uptime (4 bytes) - seconds as uint32
        result.uptime_seconds = (bytes[offset] << 24) | (bytes[offset + 1] << 16) | (bytes[offset + 2] << 8) | bytes[offset + 3];
        offset += 4;
        
        // Free heap (2 bytes) - KB as uint16
        result.free_memory_kb = (bytes[offset] << 8) | bytes[offset + 1];
        offset += 2;
        
        // RSSI (1 byte) - offset by 200 to fit in uint8 (so 100 = -100 dBm)
        result.rssi_dbm = bytes[offset] - 200;
        offset += 1;
        
        // SNR (1 byte) - multiply by 4 for 0.25 precision, offset by 128
        result.snr_db = (bytes[offset] - 128) / 4.0;
        offset += 1;
        
        // Battery voltage (2 bytes) - millivolts as uint16
        const batteryMV = (bytes[offset] << 8) | bytes[offset + 1];
        result.battery_voltage = batteryMV / 1000.0;
        offset += 2;
        
        // Battery percentage (1 byte)
        result.battery_percentage = bytes[offset];
        offset += 1;
        
        // Check if GPS data is present (need at least 13 more bytes)
        if (bytes.length >= offset + 13) {
            // Latitude (4 bytes) - float32
            const latBytes = new Uint8Array([bytes[offset], bytes[offset + 1], bytes[offset + 2], bytes[offset + 3]]);
            const latView = new DataView(latBytes.buffer);
            result.latitude = latView.getFloat32(0, false); // big-endian
            offset += 4;
            
            // Longitude (4 bytes) - float32
            const lonBytes = new Uint8Array([bytes[offset], bytes[offset + 1], bytes[offset + 2], bytes[offset + 3]]);
            const lonView = new DataView(lonBytes.buffer);
            result.longitude = lonView.getFloat32(0, false); // big-endian
            offset += 4;
            
            // Altitude (4 bytes) - float32
            const altBytes = new Uint8Array([bytes[offset], bytes[offset + 1], bytes[offset + 2], bytes[offset + 3]]);
            const altView = new DataView(altBytes.buffer);
            result.altitude = altView.getFloat32(0, false); // big-endian
            offset += 4;
            
            // Satellites (1 byte)
            result.satellites = bytes[offset];
            offset += 1;
        }
        
        // Add some useful computed fields
        result.uptime_hours = Math.round(result.uptime_seconds / 3600 * 100) / 100;
        result.has_gps = result.latitude !== undefined;
        
        return {
            data: result,
            warnings: [],
            errors: []
        };
        
    } catch (error) {
        return {
            data: {},
            warnings: [],
            errors: ["Decode error: " + error.message]
        };
    }
} 