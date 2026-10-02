# Share a device observation

DNSquachWatch accepts user observations through [Device identity research](https://github.com/your-dnsp/DNSquachWatch/issues/new?template=device_research.yml). The repository is currently private: contributors need access and a GitHub account. Nothing is sent automatically by the device.

1. Hold a current LOG or raw-scan entry to add or correct its user label. Save it to microSD.
2. Reopen that device's label and choose **RESEARCH REPORT...**.
3. Keep **FULL MAC + NAME: OMITTED** unless those details are necessary and you have permission to share them. Review the label/subtag too: those are included.
4. Choose **EXPORT REPORT**. Copy the file from `/Research Submissions/` to your computer.
5. Open the form above, attach the TXT file, and explain what you observed and how you identified the equipment. Include useful sources and uncertainty.

The report is JSON inside a plain TXT file so it can be attached easily and read by humans or tools. It includes the radio, observed prefix, label/subtag, detection rule/evidence/confidence IDs, RSSI, channel and uptime. Full MAC and advertised name are omitted by default. The optional advertised-name byte string escapes non-ASCII bytes as U+00XX so even malformed radio names remain valid JSON. No location label, GPS, network credentials, unrelated logs or whole backup is exported. The report names the firmware that produced it.

**Local labels identify an individual observation. They never automatically become OUI-wide rules.** Universally administered Wi-Fi addresses may contain a vendor prefix, but shared suppliers and spoofing limit what that proves. Locally administered/randomized addresses are not reliable vendor OUIs. For BLE, the saved observation does not establish public versus random address type. Prefixes are retained as observations, not verified manufacturer claims. The export's detector confidence describes the firmware match; it is separate from the research review status.

## Review and confirmation

Every report starts **UNVERIFIED**. Other contributors can comment using [the corroboration outline](research/CORROBORATION.md), add independent evidence, disagree, or correct it. Comments that say “confirmed on my equipment” are useful corroboration, but reactions, votes, multiple comments and repeated sightings do not automatically confirm a report.

A maintainer records one of these statuses in a comment and edits the issue title prefix accordingly:

- **UNVERIFIED**: hypothesis or observation without sufficient independent support.
- **CORROBORATED**: independent supporting evidence, still with unresolved identity or scope.
- **CONFIRMED**: sufficient evidence for the stated device/model and explicitly stated scope.
- **DISPUTED**: credible conflicting evidence; explain what needs resolving.

Maintainers may use corresponding issue labels for filtering, but the written decision must include its evidence, limits and date. Confirmation of a particular MAC/device is not confirmation of its whole prefix. A wider detection rule requires separate review of false positives, primary sources, address type and supplier sharing. Reports are never automatically imported into firmware, and they cannot overwrite DNSP detection rules.

Before making this repository public, review old reports and attachments for private identifiers. An initially private upload must not be assumed safe to publish unchanged. Report files stay on the card until the user deletes them or uses the existing destructive storage recovery/security functions; ordinary export does not erase the full local record.
