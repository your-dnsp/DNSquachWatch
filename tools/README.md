# Project tools

Run tools from the repository root unless their help says otherwise. End users can use the copies bundled with a complete firmware kit.

| Tool | Purpose |
|---|---|
| `build_version.py` | PlatformIO version stamp, pinned library adaptations and optimization flags |
| `check_flash_layout.py`, `check_image_size.py` | Protect reserved flash regions and enforce application size limits |
| `runtime_display.py`, `runtime_ble.py`, `optimization.py` | Build adapters for the pinned dependencies |
| `install_dnsp.py`, `flash_known.py` | Installation and explicit recovery operations |
| `verify_backup.py` | Check a completed backup before using it |
| `check_research_report.py` | Offline review of redacted research submissions |
| `research_issue_guard.py` | Review public issue text in GitHub Actions |
| `check_language_content.py`, `build_languages.py` | Validate and generate offline language content |
| `sign_firmware.py`, `publish_release.py` | Signed firmware and checked release publication |
| `size_audit.py` | Generate local resource reports; output is not committed |
| `ttf2gfx.py`, `verify_remmy_asset.py`, `pack_installation_guide.py` | Asset generation and integrity utilities |

Start with [Build instructions](../docs/BUILD.md), [installation](../docs/user/INSTALLATION.md), or [research contributions](../CONTRIBUTING.md).
