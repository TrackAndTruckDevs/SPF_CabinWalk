/**
 * @file SPF_CabinWalk.cpp
 * @brief The main implementation file for the SPF_CabinWalk.
 * @details This file contains the minimal implementation for a plugin to be loaded
 * and recognized by the SPF framework. It serves as a basic template for new plugins,
 * with clear explanations and commented-out sections for optional features.
 */

#include "SPF_CabinWalk.hpp" // Always include your own header first
#include "Utils/Utils.hpp"           // For general utility functions like lerp
#include "Hooks/Offsets.hpp"         // For memory offsets and signatures
#include "Hooks/CameraHookManager.hpp" // For camera hooking logic
#include "Animation/AnimationController.hpp" // For managing camera animations
#include "Animation/StandingAnimController.hpp" // For handling walking logic

#include <cmath>             // For math functions like fabsf
#include <cstring>           // For C-style string manipulation functions like strncpy_s.

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

            // `name`: (Optional) A unique name for the plugin (e.g., "SPF_CabinWalk").
            // If not specified, the framework will use the name of your DLL file, but specifying it
            // here is recommended to avoid potential conflicts.
            strncpy_s(info.name, PLUGIN_NAME, sizeof(info.name));

            // `version`: (Optional) The plugin's version string (e.g., "1.0.0").
            strncpy_s(info.version, "0.1.0", sizeof(info.version));

            // `author`: (Optional) The name of the author or organization.
            strncpy_s(info.author, "Your Name/Organization", sizeof(info.author));

            // `descriptionLiteral`: (Optional) A simple, hardcoded description for your plugin.
            // This is used as a fallback if the localized description key is not found.
            strncpy_s(info.descriptionLiteral, "A minimal template plugin for the SPF API.", sizeof(info.descriptionLiteral));
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
            policy.userConfigurableSystemsCount = 0; // To enable configurable systems, uncomment the block below and set the count accordingly
            // strncpy_s(policy.userConfigurableSystems[0], "logging", sizeof(policy.userConfigurableSystems[0]));
            // strncpy_s(policy.userConfigurableSystems[1], "settings", sizeof(policy.userConfigurableSystems[1]));
            // strncpy_s(policy.userConfigurableSystems[1], "localization", sizeof(policy.userConfigurableSystems[1]));
            // strncpy_s(policy.userConfigurableSystems[1], "ui", sizeof(policy.userConfigurableSystems[1]));

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
        out_manifest.settingsJson = nullptr;
        // Example: Define some default custom settings.
        // To provide user-friendly names and descriptions, see `customSettingsMetadata` at the end.
        /*
        out_manifest.settingsJson = R"json(
            {
                "some_number": 42,
            }
        )json";
        */

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
            logging.sinks.file = true;
        }

        // --- Localization ---
        // Requires: SPF_Localization_API.h
        // Uncomment if your plugin uses localized strings.
        /*
        {
            auto& localization = out_manifest.localization;
            // `language`: Default language code (e.g., "en", "de", "uk").
            strncpy_s(localization.language, "en", sizeof(localization.language));
        }
        */

        auto &keybinds = out_manifest.keybinds;
        keybinds.actionCount = 4; // Incremented to 4
        {
            // --- Action 0: A sample keybind to toggle a window ---
            auto &action = keybinds.actions[0];
            // `groupName`: Logical grouping for actions, used to avoid name collisions.
            // Best practice: "{PluginName}.{Feature}".
            strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
            // `actionName`: Specific action (e.g., "toggle", "activate").
            strncpy_s(action.actionName, "moveToPassengerSeat", sizeof(action.actionName));

            // Define one or more default key combinations for this action.
            action.definitionCount = 1;
            {
                // --- Definition 0 ---
                auto &def = action.definitions[0];
                // `type`: "keyboard" or "gamepad".
                strncpy_s(def.type, "keyboard", sizeof(def.type));
                // `key`: Key name (see VirtualKeyMapping.cpp or GamepadButtonMapping.cpp).
                strncpy_s(def.key, "KEY_NUMPAD3", sizeof(def.key));
                // `pressType`: "short" (tap) or "long" (hold).
                strncpy_s(def.pressType, "short", sizeof(def.pressType));
                // `pressThresholdMs`: For "long" press, time in ms to hold.
                // def.pressThresholdMs = 300;
                // `consume`: When to consume input: "never", "on_ui_focus", "always".
                strncpy_s(def.consume, "always", sizeof(def.consume));
                // `behavior`: How action behaves. Valid values: "toggle" (on/off), "hold" (while pressed).
                strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
            }
        }
		{
			// --- Action 1: Move to driver seat ---
			auto& action = keybinds.actions[1];
			strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
			strncpy_s(action.actionName, "moveToDriverSeat", sizeof(action.actionName));

			action.definitionCount = 1;
			{
				auto& def = action.definitions[0];
				strncpy_s(def.type, "keyboard", sizeof(def.type));
				strncpy_s(def.key, "KEY_NUMPAD5", sizeof(def.key));
				strncpy_s(def.pressType, "short", sizeof(def.pressType));
				strncpy_s(def.consume, "always", sizeof(def.consume));
				strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
			}
		}
		{
			// --- Action 2: Move to standing position ---
			auto& action = keybinds.actions[2];
			strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
			strncpy_s(action.actionName, "moveToStandingPosition", sizeof(action.actionName));

			action.definitionCount = 1;
			{
				auto& def = action.definitions[0];
				strncpy_s(def.type, "keyboard", sizeof(def.type));
				strncpy_s(def.key, "KEY_NUMPAD2", sizeof(def.key));
				strncpy_s(def.pressType, "short", sizeof(def.pressType));
				strncpy_s(def.consume, "always", sizeof(def.consume));
				strncpy_s(def.behavior, "hold", sizeof(def.behavior));
			}
		}
        {
			// --- Action 3: Cycle Sofa Positions ---
			auto& action = keybinds.actions[3];
			strncpy_s(action.groupName, "SPF_CabinWalk.Movement", sizeof(action.groupName));
			strncpy_s(action.actionName, "cycleSofaPositions", sizeof(action.actionName));

			action.definitionCount = 1;
			{
				auto& def = action.definitions[0];
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
            window.posX = 100;
            window.posY = 100;
            window.sizeW = 400;
            window.sizeH = 300;
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
            strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName)); // Must match the action's groupName
            strncpy_s(meta.actionName, "moveToPassengerSeat", sizeof(meta.actionName));  // Must match the action's actionName
            strncpy_s(meta.titleKey, "Move to Passenger Seat", sizeof(meta.titleKey));
            strncpy_s(meta.descriptionKey, "Moves the camera to the passenger seat.", sizeof(meta.descriptionKey));
        }
		{
			auto& meta = out_manifest.keybindsMetadata[1];
			strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
			strncpy_s(meta.actionName, "moveToDriverSeat", sizeof(meta.actionName));
			strncpy_s(meta.titleKey, "Move to Driver Seat", sizeof(meta.titleKey));
			strncpy_s(meta.descriptionKey, "Moves the camera to the driver seat.", sizeof(meta.descriptionKey));
		}
		{
			auto& meta = out_manifest.keybindsMetadata[2];
			strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
			strncpy_s(meta.actionName, "moveToStandingPosition", sizeof(meta.actionName));
			strncpy_s(meta.titleKey, "Move to Standing Position", sizeof(meta.titleKey));
			strncpy_s(meta.descriptionKey, "Moves the camera to the standing position in the cabin.", sizeof(meta.descriptionKey));
		}
        {
			auto& meta = out_manifest.keybindsMetadata[3];
			strncpy_s(meta.groupName, "SPF_CabinWalk.Movement", sizeof(meta.groupName));
			strncpy_s(meta.actionName, "cycleSofaPositions", sizeof(meta.actionName));
			strncpy_s(meta.titleKey, "Cycle Sofa Positions", sizeof(meta.titleKey));
			strncpy_s(meta.descriptionKey, "Cycles through positions on the sofa (sit, lie down).", sizeof(meta.descriptionKey));
		}
    }

    // =================================================================================================
    // 3. Plugin Lifecycle Implementations
    // =================================================================================================
    // The following functions are the core lifecycle events for the plugin.

    void OnLoad(const SPF_Load_API *load_api)
    {
        // Cache the provided API pointers in our global context.
        g_ctx.loadAPI = load_api;

        // --- Essential API Initialization ---
        // Get and cache the logger and formatting API handles.
        if (g_ctx.loadAPI)
        {
            g_ctx.loggerHandle = g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME);
            g_ctx.formattingAPI = g_ctx.loadAPI->formatting;

            if (g_ctx.loggerHandle && g_ctx.formattingAPI)
            {
                char log_buffer[256];
                g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s has been loaded!", PLUGIN_NAME);
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
            }
        }

    }

    void OnCycleSofaPositions()
    {
        if (!IsSafeToLeaveDriverSeat())
        {
            return;
        }

        if (AnimationController::IsAnimating())
        {
            return; // Don't do anything if an animation is already playing
        }

        AnimationController::CameraPosition current_pos = AnimationController::GetCurrentPosition();

        switch (current_pos)
        {
            // --- Cycle through sofa positions ---
            case AnimationController::CameraPosition::SofaSit1:
                AnimationController::MoveTo(AnimationController::CameraPosition::SofaLie);
                break;
            case AnimationController::CameraPosition::SofaLie:
                AnimationController::MoveTo(AnimationController::CameraPosition::SofaSit2);
                break;
            case AnimationController::CameraPosition::SofaSit2:
                AnimationController::MoveTo(AnimationController::CameraPosition::SofaSit1);
                break;

                    // --- Enter sofa cycle from other positions ---
                    case AnimationController::CameraPosition::Driver:
                    case AnimationController::CameraPosition::Passenger:
                    case AnimationController::CameraPosition::Standing:
                        AnimationController::OnRequestMove(AnimationController::CameraPosition::SofaSit1);
                        break;            
            default:
                // Do nothing for other states like Bed, None, etc.
                break;
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

        // --- Optional API Initialization & Callback Registration (Uncomment if needed) ---
        // Remember to also uncomment the relevant #include directives in SPF_CabinWalk.hpp
        // and add corresponding members to the PluginContext struct.

        // Keybinds API
        // Requires: SPF_KeyBinds_API.h
        if (g_ctx.coreAPI && g_ctx.coreAPI->keybinds)
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

        // Telemetry API
        // Requires: SPF_Telemetry_API.h
        if (g_ctx.coreAPI && g_ctx.coreAPI->telemetry)
        {
            g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->GetContext(PLUGIN_NAME);
        }

        // UI API
        // Requires: SPF_UI_API.h
        if (g_ctx.coreAPI)
        {
            g_ctx.uiAPI = g_ctx.coreAPI->ui;
        }

        // Camera API
        // Requires: SPF_Camera_API.h
        if (g_ctx.coreAPI)
        {
            g_ctx.cameraAPI = g_ctx.coreAPI->camera;
        }

        // Hooks API
        // Requires: SPF_Hooks_API.h
        if (g_ctx.coreAPI && g_ctx.coreAPI->hooks)
        {
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
            if ((timestamps.simulation - g_ctx.warning_start_time) > g_ctx.warning_duration_ms)
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

        // --- Optional API Cleanup (Uncomment if needed) ---
        // Example: Unregistering keybinds (often handled by framework, but good practice if explicitly registered).
        // Requires: SPF_KeyBinds_API.h

        // Nullify all cached API pointers and handles.
        g_ctx.coreAPI = nullptr;
        g_ctx.loadAPI = nullptr;
        g_ctx.loggerHandle = nullptr;
        g_ctx.formattingAPI = nullptr;

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
        if (!ui || !user_data)
            return;
        PluginContext *ctx = static_cast<PluginContext *>(user_data);

        if (ctx->is_warning_active)
        {
            // Example: Display a simple text message
            ui->TextColored(1.0f, 0.0f, 0.0f, 1.0f, "Поставте вантажівку на ручник та зупиніться!"); // Red text
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
        if (!IsSafeToLeaveDriverSeat())
        {
            return;
        }
        AnimationController::OnRequestMove(AnimationController::CameraPosition::Passenger);
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

        // Otherwise, request a move to the standing position.
        // OnRequestMove will handle getting there from any other state (sofa, seats).
        AnimationController::OnRequestMove(AnimationController::CameraPosition::Standing);
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
                // exports->OnSettingChanged = OnSettingChanged; // Assign your OnSettingChanged function if you implement it.
                return true;
            }
            return false;
        }

    } // extern "C"

} // namespace SPF_CabinWalk
