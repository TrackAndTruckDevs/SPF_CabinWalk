# SPF CabinWalk Plugin

A plugin for American Truck Simulator and Euro Truck Simulator 2 that allows you to unchain the camera from the driver's seat and freely walk around your truck's cabin. Explore your interior with smooth, animated camera movements.

### See It In Action

[Watch a demonstration of the plugin on YouTube](https://youtu.be/668ubdWqsVw)

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

This is a standard CMake project. To build it from source:

1.  Clone this repository.
2.  Ensure you have **CMake** and a compatible C++ compiler with the **MSVC toolchain** (e.g., Visual Studio) installed.
3.  Create a `build` directory inside the project folder.
4.  Run CMake from the `build` directory to generate project files (e.g., `cmake ..`).
5.  Build the project using your chosen build tool (e.g., run `cmake --build .` or open the generated `.sln` file in Visual Studio and build from there).

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