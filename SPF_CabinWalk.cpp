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
#include <string>  // For std::string and std::to_string

namespace SPF_CabinWalk
{
    // Forward Declarations
    bool IsSafeToLeaveDriverSeat();

    // =================================================================================================
    // 1. Constants & Global State
    // =================================================================================================

    /**
     * @brief A constant for the plugin's name.
     * @details This MUST match the name used in `Cfg_GetContext` calls for various APIs
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

    void BuildManifest(SPF_Manifest_Builder_Handle *h, const SPF_Manifest_Builder_API *api)
    {
        // This function defines all the metadata for your plugin. The framework calls this
        // function *before* loading your plugin DLL to understand what it is.

        // --- 2.1. Plugin Information ---
        // This section provides the basic identity of your plugin.
        {
            api->Info_SetName(h, PLUGIN_NAME);
            api->Info_SetVersion(h, "1.0.2");
            api->Info_SetMinFrameworkVersion(h, "1.1.0");
            api->Info_SetAuthor(h, "Track'n'Truck Devs");
            api->Info_SetDescriptionLiteral(h, "A plugin for American Truck Simulator and Euro Truck Simulator 2 that allows you to unchain the camera from the driver's seat and freely walk around your truck's cabin. Explore your interior with smooth, animated camera movements.");

            api->Info_SetEmail(h, "mailto:spf.framework@gmail.com");
            api->Info_SetYoutubeUrl(h, "https://www.youtube.com/@TrackAndTruck");
            api->Info_SetPatreonUrl(h, "https://www.patreon.com/TrackAndTruckDevs");
        }

        // --- 2.2. Configuration Policy ---
        // This section defines how your plugin interacts with the framework's configuration system.
        {
            // `allowUserConfig`: Set to `true` if you want a `settings.json` file to be created
            // for your plugin, allowing users (or the framework UI) to override default settings.
            api->Policy_SetAllowUserConfig(h, true);

            // Enable specific systems in the settings UI.
            api->Policy_AddConfigurableSystem(h, "settings");
            api->Policy_AddConfigurableSystem(h, "localization");
            
            // Required hooks (none for this plugin yet)
            // api->Policy_AddRequiredHook(h, "GameConsole");
        }

        // --- 2.3. Custom Settings Defaults (settingsJson) ---
        // A JSON string literal that defines the default values for your plugin's custom settings.
        // These will be inserted under a top-level key named "settings" in settings.json.
        api->Settings_SetJson(h, R"json(
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
        )json");

        // --- 2.4. System Defaults ---

        // Logging
        api->Defaults_SetLogging(h, "debug", false);

        // Localization
        api->Defaults_SetLocalization(h, "en");

        // Keybinds
        {
            api->Defaults_AddKeybind(h, "SPF_CabinWalk.Movement", "moveToPassengerSeat", "keyboard", "KEY_NUMPAD3", "short", 0, "always", "toggle");
            api->Defaults_AddKeybind(h, "SPF_CabinWalk.Movement", "moveToDriverSeat", "keyboard", "KEY_NUMPAD5", "short", 0, "always", "toggle");
            api->Defaults_AddKeybind(h, "SPF_CabinWalk.Movement", "moveToStandingPosition", "keyboard", "KEY_NUMPAD2", "short", 0, "always", "hold");
            api->Defaults_AddKeybind(h, "SPF_CabinWalk.Movement", "cycleSofaPositions", "keyboard", "KEY_NUMPAD1", "short", 0, "always", "toggle");
        }

        // UI Windows
        {
            api->Defaults_AddWindow(h, "WarningWindow", false, false, 0, 0, 400, 100, false, false);
        }

        // =============================================================================================
        // 2.5. Metadata for UI Display (Optional)
        // =============================================================================================

        // Keybinds Metadata
        {
            api->Meta_AddKeybind(h, "SPF_CabinWalk.Movement", "moveToPassengerSeat", "keybinds.moveToPassengerSeat.title", "keybinds.moveToPassengerSeat.desc");
            api->Meta_AddKeybind(h, "SPF_CabinWalk.Movement", "moveToDriverSeat", "keybinds.moveToDriverSeat.title", "keybinds.moveToDriverSeat.desc");
            api->Meta_AddKeybind(h, "SPF_CabinWalk.Movement", "moveToStandingPosition", "keybinds.moveToStandingPosition.title", "keybinds.moveToStandingPosition.desc");
            api->Meta_AddKeybind(h, "SPF_CabinWalk.Movement", "cycleSofaPositions", "keybinds.cycleSofaPositions.title", "keybinds.cycleSofaPositions.desc");
        }

        // --- Custom Settings Metadata ---
        
        // Helper wrappers for Meta_AddCustomSetting. 
        // We build the JSON string for widget parameters here to keep the loop logic clean.
        auto AddSliderMeta = [&](const char *key, const char *title, const char *desc, float min, float max, const char *format)
        {
            // Use string concatenation to build parameters JSON without precision loss.
            std::string params = "{ \"min\": " + std::to_string(min) + 
                                 ", \"max\": " + std::to_string(max) + 
                                 ", \"format\": \"" + format + "\" }";
            api->Meta_AddCustomSetting(h, key, title, desc, "slider", params.c_str(), false);
        };

        auto AddDragMeta = [&](const char *key, const char *title, const char *desc, float speed, float min, float max, const char *format)
        {
            std::string params = "{ \"speed\": " + std::to_string(speed) + 
                                 ", \"min\": " + std::to_string(min) + 
                                 ", \"max\": " + std::to_string(max) + 
                                 ", \"format\": \"" + format + "\" }";
            api->Meta_AddCustomSetting(h, key, title, desc, "drag", params.c_str(), false);
        };

        // Specialized helpers for positions
        auto AddPositionCoordMeta = [&](const char *base_key_path, const char *axis_name)
        {
            std::string key_path = std::string("positions.") + base_key_path + ".position." + axis_name;
            std::string title_key = std::string("settings.positions.") + base_key_path + ".position." + axis_name + ".title";
            AddDragMeta(key_path.c_str(), title_key.c_str(), "settings.positions.coord.desc", 0.01f, -5.0f, 5.0f, "%.2f m");
        };

        auto AddPositionRotationMeta = [&](const char *base_key_path, const char *axis_name)
        {
            std::string key_path = std::string("positions.") + base_key_path + ".rotation." + axis_name;
            std::string title_key = std::string("settings.positions.") + base_key_path + ".rotation." + axis_name + ".title";
            AddDragMeta(key_path.c_str(), title_key.c_str(), "settings.positions.rot.desc", 0.01f, -3.14159f, 3.14159f, "%.2f rad");
        };

        api->Meta_AddCustomSetting(h, "general", "settings.general.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "positions", "settings.positions.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "animation_durations", "settings.animation_durations.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "standing_movement", "settings.standing_movement.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "standing_movement.walking", "settings.standing_movement.walking.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "standing_movement.walking.walk_zone_z", "settings.standing_movement.walking.walk_zone_z.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "standing_movement.stance_control", "settings.standing_movement.stance_control.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "standing_movement.stance_control.crouch", "settings.standing_movement.stance_control.crouch.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "standing_movement.stance_control.tiptoe", "settings.standing_movement.stance_control.tiptoe.title", "", nullptr, nullptr, false);

        AddSliderMeta("general.warning_duration_ms", "settings.general.warning_duration_ms.title", "settings.general.warning_duration_ms.desc", 0.0f, 30000.0f, "%d ms");
        AddSliderMeta("general.height", "settings.general.height.title", "settings.general.height.desc", 0.0f, 1.0f, "%.2f m");

        const char *cabin_layout_options = R"json({ "options": [
            { "value": 0, "labelKey": "settings.general.cabin_layout.lhd" },
            { "value": 1, "labelKey": "settings.general.cabin_layout.rhd" }
        ]})json";
        api->Meta_AddCustomSetting(h, "general.cabin_layout", "settings.general.cabin_layout.title", "settings.general.cabin_layout.desc", "radio", cabin_layout_options, false);

        // --- Positions ---
        const char *pos_names[] = {"passenger_seat", "standing", "sofa_sit1", "sofa_lie", "sofa_sit2"};
        for (const char *name : pos_names)
        {
            // Add title for the position group itself
            std::string group_key = std::string("positions.") + name;
            std::string group_title_key = std::string("settings.positions.") + name + ".title";
            api->Meta_AddCustomSetting(h, group_key.c_str(), group_title_key.c_str(), "", nullptr, nullptr, false);

            std::string enabled_key = group_key + ".enabled";
            std::string enabled_title_key = std::string("settings.positions.") + name + ".enabled.title";
            api->Meta_AddCustomSetting(h, enabled_key.c_str(), enabled_title_key.c_str(), "settings.positions.enabled.desc", "checkbox", nullptr, false);

            // Add titles for the subgroups
            std::string pos_subgroup_key = group_key + ".position";
            api->Meta_AddCustomSetting(h, pos_subgroup_key.c_str(), "settings.positions.position_group.title", "", nullptr, nullptr, false);

            AddPositionCoordMeta(name, "x");
            AddPositionCoordMeta(name, "y");
            AddPositionCoordMeta(name, "z");

            std::string rot_subgroup_key = group_key + ".rotation";
            api->Meta_AddCustomSetting(h, rot_subgroup_key.c_str(), "settings.positions.rotation_group.title", "", nullptr, nullptr, false);

            AddPositionRotationMeta(name, "x");
            AddPositionRotationMeta(name, "y");
        }

        api->Meta_AddCustomSetting(h, "animation_durations.main_animation_speed", "settings.animation_durations.main_animation_speed.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "animation_durations.sofa_animation_speed", "settings.animation_durations.sofa_animation_speed.title", "", nullptr, nullptr, false);
        api->Meta_AddCustomSetting(h, "animation_durations.crouch_and_stand_animation_speed", "settings.animation_durations.crouch_and_stand_animation_speed.title", "", nullptr, nullptr, false);

        // --- Animation Durations ---
        const char *main_anim_names[] = {
            "driver_to_passenger", "passenger_to_driver", "driver_to_standing", "standing_to_driver",
            "passenger_to_standing", "standing_to_passenger", "standing_to_sofa", "sofa_to_standing"};
        for (const char *name : main_anim_names)
        {
            std::string key = std::string("animation_durations.main_animation_speed.") + name;
            std::string title = std::string("settings.animation_durations.main_animation_speed.") + name + ".title";
            std::string desc = std::string("settings.animation_durations.main_animation_speed.") + name + ".desc";
            AddSliderMeta(key.c_str(), title.c_str(), desc.c_str(), 100.0f, 10000.0f, "%d ms");
        }

        const char *sofa_anim_names[] = {"sofa_sit1_to_lie", "sofa_lie_to_sit2", "sofa_sit2_to_sit1", "sofa_lie_to_sit1_shortcut"};
        for (const char *name : sofa_anim_names)
        {
            std::string key = std::string("animation_durations.sofa_animation_speed.") + name;
            std::string title = std::string("settings.animation_durations.sofa_animation_speed.") + name + ".title";
            std::string desc = std::string("settings.animation_durations.sofa_animation_speed.") + name + ".desc";
            AddSliderMeta(key.c_str(), title.c_str(), desc.c_str(), 100.0f, 10000.0f, "%d ms");
        }

        const char *crouch_anim_names[] = {"crouch", "tiptoe"};
        for (const char *name : crouch_anim_names)
        {
            std::string key = std::string("animation_durations.crouch_and_stand_animation_speed.") + name;
            std::string title = std::string("settings.animation_durations.crouch_and_stand_animation_speed.") + name + ".title";
            std::string desc = std::string("settings.animation_durations.crouch_and_stand_animation_speed.") + name + ".desc";
            AddSliderMeta(key.c_str(), title.c_str(), desc.c_str(), 100.0f, 10000.0f, "%d ms");
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
        api->Meta_AddCustomSetting(h, "sofa_limits", nullptr, nullptr, nullptr, nullptr, true);
        
        // walking animation speed
        api->Meta_AddCustomSetting(h, "walking_animation_speed", nullptr, nullptr, nullptr, nullptr, true);

        // Window Description
        api->Meta_AddWindow(h, "WarningWindow", "Warning", "Displayed when it is not safe to leave the driver's seat.");
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
            return configAPI->Cfg_GetInt32(configHandle, key, def);
        };
        auto get_float = [&](const char *key, float def)
        {
            return (float)configAPI->Cfg_GetFloat(configHandle, key, (double)def);
        };
        auto get_bool = [&](const char *key, bool def)
        {
            return configAPI->Cfg_GetBool(configHandle, key, def);
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
            std::string base_path = std::string("settings.positions.") + name;

            pos_setting.enabled = get_bool((base_path + ".enabled").c_str(), default_pos.enabled);

            pos_setting.position.x = get_float((base_path + ".position.x").c_str(), default_pos.position.x);
            pos_setting.position.y = get_float((base_path + ".position.y").c_str(), default_pos.position.y);
            pos_setting.position.z = get_float((base_path + ".position.z").c_str(), default_pos.position.z);

            pos_setting.rotation.x = get_float((base_path + ".rotation.x").c_str(), default_pos.rotation.x);
            pos_setting.rotation.y = get_float((base_path + ".rotation.y").c_str(), default_pos.rotation.y);
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

    /**
     * @brief Called when the framework's global interface language is changed.
     * @details Synchronizes the plugin's language with the framework for a consistent experience.
     * @param langCode The new language code (e.g., "en", "uk").
     */
    void OnLanguageChanged(const char* langCode)
    {
        if (!g_ctx.coreAPI || !g_ctx.coreAPI->localization || !langCode) return;

        SPF_Localization_Handle* h = g_ctx.coreAPI->localization->Loc_GetContext(PLUGIN_NAME);
        
        // Smart sync: only switch if the plugin supports the new language
        if (g_ctx.coreAPI->localization->Loc_HasLanguage(h, langCode))
        {
            if (g_ctx.coreAPI->localization->Loc_SetLanguage(h, langCode))
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "Plugin language synchronized with framework.");
            }
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
            g_ctx.loggerHandle = g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME);
            g_ctx.formattingAPI = g_ctx.loadAPI->formatting;

            if (g_ctx.loggerHandle && g_ctx.formattingAPI)
            {
                char log_buffer[256];
                g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s has been loaded!", PLUGIN_NAME);
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
            }

            // Get Config API and Handle at Load time, as per API docs
            g_ctx.configAPI = g_ctx.loadAPI->config;
            if (g_ctx.configAPI)
            {
                g_ctx.configHandle = g_ctx.configAPI->Cfg_GetContext(PLUGIN_NAME);
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
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s has been activated!", PLUGIN_NAME);
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
                g_ctx.keybindsHandle = g_ctx.coreAPI->keybinds->Kbind_GetContext(PLUGIN_NAME);
                if (g_ctx.keybindsHandle)
                {
                    g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.moveToPassengerSeat", OnMoveToPassengerSeat);
                    g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.moveToDriverSeat", OnMoveToDriverSeat);
                    g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.moveToStandingPosition", OnMoveToStandingPosition);
                    g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "SPF_CabinWalk.Movement.cycleSofaPositions", OnCycleSofaPositions);
                }
            }

            if (g_ctx.coreAPI->telemetry)
            {
                g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->Tel_GetContext(PLUGIN_NAME);
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
            g_ctx.coreAPI->telemetry->Tel_GetTimestamps(g_ctx.telemetryHandle, &timestamps, sizeof(SPF_Timestamps));
            if ((timestamps.simulation - g_ctx.warning_start_time) > g_ctx.settings.general.warning_duration_ms * 1000)
            {
                g_ctx.is_warning_active = false;
                if (g_ctx.uiAPI && g_ctx.warningWindowHandle)
                {
                    g_ctx.uiAPI->UI_SetVisibility(g_ctx.warningWindowHandle, false);
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
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s is being unloaded.", PLUGIN_NAME);
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
        g_ctx.warningWindowHandle = g_ctx.uiAPI->UI_GetWindowHandle(PLUGIN_NAME, "WarningWindow");

        // Register the drawing callback for our warning window
        g_ctx.uiAPI->UI_RegisterDrawCallback(PLUGIN_NAME, "WarningWindow", DrawWarningWindow, &g_ctx);
    }

    void DrawWarningWindow(SPF_UI_API *ui, void *user_data)
    {
        if (!ui || !user_data || !g_ctx.configAPI || !g_ctx.configHandle)
            return;

        // --- Dynamic Positioning ---
        // On every frame this window is drawn, calculate its desired position and
        // update the configuration values that the UI framework reads from.
        float viewport_w, viewport_h;
        ui->UI_GetViewportSize(&viewport_w, &viewport_h);

        const float window_w = 400; // Using the width from the manifest
        const float window_h = 100; // Using a smaller, more reasonable height
        const float offset_from_bottom = 100.0f;

        int new_pos_x = static_cast<int>((viewport_w / 2.0f) - (window_w / 2.0f));
        int new_pos_y = static_cast<int>(viewport_h - window_h - offset_from_bottom);

        // Update the config values that the UI system reads from for positioning.
        g_ctx.configAPI->Cfg_SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.pos_x", new_pos_x);
        g_ctx.configAPI->Cfg_SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.pos_y", new_pos_y);
        g_ctx.configAPI->Cfg_SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.size_w", static_cast<int>(window_w));
        g_ctx.configAPI->Cfg_SetInt32(g_ctx.configHandle, "ui.windows.WarningWindow.size_h", static_cast<int>(window_h));

        PluginContext *ctx = static_cast<PluginContext *>(user_data);
        if (ctx->is_warning_active)
        {
            // Get the warning message
            char warning_message[512];
            g_ctx.loadAPI->localization->Loc_GetString(
                g_ctx.loadAPI->localization->Loc_GetContext(PLUGIN_NAME),
                "messages.warning_not_safe_to_move",
                warning_message, sizeof(warning_message));

            // Create a style for the warning message
            SPF_TextStyle_Handle warning_style = ui->UI_Style_Create();
            if (warning_style)
            {
                ui->UI_Style_SetFont(warning_style, SPF_FONT_H1);             // Make it a header
                ui->UI_Style_SetAlign(warning_style, SPF_TEXT_ALIGN_CENTER);  // Center it
                ui->UI_Style_SetColor(warning_style, 1.0f, 0.0f, 0.0f, 1.0f); // Keep it red

                // Render the styled text
                ui->UI_TextStyled(warning_style, warning_message);

                // Clean up the style object
                ui->UI_Style_Destroy(warning_style);
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
        g_ctx.coreAPI->telemetry->Tel_GetTruckData(g_ctx.telemetryHandle, &truckData, sizeof(SPF_TruckData));

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
                g_ctx.coreAPI->telemetry->Tel_GetTimestamps(g_ctx.telemetryHandle, &timestamps, sizeof(SPF_Timestamps));
                g_ctx.warning_start_time = timestamps.simulation;
                g_ctx.is_warning_active = true;
                g_ctx.uiAPI->UI_SetVisibility(g_ctx.warningWindowHandle, true);
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
                out_api->BuildManifest = BuildManifest;
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
                exports->OnLanguageChanged = OnLanguageChanged; // Enable automatic language synchronization.
                return true;
            }
            return false;
        }

    } // extern "C"

} // namespace SPF_CabinWalk
