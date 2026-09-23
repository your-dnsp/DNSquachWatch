# DNSP feature ledger — v0.7

Status describes delivered code, not a claim of real-device validation. Old proposal documents are historical plans, not release manifests.

| Area | Implementation status | Remaining validation / work |
|---|---|---|
| 15/30/45/60-second alerts; default 30; queue and X more | Implemented | Physical burst/field testing |
| Why this matched; qualitative confidence | Implemented | Model-specific captured examples; never a numeric probability |
| Flock/Raven/Axon/Meta and imported rules | Experimental implementation | False-positive controls and model validation; not confirmation |
| ALPR Radar / DeFlock voluntary follow-up | URLs implemented | Physical usability; QR coverage incomplete |
| Remote ID over BLE/Wi-Fi; ELRS setup clues | Implemented | Actual Air65/Air65 II/RadioMaster/FatShark models need testing; no ELRS flight-link or 5.8GHz receiver |
| Research sessions, notes, signature packs, redacted/raw exports | Implemented | Device/card failure testing |
| microSD status and error logging | Implemented | Volume label unavailable; physical full/removed/failing-card tests |
| PIN, lock, existing history-wipe controls | Inherited/extended | Screen lock does not encrypt flash or the microSD |
| Safe shutdown / reboot | Implemented | Real removal/power-loss tests |
| Menu organization and language/accessibility shortcuts | Implemented v0.6 | Real thumb/long-session usability |
| Language previews EN/ES/FR/DE/JA/ZH; hidden Hebrew | Implemented, partial translation | Native-speaker review; not full-system translation or general Hebrew shaping |
| Breakout | Implemented | Theme improved in this release; real frame/radio performance |
| 80 MHz display | Preserved experimental setting | Electrical stability unverified on user's board |
| One firmware, no feature stripping | Preserved | Keep both OTA slots and raw BlackBox region intact |
| Optional OSRS interface font | Planned, not delivered | Licensed readable glyphs and resource measurement |
| Additional RS3/OSRS dialogue, quests, pets, unlocks | Planned beyond inherited content | Approved creative scope is not implemented just because it appears in a plan |
| WiGLE | Deferred by user | Revisit when requested |
| External 5.8GHz receiver | Deferred #36 | Hardware/pin feasibility |
| Universal remote, WLED/HA/etc. | Research/design only | No implemented controls or pairing |
| Firmware/public-preferences backup and USB recovery | Implemented v0.7 | Real recovery drill still required |
| Feature checklist, endurance checks, demo, status help, favorites, gifting, field report | Implemented v0.7; hardware checks pending | See RELEASE-REVIEW.md for final results |
| Whole-card duress wipe, anti-forensics, decoy application | Not implemented | Existing PIN/history wipe must not be described as this |
| Beacon spam, Doom | Not implemented | Not added by this release |

Earlier numbered badge ideas remain tracked in DNSquachWatch-badgelife-ideas.md. Approval is not completion. Additional games/content, task-list concepts, pixel workshop, social experiments and other approved ideas without an implemented screen remain backlog. Features explicitly declined/deferred remain declined/deferred. This release does not reinterpret “do all eight” as approval to re-enable declined features.
