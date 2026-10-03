// SquachWatch-CYD — detection type explanations
#include "detection_info.h"
#include "privacy.h"
#include <Arduino.h>
#include "research.h"
#include "detection.h"
#include "device_info.h"
#include "signatures.h"
#include <stdio.h>

namespace DetectionInfo {

// Indexed by DetectionType -- keep in the exact same order as the enum
// in state.h (UNKNOWN..DEAUTH), one entry per value up to COUNT.
static const char* const EXPLAIN_TEXT[] = {
    // UNKNOWN
    "Nothing in the signature tables matched this one. It is the fallback the panel falls back TO, so if you are reading it about a real sighting, that sighting got logged under a type with no explanation of its own -- which is a bug worth reporting.",
    // FLOCK
    "Flock makes license-plate reader cameras. A radio match is not visual confirmation. Shared hardware prefixes also occur in ordinary electronics. Check WHY THIS MATCHED before identifying a camera.",
    // AXON
    "Possible Axon/TASER equipment. Manufacturer and service identifiers can be shared across products. This does not confirm a body camera, its operator or recording status.",
    // META
    "Possible camera glasses or another Meta-related radio. A combined signature is more useful than one identifier, but does not prove recording. Glasses can capture without a phone connection; Bluetooth disruption is not reliable protection.",
    // SKIMMER
    "A Bluetooth card skimmer, usually wired into an ATM or gas pump reader. It quietly exfiltrates stolen card data over BLE instead of needing physical pickup.",
    // RAVEN
    "A Raven gunshot-detection sensor, usually mounted on a streetlight or rooftop. It listens constantly, not just after something happens. Matched on Raven's own Bluetooth service IDs, which have not been checked against real hardware -- hence the middling confidence. NOT ShotSpotter: those hold no registered hardware ID at all and backhaul over cellular rather than broadcasting, so nothing here can see one.",
    // AIRTAG
    "An Apple AirTag, riding Apple's Find My network. Legitimate for keys and luggage -- also a known method for tracking a person or vehicle without consent.",
    // DRONE
    "Remote ID broadcast. This firmware receives legacy Bluetooth and 2.4 GHz WiFi Beacon/NAN messages. Broadcast identity and position are unauthenticated. FPV & DRONES shows separate aircraft records, freshness and consistency warnings. Many FPV whoops emit no Remote ID. No video is received; Bluetooth 5 extended broadcasts are unsupported.",
    // ALPR
    "A manufacturer associated with some license-plate readers. A vendor prefix alone does not confirm ALPR. Model-specific support remains experimental; wired or cellular cameras may be invisible.",
    // CAMERA
    "A camera-brand WiFi or Bluetooth radio, matched by hardware vendor rather than a specific known network. Could be a doorbell, a security cam, almost anything with a lens.",
    // SAMSUNG_TAG
    "A Samsung Galaxy SmartTag, riding Samsung's own item-finder network. Same tracking-without-consent concern as an AirTag, different ecosystem.",
    // GOOGLE_TAG
    "A tracker on Google's Find My Device network -- Chipolo, Pebblebee, Moto Tag and others all ride the same system. Android's answer to Find My.",
    // TILE
    "A Tile Bluetooth tracker, one of the original item-finders. Independent of Apple/Google/Samsung's networks, same basic capability either way.",
    // RING
    "A Ring doorbell or camera, Amazon's video doorbell line. Often networked into neighborhood-wide sharing through the Neighbors app.",
    // DEAUTH
    "Not a hardware identification. This records a burst of WiFi deauthentication frames from one claimed transmitter address. One frame is normal traffic; repeated frames may be a flood, but source addresses can be spoofed and Squachy only samples each channel during its dwell time.",
    // EVILTWIN
    "Two different boxes are broadcasting the same network name, and they disagree about security -- one wants a password, the other is wide open. That's how a fake hotspot lures you on. A mesh system never argues with itself about encryption, which is what separates this from your own router.",
    // IBEACON
    "A proximity beacon, the kind bolted inside shops, stadiums and airports. It does not track you by itself -- it shouts an ID, and an app you already installed notices and reports where you are. The number shown is which deployment and which unit, so the same first half in two places is the same operator. This is the one detection that ships switched OFF, and about volume rather than importance: one shop can put more beacons in range than this device would otherwise see all week. Turn it on in DETECTION FILTER.",
    // HACKER
    "Wireless testing hardware: a Flipper Zero, a Pwnagotchi, a WiFi Pineapple or an ESP deauther. These are legitimate tools and most owners are hobbyists or people paid to break things -- but unlike everything else here, this is gear that transmits at other radios rather than just watching. A Pwnagotchi reports its own name and how many WiFi handshakes it has captured, because it is trying to be seen by others like it. Nothing here flags a bare ESP32 dev board: that would flag half the electronics in the room, and this detector too.",
    "FPV equipment clue: an ExpressLRS setup or Backpack network name. It may belong to a receiver, controller or accessory. Names can be imitated. This does not confirm a flying drone. Many Air65 whoops do not advertise WiFi during flight. See FPV & DRONES for the pit board and Remote ID readings.",
};
static const uint8_t EXPLAIN_TEXT_N = sizeof(EXPLAIN_TEXT) / sizeof(EXPLAIN_TEXT[0]);

// Add a DetectionType without writing its MORE INFO paragraph and this
// fails the build. Without it explain() quietly clamps out-of-range to 0,
// so the new detection would show UNKNOWN's text -- wrong, plausible, and
// invisible until somebody happened to tap it.
static_assert(EXPLAIN_TEXT_N == (uint8_t)DetectionType::COUNT,
              "every DetectionType needs a MORE INFO entry, in enum order");

const char* explain(DetectionType t) {
    uint8_t idx = (uint8_t)t;
    if (idx >= EXPLAIN_TEXT_N) idx = 0;
    return EXPLAIN_TEXT[idx];
}

const char* explainLive(DetectionType t, const DetectionEngine& eng) {
    if (t != DetectionType::DRONE) return explain(t);

    const RemoteId::Info& rid = eng.remoteId();
    if (!rid.haveBasic && !rid.haveLoc && !rid.haveOperator) return explain(t);

    // Replaces the paragraph rather than appending to it: Theme::wrapText
    // caps at 320 characters and the stock DRONE text already spends 185.
    // Only the parts actually received are printed -- the three messages
    // arrive as separate adverts, and 0,0 is a real place off Africa rather
    // than a safe stand-in for "not known yet".
    static char buf[320];
    int n = 0;
    if (rid.haveBasic)
        n += snprintf(buf + n, sizeof(buf) - n, "ID %s, a %s. ",
                      rid.serial, RemoteId::uaTypeName(rid.uaType));
    if (rid.haveLoc && n < (int)sizeof(buf))
        n += snprintf(buf + n, sizeof(buf) - n, "AIRCRAFT %.5f, %.5f at %dm. ",
                      (double)rid.lat, (double)rid.lon, (int)rid.altM);
    if (rid.haveOperator && n < (int)sizeof(buf))
        n += snprintf(buf + n, sizeof(buf) - n,
                      "Broadcast operator position %.5f, %.5f. ",
                      (double)rid.opLat, (double)rid.opLon);
    if(n<(int)sizeof buf)snprintf(buf+n,sizeof buf-n," %s.",RemoteId::qualityText(rid,millis()));
    buf[sizeof(buf) - 1] = '\0';
    return buf;
}

const char* explainFor(DetectionType t, const char* vendor, const char* name,
                       const DetectionEngine& eng) {
    if (t == DetectionType::DRONE) return explain(t); // no aircraft MAC here: never attach another aircraft's live position
    const DeviceInfo::Device* d = DeviceInfo::find(t, vendor, name);
    return d ? d->text : explain(t);
}

const char* titleFor(DetectionType t, const char* vendor, const char* name) {
    const DeviceInfo::Device* d = DeviceInfo::find(t, vendor, name);
    return d ? d->title : detectionTypeName(t);
}

const char* why(const Detection& d) {
    static char text[256];
    const char* reason = "The original match evidence was not saved. No specific rule can be confirmed for this record.";
    switch (d.evidence) {
        case MatchEvidence::OUI:
            reason = d.conf == Confidence::LOW_CONF
                ? "Shared manufacturer prefix. This does not confirm the device type."
                : "The MAC prefix matches a registered manufacturer in the signature list. It does not prove the model or owner.";
            break;
        case MatchEvidence::RESEARCH_COMPOSITE:
            snprintf(text, sizeof text, "Experimental rule %u. Fields:%s%s%s%s%s%s%s%s. Grade: %s. Does not confirm model, owner or recording.",
                     d.signature, d.evidenceBits & Research::COMPANY ? " company" : "",
                     d.evidenceBits & Research::SERVICE ? " service" : "",
                     d.evidenceBits & Research::NAME ? " name" : "",
                     d.evidenceBits & Research::OUI ? " prefix" : "",
                     d.evidenceBits & Research::SSID ? " WiFi-name" : "",
                     d.evidenceBits & Research::SERIAL_PATTERN ? " serial-pattern" : "",
                     d.evidenceBits & Research::FINGERPRINT ? " IE-shape" : "",
                     d.evidenceBits & Research::IMPORTED ? " imported-rule" : "", confidenceLabel(d.conf));
            return text;
        case MatchEvidence::BLE_REMOTE_ID: reason = "A Remote ID message was decoded from Bluetooth service data. Broadcast identity and position are not authenticated."; break;
        case MatchEvidence::WIFI_REMOTE_ID: reason = "A structurally valid WiFi Remote ID message pack was received. Broadcast identity and position are not authenticated."; break;
        case MatchEvidence::SSID: reason = "The WiFi network name matches a known naming pattern. Names can be changed or imitated."; break;
        case MatchEvidence::BLE_COMPANY: reason = "Bluetooth company data matches a listed manufacturer. It does not prove a particular model."; break;
        case MatchEvidence::BLE_SERVICE: reason = "An advertised Bluetooth service matches the signature list. Other products can share or imitate a service."; break;
        case MatchEvidence::BLE_NAME: reason = "The Bluetooth name matches a listed pattern. Names can be changed or imitated."; break;
        case MatchEvidence::FIND_MY: reason = "The advert matches a Find My tag payload pattern. This cannot establish its owner or intent."; break;
        case MatchEvidence::IBEACON: reason = "The manufacturer payload has the iBeacon structure. This is a proximity beacon, not proof of a camera."; break;
        case MatchEvidence::DEAUTH_BURST: {
            const unsigned targets = d.evidenceBits & DEAUTH_META_TARGET_MASK;
            const unsigned long duration = (unsigned long)(d.lastSeen - d.firstSeen);
            const char* protection = (d.evidenceBits & DEAUTH_META_PROTECTED_SEEN) &&
                                     (d.evidenceBits & DEAUTH_META_UNPROTECTED_SEEN)
                ? "mixed protected/unprotected"
                : (d.evidenceBits & DEAUTH_META_PROTECTED_SEEN) ? "protected" : "unprotected";
            char reasonPart[36] = "reason unavailable";
            if (d.evidenceBits & DEAUTH_META_REASON_VALID)
                snprintf(reasonPart, sizeof reasonPart, "reason %u", (unsigned)d.signature);
            char bssidPart[52] = "BSSID varied";
            if ((d.evidenceBits & DEAUTH_META_SAME_BSSID) && d.name[0])
                {char pv[40];snprintf(bssidPart, sizeof bssidPart, "BSSID %s",Privacy::name(d.name,pv,sizeof pv));}
            snprintf(text, sizeof text,
                     "%u deauth frames from one claimed transmitter in %lums; %u target%s, %s, %s, %s. Source MACs can be spoofed. Channel hopping means Squachy observed only part of the traffic. This does not prove an attack.",
                     (unsigned)d.hits, duration, targets, targets == 1 ? "" : "s",
                     bssidPart, reasonPart, protection);
            return text;
        }
        case MatchEvidence::EVIL_TWIN: reason = "One WiFi name appeared with different manufacturer prefixes and encryption. Legitimate networks can do this too."; break;
        case MatchEvidence::PWNAGOTCHI: reason = "A beacon contains Pwnagotchi-format data. Radio identity can be imitated."; break;
        default: break;
    }
    if (d.evidence == MatchEvidence::BLE_COMPANY || d.evidence == MatchEvidence::BLE_SERVICE)
        snprintf(text, sizeof text, "%s Rule ID: 0x%04X. Grade: %s.", reason, d.signature, confidenceLabel(d.conf));
    else if (d.evidence == MatchEvidence::OUI)
        snprintf(text, sizeof text, "%s Prefix: %02X:%02X:%02X. Grade: %s.", reason, d.mac[0], d.mac[1], d.mac[2], confidenceLabel(d.conf));
    else snprintf(text, sizeof text, "%s Grade: %s.", reason, confidenceLabel(d.conf));
    return text;
}

const char* rssiConfidencePrimer() {
    // Trimmed to fit the bigger text size this now renders at --
    // shorter sentences, same two facts.
    return "RSSI measures signal strength, not distance. Walls, antennas and interference change it. "
           "Confidence is how sure the match is: HIGH is a strong match, MED/LOW are looser guesses.";
}

}
