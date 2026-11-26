#ifndef CANNONCRUISE_SFXMIXER_H
#define CANNONCRUISE_SFXMIXER_H

// ============================================================================
// CannonCruise - Sound effect mixer (SfxMixer.h)
//
// DirectSound voices for sound effects: wav files are loaded once and kept in
// memory, and any number of effects play at the same time. (Win32
// PlaySoundA, used before, plays one sound at a time: each new effect cut the
// previous one and every play re-read the file from disk.)
// ============================================================================

/// Plays a PCM .wav file at volume 0..1, pan -1 (left) .. +1 (right).
/// Returns false if the mixer is not
/// available or the file cannot be played (the caller may fall back).
bool SfxMixer_Play(const char *pPath, float volume, float pan = 0.0f);

/// Stops and releases every playing voice.
void SfxMixer_StopAll(void);

/// Releases DirectSound and the cached samples.
void SfxMixer_Shutdown(void);

#endif // CANNONCRUISE_SFXMIXER_H
