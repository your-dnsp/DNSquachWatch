# v1.5.3 gift-readiness fixes (local; unpublished)

The connection-only Wi-Fi screen offers **Set location for this Wi-Fi**. It captures the authenticated network before releasing the radio to scanning. The location editor displays the currently connected or last verified network, and the active label's saved association. **Save for Wi-Fi** explicitly stores a binding. Manual presets and custom session labels do not replace that binding. **Clear Label** clears only the session; **Forget Wi-Fi** removes the network association. Historical labels remain available. Leaving this shortcut returns to Wi-Fi Networks. PIN auto-lock releases connection-only Wi-Fi ownership rather than leaving acquisition paused.

Credential saves check NVS writes and readback; failures report failure and attempt rollback. This does not make NVS a multi-key atomic database. Device Health exports use the running firmware version.

Safe Shutdown cancels backup, readable-history refresh, backup maintenance, and content-reader state; stops research, drone recording and OTA Wi-Fi; then waits for acquisition/storage workers before closing the card. Card recovery refuses concurrent storage work and cancels the readable refresh before proceeding. A canceled backup remains incomplete, never reported complete.

**Storage & Recovery > Backup & Restore > Files / Slots** offers deliberately confirmed removal of incomplete backup slots and archival of complete backups from older firmware into **/Archive**. Cleanup is limited to ten known slot directories, refuses nested directories, and checks up to 64 leaves before deletion. Archive destinations never overwrite existing archives. Same/newer-version backups are retained. Archival frees a slot; restore does not directly browse archives. Keep a computer copy before deleting an incomplete backup containing useful recovery evidence.

Optional checkpoint reads/removes check existence first, avoiding misleading missing-file error messages while retaining actual storage failure reporting.

## Wi-Fi OTA content compatibility

Wi-Fi OTA now requires a signed `manifest-<target>.json` plus `.sig`. The signed domain is `DNSP_CONTENT1\n<target>\n` followed by the exact JSON bytes. The manifest contains `required_content` and `signed_image_sha256`, the SHA-256 of the existing image signature domain (`SQWOTA1\n<target>\n` plus image). The compiled P-256 public key verifies the contract; the installer compares the streamed image digest against it before activating the slot. Unknown/missing content requirements, absent signatures, altered contracts, wrong-board contracts or mismatched images fail closed.

The card's v1.5 guides, glyphs, translations and installation document are checked with compiled sizes/checksums before download and again before writing firmware. Reads stream through a 128-byte buffer; oversized guide files are rejected. These checks detect missing/corrupt card assets; the checksums do not authenticate removable-card content against deliberate replacement. No card assets are downloaded over Wi-Fi.

`tools/sign_content_manifest.py` prepares the detached contract for a future release. Existing public manifests do not contain this contract, so this build refuses their Wi-Fi installation until a compatible signed release is published. USB flashing remains unchanged. Bluetooth OTA retains its previous signed-image protocol and does not gain this Wi-Fi manifest gate in this patch. OTA slot switching/rollback also does not negotiate a different card content version. The card content itself is unchanged.

## Validation

Full host tests and UI simulation passed. Added regressions cover explicit label binding and reboot recall, preservation of a Home binding after a manual Driving label, card-content corruption/missing assets, bounded backup cleanup and archive collisions/cancel, and malformed content contracts. Signing tests verify a valid contract and reject tampered requirements/digests and wrong targets. ST7789-80MHz compilation and complete-image checks are required for packaging.

The user confirmed backup and Safe Shutdown on the previously flashed firmware, without a panic; the serial disconnection followed deliberate unplugging after the safe message. This is not hardware validation of v1.5.3. Retest its Wi-Fi label/reboot, backup through PIN auto-lock, and Safe Shutdown on one or two units. OTA end-to-end installation and slow/failing-card timing still require hardware validation. No files have been pushed for this pass.
