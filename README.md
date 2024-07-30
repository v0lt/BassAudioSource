# Bass Audio Source

Bass Audio Source is a free and open-source DirectShow filter. The filter allows you to play audio files supported by the BASS library in media players with DirectShow support.


## Installation

1. Download ZIP file: BassAudioSource.zip or BassAudioSource64.zip.

2. Unpack the ZIP file into a separate folder.

3. Run Install_BassAudioSource_32.cmd or Install_BassAudioSource_64.cmd as administrator.

4. If you want to use a filter for all supported files and streams, then add BassAudioSource.ax or BassAudioSource64.ax to the list of external filters of MPC-BE, MPC-HC or other DirectShow player and set "prefer".

5. Open the audio file in the DirectShow player.


## MIDI support

To play MIDI files, you need to copy the SoundFont file (.sf2, .sfz) to the folder with the installed BassAudioSource and select it in the settings.

PS: Links to a couple of SF2 files are available in the [BASSMIDI plugin description](https://www.un4seen.com/bass.html#addons).


## Links

BASS audio library and Add-ons - http://www.un4seen.com/bass.html

BASSZXTUNE - chiptune support for BASS - http://www.un4seen.com/forum/?topic=15694

DC-Bass Source Mod - https://rwijnsma.home.xs4all.nl/dc-bsm/index.htm


Bass Audio Source was based on the code of the following projects:

DC-Bass Source filter - http://www.dsp-worx.de/index.php?n=15

DC-Bass Source Filter C++ porting - https://github.com/frafv/DCBassSource


These components are provided "AS IS". Use at own risk.