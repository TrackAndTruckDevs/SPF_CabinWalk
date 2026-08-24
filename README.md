<p align="center">
    <a href=""><img src="https://github.com/user-attachments/assets/fa2b494e-508a-4892-b2a3-cd5ce8556e98" alt="Logo SPF CabinWalk" height="433px" /></a>
</p>

<h1 align="center">SPF CabinWalk Plugin</h1>

<p align="center">
    <a href="https://github.com/TrackAndTruckDevs/SPF_CabinWalk/releases/latest/" target="_blank" title="SPF CabinWalk Plugin"><img alt="GitHub Release" src="https://img.shields.io/github/v/release/TrackAndTruckDevs/SPF_CabinWalk"></a>
    <a href="/LICENSE" title="SPF CabinWalk Plugin license"><img alt="GitHub License" src="https://img.shields.io/github/license/TrackAndTruckDevs/SPF_CabinWalk"></a>
</p>

<p align="center">
    <a href="https://www.patreon.com/TrackAndTruckDevs" target="_blank" title="Support us on Patreon"><img alt="Patreon" src="https://img.shields.io/badge/patreon-Becoming a patron-3404021712?style=flat&logo=patreon"></a>
    <a href="https://github.com/TrackAndTruckDevs/SPF_CabinWalk/stargazers" title="Liked it? Starred"><img src="https://img.shields.io/github/stars/TrackAndTruckDevs/SPF_CabinWalk?style=flat&logo=github" alt="Stars" /></a>
    <a href="https://discord.gg/kadd8AQuMt" target="_blank" title="Join our Discord"><img alt="Discord" src="https://img.shields.io/badge/discord-join-7289da?style=flat&logo=discord&logoColor=white"></a>
    <a href="https://youtube.com/@TrackAndTruck" target="_blank" title="Subscribe to our channel"><img alt="Youtube" src="https://img.shields.io/badge/youtube-subscribe-orange?logo=youtube&style=flat"></a>
</p>

---

A plugin for American Truck Simulator and Euro Truck Simulator 2 that allows you to unchain the camera from the driver's seat and freely walk around your truck's cabin. Explore your interior with smooth, animated camera movements.

### See It In Action

[Watch a demonstration of the plugin on YouTube](https://youtu.be/668ubdWqsVw)

## ⚠️ Known Issues

**Windows SmartScreen / Smart App Control may block this plugin.**

When enabling the plugin in the SPF Framework, you may see the following error in the game log:

```
[PluginManager] -> Failed to temporarily load library for manifest extraction. Win32 Error: 126
[PluginManager] -> Failed to load library. Win32 Error: 126
```

As a result, the plugin **cannot be activated** in the SPF Framework — the toggle remains permanently off and the plugin simply does not load.

This is **not a bug in the plugin**. Windows Smart App Control (SAC) or Windows Defender SmartScreen incorrectly flags the DLL as untrusted and blocks it from loading.

> [!NOTE]
> The blocking behavior is unpredictable — on the same computer, the plugin may work in American Truck Simulator but be blocked in Euro Truck Simulator 2. How Windows determines which DLLs to trust is known only to Microsoft.

> [!TIP]
> The plugin contains no malicious code — the [VirusTotal scan](https://www.virustotal.com/gui/file/94c3f02f782152db120a07e3c868ee92bcb4679f7152105613043114bc2eb871) is clean (0 detections). The AI-based protection system produces false positives.

For more details, see: [Smart App Control — Microsoft Support](https://support.microsoft.com/windows/security/threat-malware-protection/smart-app-control-has-blocked-part-of-this-app)

### How to Fix

**Option 1 — Build from source**

Compile the plugin yourself using your own toolchain. Your compiler may produce a DLL that Smart App Control does not block. See the [How to Build](#how-to-build-) section below.

**Option 2 — Trust the certificate (Recommended)**

The plugin DLL is signed with a self-signed certificate. You can add it to Windows' trusted certificates:

1. Right-click `SPF_CabinWalk.dll` → **Properties** (or press `Alt + Enter`).
2. Go to the **Digital Signatures** tab.
3. Select the `Track'n'Truck Devs` signature → click **Details**.
4. Click **View Certificate** → **Install Certificate**.
5. Choose **Local Machine** (requires admin) or **Current User** → click **Next**.
6. Select **Place all certificates in the following store** → click **Browse** → choose **Trusted Root Certification Authorities** → click **OK**.
7. Click **Next** → **Finish**.

**Option 3 — Disable Smart App Control**

> [!WARNING]
> This reduces your system's security. Only do this if you understand the risks.

1. Open **Windows Security** → **App & browser control**.
2. Click on **Smart App Control settings** (or **Reputation-based protection settings**).
3. Toggle Smart App Control to **Off**.

If you cannot find the setting, you can also disable it via Group Policy or registry — search for "disable Smart App Control" for your Windows version.

---

## Features
*   **Free Camera Movement**: Move the camera between the driver's seat, passenger seat, a standing position, and multiple spots on the sleeper sofa.
*   **Smooth Animations**: Enjoy configurable, fluid transitions between all camera positions.
*   **Interactive Walking Mode**: When in the standing position, you can:
    *   Walk forwards and backwards within a defined area of the cabin.
    *   Experience natural camera bobbing that simulates head movement.
    *   Crouch down or stand on your tiptoes to get a better look at your surroundings.
*   **Safety First**: A safety feature prevents you from leaving the driver's seat unless the truck is stationary and the parking brake is engaged.
*   **Live Configuration**: Interactively adjust camera positions, animation speeds, walking parameters, and more in real-time to perfectly suit any truck.
*   **Customizable Keybinds**: Configure all actions through the SPF Framework menu, with support for "Toggle" and "Hold" behaviors.
*   **Full Localization**: All text is translatable, with support for multiple languages.

## Support the Project

If you enjoy this plugin and want to support the development of future projects, consider supporting us on Patreon.

► **[Support on Patreon](https://www.patreon.com/TrackAndTruckDevs)**

## How to Build 🛠️

This project uses **CMake presets** for configuration and building.

### Prerequisites

- **CMake** 4.4 or newer
- A compatible C++20 compiler:
  - **Windows**: MSVC (Visual Studio 2022) or MinGW-w64
  - **Linux**: MinGW-w64 (cross-compile)

### Steps

1.  Clone this repository.
2.  Copy the user presets template:

    ```bash
    cp CMakeUserPresets.json.example CMakeUserPresets.json
    ```

3.  Edit `CMakeUserPresets.json` and set your game install paths:

    ```json
    {
      "environment": {
        "ATS_PLUGINS_DIR": "F:/SteamLibrary/steamapps/common/American Truck Simulator/bin/win_x64/plugins",
        "ETS2_PLUGINS_DIR": "F:/SteamLibrary/steamapps/common/Euro Truck Simulator 2/bin/win_x64/plugins"
      }
    }
    ```

4.  Configure and build using a preset that matches your toolchain:

    ```bash
    # Windows — Visual Studio 2022
    cmake --preset user-win-release
    cmake --build --preset user-win-release

    # Windows — Ninja (MSVC)
    cmake --preset user-ninja-release
    cmake --build --preset user-ninja-release

    # Linux — MinGW cross-compile (Ninja)
    cmake --preset user-mingw-release
    cmake --build --preset user-mingw-release
    ```

    Run `cmake --list-presets` to see all available presets (Debug, RelWithDebInfo, MinSizeRel, etc.).

## Installation

### Prerequisites

You must have the **SPF Framework** installed for this plugin to work.
*   **[Download the SPF-Framework here](https://github.com/TrackAndTruckDevs/SPF-Framework)**

### Steps

1.  If you haven't already, download and install the SPF Framework according to its instructions.
2.  Download the latest release of this plugin from the **[Releases](https://github.com/TrackAndTruckDevs/SPF_CabinWalk/releases)** page.
3.  You will have a folder named `SPF_CabinWalk`. Copy this entire folder into your game's `\bin\win_x64\plugins\spfPlugins\` directory.

The final folder structure should look like this:

```
...your_game_root\bin\win_x64\plugins\spfPlugins\
└───SPF_CabinWalk
    │   SPF_CabinWalk.dll
    │
    └───localization
            en.json
            ... (other languages)
```

## How to Use

1.  Start the game.
2.  Press the `DELETE` key to open the main SPF Framework window.
3.  In the plugin list, find **SPF_CabinWalk** and enable it.

### Default Keybinds:

*   **Move to Passenger Seat**: `NUMPAD 3`
*   **Move to Driver Seat**: `NUMPAD 5`
*   **Move to Standing Position / Walk**: `NUMPAD 2` (Tap to move, Hold to walk when standing)
*   **Cycle Sofa Positions**: `NUMPAD 1`

You can change these keybinds at any time in the "Key Binds" tab of the SPF menu.

### Configuration:

To adjust the plugin's behavior, go to the "Plugin Settings" tab, select **SPF_CabinWalk**, and explore the available options. You can configure:
*   **Positions**: Fine-tune the X, Y, Z coordinates and rotation for the passenger seat, standing position, and all sofa spots.
*   **Animation Durations**: Adjust the speed of every transition animation.
*   **Standing Movement**: Modify walking speed, camera bob amount, crouch depth, tiptoe height, and the angles required to trigger them.
All changes are applied instantly for a live preview.
