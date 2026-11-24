#pragma once

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <Windows.h>  // GetModuleHandleW, VirtualProtect, PE headers

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

using namespace std::literals;

namespace logger = F4SE::log;

#include "Plugin.h"
