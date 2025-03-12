#include <cstddef>
#include <Memory.h>
#include "Core/MemMap.h"
#include "Core/Core.h"
#include "Core/SaveState.h"
#include "Core/System.h"
#include "UI/MainScreen.h"
#include "Windows/MainWindow.h"
#include "VanguardHelpers.h"
#include "VanguardClientInitializer.h"
#include "VanguardJsonParser.h"
#include "VanguardEmuSettings.h"
#include <codecvt>
#include <sstream>

void FormatJsonData(VanguardSettings& settings, std::ostringstream& json_string);

unsigned char Vanguard_peekbyte(long long addr, int selection)
{
	u8 byte = 0;
	u16 data = 0;
	long long mod = 0;
	long long newAddr = 0;

	switch (selection)
	{
		// RAM read
		case 0:
			byte = Memory::Read_U8(addr);
			break;
		default:
			break;
	}
	return byte;
}

void Vanguard_pokebyte(long long addr, unsigned char val, int selection)
{
	u16 value = 0;

	switch (selection)
	{
		// RAM write
		case 0:
			Memory::Write_U8(val, addr);
			break;
		default:
			break;

	}

}

bool VanguardClient::ok_to_corestep = true;
void Vanguard_pause(bool pauseUntilCorrupt)
{
	Core_UpdateState(CORE_STEPPING);
	VanguardClient::ok_to_corestep = false;
}

void Vanguard_resume()
{
	Core_UpdateState(CORE_RUNNING);
	VanguardClient::ok_to_corestep = true;
}


void Vanguard_savesavestate(BSTR filename, bool wait)
{
    if (coreState == CORE_RUNNING || coreState == CORE_STEPPING)
    {
		//Convert the BSTR sent by Vanguard to std::string
		std::string filename_converted = BSTRToString(filename);
		SaveState::Save(Path(filename_converted), -1);
    }
}


void Vanguard_loadsavestate(BSTR filename)
{
  // Convert the BSTR sent by Vanguard to std::string
  std::string filename_converted = BSTRToString(filename);

  if (GetUIState() == UISTATE_EXCEPTION)
	  System_PostUIMessage(UIMessage::REQUEST_GAME_RESET);

  SaveState::Load(Path(filename_converted), -1);

  if (coreState == CORE_STEPPING)
	Core_UpdateState(CORE_RUNNING);
}


bool VanguardClient::loading = false;
void Vanguard_loadROM(BSTR filename)
{
  VanguardClient::loading = true;
  VanguardClient::ok_to_corestep = false;

  std::string converted_filename = BSTRToString(filename);

  switch (GetUIState())
  {
	case UISTATE_MENU:
		VanguardClientInitializer::win->MainScreen::sendMessage(UIMessage::REQUEST_GAME_BOOT, converted_filename.c_str());
		break;
	default:
		PSP_Shutdown();
		VanguardClientInitializer::win->MainScreen::sendMessage(UIMessage::REQUEST_GAME_BOOT, converted_filename.c_str());
		break;
  }


  MSG msg;
  // We have to do it this way to prevent deadlock due to synced calls. It sucks but it's required
  // at the moment
  while (VanguardClient::loading)
  {
    Sleep(20);
    //these lines of code perform the equivalent of the Application.DoEvents method
    ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    ::GetMessage(&msg, NULL, 0, 0);
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  VanguardClient::ok_to_corestep = true; // We're good to peek/pokebyte now

  Sleep(100);  // Give the emu thread a chance to recover
  
}

void Vanguard_finishLoading()
{
  VanguardClient::loading = false;
}

void Vanguard_closeGame()
{
	PSP_Shutdown();
}

void Vanguard_prepShutdown()
{
	PSP_Shutdown();
}

void Vanguard_forceStop()
{
	System_ExitApp();
}

std::string VanguardClient::system_core = "EMPTY";
char* Vanguard_getSystemCore()
{
	// store the output as a string, then convert it to char*
	std::string tmp = VanguardClient::system_core;

	std::vector<char> _output(tmp.begin(), tmp.end());
	_output.push_back('\0');

	char* output = (char*)LocalAlloc(LMEM_FIXED, _output.size() + 1);
	if (!output)
		return NULL;

	memcpy(output, _output.data(), _output.size() + 1);

	return output;
}

// Saves all required emulator settings and returns it to the hook DLL to store with the savestate
char* Vanguard_saveEmuSettings()
{
	// create a new settings class and store all values
	VanguardSettings _settings;
	_settings.SaveSettings();

	// write the json data to a stringstream
	std::ostringstream out;
	FormatJsonData(_settings, out);

	// store the output as a string, then convert it to char*
	std::string tmp = out.str();

	std::vector<char> _output(tmp.begin(), tmp.end());
	_output.push_back('\0');

	char* output = (char*)LocalAlloc(LMEM_FIXED, _output.size() + 1);
	if (!output)
		return NULL;

	memcpy(output, _output.data(), _output.size() + 1);
	return output;
}

// Loads all required emulator settings sent by the hook DLL before loading the savestate
void Vanguard_loadEmuSettings(BSTR settings)
{
	JsonParser::JsonValue parsed_settings = JsonParser::ParseJson(settings);

	VanguardSettings _settings;
	_settings.LoadSettings(parsed_settings);
}

//converts a BSTR received from the Vanguard client to std::string
std::string BSTRToString(BSTR string)
{
	std::wstring ws(string, SysStringLen(string));
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string converted_string = converter.to_bytes(ws);
	return converted_string;
}

std::string getDirectory()
{
  char buffer[MAX_PATH] = {0};
  GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  return std::string(buffer).substr(0, pos);
}

// formats the saved settings into a JSON format
void FormatJsonData(VanguardSettings& settings, std::ostringstream& json_string)
{
	// beginning of json string
	json_string << "{\n";

	// iterate through all settings
	for (int i = 0; i < settings.array.size(); i++)
	{
		std::string value(settings.to_string(settings.array[i].second));

		json_string << "  \"" << settings.array[i].first
			<< "\": " << value;

		// if the value is a whole number float, add ".0" so the parser understands
		if (std::holds_alternative<float>(settings.array[i].second) &&
			value.length() == 1)
		{
			json_string << ".0";
		}

		// only add a comma if there are more values to be parsed
		if (i + 1 < settings.array.size())
			json_string << ",\n";
		else
			json_string << "\n";
	}

	// end of json string
	json_string << "}";
}
