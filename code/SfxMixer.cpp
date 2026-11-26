// ============================================================================
// CannonCruise - Sound effect mixer (SfxMixer.cpp) -- see SfxMixer.h
// ============================================================================

#include "SfxMixer.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

struct Sample {
    WAVEFORMATEX format;
    std::vector<unsigned char> data;
};

IDirectSound8 *s_pDevice = nullptr;
bool s_triedInit = false;
std::map<std::string, Sample> s_samples;
std::vector<IDirectSoundBuffer *> s_voices;
const size_t kMaxVoices = 32;

bool EnsureDevice() {
    if (s_pDevice != nullptr) return true;
    if (s_triedInit) return false;
    s_triedInit = true;
    if (FAILED(DirectSoundCreate8(nullptr, &s_pDevice, nullptr)) || s_pDevice == nullptr) {
        s_pDevice = nullptr;
        std::printf("SfxMixer: DirectSound unavailable, using PlaySound\n");
        return false;
    }
    // Voices use DSBCAPS_GLOBALFOCUS, so the desktop window is enough here.
    if (FAILED(s_pDevice->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY))) {
        s_pDevice->Release();
        s_pDevice = nullptr;
        return false;
    }
    return true;
}

const Sample *LoadSample(const char *pPath) {
    std::map<std::string, Sample>::iterator it = s_samples.find(pPath);
    if (it != s_samples.end()) {
        return it->second.data.empty() ? nullptr : &it->second;
    }
    Sample &s = s_samples[pPath]; // cached even when it fails (no retries)
    FILE *f = std::fopen(pPath, "rb");
    if (f == nullptr) return nullptr;
    std::vector<unsigned char> d;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size > 12) {
        d.resize((size_t)size);
        if (std::fread(&d[0], 1, d.size(), f) != d.size()) d.clear();
    }
    std::fclose(f);
    if (d.size() < 12 || std::memcmp(&d[0], "RIFF", 4) != 0 || std::memcmp(&d[8], "WAVE", 4) != 0) {
        return nullptr;
    }
    bool haveFmt = false;
    size_t o = 12;
    while (o + 8 <= d.size()) {
        unsigned int len = 0;
        std::memcpy(&len, &d[o + 4], 4);
        const size_t body = o + 8;
        if (body + len > d.size()) len = (unsigned int)(d.size() - body);
        if (std::memcmp(&d[o], "fmt ", 4) == 0 && len >= 16) {
            std::memset(&s.format, 0, sizeof(s.format));
            std::memcpy(&s.format, &d[body], 16);
            haveFmt = s.format.wFormatTag == WAVE_FORMAT_PCM;
        } else if (std::memcmp(&d[o], "data", 4) == 0 && haveFmt) {
            s.data.assign(d.begin() + (std::ptrdiff_t)body, d.begin() + (std::ptrdiff_t)(body + len));
            break;
        }
        o = body + len + (len & 1);
    }
    return s.data.empty() ? nullptr : &s;
}

void ReapFinished() {
    for (size_t i = 0; i < s_voices.size();) {
        DWORD status = 0;
        if (FAILED(s_voices[i]->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0) {
            s_voices[i]->Release();
            s_voices.erase(s_voices.begin() + (std::ptrdiff_t)i);
        } else {
            ++i;
        }
    }
}

} // namespace

bool SfxMixer_Play(const char *pPath, float volume, float pan) {
    if (pPath == nullptr || !EnsureDevice()) return false;
    const Sample *pSample = LoadSample(pPath);
    if (pSample == nullptr) return false;
    ReapFinished();
    if (s_voices.size() >= kMaxVoices) {
        s_voices.front()->Stop();
        s_voices.front()->Release();
        s_voices.erase(s_voices.begin());
    }
    DSBUFFERDESC desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_GLOBALFOCUS;
    desc.dwBufferBytes = (DWORD)pSample->data.size();
    WAVEFORMATEX fmt = pSample->format;
    desc.lpwfxFormat = &fmt;
    IDirectSoundBuffer *pBuffer = nullptr;
    if (FAILED(s_pDevice->CreateSoundBuffer(&desc, &pBuffer, nullptr)) || pBuffer == nullptr) {
        return false;
    }
    void *p1 = nullptr, *p2 = nullptr;
    DWORD n1 = 0, n2 = 0;
    if (SUCCEEDED(pBuffer->Lock(0, desc.dwBufferBytes, &p1, &n1, &p2, &n2, 0))) {
        std::memcpy(p1, &pSample->data[0], n1);
        if (p2 != nullptr && n2 > 0) std::memcpy(p2, &pSample->data[n1], n2);
        pBuffer->Unlock(p1, n1, p2, n2);
    }
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    // DirectSound volume is attenuation in hundredths of a decibel.
    const LONG db = volume <= 0.0001f ? DSBVOLUME_MIN
                                      : (LONG)(2000.0f * std::log10(volume));
    pBuffer->SetVolume(db < DSBVOLUME_MIN ? DSBVOLUME_MIN : db);
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    pBuffer->SetPan((LONG)(pan * 2500.0f)); // up to 25 dB off the far side
    if (FAILED(pBuffer->Play(0, 0, 0))) {
        pBuffer->Release();
        return false;
    }
    s_voices.push_back(pBuffer);
    return true;
}

void SfxMixer_StopAll(void) {
    for (size_t i = 0; i < s_voices.size(); ++i) {
        s_voices[i]->Stop();
        s_voices[i]->Release();
    }
    s_voices.clear();
}

void SfxMixer_Shutdown(void) {
    SfxMixer_StopAll();
    s_samples.clear();
    if (s_pDevice != nullptr) {
        s_pDevice->Release();
        s_pDevice = nullptr;
    }
}

#else
bool SfxMixer_Play(const char *, float, float) { return false; }
void SfxMixer_StopAll(void) {}
void SfxMixer_Shutdown(void) {}
#endif
