// ============================================================================
// CannonCruise - Rt2dMaestro real implementation (Rt2dMaestro.cpp)
//
// Old: rwstubs.cpp no-op (Rt2dMaestroDestroy/Update/Render) -- removed.
// New: parsed stream + selection state + Im2D placeholder render.
// ============================================================================

#include "Rt2dMaestro.h"
#include "RenderPlatform.h"
#include "ShowText.h"

#include <rw.h> // RwIm2DVertexSetScreenX -- C API (same as RenderPlatform.cpp)

#include <cstring>

// NOTE: Im2D draws must be in pixel coordinates (CShowText::Render pattern).
// The old matrix-based DrawQuad produced a view-window (-1..1) -> ~1 pixel
// triangle (invisible). RenderPlatform_DrawOverlayQuad is used now.

Rt2dMaestro *Rt2dMaestroCreate(void)
{
    return nullptr; // no streamless create (the binary also requires a stream)
}

Rt2dMaestro *Rt2dMaestroCreateFromMemory(const void *pData, RwUInt32 len)
{
    if (pData == nullptr) {
        return nullptr;
    }
    Rt2dMaestro *p = new Rt2dMaestro;
    std::memset(p, 0, sizeof(*p));
    if (Rt2dMaestro_Inspect(pData, len, &p->info) != TRUE) {
        delete p;
        return nullptr;
    }
    // gather command indices (menu items)
    for (RwUInt32 i = 0; i < p->info.nodeCount; ++i) {
        if (p->info.nodes[i].type == rwrt2dNODE_COMMAND) {
            p->commands[p->commandCount++] = (RwInt32)i;
        }
    }
    p->selection = 0;
    p->time = 0.0f;
    return p;
}

RwBool Rt2dMaestroDestroy(Rt2dMaestro *maestro)
{
    delete maestro;
    return TRUE;
}

void Rt2dMaestroUpdate(Rt2dMaestro *maestro, RwReal deltaTime)
{
    if (maestro != nullptr) {
        maestro->time += deltaTime;
    }
}

// Im2D overlay: dark background + title + command list (readable labels with
// the embedded 5x7 bitmap font -- real menu visuals {GUID} txd/dff are absent).
// All drawing is in PIXEL coordinates (under RenderPlatform_BeginOverlay2D).
void Rt2dMaestroRender(Rt2dMaestro *maestro)
{
    if (maestro == nullptr) {
        return;
    }
    // screen pixel -- RenderPlatform_Initialize(1280, 720)
    const RwReal kScreenW = 1280.0f;
    const RwReal kScreenH = 720.0f;
    const int scale = 3;

    // Im2D render states -- the 3D scene wrote depth; ZTEST must be off or they get culled
    RenderPlatform_BeginOverlay2D();

    // background (dark blue-black)
    RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, kScreenW, kScreenH,
                                   0xFF0C1420);
    // title bar
    RenderPlatform_DrawOverlayQuad(kScreenW * 0.025f, kScreenH * 0.045f,
                                   kScreenW * 0.95f, kScreenH * 0.12f,
                                   0xFF204068);

    // title text
    ShowText_DrawBitmapText("CANNONCRUISE MAINMENU", kScreenW * 0.05f,
                            kScreenH * 0.06f, scale, 220, 230, 255, 255);

    // command list -- selection bar + label
    const RwInt32 kVisible = 12; // number of rows fitting on screen
    const RwInt32 total = maestro->commandCount;
    const RwInt32 sel = (total > 0) ? maestro->selection : 0;
    RwInt32 first = 0;
    if (total > kVisible) {
        first = sel - kVisible / 2;
        if (first < 0) {
            first = 0;
        }
        if (first > total - kVisible) {
            first = total - kVisible;
        }
    }
    const RwReal rowH = kScreenH * 0.055f;
    const RwReal listX = kScreenW * 0.08f;
    const RwReal listY = kScreenH * 0.22f;
    for (RwInt32 i = 0; i < kVisible && first + i < total; ++i) {
        const RwInt32 ci = first + i;
        const RwReal y = listY + (RwReal)i * rowH;
        const RwBool active = (ci == sel) ? TRUE : FALSE;
        // selection/label background (pixel-coordinate row band)
        RenderPlatform_DrawOverlayQuad(listX, y - rowH * 0.1f,
                                       kScreenW * 0.84f, rowH * 0.7f,
                                       active ? 0xFFC89030u : 0xFF182838u);
        const char *name =
            maestro->info.nodes[maestro->commands[ci]].name;
        ShowText_DrawBitmapText(name, listX + kScreenW * 0.02f,
                                y, scale,
                                active ? 20 : 200,
                                active ? 20 : 210,
                                active ? 20 : 230, 255);
    }
    // total / hint
    char status[96];
    std::snprintf(status, sizeof(status),
                  "SELECTED: %s   [%d/%d]  W/S=MOVE ENTER=OK ESC=QUIT",
                  (total > 0)
                      ? maestro->info.nodes[maestro->commands[sel]].name
                      : "-",
                  (int)sel + 1, (int)total);
    ShowText_DrawBitmapText(status, kScreenW * 0.05f, kScreenH * 0.93f, 2,
                            180, 190, 200, 255);

    RenderPlatform_EndOverlay2D();
}

RwBool Rt2dMaestroSelectNext(Rt2dMaestro *maestro)
{
    if (maestro == nullptr || maestro->commandCount == 0) {
        return FALSE;
    }
    maestro->selection = (maestro->selection + 1) % maestro->commandCount;
    return TRUE;
}

RwBool Rt2dMaestroSelectPrev(Rt2dMaestro *maestro)
{
    if (maestro == nullptr || maestro->commandCount == 0) {
        return FALSE;
    }
    maestro->selection =
        (maestro->selection + maestro->commandCount - 1) % maestro->commandCount;
    return TRUE;
}

const char *Rt2dMaestroGetSelectedCommand(const Rt2dMaestro *maestro)
{
    if (maestro == nullptr || maestro->commandCount == 0) {
        return nullptr;
    }
    return maestro->info.nodes[maestro->commands[maestro->selection]].name;
}
