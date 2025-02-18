#pragma once
#include "Core/ConfigSettings.h"
#include "Core/Config.h"
#include "Windows/Vanguard/VanguardJsonParser.h"
#include <string>
#include <variant>
#include <format>

#define VAR_NAME(Variable) (#Variable)

using SettingsTypes = std::variant<bool, int, float>;
using SettingsArray = std::vector< std::pair<std::string, SettingsTypes>>;

class VanguardSettings
{
public:
  SettingsArray array;

  std::pair<std::string, bool> Vsync;

  std::pair<std::string, int> FrameSkip;
  std::pair<std::string, int> FrameSkipType;
  std::pair<std::string, bool> AutoFrameSkip;

  std::pair<std::string, bool> SkipBufferEffects;
  std::pair<std::string, int> SkipGPUReadbackMode;

  // save the current value of required settings
  void SaveSettings()
  {
    save_setting<bool>(VAR_NAME(Vsync), Vsync, &g_Config.bVSync);

	save_setting<int>(VAR_NAME(FrameSkip), FrameSkip, &g_Config.iFrameSkip);
	save_setting<int>(VAR_NAME(FrameSkipType), FrameSkipType, &g_Config.iFrameSkipType);
	save_setting<bool>(VAR_NAME(AutoFrameSkip), AutoFrameSkip, &g_Config.bAutoFrameSkip);
	
	save_setting<bool>(VAR_NAME(SkipBufferEffects), SkipBufferEffects, &g_Config.bSkipBufferEffects);
	save_setting<int>(VAR_NAME(SkipGPUReadbackMode), SkipGPUReadbackMode, &g_Config.iSkipGPUReadbackMode);

  }

  // load the settings values sent from the dll hook into the emulator's settings
  void LoadSettings(JsonParser::JsonValue settings)
  {
    load_setting<bool>(VAR_NAME(Vsync), (*settings.json)[VAR_NAME(Vsync)], &g_Config.bVSync);

	load_setting<int>(VAR_NAME(FrameSkip), (*settings.json)[VAR_NAME(FrameSkip)], &g_Config.iFrameSkip);
	load_setting<int>(VAR_NAME(FrameSkipType), (*settings.json)[VAR_NAME(FrameSkipType)], &g_Config.iFrameSkipType);
	load_setting<bool>(VAR_NAME(AutoFrameSkip), (*settings.json)[VAR_NAME(AutoFrameSkip)], &g_Config.bAutoFrameSkip);

	load_setting<bool>(VAR_NAME(SkipBufferEffects), (*settings.json)[VAR_NAME(SkipBufferEffects)], &g_Config.bSkipBufferEffects);
	load_setting<int>(VAR_NAME(SkipGPUReadbackMode), (*settings.json)[VAR_NAME(SkipGPUReadbackMode)], &g_Config.iSkipGPUReadbackMode);

	g_Config.Save("Vanguard Settings Saved");
  }

  // Visits the variant value in a pair, determines the correct data type and returns it as a string
  std::string to_string(SettingsTypes var)
  {
    return std::visit([](auto arg) { return std::format("{}", arg); }, var);
  }

private:
  // saves the name and value of the setting to a pair, then push it onto the array
  template<typename T>
  void save_setting(std::string name, std::pair<std::string, SettingsTypes> variable, T* setting)
  {
    variable.first = name;
    variable.second = *setting;
    array.push_back(variable);
  }

  // loads the value of the parsed setting based on the requested data type
  template<typename T>
  void load_setting(std::string name, JsonParser::JsonValue value, T* setting)
  {
    if (typeid(T) == typeid(bool))
      *setting = value.b;

    else if (typeid(T) == typeid(int))
      *setting = value.i;

    else if (typeid(T) == typeid(float))
      *setting = value.d;
  }
};
