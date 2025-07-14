// LMIC project configuration for Heltec Wireless Tracker v1.1
// This file overrides the default LMIC configuration

#ifndef _LMIC_PROJECT_CONFIG_H_
#define _LMIC_PROJECT_CONFIG_H_

// Select the radio type
#define CFG_sx1262_radio 1

// Select the region
#define CFG_us915 1

// Enable sub-band selection
#define CFG_LMIC_US_like 1

// Disable other radios
#undef CFG_sx1276_radio
#undef CFG_sx1272_radio

// Disable other regions
#undef CFG_eu868
#undef CFG_au915
#undef CFG_as923
#undef CFG_in866
#undef CFG_kr920

// Enable device time requests
#define LMIC_ENABLE_DeviceTimeReq 1

// Disable ping and beacon functionality to save memory
#define DISABLE_PING 1
#define DISABLE_BEACONS 1

// Disable join functionality if using ABP (we're using OTAA)
// #define DISABLE_JOIN 1

// Enable/disable specific features
#define LMIC_ENABLE_arbitrary_clock_error 1

#endif // _LMIC_PROJECT_CONFIG_H_ 