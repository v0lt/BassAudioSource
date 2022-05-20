//BassAudioSource.h

#include <../Include/bass.h>

extern HMODULE HInstance;

#define BASS_TFLAGS BASS_UNICODE

struct BassExtension {
public:
	LPCWSTR Extension;
	const bool IsMOD;
	LPCWSTR DLL;
};

extern BassExtension BASS_EXTENSIONS[];
extern const int BASS_EXTENSIONS_COUNT;

extern LPWSTR BASS_PLUGINS[];
extern const int BASS_PLUGINS_COUNT;

//------------------------------------------------------------------------------
// Define GUIDS used in this filter
//------------------------------------------------------------------------------
// {A351970E-4601-4BEC-93DE-CEE7AF64C636}
DEFINE_GUID(CLSID_BassAudioSource, 0xA351970E, 0x4601, 0x4BEC, 0x93, 0xDE, 0xCE, 0xE7, 0xAF, 0x64, 0xC6, 0x36);
#define STR_CLSID_BassAudioSource L"{A351970E-4601-4BEC-93DE-CEE7AF64C636}"

#define LABEL_BassAudioSource L"Bass Audio Source"

#define DIRECTSHOW_SOURCE_FILTER_PATH L"Media Type\\Extensions"
#define REGISTER_EXTENSION_FILE       L"Registration.ini"
