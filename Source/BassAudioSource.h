//BassAudioSource.h

#include <../Include/bass.h>

extern HMODULE HInstance;

struct BassExtension {
	LPCWSTR Extension;
	LPCWSTR DLL;
};

extern BassExtension BassExtensions[];
extern const int BassExtensionsCount;

extern LPWSTR BassPlugins[];
extern const int BassPluginsCount;
