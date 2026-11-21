// ============================================================================
// CannonCruise - Level script (LevelScript.cpp) -- see LevelScript.h
// ============================================================================

#include "LevelScript.h"

#include "LocalizationManager.h"
#include "RenderPlatform.h"
#include "RwsResources.h"
#include "rpworld.h"
#include "RwStudioStream.h"
#include "ShowText.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/csystemcommands.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <set>
#include <string>
#include <vector>

namespace {

struct ShowTextRec {
    RwInt32 textId;
    RwReal duration; // seconds; 0 = wait for NEXT
    std::string startEvent, endEvent;
};
struct IndicatorRec {
    RwReal x, z;
    std::string startEvent, endEvent;
    RwBool active;
    RwUInt32 activatedAt; // ordering: the latest active one is the goal
};
struct CollisionTriggerRec {
    RwReal x, z;
    RwBool hasBox;               // volume from the trigger's hull model
    RwReal minX, maxX, minZ, maxZ;
    std::string armEvent, disarmEvent;
    std::string enterEvent, exitEvent, sendEvent;
    RwBool armed, inside;
};
struct EventTriggerRec {
    RwInt32 needed;
    std::string countEvent, armEvent, disarmEvent, sendEvent;
    RwBool armed;
    RwInt32 count;
};
struct FadeRec {
    std::string inEvent, outEvent;
};
struct LoadLevelRec {
    std::string event, level;
};
struct PendingSend {
    RwReal delay;
    std::string event;
};
struct ActiveText {
    const ShowTextRec *pRec;
    RwReal elapsed;
};

std::vector<ShowTextRec> s_texts;
std::vector<IndicatorRec> s_indicators;
std::vector<CollisionTriggerRec> s_collTriggers;
std::vector<EventTriggerRec> s_eventTriggers;
std::vector<FadeRec> s_fades;
std::vector<LoadLevelRec> s_loadLevels;

std::deque<std::string> s_eventQueue;
std::set<std::string> s_sent;
std::vector<PendingSend> s_pending;
std::deque<const ShowTextRec *> s_textQueue;
ActiveText s_current = {nullptr, 0.0f};
RwUInt32 s_activationCounter = 0;
std::set<std::string> s_producers; // events game code sends
std::string s_exitLevel;
RwBool s_exitReached = FALSE;

// Collision trigger volumes are separate hull models; this radius stands in
// for them (the tutorial goals are buoys/light towers a ship sails up to).
const RwReal kTriggerRadius = 30.0f;
const RwReal kFadeSeconds = 1.0f;
const RwReal kGoalReachRadius = 18.0f;

RwBool IsProduced(const std::string &ev) {
    if (s_producers.count(ev) > 0) return TRUE;
    for (size_t i = 0; i < s_texts.size(); ++i)
        if (s_texts[i].endEvent == ev) return TRUE;
    for (size_t i = 0; i < s_collTriggers.size(); ++i) {
        const CollisionTriggerRec &r = s_collTriggers[i];
        if (r.enterEvent == ev || r.exitEvent == ev || r.sendEvent == ev) return TRUE;
    }
    for (size_t i = 0; i < s_eventTriggers.size(); ++i)
        if (s_eventTriggers[i].sendEvent == ev) return TRUE;
    for (size_t i = 0; i < s_fades.size(); ++i)
        if (s_fades[i].outEvent == ev) return TRUE;
    return FALSE;
}

std::string ReadString(const RWS::CAttributePacket &packet, const char *pClass,
                       RwUInt32 id) {
    for (RWS::CAttributeCommandIterator it(packet, pClass); !it.IsFinished(); ++it) {
        if (it->GetCommandId() != id || it->GetDataSize() <= 1) {
            continue;
        }
        const RwUInt8 *p = static_cast<const RwUInt8 *>(it->GetCommandData());
        std::string s;
        for (RwUInt32 i = 0; i < it->GetDataSize(); ++i) {
            if (p[i] < 0x20 || p[i] >= 0x7F) {
                break;
            }
            s += (char)p[i];
        }
        return s;
    }
    return std::string();
}

RwBool ReadU32(const RWS::CAttributePacket &packet, const char *pClass, RwUInt32 id,
               RwUInt32 *pOut) {
    for (RWS::CAttributeCommandIterator it(packet, pClass); !it.IsFinished(); ++it) {
        if (it->GetCommandId() == id && it->GetDataSize() == 4) {
            std::memcpy(pOut, it->GetCommandData(), 4);
            return TRUE;
        }
    }
    return FALSE;
}

RwBool ReadPos(const RWS::CAttributePacket &packet, RwReal *pX, RwReal *pZ) {
    RwReal m[16];
    if (RWS::CSystemCommands::ExtractMatrix(packet, m) != TRUE) {
        return FALSE;
    }
    *pX = m[12];
    *pZ = m[14];
    return TRUE;
}

// World-space XZ box of the trigger's hull model (first clump resource the
// record references), so the volume matches the level design.
void ReadTriggerBox(const RWS::CAttributePacket &packet, CollisionTriggerRec *pRec) {
    RwReal m[16];
    if (RWS::CSystemCommands::ExtractMatrix(packet, m) != TRUE) {
        return;
    }
    for (RWS::CAttributeCommandIterator it(packet, "CSystemCommands"); !it.IsFinished(); ++it) {
        if (it->GetCommandId() != 0 || it->GetDataSize() != 16) {
            continue;
        }
        const RwsResource *pRes =
            RwsResources_Find(static_cast<const RwUInt8 *>(it->GetCommandData()));
        if (pRes == nullptr || pRes->embeddedType != rwsRESOURCE_CLUMP) {
            continue;
        }
        RpClump *pClump = RpClumpStreamReadMemory(pRes->pData, pRes->size);
        if (pClump == nullptr) {
            continue;
        }
        RwV3d bmin, bmax;
        if (RpClumpPortGetLocalBounds(pClump, &bmin, &bmax) == TRUE) {
            RwBool first = TRUE;
            for (int c = 0; c < 8; ++c) {
                const RwReal lx = (c & 1) ? bmax.x : bmin.x;
                const RwReal ly = (c & 2) ? bmax.y : bmin.y;
                const RwReal lz = (c & 4) ? bmax.z : bmin.z;
                const RwReal wx = lx * m[0] + ly * m[4] + lz * m[8] + m[12];
                const RwReal wz = lx * m[2] + ly * m[6] + lz * m[10] + m[14];
                if (first == TRUE || wx < pRec->minX) pRec->minX = wx;
                if (first == TRUE || wx > pRec->maxX) pRec->maxX = wx;
                if (first == TRUE || wz < pRec->minZ) pRec->minZ = wz;
                if (first == TRUE || wz > pRec->maxZ) pRec->maxZ = wz;
                first = FALSE;
            }
            pRec->hasBox = TRUE;
        }
        RpClumpDestroy(pClump);
        return;
    }
}

void SendNow(const std::string &ev) {
    if (!ev.empty()) {
        s_eventQueue.push_back(ev);
    }
}

void FinishCurrentText() {
    if (s_current.pRec == nullptr) {
        return;
    }
    const std::string endEvent = s_current.pRec->endEvent;
    s_current.pRec = nullptr;
    SendNow(endEvent);
}

void ProcessEvent(const std::string &ev) {
    s_sent.insert(ev);
    std::printf("LevelScript: event %s\n", ev.c_str());
    for (size_t i = 0; i < s_texts.size(); ++i) {
        if (s_texts[i].startEvent == ev) {
            s_textQueue.push_back(&s_texts[i]);
        }
    }
    for (size_t i = 0; i < s_indicators.size(); ++i) {
        IndicatorRec &r = s_indicators[i];
        if (r.endEvent == ev) {
            r.active = FALSE;
        }
        if (r.startEvent == ev) {
            r.active = TRUE;
            r.activatedAt = ++s_activationCounter;
        }
    }
    for (size_t i = 0; i < s_collTriggers.size(); ++i) {
        CollisionTriggerRec &r = s_collTriggers[i];
        if (r.disarmEvent == ev) {
            r.armed = FALSE;
        }
        if (r.armEvent == ev) {
            r.armed = TRUE;
            r.inside = FALSE;
        }
    }
    for (size_t i = 0; i < s_eventTriggers.size(); ++i) {
        EventTriggerRec &r = s_eventTriggers[i];
        if (r.armEvent == ev) {
            r.armed = TRUE;
            r.count = 0;
        }
        if (r.armed == TRUE && r.countEvent == ev && ++r.count >= r.needed) {
            r.armed = FALSE;
            SendNow(r.sendEvent);
        }
        if (r.disarmEvent == ev) {
            r.armed = FALSE;
        }
    }
    for (size_t i = 0; i < s_fades.size(); ++i) {
        if (s_fades[i].inEvent == ev && !s_fades[i].outEvent.empty()) {
            PendingSend p = {kFadeSeconds, s_fades[i].outEvent};
            s_pending.push_back(p);
        }
    }
    for (size_t i = 0; i < s_loadLevels.size(); ++i) {
        const LoadLevelRec &r = s_loadLevels[i];
        if (r.event == ev && r.event != "iMsgPlayerDead") {
            s_exitLevel = r.level;
            s_exitReached = TRUE;
            std::printf("LevelScript: level exit -> %s\n", r.level.c_str());
        }
    }
}

// Localized text as printable ASCII lines for the bitmap font.
// '|' breaks a line, the "<currency> NEXT" tail is dropped (shown as a
// prompt instead), the fire-button glyph reads SPACE.
std::vector<std::string> TextLines(RwInt32 textId, RwBool *pWantsNext) {
    std::string raw;
    char key[16];
    std::snprintf(key, sizeof(key), "%d", (int)textId);
    CLocalizationManager *pLoc = CLocalizationManager::GetSingletonPtr();
    const wchar_t *w = pLoc != nullptr ? pLoc->GetString(key) : nullptr;
    *pWantsNext = FALSE;
    if (w != nullptr) {
        for (; *w; ++w) {
            const wchar_t c = *w;
            if (c == 0x00A4) { // currency sign: "NEXT" prompt follows
                *pWantsNext = TRUE;
                break;
            }
            if (c == 0x00AB) {
                raw += "SPACE";
            } else if (c == L'|') {
                raw += '\n';
            } else if (c >= 0x20 && c < 0x7F) {
                raw += (char)c;
            }
        }
    } else {
        raw = std::string("TEXT ") + key;
    }
    // Word wrap.
    const size_t kMaxChars = 44;
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= raw.size()) {
        size_t nl = raw.find('\n', start);
        std::string para = raw.substr(start, nl == std::string::npos ? std::string::npos
                                                                      : nl - start);
        while (!para.empty() && para[0] == ' ') para.erase(0, 1);
        while (para.size() > kMaxChars) {
            size_t cut = para.rfind(' ', kMaxChars);
            if (cut == std::string::npos || cut == 0) cut = kMaxChars;
            lines.push_back(para.substr(0, cut));
            para.erase(0, cut);
            while (!para.empty() && para[0] == ' ') para.erase(0, 1);
        }
        if (!para.empty()) lines.push_back(para);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return lines;
}

} // namespace

void LevelScript_Reset(void) {
    s_texts.clear();
    s_indicators.clear();
    s_collTriggers.clear();
    s_eventTriggers.clear();
    s_fades.clear();
    s_loadLevels.clear();
    s_eventQueue.clear();
    s_sent.clear();
    s_pending.clear();
    s_textQueue.clear();
    s_current.pRec = nullptr;
    s_current.elapsed = 0.0f;
    s_activationCounter = 0;
    s_exitLevel.clear();
    s_exitReached = FALSE;
    s_producers.clear();
}

void LevelScript_RegisterProducer(const char *pEvent) {
    if (pEvent != nullptr && pEvent[0] != '\0') {
        s_producers.insert(pEvent);
    }
}

void LevelScript_CollectObject(const RwStudioObject *pObject,
                               const RWS::CAttributePacket &packet) {
    if (pObject == nullptr || pObject->behaviourCount == 0) {
        return;
    }
    const char *cls = pObject->behaviours[0].className;
    if (std::strcmp(cls, "CShowText") == 0) {
        ShowTextRec r;
        RwUInt32 id = 0, dur = 0;
        ReadU32(packet, "CShowText", 0, &id);
        ReadU32(packet, "CShowText", 1, &dur);
        r.textId = (RwInt32)id;
        std::memcpy(&r.duration, &dur, 4);
        if (!(r.duration >= 0.0f) || r.duration > 600.0f) r.duration = 0.0f;
        r.startEvent = ReadString(packet, "CShowText", 6);
        r.endEvent = ReadString(packet, "CShowText", 7);
        if (!r.startEvent.empty()) s_texts.push_back(r);
    } else if (std::strcmp(cls, "CMissionIndicator") == 0) {
        IndicatorRec r;
        r.x = r.z = 0.0f;
        ReadPos(packet, &r.x, &r.z);
        r.startEvent = ReadString(packet, "CMissionIndicator", 1);
        r.endEvent = ReadString(packet, "CMissionIndicator", 2);
        r.active = FALSE;
        r.activatedAt = 0;
        s_indicators.push_back(r);
    } else if (std::strcmp(cls, "CCollisionTrigger") == 0) {
        CollisionTriggerRec r;
        r.x = r.z = 0.0f;
        if (ReadPos(packet, &r.x, &r.z) != TRUE) return;
        r.hasBox = FALSE;
        r.minX = r.maxX = r.minZ = r.maxZ = 0.0f;
        ReadTriggerBox(packet, &r);
        r.enterEvent = ReadString(packet, "CCollisionTrigger", 1);
        r.exitEvent = ReadString(packet, "CCollisionTrigger", 3);
        r.armEvent = ReadString(packet, "CTrigger", 0);
        r.disarmEvent = ReadString(packet, "CTrigger", 2);
        r.sendEvent = ReadString(packet, "CTrigger", 4);
        r.armed = r.armEvent.empty() ? TRUE : FALSE;
        r.inside = FALSE;
        s_collTriggers.push_back(r);
    } else if (std::strcmp(cls, "CEventTrigger") == 0) {
        EventTriggerRec r;
        RwUInt32 n = 1;
        ReadU32(packet, "CEventTrigger", 0, &n);
        r.needed = (n == 0 || n > 1000) ? 1 : (RwInt32)n;
        r.countEvent = ReadString(packet, "CEventTrigger", 1);
        r.armEvent = ReadString(packet, "CTrigger", 0);
        r.disarmEvent = ReadString(packet, "CTrigger", 2);
        r.sendEvent = ReadString(packet, "CTrigger", 4);
        r.armed = r.armEvent.empty() ? TRUE : FALSE;
        r.count = 0;
        if (!r.countEvent.empty()) s_eventTriggers.push_back(r);
    } else if (std::strcmp(cls, "CFade") == 0) {
        FadeRec r;
        r.inEvent = ReadString(packet, "CFade", 2);
        r.outEvent = ReadString(packet, "CFade", 3);
        if (!r.inEvent.empty()) s_fades.push_back(r);
    } else if (std::strcmp(cls, "CLoadLevel") == 0) {
        LoadLevelRec r;
        r.event = ReadString(packet, "CLoadLevel", 0);
        r.level = ReadString(packet, "CLoadLevel", 1);
        if (!r.event.empty()) s_loadLevels.push_back(r);
    }
}

void LevelScript_DumpTriggers(void) {
    for (size_t i = 0; i < s_collTriggers.size(); ++i) {
        const CollisionTriggerRec &r = s_collTriggers[i];
        std::printf("LevelScript: trigger at (%.0f,%.0f) box=%d x[%.0f..%.0f] z[%.0f..%.0f] enter=%s send=%s\n",
                    r.x, r.z, (int)r.hasBox, r.minX, r.maxX, r.minZ, r.maxZ,
                    r.enterEvent.c_str(), r.sendEvent.c_str());
    }
}

RwUInt32 LevelScript_RecordCount(void) {
    return (RwUInt32)(s_texts.size() + s_indicators.size() + s_collTriggers.size() +
                      s_eventTriggers.size() + s_fades.size() + s_loadLevels.size());
}

RwBool LevelScript_HasLevelExit(void) {
    for (size_t i = 0; i < s_loadLevels.size(); ++i) {
        if (s_loadLevels[i].event != "iMsgPlayerDead") {
            return TRUE;
        }
    }
    return FALSE;
}

void LevelScript_Start(void) { SendNow("iMsgPostSceneCreate"); }

void LevelScript_Send(const char *pEvent) {
    if (pEvent != nullptr && pEvent[0] != '\0') {
        SendNow(pEvent);
    }
}

RwBool LevelScript_WasSent(const char *pEvent) {
    return (pEvent != nullptr && s_sent.count(pEvent) > 0) ? TRUE : FALSE;
}

void LevelScript_Update(RwReal dt, RwReal shipX, RwReal shipZ, RwBool nextPressed) {
    // CANNONCRUISE_AUTONEXT=seconds: confirm NEXT texts automatically (tests).
    static const char *s_autoNextEnv = std::getenv("CANNONCRUISE_AUTONEXT");
    static const RwReal s_autoNext = s_autoNextEnv ? (RwReal)std::atof(s_autoNextEnv) : 0.0f;

    for (size_t i = 0; i < s_pending.size();) {
        s_pending[i].delay -= dt;
        if (s_pending[i].delay <= 0.0f) {
            SendNow(s_pending[i].event);
            s_pending.erase(s_pending.begin() + (std::ptrdiff_t)i);
        } else {
            ++i;
        }
    }
    for (size_t i = 0; i < s_collTriggers.size(); ++i) {
        CollisionTriggerRec &r = s_collTriggers[i];
        RwBool inside = FALSE;
        if (r.hasBox == TRUE) {
            const RwReal kSlack = 3.0f; // ship hull half-length
            inside = (shipX > r.minX - kSlack && shipX < r.maxX + kSlack &&
                      shipZ > r.minZ - kSlack && shipZ < r.maxZ + kSlack) ? TRUE : FALSE;
        } else {
            const RwReal dx = shipX - r.x, dz = shipZ - r.z;
            inside = (dx * dx + dz * dz < kTriggerRadius * kTriggerRadius) ? TRUE : FALSE;
        }
        if (r.armed == TRUE && inside == TRUE && r.inside != TRUE) {
            SendNow(r.enterEvent);
            SendNow(r.sendEvent);
        } else if (r.armed == TRUE && inside != TRUE && r.inside == TRUE) {
            SendNow(r.exitEvent);
        }
        r.inside = inside;
    }
    // Goal markers nothing else completes: reached by sailing up to them.
    for (size_t i = 0; i < s_indicators.size(); ++i) {
        IndicatorRec &r = s_indicators[i];
        if (r.active != TRUE || r.endEvent.empty() || IsProduced(r.endEvent) == TRUE) {
            continue;
        }
        const RwReal dx = shipX - r.x, dz = shipZ - r.z;
        if (dx * dx + dz * dz < kGoalReachRadius * kGoalReachRadius) {
            std::printf("LevelScript: goal reached -> %s\n", r.endEvent.c_str());
            SendNow(r.endEvent);
            r.active = FALSE;
        }
    }
    // Current text.
    if (s_current.pRec != nullptr) {
        s_current.elapsed += dt;
        const RwReal dur = s_current.pRec->duration;
        if ((dur > 0.0f && s_current.elapsed >= dur) ||
            (dur <= 0.0f && (nextPressed == TRUE ||
                             (s_autoNext > 0.0f && s_current.elapsed >= s_autoNext))) ||
            (dur > 0.0f && nextPressed == TRUE && s_current.elapsed > 0.3f)) {
            FinishCurrentText();
        }
    }
    // Events (bounded: a misbehaving chain cannot hang the frame).
    for (int guard = 0; guard < 256 && !s_eventQueue.empty(); ++guard) {
        const std::string ev = s_eventQueue.front();
        s_eventQueue.pop_front();
        ProcessEvent(ev);
    }
    if (s_current.pRec == nullptr && !s_textQueue.empty()) {
        s_current.pRec = s_textQueue.front();
        s_current.elapsed = 0.0f;
        s_textQueue.pop_front();
        std::printf("LevelScript: text %d\n", (int)s_current.pRec->textId);
    }
}

void LevelScript_Render(void) {
    if (s_current.pRec == nullptr) {
        return;
    }
    RwInt32 W = 1280, H = 720;
    RenderPlatform_GetScreenSize(&W, &H);
    RwBool wantsNext = FALSE;
    const std::vector<std::string> lines = TextLines(s_current.pRec->textId, &wantsNext);
    const RwBool showNext = (wantsNext == TRUE || s_current.pRec->duration <= 0.0f) ? TRUE : FALSE;
    const int scale = H >= 600 ? 3 : 2;
    const RwReal glyph = 6.0f * scale;
    const RwReal lineH = 9.0f * scale;
    size_t maxLen = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].size() > maxLen) maxLen = lines[i].size();
    }
    const RwReal pad = 14.0f;
    const RwReal boxW = (RwReal)maxLen * glyph + pad * 2.0f;
    const RwReal boxH = (RwReal)(lines.size() + (showNext ? 1 : 0)) * lineH + pad * 2.0f;
    const RwReal boxX = ((RwReal)W - boxW) * 0.5f;
    const RwReal boxY = (RwReal)H * 0.10f;
    RenderPlatform_BeginOverlay2D();
    RenderPlatform_DrawOverlayQuad(boxX - 3.0f, boxY - 3.0f, boxW + 6.0f, boxH + 6.0f, 0xE0D8B060);
    RenderPlatform_DrawOverlayQuad(boxX, boxY, boxW, boxH, 0xE0281A0C);
    for (size_t i = 0; i < lines.size(); ++i) {
        const RwReal lx = boxX + (boxW - (RwReal)lines[i].size() * glyph) * 0.5f;
        ShowText_DrawBitmapText(lines[i].c_str(), lx, boxY + pad + (RwReal)i * lineH, scale,
                                255, 236, 180, 255);
    }
    if (showNext == TRUE) {
        const char *prompt = "ENTER: NEXT";
        const RwReal px = boxX + boxW - pad - (RwReal)std::strlen(prompt) * 6.0f * 2;
        ShowText_DrawBitmapText(prompt, px, boxY + pad + (RwReal)lines.size() * lineH + 4.0f, 2,
                                210, 190, 120, 255);
    }
    RenderPlatform_EndOverlay2D();
}

RwBool LevelScript_GetGoal(RwReal *pX, RwReal *pZ) {
    const IndicatorRec *pBest = nullptr;
    for (size_t i = 0; i < s_indicators.size(); ++i) {
        if (s_indicators[i].active == TRUE &&
            (pBest == nullptr || s_indicators[i].activatedAt > pBest->activatedAt)) {
            pBest = &s_indicators[i];
        }
    }
    if (pBest == nullptr) {
        return FALSE;
    }
    if (pX) *pX = pBest->x;
    if (pZ) *pZ = pBest->z;
    return TRUE;
}

RwBool LevelScript_LevelExitReached(char *pNext, RwUInt32 nextSize) {
    if (s_exitReached != TRUE) {
        return FALSE;
    }
    if (pNext != nullptr && nextSize > 0) {
        std::snprintf(pNext, nextSize, "%s", s_exitLevel.c_str());
    }
    return TRUE;
}
