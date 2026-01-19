/**
 * @file SPF_CabinWalk.cpp
 * @brief The main implementation file for the SPF_CabinWalk.
 * @details This file contains the minimal implementation for a plugin to be loaded
 * and recognized by the SPF framework. It serves as a basic template for new plugins,
 * with clear explanations and commented-out sections for optional features.
 */

#include "SPF_CabinWalk.hpp"                    // Always include your own header first
#include "Hooks/Offsets.hpp"                    // For memory offsets and signatures
#include "Hooks/CameraHookManager.hpp"          // For camera hooking logic
#include "Animation/AnimationController.hpp"    // For managing camera animations
#include "Animation/StandingAnimController.hpp" // For handling walking logic

#include <cmath>   // For math functions like fabsf
#include <cstring> // For C-style string manipulation functions like strncpy_s.

namespace SPF_CabinWalk
{
    // Forward Declarations
    bool IsSafeToLeaveDriverSeat();

    // =================================================================================================
    // 1. Constants & Global State
    // =================================================================================================

    /**
     * @brief A constant for the plugin's name.
     * @details This MUST match the name used in `GetContext` calls for various APIs
     * and the plugin's directory name.
     */
    const char *PLUGIN_NAME = "SPF_CabinWalk";

    /**
     * @brief The single, global instance of the plugin's context.
     * @details This is the central point for accessing all plugin state.
     */
    PluginContext g_ctx;

    /**
     * @brief Tracks the 'hold' state of the walk key.
     */
    static bool g_is_walk_key_down = false;

    // =================================================================================================
    // 2. Manifest Implementation
    // =================================================================================================

    void GetManifestData(SPF_ManifestData_C &out_manifest)
    {
        // This function defines all the metadata for your plugin. The framework calls this
        // function *before* loading your plugin DLL to understand what it is.

        // --- 2.1. Plugin Information (SPF_InfoData_C) ---
        // This section provides the basic identity of your plugin.
        {
            auto &info = out_manifest.info;
            strncpy_s(info.name, PLUGIN_NAME, sizeof(info.name));
            strncpy_s(info.version, "1.0.0", sizeof(info.version));
            strncpy_s(info.author, "Track'n'Truck Devs", sizeof(info.author));
            strncpy_s(info.descriptionLiteral, "A plugin for American Truck Simulator and Euro Truck Simulator 2 that allows you to unchain the camera from the driver's seat and freely walk around your truck's cabin. Explore your interior with smooth, animated camera movements.", sizeof(info.descriptionKey));

            strncpy_s(info.email, "mailto:spf.framework@gmail.com", sizeof(info.email));
            strncpy_s(info.youtubeUrl, "https://www.youtube.com/@TrackAndTruck", sizeof(info.youtubeUrl));
            strncpy_s(info.patreonUrl, "https://www.patreon.com/TrackAndTruckDevs", sizeof(info.patreonUrl));
        }

        // --- 2.2. Configuration Policy (SPF_ConfigPolicyData_C) ---
        // This section defines how your plugin interacts with the framework's configuration system.
        {
            auto &policy = out_manifest.configPolicy;

            // `allowUserConfig`: Set to `true` if you want a `settings.json` file to be created
            // for your plugin, allowing users (or the framework UI) to override default settings.
            policy.allowUserConfig = true;

            // `userConfigurableSystemsCount`: The number of framework systems (e.g., "settings", "logging", "localization", "ui")
            // that should have a configuration section generated in the settings UI for your plugin.
            // IMPORTANT: Always initialize this to 0 if you are not listing any systems to avoid errors.
            policy.userConfigurableSystemsCount = 2; // To enable configurable systems, uncomment the block below and set the count accordingly
            // strncpy_s(policy.userConfigurableSystems[0], "logging", sizeof(policy.userConfigurableSystems[0]));
            strncpy_s(policy.userConfigurableSystems[0], "settings", sizeof(policy.userConfigurableSystems[0]));
            strncpy_s(policy.userConfigurableSystems[1], "localization", sizeof(policy.userConfigurableSystems[1]));
            // strncpy_s(policy.userConfigurableSystems[2], "ui", sizeof(policy.userConfigurableSystems[2]));

            // `requiredHooksCount`: List any game hooks your plugin absolutely requires to function.
            // The framework will ensure these hooks are enabled whenever your plugin is active,
            // regardless of user settings.
            // IMPORTANT: Always initialize this to 0 if you are not listing any hooks to avoid errors.
            policy.requiredHooksCount = 0; // To enable required hooks, uncomment the lines below and set the count accordingly.
            // strncpy_s(policy.requiredHooks[0], "GameConsole", sizeof(policy.requiredHooks[0])); // Example: Requires GameConsole hook
        }

        // --- 2.3. Custom Settings (settingsJson) ---
        // A JSON string literal that defines the default values for your plugin's custom settings.
        // If `policy.allowUserConfig` is true, the framework creates a `settings.json` file.
        // The JSON object you provide here will be inserted under a top-level key named "settings".
        out_manifest.settingsJson = R"json(
            {
                "general": {
                    "warning_duration_ms": 3000,
                    "cabin_layout": 0,
                    "height": 0.25
                },
                "positions": {
                    "passenger_seat": { "enabled": true, "position": { "x": 0.95, "y": 0.0, "z": -0.03 }, "rotation": { "x": 0.03, "y": 0.03 } },
                    "standing":       { "enabled": true, "position": { "x": 0.5, "y": 0.2, "z": 0.25 }, "rotation": { "x": -0.17, "y": -0.3 } },
                    "sofa_sit1":      { "enabled": true, "position": { "x": 0.5, "y": 0.0, "z": 0.8 }, "rotation": { "x": 0.0, "y": 0.0 } },
                    "sofa_lie":       { "enabled": true, "position": { "x": -0.15, "y": -0.25, "z": 1.25 }, "rotation": { "x": -1.65, "y": 0.35 } },
                    "sofa_sit2":      { "enabled": true, "position": { "x": 0.65, "y": 0.0, "z": 1.0 }, "rotation": { "x": -1.0, "y": -0.10 } }
                },
                "animation_durations": {
                    "main_animation_speed": {
                        "driver_to_passenger": 4000,
                        "passenger_to_driver": 3000,
                        "driver_to_standing": 3600,
                        "standing_to_driver": 4300,
                        "passenger_to_standing": 3300,
                        "standing_to_passenger": 4500,
                        "standing_to_sofa": 2900,
                        "sofa_to_standing": 1700
                    },
                    "sofa_animation_speed": {
                        "sofa_sit1_to_lie": 4000,
                        "sofa_lie_to_sit2": 2500,
                        "sofa_sit2_to_sit1": 1200,
                        "sofa_lie_to_sit1_shortcut": 1700
                    },
                    "crouch_and_stand_animation_speed": {
                        "crouch": 1250,
                        "tiptoe": 1100
                    }                    
                },
                "walking_animation_speed": {
                        "walk_step": 450,
                        "walk_first_step_base": 250000,
                        "walk_first_step_turn_extra": 1000000
                },
                "standing_movement": {
                    "walking": {
                        "step_amount": 0.35,
                        "bob_amount": 0.02,
                        "walk_zone_z": {
                            "min": -0.55,
                            "max": 0.65
                        }
                    },
                    "stance_control": {
                        "hold_time_ms": 1000,
                        "crouch": {
                            "depth": 0.5,
                            "activation_angle": -0.7,
                            "deactivation_angle": 0.3
                        },
                        "tiptoe": {
                            "height": 0.17,
                            "activation_angle": 0.5,
                            "deactivation_angle": -0.3
                        }
                    }
                },
                "sofa_limits": {
                    "yaw_left": 180.0,
                    "yaw_right": -180.0,
                    "pitch_up": 90.0,
                    "pitch_down": -65.0
                }
            }
        )json";

        // --- 2.4. Default Settings for Framework Systems ---
        // Here you can provide default configurations for various framework systems.

        // --- Logging ---
        // Requires: SPF_Logger_API.h
        {
            auto &logging = out_manifest.logging;
            // `level`: Default minimum log level for this plugin (e.g., "trace", "debug", "info", "warn", "error", "critical").
            strncpy_s(logging.level, "debug", sizeof(logging.level));
            // `sinks.file`: If true, logs from this plugin will be written to a dedicated file
            // (e.g., `SPF_CabinWalk/logs/SPF_CabinWalk.log`) in addition to the main framework log.
            logging.sinks.file = false;
        }

        // --- Localization ---
        // Requires: SPF_Localization_API.h
        // Uncomment if your plugin uses localized strings.
        {
            auto &localization = out_manifest.localization;
            // `language`: Default language code (e.g., "en", "de", "uk").
            strncpy_s(localization.language, "en", sizeof(localization.language));
        }

        auto &keybinds = out_manifest.keybinds;
        keybinds.actionCount = 4; // Incremented to 4
        {
            // --- Action 0: Move to Passenger seat ---
            auto &action = keybinds.actions[0];
            strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
            strncpy_s(action.actionName, "moveToPassengerSeat", sizeof(action.actionName));

            // Define one or more default key combinations for this action.
            action.definitionCount = 1;
            {
                auto &def = action.definitions[0];
                strncpy_s(def.type, "keyboard", sizeof(def.type));
                strncpy_s(def.key, "KEY_NUMPAD3", sizeof(def.key));
                strncpy_s(def.pressType, "short", sizeof(def.pressType));
                strncpy_s(def.consume, "always", sizeof(def.consume));
                strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
            }
        }
        {
            // --- Action 1: Move to driver seat ---
            auto &action = keybinds.actions[1];
            strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
            strncpy_s(action.actionName, "moveToDriverSeat", sizeof(action.actionName));

            action.definitionCount = 1;
            {
                auto &def = action.definitions[0];
                strncpy_s(def.type, "keyboard", sizeof(def.type));
                strncpy_s(def.key, "KEY_NUMPAD5", sizeof(def.key));
                strncpy_s(def.pressType, "short", sizeof(def.pressType));
                strncpy_s(def.consume, "always", sizeof(def.consume));
                strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
            }
        }
        {
            // --- Action 2: Move to standing position ---
            auto &action = keybinds.actions[2];
            strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
            strncpy_s(action.actionName, "moveToStandingPosition", sizeof(action.actionName));

            action.definitionCount = 1;
            {
                auto &def = action.definitions[0];
                strncpy_s(def.type, "keyboard", sizeof(def.type));
                strncpy_s(def.key, "KEY_NUMPAD2", sizeof(def.key));
                strncpy_s(def.pressType, "short", sizeof(def.pressType));
                strncpy_s(def.consume, "always", sizeof(def.consume));
                strncpy_s(def.behavior, "hold", sizeof(def.behavior));
            }
        }
        {
            // --- Action 3: Cycle Sofa Positions ---
            auto &action = keybinds.actions[3];
            strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
            strncpy_s(action.actionName, "cycleSofaPositions", sizeof(action.actionName));

            action.definitionCount = 1;
            {
                auto &def = action.definitions[0];
                strncpy_s(def.type, "keyboard", sizeof(def.type));
                strncpy_s(def.key, "KEY_NUMPAD1", sizeof(def.key));
                strncpy_s(def.pressType, "short", sizeof(def.pressType));
                strncpy_s(def.consume, "always", sizeof(def.consume));
                strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
            }
        }

        auto &ui = out_manifest.ui;
        ui.windowsCount = 1; // Number of UI windows defined by your plugin.
        {
            // --- Window 0: The main window for the plugin ---
            auto &window = ui.windows[0];
            // `name`: Unique ID for this window within the plugin.
            strncpy_s(window.name, "WarningWindow", sizeof(window.name));
            // `isVisible`: Default visibility state.
            window.isVisible = false;
            // `isInteractive`: If false, mouse clicks pass through the window to the game.
            window.isInteractive = false;
            // Default position and size on screen.
            window.posX = 0;
            window.posY = 0;
            window.sizeW = 400;
            window.sizeH = 100;
            // `isCollapsed`: Default collapsed state.
            window.isCollapsed = false;
            // `autoScroll`: If the window should auto-scroll to the bottom on new content.
            window.autoScroll = false;
        }

        // =============================================================================================
        // 2.5. Metadata for UI Display (Optional)
        // =============================================================================================
        // These sections are used to provide human-readable names and descriptions for your
        // settings, keybinds, and UI windows in the framework's settings panel.
        // If you don't provide metadata for an item, the framework will use its raw key as a label.
        //==============================================================================================

        out_manifest.keybindsMetadataCount = 4; // Incremented to 4
        {
            auto &meta = out_manifest.keybindsMetadata[0];
            strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
            strncpy_s(meta.actionName, "moveToPassengerSeat", sizeof(meta.actionName));
            strncpy_s(meta.titleKey, "keybinds.moveToPassengerSeat.title", sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, "keybinds.moveToPassengerSeat.desc", sizeof(meta.descriptionKey));
        }
        {
            auto &meta = out_manifest.keybindsMetadata[1];
            strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
            strncpy_s(meta.actionName, "moveToDriverSeat", sizeof(meta.actionName));
            strncpy_s(meta.titleKey, "keybinds.moveToDriverSeat.title", sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, "keybinds.moveToDriverSeat.desc", sizeof(meta.descriptionKey));
        }
        {
            auto &meta = out_manifest.keybindsMetadata[2];
            strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
            strncpy_s(meta.actionName, "moveToStandingPosition", sizeof(meta.actionName));
            strncpy_s(meta.titleKey, "keybinds.moveToStandingPosition.title", sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, "keybinds.moveToStandingPosition.desc", sizeof(meta.descriptionKey));
        }
        {
            auto &meta = out_manifest.keybindsMetadata[3];
            strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
            strncpy_s(meta.actionName, "cycleSofaPositions", sizeof(meta.actionName));
            strncpy_s(meta.titleKey, "keybinds.cycleSofaPositions.title", sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, "keybinds.cycleSofaPositions.desc", sizeof(meta.descriptionKey));
        }

        // --- Custom Settings Metadata ---
        out_manifest.customSettingsMetadataCount = 0;
        auto &settings_meta = out_manifest.customSettingsMetadata;

        auto add_meta = [&](const char *key, const char *title, const char *desc)
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, key, sizeof(meta.keyPath));
            strncpy_s(meta.titleKey, title, sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, desc, sizeof(meta.descriptionKey));
        };

        auto AddSliderMeta = [&](const char *key, const char *title, const char *desc, float min, float max, const char *format)
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, key, sizeof(meta.keyPath));
            strncpy_s(meta.titleKey, title, sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, desc, sizeof(meta.descriptionKey));
            strncpy_s(meta.widget, "slider", sizeof(meta.widget));
            meta.widget_params.slider.min_val = min;
            meta.widget_params.slider.max_val = max;
            strncpy_s(meta.widget_params.slider.format, format, sizeof(meta.widget_params.slider.format));
        };

        auto AddDragMeta = [&](const char *key, const char *title, const char *desc, float speed, float min, float max, const char *format)
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, key, sizeof(meta.keyPath));
            strncpy_s(meta.titleKey, title, sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, desc, sizeof(meta.descriptionKey));
            strncpy_s(meta.widget, "drag", sizeof(meta.widget));
            meta.widget_params.drag.speed = speed;
            meta.widget_params.drag.min_val = min;
            meta.widget_params.drag.max_val = max;
            strncpy_s(meta.widget_params.drag.format, format, sizeof(meta.widget_params.drag.format));
        };

        auto AddRadioMeta = [&](const char *key, const char *title, const char *desc, const char *options_json)
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, key, sizeof(meta.keyPath));
            strncpy_s(meta.titleKey, title, sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, desc, sizeof(meta.descriptionKey));
            strncpy_s(meta.widget, "radio", sizeof(meta.widget));
            strncpy_s(meta.widget_params.choice.options_json, options_json, sizeof(meta.widget_params.choice.options_json));
        };

        auto AddCheckboxMeta = [&](const char *key, const char *title, const char *desc)
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, key, sizeof(meta.keyPath));
            strncpy_s(meta.titleKey, title, sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, desc, sizeof(meta.descriptionKey));
        };

        // Specialized helpers for positions
        auto AddPositionCoordMeta = [&](const char *base_key_path, const char *axis_name)
        {
            char key_path_buffer[256];
            char title_key_buffer[256];
            sprintf_s(key_path_buffer, sizeof(key_path_buffer), "positions.%s.position.%s", base_key_path, axis_name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.positions.%s.position.%s.title", base_key_path, axis_name);
            AddDragMeta(key_path_buffer, title_key_buffer, "settings.positions.coord.desc", 0.01f, -5.0f, 5.0f, "%.2f m");
        };

        auto AddPositionRotationMeta = [&](const char *base_key_path, const char *axis_name)
        {
            char key_path_buffer[256];
            char title_key_buffer[256];
            sprintf_s(key_path_buffer, sizeof(key_path_buffer), "positions.%s.rotation.%s", base_key_path, axis_name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.positions.%s.rotation.%s.title", base_key_path, axis_name);
            AddDragMeta(key_path_buffer, title_key_buffer, "settings.positions.rot.desc", 0.01f, -3.14159f, 3.14159f, "%.2f rad");
        };

        add_meta("general", "settings.general.title", "");
        add_meta("positions", "settings.positions.title", "");
        add_meta("animation_durations", "settings.animation_durations.title", "");
        add_meta("standing_movement", "settings.standing_movement.title", "");
        add_meta("standing_movement.walking", "settings.standing_movement.walking.title", "");
        add_meta("standing_movement.walking.walk_zone_z", "settings.standing_movement.walking.walk_zone_z.title", "");
        add_meta("standing_movement.stance_control", "settings.standing_movement.stance_control.title", "");
        add_meta("standing_movement.stance_control.crouch", "settings.standing_movement.stance_control.crouch.title", "");
        add_meta("standing_movement.stance_control.tiptoe", "settings.standing_movement.stance_control.tiptoe.title", "");

        AddSliderMeta("general.warning_duration_ms", "settings.general.warning_duration_ms.title", "settings.general.warning_duration_ms.desc", 0.0f, 30000.0f, "%d ms");
        AddSliderMeta("general.height", "settings.general.height.title", "settings.general.height.desc", 0.0f, 1.0f, "%.2f m");

        const char *cabin_layout_options = R"json([
            { "value": 0, "labelKey": "settings.general.cabin_layout.lhd" },
            { "value": 1, "labelKey": "settings.general.cabin_layout.rhd" }
        ])json";
        AddRadioMeta("general.cabin_layout", "settings.general.cabin_layout.title", "settings.general.cabin_layout.desc", cabin_layout_options);

        // --- Positions ---
        const char *pos_names[] = {"passenger_seat", "standing", "sofa_sit1", "sofa_lie", "sofa_sit2"};
        for (const char *name : pos_names)
        {
            char key_buffer[256];
            char title_key_buffer[256];

            // Add title for the position group itself
            sprintf_s(key_buffer, sizeof(key_buffer), "positions.%s", name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.positions.%s.title", name);
            add_meta(key_buffer, title_key_buffer, "");

            sprintf_s(key_buffer, sizeof(key_buffer), "positions.%s.enabled", name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.positions.%s.enabled.title", name);
            AddCheckboxMeta(key_buffer, title_key_buffer, "settings.positions.enabled.desc");

            // Add titles for the subgroups
            char group_key[256], group_title[256];
            sprintf_s(group_key, sizeof(group_key), "positions.%s.position", name);
            sprintf_s(group_title, sizeof(group_title), "settings.positions.position_group.title");
            add_meta(group_key, group_title, "");

            AddPositionCoordMeta(name, "x");
            AddPositionCoordMeta(name, "y");
            AddPositionCoordMeta(name, "z");

            sprintf_s(group_key, sizeof(group_key), "positions.%s.rotation", name);
            sprintf_s(group_title, sizeof(group_title), "settings.positions.rotation_group.title");
            add_meta(group_key, group_title, "");

            AddPositionRotationMeta(name, "x");
            AddPositionRotationMeta(name, "y");
        }

        add_meta("animation_durations.main_animation_speed", "settings.animation_durations.main_animation_speed.title", "");
        add_meta("animation_durations.sofa_animation_speed", "settings.animation_durations.sofa_animation_speed.title", "");
        add_meta("animation_durations.crouch_and_stand_animation_speed", "settings.animation_durations.crouch_and_stand_animation_speed.title", "");
        // add_meta("animation_durations.walking_animation_speed", "settings.animation_durations.walking_animation_speed.title", "");

        // --- Animation Durations ---
        const char *main_anim_names[] = {
            "driver_to_passenger", "passenger_to_driver", "driver_to_standing", "standing_to_driver",
            "passenger_to_standing", "standing_to_passenger", "standing_to_sofa", "sofa_to_standing"};
        for (const char *name : main_anim_names)
        {
            char key_buffer[256];
            char title_key_buffer[256];
            char desc_key_buffer[256];
            sprintf_s(key_buffer, sizeof(key_buffer), "animation_durations.main_animation_speed.%s", name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.animation_durations.main_animation_speed.%s.title", name);
            sprintf_s(desc_key_buffer, sizeof(desc_key_buffer), "settings.animation_durations.main_animation_speed.%s.desc", name);
            AddSliderMeta(key_buffer, title_key_buffer, desc_key_buffer, 100.0f, 10000.0f, "%d ms");
        }

        const char *sofa_anim_names[] = {"sofa_sit1_to_lie", "sofa_lie_to_sit2", "sofa_sit2_to_sit1", "sofa_lie_to_sit1_shortcut"};
        for (const char *name : sofa_anim_names)
        {
            char key_buffer[256];
            char title_key_buffer[256];
            char desc_key_buffer[256];
            sprintf_s(key_buffer, sizeof(key_buffer), "animation_durations.sofa_animation_speed.%s", name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.animation_durations.sofa_animation_speed.%s.title", name);
            sprintf_s(desc_key_buffer, sizeof(desc_key_buffer), "settings.animation_durations.sofa_animation_speed.%s.desc", name);
            AddSliderMeta(key_buffer, title_key_buffer, desc_key_buffer, 100.0f, 10000.0f, "%d ms");
        }

        const char *crouch_anim_names[] = {"crouch", "tiptoe"};
        for (const char *name : crouch_anim_names)
        {
            char key_buffer[256];
            char title_key_buffer[256];
            char desc_key_buffer[256];
            sprintf_s(key_buffer, sizeof(key_buffer), "animation_durations.crouch_and_stand_animation_speed.%s", name);
            sprintf_s(title_key_buffer, sizeof(title_key_buffer), "settings.animation_durations.crouch_and_stand_animation_speed.%s.title", name);
            sprintf_s(desc_key_buffer, sizeof(desc_key_buffer), "settings.animation_durations.crouch_and_stand_animation_speed.%s.desc", name);
            AddSliderMeta(key_buffer, title_key_buffer, desc_key_buffer, 100.0f, 10000.0f, "%d ms");
        }

        // --- Standing Movement ---
        // Walking
        AddSliderMeta("standing_movement.walking.step_amount", "settings.standing_movement.walking.step_amount.title", "settings.standing_movement.walking.step_amount.desc", 0.01f, 1.0f, "%.2f m");
        AddSliderMeta("standing_movement.walking.bob_amount", "settings.standing_movement.walking.bob_amount.title", "settings.standing_movement.walking.bob_amount.desc", 0.0f, 0.2f, "%.3f m");
        AddSliderMeta("standing_movement.walking.walk_zone_z.min", "settings.standing_movement.walking.walk_zone_z.min.title", "settings.standing_movement.walking.walk_zone_z.desc", -2.0f, 2.0f, "%.2f m");
        AddSliderMeta("standing_movement.walking.walk_zone_z.max", "settings.standing_movement.walking.walk_zone_z.max.title", "settings.standing_movement.walking.walk_zone_z.desc", -2.0f, 2.0f, "%.2f m");

        // Stance Control
        AddSliderMeta("standing_movement.stance_control.hold_time_ms", "settings.standing_movement.stance_control.hold_time_ms.title", "settings.standing_movement.stance_control.hold_time_ms.desc", 100.0f, 5000.0f, "%d ms");

        // Crouch
        AddSliderMeta("standing_movement.stance_control.crouch.depth", "settings.standing_movement.stance_control.crouch.depth.title", "settings.standing_movement.stance_control.crouch.depth.desc", 0.1f, 1.0f, "%.2f m");
        AddSliderMeta("standing_movement.stance_control.crouch.activation_angle", "settings.standing_movement.stance_control.crouch.activation_angle.title", "settings.standing_movement.stance_control.activation_angle.desc", -1.57f, 0.0f, "%.2f rad");
        AddSliderMeta("standing_movement.stance_control.crouch.deactivation_angle", "settings.standing_movement.stance_control.crouch.deactivation_angle.title", "settings.standing_movement.stance_control.deactivation_angle.desc", 0.0f, 1.57f, "%.2f rad");

        // Tiptoe
        AddSliderMeta("standing_movement.stance_control.tiptoe.height", "settings.standing_movement.stance_control.tiptoe.height.title", "settings.standing_movement.stance_control.tiptoe.height.desc", 0.05f, 0.5f, "%.2f m");
        AddSliderMeta("standing_movement.stance_control.tiptoe.activation_angle", "settings.standing_movement.stance_control.tiptoe.activation_angle.title", "settings.standing_movement.stance_control.activation_angle.desc", 0.0f, 1.57f, "%.2f rad");
        AddSliderMeta("standing_movement.stance_control.tiptoe.deactivation_angle", "settings.standing_movement.stance_control.tiptoe.deactivation_angle.title", "settings.standing_movement.stance_control.deactivation_angle.desc", -1.57f, 0.0f, "%.2f rad");

        // Sofa Limits
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, "sofa_limits", sizeof(meta.keyPath));
            meta.hide_in_ui = true;
        }
        // walking animation speed
        {
            auto &meta = settings_meta[out_manifest.customSettingsMetadataCount++];
            strncpy_s(meta.keyPath, "walking_animation_speed", sizeof(meta.keyPath));
            meta.hide_in_ui = true;
        }
    }

    // =================================================================================================
    // 3. Plugin Lifecycle Implementations
    // =================================================================================================
    // The following functions are the core lifecycle events for the plugin.

    void LoadSettings(const SPF_Config_API *configAPI, SPF_Config_Handle *configHandle)
    {
        if (!configAPI || !configHandle)
        {
            if (g_ctx.loggerHandle)
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[LoadSettings] Aborted due to NULL API handles.");
            return;
        }

        // A helper for logging
        auto log = [&](const char *msg)
        {
            if (g_ctx.loggerHandle)
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, msg);
        };
        log("[LoadSettings] Starting to load settings...");

        // Helper lambdas to simplify getting values
        auto get_int = [&](const char *key, int32_t def)
        {
            return configAPI->GetInt32(configHandle, key, def);
        };
        auto get_float = [&](const char *key, float def)
        {
            return (float)configAPI->GetFloat(configHandle, key, (double)def);
        };
        auto get_bool = [&](const char *key, bool def)
        {
            return configAPI->GetBool(configHandle, key, def);
        };

        // General
        g_ctx.settings.general.warning_duration_ms = get_int("settings.general.warning_duration_ms", 5000);
        g_ctx.settings.general.cabin_layout = get_int("settings.general.cabin_layout", 0);
        g_ctx.settings.general.height = get_float("settings.general.height", 0.25f);

        // Animation Durations
        g_ctx.settings.animation_durations.main_animation_speed.driver_to_passenger = get_int("settings.animation_durations.main_animation_speed.driver_to_passenger", 3000);
        g_ctx.settings.animation_durations.main_animation_speed.passenger_to_driver = get_int("settings.animation_durations.main_animation_speed.passenger_to_driver", 3000);
        g_ctx.settings.animation_durations.main_animation_speed.driver_to_standing = get_int("settings.animation_durations.main_animation_speed.driver_to_standing", 2500);
        g_ctx.settings.animation_durations.main_animation_speed.standing_to_driver = get_int("settings.animation_durations.main_animation_speed.standing_to_driver", 2000);
        g_ctx.settings.animation_durations.main_animation_speed.passenger_to_standing = get_int("settings.animation_durations.main_animation_speed.passenger_to_standing", 2500);
        g_ctx.settings.animation_durations.main_animation_speed.standing_to_passenger = get_int("settings.animation_durations.main_animation_speed.standing_to_passenger", 2000);
        g_ctx.settings.animation_durations.main_animation_speed.standing_to_sofa = get_int("settings.animation_durations.main_animation_speed.standing_to_sofa", 1500);
        g_ctx.settings.animation_durations.main_animation_speed.sofa_to_standing = get_int("settings.animation_durations.main_animation_speed.sofa_to_standing", 1800);

        g_ctx.settings.animation_durations.sofa_animation_speed.sofa_sit1_to_lie = get_int("settings.animation_durations.sofa_animation_speed.sofa_sit1_to_lie", 5000);
        g_ctx.settings.animation_durations.sofa_animation_speed.sofa_lie_to_sit2 = get_int("settings.animation_durations.sofa_animation_speed.sofa_lie_to_sit2", 2500);
        g_ctx.settings.animation_durations.sofa_animation_speed.sofa_sit2_to_sit1 = get_int("settings.animation_durations.sofa_animation_speed.sofa_sit2_to_sit1", 1200);
        g_ctx.settings.animation_durations.sofa_animation_speed.sofa_lie_to_sit1_shortcut = get_int("settings.animation_durations.sofa_animation_speed.sofa_lie_to_sit1_shortcut", 2800);

        g_ctx.settings.animation_durations.crouch_and_stand_animation_speed.crouch = get_int("settings.animation_durations.crouch_and_stand_animation_speed.crouch", 1250);
        g_ctx.settings.animation_durations.crouch_and_stand_animation_speed.tiptoe = get_int("settings.animation_durations.crouch_and_stand_animation_speed.tiptoe", 1100);

        g_ctx.settings.walking_animation_speed.walk_step = get_int("settings.walking_animation_speed.walk_step", 450);
        g_ctx.settings.walking_animation_speed.walk_first_step_base = get_int("settings.walking_animation_speed.walk_first_step_base", 250);
        g_ctx.settings.walking_animation_speed.walk_first_step_turn_extra = get_int("settings.walking_animation_speed.walk_first_step_turn_extra", 1000);

        // Standing Movement
        g_ctx.settings.standing_movement.walking.step_amount = get_float("settings.standing_movement.walking.step_amount", 0.35f);
        g_ctx.settings.standing_movement.walking.bob_amount = get_float("settings.standing_movement.walking.bob_amount", 0.02f);
        g_ctx.settings.standing_movement.walking.walk_zone_z.min = get_float("settings.standing_movement.walking.walk_zone_z.min", -0.55f);
        g_ctx.settings.standing_movement.walking.walk_zone_z.max = get_float("settings.standing_movement.walking.walk_zone_z.max", 0.65f);

        g_ctx.settings.standing_movement.stance_control.hold_time_ms = get_int("settings.standing_movement.stance_control.hold_time_ms", 1000);

        g_ctx.settings.standing_movement.stance_control.crouch.depth = get_float("settings.standing_movement.stance_control.crouch.depth", 0.5f);
        g_ctx.settings.standing_movement.stance_control.crouch.activation_angle = get_float("settings.standing_movement.stance_control.crouch.activation_angle", -0.7f);
        g_ctx.settings.standing_movement.stance_control.crouch.deactivation_angle = get_float("settings.standing_movement.stance_control.crouch.deactivation_angle", 0.3f);

        g_ctx.settings.standing_movement.stance_control.tiptoe.height = get_float("settings.standing_movement.stance_control.tiptoe.height", 0.17f);
        g_ctx.settings.standing_movement.stance_control.tiptoe.activation_angle = get_float("settings.standing_movement.stance_control.tiptoe.activation_angle", 0.5f);
        g_ctx.settings.standing_movement.stance_control.tiptoe.deactivation_angle = get_float("settings.standing_movement.stance_control.tiptoe.deactivation_angle", -0.3f);

        // Sofa Limits
        g_ctx.settings.sofa_limits.yaw_left = get_float("settings.sofa_limits.yaw_left", 180.0f);
        g_ctx.settings.sofa_limits.yaw_right = get_float("settings.sofa_limits.yaw_right", -180.0f);
        g_ctx.settings.sofa_limits.pitch_up = get_float("settings.sofa_limits.pitch_up", 90.0f);
        g_ctx.settings.sofa_limits.pitch_down = get_float("settings.sofa_limits.pitch_down", -65.0f);

        // Positions
        auto load_pos = [&](const char *name, AppSettings::PositionSetting &pos_setting, const AppSettings::PositionSetting &default_pos)
        {
            char key_buffer[256];
            auto format = g_ctx.formattingAPI->Format; // cache the function pointer

            format(key_buffer, sizeof(key_buffer), "settings.positions.%s.enabled", name);
            pos_setting.enabled = get_bool(key_buffer, default_pos.enabled);

            format(key_buffer, sizeof(key_buffer), "settings.positions.%s.position.x", name);
            pos_setting.position.x = get_float(key_buffer, default_pos.position.x);
            format(key_buffer, sizeof(key_buffer), "settings.positions.%s.position.y", name);
            pos_setting.position.y = get_float(key_buffer, default_pos.position.y);
            format(key_buffer, sizeof(key_buffer), "settings.positions.%s.position.z", name);
            pos_setting.position.z = get_float(key_buffer, default_pos.position.z);

            format(key_buffer, sizeof(key_buffer), "settings.positions.%s.rotation.x", name);
            pos_setting.rotation.x = get_float(key_buffer, default_pos.rotation.x);
            format(key_buffer, sizeof(key_buffer), "settings.positions.%s.rotation.y", name);
            pos_setting.rotation.y = get_float(key_buffer, default_pos.rotation.y);
        };

        load_pos("passenger_seat", g_ctx.settings.positions.passenger_seat, {true, {0.95f, 0.0f, -0.03f}, {0.03f, 0.03f, 0.0f}});
        load_pos("standing", g_ctx.settings.positions.standing, {true, {0.5f, 0.2f, 0.25f}, {-0.17f, -0.3f, 0.0f}});
        load_pos("sofa_sit1", g_ctx.settings.positions.sofa_sit1, {true, {0.5f, 0.0f, 0.8f}, {0.0f, 0.0f, 0.0f}});
        load_pos("sofa_lie", g_ctx.settings.positions.sofa_lie, {true, {-0.15f, -0.25f, 1.25f}, {-1.65f, 0.35f, 0.0f}});
        load_pos("sofa_sit2", g_ctx.settings.positions.sofa_sit2, {true, {0.2f, 0.0f, 0.8f}, {0.0f, 0.0f, 0.0f}});

        log("[LoadSettings] All settings reloaded successfully.");
    }
    void OnSettingChanged(SPF_Config_Handle *config_handle, const char *keyPath)
    {
        // For simplicity, we can just reload all settings.
        LoadSettings(g_ctx.configAPI, config_handle);

        // ONLY notify the animation controller if a setting that affects position has changed.
        if (strstr(keyPath, "settings.positions") != NULL || strstr(keyPath, "settings.standing_movement") != NULL)
        {
            // Notify the animation controller that settings have changed, so it can react (e.g., snap camera to new position).
            AnimationController::NotifySettingsUpdated();
        }
    }

    void OnLoad(const SPF_Load_API *load_api)
    {
        // Cache the provided API pointers in our global context.
        g_ctx.loadAPI = load_api;

        // --- Essential API Initialization ---
        if (g_ctx.loadAPI)
        {
            // Get Logger and Formatting
            g_ctx.loggerHandle = g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME);
            g_ctx.formattingAPI = g_ctx.loadAPI->formatting;

            if (g_ctx.loggerHandle && g_ctx.formattingAPI)
            {
                char log_buffer[256];
                g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s has been loaded!", PLUGIN_NAME);
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
            }

            // Get Config API and Handle at Load time, as per API docs
            g_ctx.configAPI = g_ctx.loadAPI->config;
            if (g_ctx.configAPI)
            {
                g_ctx.configHandle = g_ctx.configAPI->GetContext(PLUGIN_NAME);
                if (g_ctx.loggerHandle && !g_ctx.configHandle)
                {
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[OnLoad] configHandle is NULL. Plugin may not have 'allowUserConfig=true' in manifest.");
                }
            }
            else
            {
                if (g_ctx.loggerHandle)
                {
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[OnLoad] configAPI is NULL.");
                }
            }
        }
    }

    AnimationController::CameraPosition GetNextEnabledSofaPos(AnimationController::CameraPosition current_pos)
    {
        // The order of the sofa cycle
        const AnimationController::CameraPosition cycle[] = {
            AnimationController::CameraPosition::SofaSit1,
            AnimationController::CameraPosition::SofaLie,
            AnimationController::CameraPosition::SofaSit2};
        const int cycle_len = sizeof(cycle) / sizeof(cycle[0]);

        // Helper to check if a sofa position is enabled
        auto is_enabled = [&](AnimationController::CameraPosition pos)
        {
            switch (pos)
            {
            case AnimationController::CameraPosition::SofaSit1:
                return g_ctx.settings.positions.sofa_sit1.enabled;
            case AnimationController::CameraPosition::SofaLie:
                return g_ctx.settings.positions.sofa_lie.enabled;
            case AnimationController::CameraPosition::SofaSit2:
                return g_ctx.settings.positions.sofa_sit2.enabled;
            default:
                return false;
            }
        };

        // Find the index of the current position in the cycle
        int current_idx = -1;
        for (int i = 0; i < cycle_len; ++i)
        {
            if (cycle[i] == current_pos)
            {
                current_idx = i;
                break;
            }
        }

        // If not currently on the sofa, find the first enabled position to start the cycle
        if (current_idx == -1)
        {
            for (int i = 0; i < cycle_len; ++i)
            {
                if (is_enabled(cycle[i]))
                {
                    return cycle[i];
                }
            }
            return AnimationController::CameraPosition::None; // No sofa spots enabled at all
        }

        // Starting from the current position, iterate through the cycle to find the *next* enabled one
        for (int i = 1; i <= cycle_len; ++i)
        {
            int next_idx = (current_idx + i) % cycle_len;
            if (is_enabled(cycle[next_idx]))
            {
                // To prevent getting stuck if only one position is enabled
                if (cycle[next_idx] != current_pos)
                {
                    return cycle[next_idx];
                }
            }
        }

        return AnimationController::CameraPosition::None; // No other spots are enabled, so stay put.
    }

    void OnCycleSofaPositions()
    {
        if (!IsSafeToLeaveDriverSeat())
        {
            return;
        }

        if (AnimationController::IsAnimating() || AnimationController::HasPendingMoves())
        {
            return; // Don't do anything if an animation is already playing or pending
        }

        AnimationController::CameraPosition current_pos = AnimationController::GetCurrentPosition();
        AnimationController::CameraPosition next_pos = GetNextEnabledSofaPos(current_pos);

        if (next_pos != AnimationController::CameraPosition::None)
        {
            AnimationController::OnRequestMove(next_pos);
        }
    }

    void OnActivated(const SPF_Core_API *core_api)
    {
        g_ctx.coreAPI = core_api;

        if (g_ctx.loggerHandle && g_ctx.formattingAPI)
        {
            char log_buffer[256];
            g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s has been activated!", PLUGIN_NAME);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }

        // --- API Initialization & Settings Load ---
        if (g_ctx.coreAPI)
        {
            // Get JsonReader API (only available in OnActivated)
            g_ctx.jsonReaderAPI = g_ctx.coreAPI->json_reader;

            // Now that we have all APIs, load settings.
            // configAPI and configHandle should have been acquired in OnLoad.
            if (g_ctx.configAPI && g_ctx.configHandle)
            {
                LoadSettings(g_ctx.configAPI, g_ctx.configHandle);
            }
            else
            {
                if (g_ctx.loggerHandle)
                {
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[OnActivated] LoadSettings was SKIPPED due to missing handles (check OnLoad logs).");
                }
            }

            // --- Other API Initialization ---
            if (g_ctx.coreAPI->keybinds)
            {
                g_ctx.keybindsHandle = g_ctx.coreAPI->keybinds->GetContext(PLUGIN_NAME);
                if (g_ctx.keybindsHandle)
                {
                    g_ctx.coreAPI->keybinds->Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.moveToPassengerSeat", OnMoveToPassengerSeat);
                    g_ctx.coreAPI->keybinds->Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.moveToDriverSeat", OnMoveToDriverSeat);
                    g_ctx.coreAPI->keybinds->Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.moveToStandingPosition", OnMoveToStandingPosition);
                    g_ctx.coreAPI->keybinds->Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.cycleSofaPositions", OnCycleSofaPositions);
                }
            }

            if (g_ctx.coreAPI->telemetry)
            {
                g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->GetContext(PLUGIN_NAME);
            }

            g_ctx.uiAPI = g_ctx.coreAPI->ui;
            g_ctx.cameraAPI = g_ctx.coreAPI->camera;
            g_ctx.hooksAPI = g_ctx.coreAPI->hooks;
        }

        // Initialize controller modules
        AnimationController::Initialize(&g_ctx);
    }
    void OnUpdate()
    {
        // This function is called every frame while the plugin is active.
        // Avoid performing heavy or blocking operations here, as it will directly impact game performance.

        // Update our modules
        AnimationController::Update();

        // --- Warning Window Timer ---
        if (g_ctx.is_warning_active && g_ctx.coreAPI && g_ctx.coreAPI->telemetry && g_ctx.telemetryHandle)
        {
            SPF_Timestamps timestamps;
            g_ctx.coreAPI->telemetry->GetTimestamps(g_ctx.telemetryHandle, &timestamps);
            if ((timestamps.simulation - g_ctx.warning_start_time) > g_ctx.settings.general.warning_duration_ms * 1000)
            {
                g_ctx.is_warning_active = false;
                if (g_ctx.uiAPI && g_ctx.warningWindowHandle)
                {
                    g_ctx.uiAPI->SetVisibility(g_ctx.warningWindowHandle, false);
                }
            }
        }
    }

    void OnUnload()
    {
        // Perform cleanup. Nullify cached API pointers to prevent use-after-free
        // and ensure a clean shutdown. This is the last chance for cleanup.

        if (g_ctx.loadAPI && g_ctx.loggerHandle && g_ctx.formattingAPI)
        {
            char log_buffer[256];
            g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s is being unloaded.", PLUGIN_NAME);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }

        // Nullify all cached API pointers and handles.
        g_ctx.coreAPI = nullptr;
        g_ctx.loadAPI = nullptr;
        g_ctx.loggerHandle = nullptr;
        g_ctx.formattingAPI = nullptr;
        g_ctx.configAPI = nullptr;
        g_ctx.configHandle = nullptr;
        g_ctx.jsonReaderAPI = nullptr;

        g_ctx.keybindsHandle = nullptr;
        g_ctx.uiAPI = nullptr;
        g_ctx.warningWindowHandle = nullptr;
        g_ctx.telemetryHandle = nullptr;
        g_ctx.hooksAPI = nullptr;
        g_ctx.cameraAPI = nullptr;
    }

    void OnRegisterUI(SPF_UI_API *ui_api)
    {
        if (!ui_api)
        {
            return;
        }
        // We already cached g_ctx.uiAPI in OnActivated, but it's good practice to ensure it's set here too.
        g_ctx.uiAPI = ui_api;

        // Get the handle for our warning window
        g_ctx.warningWindowHandle = g_ctx.uiAPI->GetWindowHandle(PLUGIN_NAME, "WarningWindow");

        // Register the drawing callback for our warning window
        g_ctx.uiAPI->RegisterDrawCallback(PLUGIN_NAME, "WarningWindow", DrawWarningWindow, &g_ctx);
    }

    void DrawWarningWindow(SPF_UI_API *ui, void *user_data)
    {
        if (!ui || !user_data || !g_ctx.configAPI || !g_ctx.configHandle)
            return;

        // --- Dynamic Positioning ---
        // On every frame this window is drawn, calculate its desired position and
        // update the configuration values that the UI framework reads from.
        float viewport_w, viewport_h;
        ui->GetViewportSize(&viewport_w, &viewport_h);

        const float window_w = 400; // Using the width from the manifest
        const float window_h = 100; // Using a smaller, more reasonable height
        const float offset_from_bottom = 100.0f;

        int new_pos_x = static_cast<int>((viewport_w / 2.0f) - (window_w / 2.0f));
        int new_pos_y = static_cast<int>(viewport_h - window_h - offset_from_bottom);

        // Update the config values that the UI system reads from for positioning.
        g_ctx.configAPI->SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.pos_x", new_pos_x);
        g_ctx.configAPI->SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.pos_y", new_pos_y);
        g_ctx.configAPI->SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.size_w", static_cast<int>(window_w));
        g_ctx.configAPI->SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.size_h", static_cast<int>(window_h));

        PluginContext *ctx = static_cast<PluginContext *>(user_data);
        if (ctx->is_warning_active)
        {
            // Get the warning message
            char warning_message[512];
            g_ctx.loadAPI->localization->GetString(
                g_ctx.loadAPI->localization->GetContext(PLUGIN_NAME),
                "messages.warning_not_safe_to_move",
                warning_message, sizeof(warning_message));

            // Create a style for the warning message
            SPF_TextStyle_Handle warning_style = ui->Style_Create();
            if (warning_style)
            {
                ui->Style_SetFont(warning_style, SPF_FONT_H1);             // Make it a header
                ui->Style_SetAlign(warning_style, SPF_TEXT_ALIGN_CENTER);  // Center it
                ui->Style_SetColor(warning_style, 1.0f, 0.0f, 0.0f, 1.0f); // Keep it red

                // Render the styled text
                ui->TextStyled(warning_style, warning_message);

                // Clean up the style object
                ui->Style_Destroy(warning_style);
            }
        }
    }

    bool IsSafeToLeaveDriverSeat()
    {
        // This check only applies if we are currently in the driver's seat.
        if (AnimationController::GetCurrentPosition() != AnimationController::CameraPosition::Driver)
        {
            return true; // It's always safe to move if not in the driver's seat.
        }

        if (!g_ctx.coreAPI || !g_ctx.coreAPI->telemetry || !g_ctx.telemetryHandle)
        {
            return false; // Not ready, prevent movement just in case.
        }

        SPF_TruckData truckData;
        g_ctx.coreAPI->telemetry->GetTruckData(g_ctx.telemetryHandle, &truckData);

        const bool isStationary = fabsf(truckData.speed) < 0.1f;
        if (isStationary && truckData.parking_brake)
        {
            return true; // Conditions are met.
        }
        else
        {
            // Conditions are not met. Show the warning window.
            if (g_ctx.uiAPI && g_ctx.warningWindowHandle && !g_ctx.is_warning_active)
            {
                SPF_Timestamps timestamps;
                g_ctx.coreAPI->telemetry->GetTimestamps(g_ctx.telemetryHandle, &timestamps);
                g_ctx.warning_start_time = timestamps.simulation;
                g_ctx.is_warning_active = true;
                g_ctx.uiAPI->SetVisibility(g_ctx.warningWindowHandle, true);
            }
            return false; // Movement is not safe.
        }
    }

    void OnMoveToPassengerSeat()
    {
        if (g_ctx.loggerHandle)
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "[Keybind] OnMoveToPassengerSeat triggered.");
        if (!IsSafeToLeaveDriverSeat())
        {
            if (g_ctx.loggerHandle)
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_WARN, "[Keybind] OnMoveToPassengerSeat aborted: not safe to leave driver seat.");
            return;
        }

        if (g_ctx.settings.positions.passenger_seat.enabled)
        {
            AnimationController::OnRequestMove(AnimationController::CameraPosition::Passenger);
        }
    }
    void OnMoveToDriverSeat()
    {
        // OnRequestMove now handles all pathfinding logic internally.
        AnimationController::OnRequestMove(AnimationController::CameraPosition::Driver);
    }

    void OnMoveToStandingPosition()
    {
        if (!IsSafeToLeaveDriverSeat())
        {
            return;
        }

        // If we are already standing, this key toggles the walking state.
        if (AnimationController::GetCurrentPosition() == AnimationController::CameraPosition::Standing)
        {
            g_is_walk_key_down = !g_is_walk_key_down;
            return;
        }

        // Otherwise, request a move to the standing position if it's enabled.
        if (g_ctx.settings.positions.standing.enabled)
        {
            // OnRequestMove will handle getting there from any other state (sofa, seats).
            AnimationController::OnRequestMove(AnimationController::CameraPosition::Standing);
        }
    }

    bool IsWalkKeyDown()
    {
        return g_is_walk_key_down;
    }

    void OnGameWorldReady()
    {
        // All offset finding is now centralized in the Offsets module.
        // We call it here, when the game world is ready and code is in memory.
        if (Offsets::Find(g_ctx.hooksAPI))
        {
            // If offsets were found successfully, initialize the camera hook manager.
            if (CameraHookManager::Initialize(g_ctx.hooksAPI, PLUGIN_NAME))
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "[OnGameWorldReady] Camera hook initialized successfully.");
            }
            else
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[OnGameWorldReady] Failed to initialize camera hook.");
            }
        }
        else
        {
            // Log that initialization failed. The log is already written by the Find function.
            // We could show a UI warning here if desired.
        }
    }

    // =================================================================================================
    // 6. Plugin Exports
    // =================================================================================================
    // These are the two mandatory, C-style functions that the plugin DLL must export.
    // The `extern "C"` block is essential to prevent C++ name mangling, ensuring the framework
    // can find them by name.

    extern "C"
    {

        /**
         * @brief Exports the manifest API to the framework.
         * @details This function is mandatory for the framework to properly identify and configure the plugin.
         */
        SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API *out_api)
        {
            if (out_api)
            {
                out_api->GetManifestData = GetManifestData;
                return true;
            }
            return false;
        }

        /**
         * @brief Exports the plugin's main lifecycle and callback functions to the framework.
         * @details This function is mandatory for the framework to interact with the plugin's lifecycle.
         */
        SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports *exports)
        {
            if (exports)
            {
                // Connect the internal C++ functions to the C-style export struct.
                exports->OnLoad = OnLoad;
                exports->OnActivated = OnActivated;
                exports->OnUnload = OnUnload;
                exports->OnUpdate = OnUpdate;

                // Optional callbacks are set to nullptr by default.
                // Uncomment and assign your implementation if you use them.
                exports->OnGameWorldReady = OnGameWorldReady; // Assign your OnGameWorldReady function for game-world-dependent logic.
                exports->OnRegisterUI = OnRegisterUI;         // Assign your OnRegisterUI function if you have UI windows.
                exports->OnSettingChanged = OnSettingChanged; // Assign your OnSettingChanged function if you implement it.
                return true;
            }
            return false;
        }

    } // extern "C"

} // namespace SPF_CabinWalk
