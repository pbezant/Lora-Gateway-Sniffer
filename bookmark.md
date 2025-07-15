# Debugging Bookmark: LoRaWAN Device Send Failures

**Date:** [Fill in when you return]

---

## **Main Issue**
- Device joins LoRaWAN network (OTAA) and sends the first uplink successfully.
- All subsequent sends fail with error `-1108` (frame counter or replay protection issue).
- Serial output: "Failed to send combined data" after the first successful send.

## **What We’ve Tried**
- **Confirmed:** No accidental re-initialization or re-join between sends.
- **Debug prints:** Added for frame counter (`fCntUp`) before/after every send.
- **Frame counter persistence:** Now saving `fCntUp` to NVS after every send.
- **Manual increment:** As of last change, we manually increment `session.fCntUp` after every successful binary send in `sendStatusData`.
- **Checked RadioLib version:** Using latest stable, but RadioLib may not increment frame counter for binary sends.

## **Current Code State**
- `sendStatusData` (binary payloads) now increments and persists `session.fCntUp` after every successful send.
- Debug output will show the frame counter before and after increment.
- No changes to string/JSON send path (`sendData`), which already worked.

## **Next Steps for Tomorrow**
1. **Test the manual increment:**
   - Rebuild and flash firmware.
   - Watch serial output for `[LoRa][DEBUG] Manually incremented fCntUp: ...`.
   - See if device keeps sending data successfully after the first uplink.
2. **If still failing:**
   - Investigate RadioLib’s internal frame counter handling for binary payloads.
   - Consider forcing the frame counter in the RadioLib node object if possible.
   - Check ChirpStack/server logs for more details on rejected frames.
3. **If working:**
   - Clean up debug prints.
   - Document workaround and consider PR/issue for RadioLib if it’s a library bug.

---

**DO NOT LOSE THIS CONTEXT, BIG DADDY!**

Pick up here tomorrow and keep cursing at those bytes until they behave. 