#pragma once
#include "UI/MainScreen.h"
#include "UI/EmuScreen.h"

class VanguardClientInitializer
{
public:
  static inline MainScreen* win = nullptr;
  static inline EmuScreen* winEmu = nullptr;
};
