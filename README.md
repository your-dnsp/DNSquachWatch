# DNSquachWatch v0.7-draft

A private, friends-and-family remix of **SquachWatch 1.19.1** for the **ESP32 CYD ST7789**. Original authorship and GPLv3 licensing are preserved; see [LICENSE](LICENSE) and the [original upstream README](README-UPSTREAM.md).

DNSP adds adjustable queued alerts and match explanations, experimental camera/wearable clues, research exports, FPV tools, friendlier menus, favorites, walkthroughs, partial translations, microSD firmware/public-preferences backups, recovery tools, and **Squach Snacks** Breakout. Existing features are retained. [Full details](README-DNSP.md) · [Feature status](FEATURE-STATUS.md) · [Tests](TEST-REPORT.md)

**This is a development draft.** Software checks passed; real-device endurance, SD failure handling and USB recovery still need testing before gifting. The included `cyd-fast` build retains the **experimental 80 MHz display**. Flash memory runs at **40 MHz**; these are different settings. Do not use this image on an ILI9341 or 3.5-inch CYD.

## Flash locally: Mac, Debian or Fedora

You do not drag the firmware onto the microSD or a mounted USB drive. Use a data-capable USB cable and write the four supplied images to their specified addresses. The original SquachWatch web flasher installs upstream firmware, replacing DNSP features; this kit is a local USB build, not an upstream-signed OTA package.

### 1. Get the files and Python

Download this private repository using **Code → Download ZIP**, extract it, and open Terminal in `firmware/v0.7-cyd-fast` inside the extracted folder. All four `.bin` files, `manifest.json`, and `SHA256SUMS` must be there. Friends need repository access or a kit you share with them. The separately supplied `DNSquachWatch-v0.7-cyd-fast.zip` has the same files at its top level.

**macOS:** use Python 3 from [python.org](https://www.python.org/downloads/macos/). If `python3 --version` already works, try the setup below first.

**Debian:**

```sh
sudo apt update
sudo apt install python3 python3-venv python3-pip
```

**Fedora:**

```sh
sudo dnf install python3 python3-pip
```

Then, on any of these systems, from the firmware folder:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install 'esptool==4.5.1'
```

This pins the tool version used for this kit. In a new Terminal session, return to this folder and activate `.venv` again.

### 2. Check the files and find the serial port

Check the kit's files before connecting:

```sh
# macOS
shasum -a 256 -c SHA256SUMS

# Debian / Fedora
sha256sum -c SHA256SUMS
```

Run only the command for your system. All four files should report OK. Checksums detect damage; they do not authenticate an untrusted download.

Connect the CYD and close any web flasher or serial monitor. Compare the port list before and after connecting:

```sh
# macOS
ls /dev/cu.*

# Debian / Fedora
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Use the newly appearing USB port, not a Bluetooth port. Replace **PORT** in every command below with that exact path, such as `/dev/cu.usbserial-1234` or `/dev/ttyUSB0`. These examples are not your device's guaranteed name.

On Debian/Fedora, if opening the port reports permission denied, inspect its group with `ls -l PORT`. When it is `dialout`, run:

```sh
sudo usermod -aG dialout "$USER"
```

Log out completely and back in, then reactivate the virtual environment. If the device uses another group, follow that distribution's serial-access policy. Do not solve this by making the port world-writable or running pip as root.

### 3. Back up the current firmware before changing it

```sh
python -m esptool --chip esp32 --port PORT flash_id
python -m esptool --chip esp32 --port PORT read_flash 0 0x400000 before-dnsp.bin
```

Confirm the detected flash is **4 MB** before continuing, and keep the successful 4,194,304-byte backup somewhere safe. Choose a new filename if `before-dnsp.bin` already exists. This is a full device-flash backup and may contain credentials and history: do not commit it or share it casually. It does not include the microSD contents.

### 4. Install DNSP

From the folder containing the four kit images:

```sh
python -m esptool --chip esp32 --port PORT write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Wait for successful verification and reset. **Do not erase all flash, use `--force`, or write `firmware.bin` at address zero.** These separate writes leave the NVS settings and reserved BlackBox history regions untouched on the expected layout; the full backup is your fallback if the installed firmware/layout differs. The command initializes the boot selection to the newly installed app0 image.

If connection stalls, check the cable, close other serial programs and use the board's BOOT/reset procedure to enter its loader. After boot, check touch, screen colors, Wi-Fi/Bluetooth reception and microSD operation. Display corruption or instability warrants testing the normal-speed ST7789 `cyd` build; the 80 MHz overclock is experimental.

For later SD-backup restoration, use [RECOVERY.md](RECOVERY.md). Complete the [hardware checklist](ENDURANCE-CHECKLIST.md) before treating this as gift-ready.

### Windows / Microsoft

**DNSP's editorial opinion:** you shouldn't be using Windows 11 or Microsoft's platform at all. Ditch it and try Debian or Fedora; we think the Microsoft ecosystem is icky. Yes, flashing from Windows is completely possible—this is a platform preference, not a technical limitation. This README documents Mac and Linux.

### Build from source instead

Install PlatformIO in a separate Python environment, open the repository root, and run:

```sh
pio run -e cyd-fast
pio run -e cyd-fast -t upload --upload-port PORT
```

The second command writes to the device. `cyd` is the normal-speed ST7789 alternative; `cyd-ili9341` is a different display. Use the supplied pinned dependencies and retain the partition layout.

### References

[Espressif flashing and backup commands](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html) · [Serial-port permissions](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-options.html) · [Debian virtual environments](https://packages.debian.org/search?keywords=python3-venv) · [Fedora Python environments](https://developer.fedoraproject.org/tech/languages/python/pypi-installation.html)
