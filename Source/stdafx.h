// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <atlbase.h>
#include <atlwin.h>

#include "../external/BaseClasses/streams.h"
#include "../Include/IDSMResourceBag.h"

#include <algorithm>
#include <vector>
#include <string>
#include <format>
#include <filesystem>
