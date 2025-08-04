# PS5 HD Camera Driver Files for PC Use

This repository provides the necessary files and instructions to use the Sony PlayStation 5 HD Camera (Model CFI-ZEY1) on a Windows PC.

---

# Updates

* **26/08/2024:**
    * If you experience dark/no video, try the **PS5 Camera Tweaker** by @PavlikBender: [PavlikBender/PS5CameraTweaker](https://github.com/PavlikBender/PS5CameraTweaker)
    * ![PS5 Camera Tweaker App](https://github.com/user-attachments/assets/5ad6bea1-aede-4d9b-b7d4-1d4cedd78785)
* **08/05/2024:**
    * Added new firmware (`firmware_fix.bin`) to address dark video output issues.
    * Thanks to [PavlikBender](https://github.com/PavlikBender) (report) and [Макаров Алексей](https://github.com/Lightbass) (firmware edit).
    * See original discussion: [psxdev/OrbisEyeCam Issue #10](https://github.com/psxdev/OrbisEyeCam/issues/10)
* **07/04/2022:**
    * Updated installer (`OV580 USB Camera updated.exe`) with `libusbk` v3.1.0 (upgraded from v3.0.7). Use this installer for new driver installations.

---

# Installation Instructions

**Prerequisites:** Download the required files from this repository.

1.  **Install Base Driver (if needed):** If you haven't installed the PS4/PS5 camera driver before, run `OV580 USB Camera updated.exe`. If you have an older version installed, it's recommended to uninstall it first (see Uninstallation Instructions below).
2.  **Connect Camera:** Plug in the PS5 HD Camera. Open **Device Manager** (right-click Start -> Device Manager). You should see `USB BOOT` listed under `Universal Serial Bus devices`.
3.  **Load Firmware:** Ensure `OrbisEyeCameraFirmwareLoader.exe` is in the same directory as the desired firmware file (e.g., `firmware.bin` or `firmware_fix.bin`). Run `OrbisEyeCameraFirmwareLoader.exe`.
4.  **Verify Installation:** Check **Device Manager** again. The camera should now appear under `Cameras` as `USB Camera-OV580`.

**Setup Complete.** The camera should now be usable in most webcam applications.

> **Important:** You must repeat Step 3 (running `OrbisEyeCameraFirmwareLoader.exe`) every time you disconnect and reconnect the camera to your PC, or after restarting your computer.

---

## Uninstallation Instructions (Windows 10/11)

If you need to remove the driver completely, follow these methods. Administrator privileges are typically required.

### Method 1: Via Apps & Features (Recommended First Step)

This works if the driver registered an uninstaller.

1.  **Open Settings:** Press `Win` + `I` (or right-click Start -> **Settings**).
2.  **Navigate to Apps:**
    * _Windows 10:_ Go to **Apps** -> **Apps & features**.
    * _Windows 11:_ Go to **Apps** -> **Installed apps**.
3.  **Search:** Use the search bar to look for terms like `PS4 Camera`, `PS5 Camera`, `OV580`, `USB Boot`, or the specific driver name.
4.  **Uninstall:** If found, click the item (or its `...` menu) and select **Uninstall**. Follow prompts.
5.  **Restart:** Reboot your PC to complete the removal.

> If the driver isn't listed here, proceed to Method 2.

### Method 2: Via Device Manager

This method removes the driver directly from the hardware level.

1.  **Open Device Manager:** Right-click the Start button -> **Device Manager**.
2.  **Locate Device:** Find the camera under `Cameras`, `Imaging devices`, `Sound, video and game controllers`, or `Universal Serial Bus devices` (it might be listed as `USB Camera-OV580`, `PS4/PS5 Camera`, or `USB BOOT` if firmware isn't loaded).
3.  **Uninstall:** Right-click the device -> **Uninstall device**.
4.  **Delete Driver Software:** **Crucially**, in the confirmation dialog, check the box:
    * `Delete the driver software for this device.` (Windows 10)
    * `Attempt to remove the driver software for this device.` (Windows 11)
    * Then click **Uninstall**.
5.  **Restart:** Reboot your PC.

### Method 3: Via Command Prompt (Advanced)

Use this command-line tool for forceful driver package removal.

1.  **Open Admin Command Prompt:** Search `cmd`, right-click **Command Prompt** -> **Run as administrator**.
2.  **List Drivers:** Execute `pnputil /enum-drivers`.
3.  **Identify Driver:** Carefully review the output. Look for entries related to the camera (check "Provider Name", "Original Name" for keywords like `OV580`, `Sony`, etc.). Note the `Published Name` (e.g., `oemXX.inf`).
4.  **Delete Driver:** Execute the following, replacing `oemXX.inf` with the actual name found:
    ```bash
    pnputil /delete-driver oemXX.inf /uninstall /force
    ```
    > **Warning:** Ensure you have the correct `oemXX.inf` file. Deleting the wrong driver can cause system instability.
5.  **Restart:** Reboot your PC.

---

# Acknowledgements & Sources

This work builds upon the efforts of the following projects and individuals:

* **OrbisEyeCam Project:** [psxdev/OrbisEyeCam](https://github.com/psxdev/OrbisEyeCam)
* **ProsperoDev HD Camera:** [prosperodev/hdcamera](https://github.com/prosperodev/hdcamera)
* **Initial Findings & Discussion:** [PSXHax Thread](https://www.psxhax.com/threads/ps5-hd-camera-firmware-files-dump-and-playstation-5-camera-on-pc.10117/)

---

# Credits

Special thanks to @psxdev and colleagues @frangar & @fjtrujy for their foundational work. Thanks also to @PavlikBender and @Lightbass for recent firmware contributions.
