# Drone reception in DNSquachWatch v0.8.0

Open **Field Tools → Drone Readings → Drone Tools**.

## Search mode

NORMAL retains general scanning. FOCUSED alternates 15 seconds of Wi-Fi-only listening and 5 seconds of BLE-only listening. Wi-Fi dwell grows from approximately 300 to 900 ms. A decoded Remote ID candidate can hold its channel for up to two seconds, with a gap before another hold so a busy transmitter cannot keep the receiver parked indefinitely. The hop range follows the driver's country configuration; channel-set failures are counted and the actual channel is queried. Focus ends when leaving Field Tools, locking, entering own telemetry, or shutting down. It is not saved across reboot. Other alerts have listening gaps while focus is active. Focus is a reception strategy requiring measurement on real radios, not a guarantee of better reception in every situation.

## Reception diagnostics

Counters distinguish received management frames / Bluetooth advertisements, decoded Remote ID messages, name-only clues, recognizable but unsupported Remote ID forms, malformed message-pack lengths, channel errors and aircraft queue drops. Counts are packets, not aircraft. A zero channel means not yet known (or simulated), not a real Wi-Fi channel. The channel shown during BLE focus is the last queried Wi-Fi channel, not evidence of simultaneous Wi-Fi reception.

A `RID-` or `DJI` Wi-Fi name is only a clue. It never becomes a confirmed model from that name. Valid Bluetooth Remote ID service data now creates a drone detection independently of the optional service UUID list. Unsupported versions remain unsupported; the decoder does not guess their layouts. Authentication is not added by this release.

## Raw capture

Choose **CAPTURE TO microSD → START RAW CAPTURE… → CONFIRM RAW CAPTURE**. The PREPARING screen is drawn before storage setup begins. RECORDING shows elapsed time, saved records and drops; STOP AND SAVE drains pending records before FINISHED. Errors stay visible. Recording stops on leaving the capture screen, leaving Field Tools, locking or safe shutdown. Wait for FINISHED or safe shutdown before removing power/card.

The file is `/dnsp-rid-capture.jsonl` at the microSD root. Starting a new capture rotates the prior file to `/dnsp-rid-capture.previous.jsonl`; the older previous file is replaced. Files contain raw identifiers and potentially broadcast aircraft/operator positions. Captures include only Remote ID candidates and DJI/RID name clues, not Wi-Fi data-frame payloads. Wi-Fi bytes start at the 802.11 header, without FCS; Bluetooth bytes are advertising data structures. Packets up to 1024 bytes are retained in full; longer packets explicitly record original/captured lengths and increment the truncation counter. The queue holds three records. Capture stops after 60 seconds or approximately 512 saved records (up to three already queued records may also drain); worst-case file size is under 1.2 MiB. A final summary records saved/dropped/truncated counts. Missing summary means completion was not confirmed. Timestamps are uptime, not calendar time. History wipe includes both capture files; deletion is not secure erasure.

## Identity association

Recent equal Basic IDs on Wi-Fi and BLE are associated for display, with both observations retained. Consistent paired entries are skipped as duplicates during NEXT navigation; conflicting pairs remain separately accessible. The pairing expires after 15 seconds without a fresh Basic ID. Different aircraft types or recent positions over one kilometre apart flag a conflict. This heuristic does not prove spoofing or authenticate identity. A no-fix location message clears the old location so it is no longer displayed as a current fix.

## Hardware and verification

This classic ESP32 receives 2.4 GHz Wi-Fi and legacy Bluetooth LE. Software cannot add 5 GHz reception, Bluetooth 5 extended/coded-PHY reception, or a decoder for arbitrary DJI O4 radio traffic. There is no external-receiver bridge in this release: a suitable phone/receiver is an independent comparison tool, not automatically connected to DNSP. Its eventual interface depends on the chosen receiver.

For Avata 2 testing, note the aircraft firmware, goggles Remote ID status, motors-running state during normal safe operation, DNSP version, and whether an independent receiver detects the aircraft. Compare NORMAL and FOCUSED scans separately. DJI's FAA Remote ID FAQ describes broadcast activation with propellers turning; powered-on alone is not confirmation of transmission. A missed Avata 2 has not yet been reproduced on a connected board, and this release does not claim a verified Avata 2 fix.

Primary references used during investigation:
- DJI Remote ID FAQ: https://repair.dji.com/help/content?customId=01700007747&lang=en&paperDocType=ARTICLE&re=US&spaceId=17
- ESP32 shared-radio coexistence limitations: https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/coexist.html
- OpenDroneID reference: https://github.com/opendroneid/opendroneid-core-c
- Independent Android receiver (phone-dependent transport support): https://github.com/opendroneid/receiver-android
