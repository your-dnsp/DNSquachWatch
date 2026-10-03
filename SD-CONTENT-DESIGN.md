# Versioned offline card content

Implemented for v1.4: `microSD-content/DNSP Content/v1.4/` contains bounded walkthrough pages and the unchanged installation document. Firmware checks page hashes/lengths and the document before using it. Missing content gives a clear tour message or backup preflight refusal; it does not block boot, scanning or essential recovery.

The release kit includes the whole folder and an offline Python installer. The same folder is prepared for GitHub so people flashing their own device can obtain it. esptool does not write card content. No post-install Wi-Fi fetch is used. The installer writes only the version-specific content namespace, verifies hashes and read-back, preserves logs, and never formats a card.

Remington's photograph and shooting-star sprite MUST remain compiled in. They are not supplied as replaceable card content. Existing multilingual font assets also remain compiled in for this release. Essential recovery information remains onboard.

Future movement of other reference material requires measured benefit and safe fallback. Compiling a seed into the same initial image does not by itself shrink that image. Firmware backup on the same card is not independent disaster protection; keep a computer copy of the card.
