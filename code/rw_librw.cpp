// ============================================================================
// CannonCruise - librw Engine/Stream Shim (rw_librw.cpp)
// Original path: D:\Projects\CannonCruisePC\code\rw_librw.cpp
//
// REAL implementation of the NO-OP engine/stream symbols in rwstubs.cpp:
// the game's C API (Rw*/Rp* — RW SDK naming) connects to librw's C++ API
// (rw::/rp::). First step of Gate 4: real .RWS asset streaming.
//
// Binary sources: FUN_0050b640 RwEngineInit, FUN_00422960 RwEngineOpen,
// FUN_0050b340 RwEngineStart, FUN_0050b2f0 RwEngineClose, FUN_0050b3d0
// RwEngineStop, FUN_0050b600 RwEngineTerm; RwStream* family + FindChunk;
// RpClumpStreamRead; RwTexDictionaryStreamRead.
//
// DEVIATIONS (documented):
//   - NULL platform: no device (headless); Engine::open(nil) does not accept parameters
//     (device DEVICEOPEN no-op).
//   - D3D9: the device target is a Win32 window; EngineOpenParams{ HWND }
//     window = produced with CCD3DCreateWindow(). NULL's Engine::open(nil)
//     leads to 0xC0000005 (2026-10-15) on this platform → also, RW_KEY_* (GLFW key
//     codes) are mapped to Win32 VKs (GLFWKeyToVK).
//   - RpWorldStreamRead: librw has NO World streamRead (BSP) → stub 0.
//   - RwMemoryFunctions mapping: only default memfuncs are used
//     (non-null parameter is ignored).
//   - RwEngineSetConfig: no librw counterpart → TRUE (no-op).
// ============================================================================

#ifdef RW_D3D9
// D3D9: librw's d3d/rwd3d.h EngineOpenParams{ HWND window; } definition is only
// given with the HWND field if <windows.h> was included BEFORE (_WINDOWS_ guard);
// rw.h includes it through src/d3d/rwd3d.h → this include
// must come BEFORE rw.h. Otherwise the window field becomes
// "please_include_windows_h" (uint32) and cannot be resolved as HWND.
#include <windows.h>
#include <d3d9.h>
#endif

#include "rwcore.h"
#include "rpworld.h"

#include <rw.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
    // RwStream wrapper — librw StreamFile ownership
    struct ShimStream {
        rw::StreamFile file;
    };

    RwBool s_engineRunning = FALSE; // RwEngineIsRunning state
#ifdef RW_GL3
    GLFWwindow *s_glfwWindow = nullptr; // created by Engine::start
    double s_lastMouseX = 0.0;          // mouse delta calibration
    double s_lastMouseY = 0.0;
    RwBool s_mouseCalibrated = FALSE;
#endif

#ifdef RW_D3D9
    RwBool HasD3D9HalAdapter() {
        IDirect3D9 *pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (pD3D == nullptr) {
            std::fprintf(stderr,
                         "CannonCruise: Direct3DCreate9 failed; D3D9 startup "
                         "cannot continue\n");
            return FALSE;
        }

        RwBool hasHalAdapter = FALSE;
        const UINT adapterCount = pD3D->GetAdapterCount();
        HMODULE d3dModule = GetModuleHandleA("d3d9.dll");
        char d3dPath[MAX_PATH] = {};
        if (d3dModule != nullptr) {
            GetModuleFileNameA(d3dModule, d3dPath, MAX_PATH);
        }
        std::fprintf(stderr, "CannonCruise: D3D9 runtime '%s', adapters=%u\n",
                     d3dPath, adapterCount);
        for (UINT adapter = 0; adapter < adapterCount; ++adapter) {
            D3DCAPS9 caps = {};
            const HRESULT result =
                pD3D->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);
            if (result == D3D_OK) {
                hasHalAdapter = TRUE;
                break;
            }
            std::fprintf(stderr,
                         "CannonCruise: D3D9 adapter %u HAL caps failed "
                         "(HRESULT=0x%08lX)\n",
                         adapter, (unsigned long)result);
        }
        pD3D->Release();

        if (hasHalAdapter != TRUE) {
            std::fprintf(stderr,
                         "CannonCruise: no Direct3D9 HAL adapter is available; "
                         "D3D9 startup cannot continue\n");
        }
        return hasHalAdapter;
    }

    HWND s_hwnd = nullptr;     // D3D9 device target window
    RwBool s_winQuit = FALSE;  // WM_CLOSE/WM_DESTROY → game loop exits
    double s_lastMouseX = 0.0; // mouse delta calibration (same as GL3)
    double s_lastMouseY = 0.0;
    RwBool s_mouseCalibrated = FALSE;

    // GLFW key codes (RW_KEY_*) → Win32 virtual key code.
    // RW_KEY_ESCAPE=256, ENTER=257, RIGHT=262, LEFT=263, DOWN=264, UP=265
    int GLFWKeyToVK(int key)
    {
        switch (key) {
        case 256: return VK_ESCAPE;
        case 257: return VK_RETURN;
        case 262: return VK_RIGHT;
        case 263: return VK_LEFT;
        case 264: return VK_DOWN;
        case 265: return VK_UP;
        case 294: return VK_F5;
        case 295: return VK_F6;
        default:  return key; // ASCII 'A'-'Z' / ' ' → VK equivalent
        }
    }

    LRESULT CALLBACK CCD3DWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
    {
        switch (uMsg) {
        case WM_CLOSE:
            s_winQuit = TRUE; // MainLoop / RwPlatformShouldClose exits
            return 0;
        case WM_DESTROY:
            s_winQuit = TRUE;
            s_hwnd = nullptr;
            return 0;
        default:
            break;
        }
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }

    // Win32 window for the D3D9 device target (CS_OWNDC — DC persists).
    HWND CCD3DCreateWindow(int width, int height, const char *pTitle)
    {
        HINSTANCE hInst = GetModuleHandleA(nullptr);
        WNDCLASSA wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = CCD3DWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "CannonCruiseD3D9";
        RegisterClassA(&wc); // FALSE if the class is already registered — not a problem

        DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rc = {0, 0, width, height};
        AdjustWindowRect(&rc, style, FALSE);

        HWND hWnd = CreateWindowA(wc.lpszClassName, pTitle, style,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  rc.right - rc.left, rc.bottom - rc.top,
                                  nullptr, nullptr, hInst, nullptr);
        if (hWnd != nullptr) {
            ShowWindow(hWnd, SW_SHOW);
            UpdateWindow(hWnd);
        }
        return hWnd;
    }
#endif
}

// ---------------------------------------------------------------------------
// Engine lifecycle — rw::Engine::init/open/start/stop/close/term
// ---------------------------------------------------------------------------
RwBool RwEngineInit(RwMemoryFunctions *memFuncs, RwUInt32 flags,
                    RwUInt32 resEntrySize) {
    (void)memFuncs; // default memfuncs (see header deviation)
    (void)flags;
    (void)resEntrySize;
    if (rw::Engine::init() != TRUE) {
        return FALSE;
    }
    // Geometry MESH plugin — builds meshHeader from the streamRead EXTENSION;
    // GL3 instance/render crashes without meshHeader (there was no instance in NULL).
    rw::registerMeshPlugin();
    return TRUE;
}

RwBool RwEngineOpen(void) {
#ifdef RW_GL3
    // GL3: the gl3-glfw device describes the window with EngineOpenParams
    // (openGLFW -> glfwInit; the window is created in Engine::start).
    rw::EngineOpenParams params;
    params.window = &s_glfwWindow;
    params.width = 1280;
    params.height = 720;
    params.windowtitle = "CannonCruise";
    return rw::Engine::open(&params) ? TRUE : FALSE;
#elif defined(RW_D3D9)
    // D3D9: openD3D() resolves the params->window (HWND) pointer directly.
    // NULL platform's Engine::open(nil) was leading to a NULL pointer
    // dereference (0xC0000005 ACCESS_VIOLATION) on this platform → the window is
    // created here and the HWND is given to Engine::open.
    // librw's Engine::open currently ignores DEVICEOPEN's result, so reject
    // unsupported adapters here before Engine::start can dereference them.
    if (HasD3D9HalAdapter() != TRUE) {
        return FALSE;
    }
    if (s_hwnd == nullptr) {
        s_hwnd = CCD3DCreateWindow(640, 480, "CannonCruise");
        if (s_hwnd == nullptr) {
            return FALSE;
        }
    }
    rw::EngineOpenParams params;
    params.window = s_hwnd;
    return rw::Engine::open(&params) ? TRUE : FALSE;
#else
    // NULL platform: EngineOpenParams is undefined — device open is a no-op
    return rw::Engine::open(nil) ? TRUE : FALSE;
#endif
}

RwBool RwPlatformPollEvents(void) {
#ifdef RW_GL3
    glfwPollEvents();
    return TRUE;
#elif defined(RW_D3D9)
    // Win32 message queue — WM_PAINT / WM_SIZE / WM_CLOSE are processed through this path.
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            s_winQuit = TRUE;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return TRUE;
#else
    return FALSE;
#endif
}

RwBool RwPlatformShouldClose(void) {
#ifdef RW_GL3
    return (s_glfwWindow != nullptr && glfwWindowShouldClose(s_glfwWindow))
               ? TRUE
               : FALSE;
#elif defined(RW_D3D9)
    return s_winQuit;
#else
    return FALSE;
#endif
}

RwBool RwPlatformKeyDown(int key) {
#ifdef RW_GL3
    // GLFW_KEY_W == 'W' — ASCII codes pass directly.
    return (s_glfwWindow != nullptr && glfwGetKey(s_glfwWindow, key) == GLFW_PRESS)
               ? TRUE
               : FALSE;
#elif defined(RW_D3D9)
    // GetAsyncKeyState — key state read independently of the message queue.
    // GetAsyncKeyState reads the keyboard system-wide; ignore it unless our
    // window is in front, otherwise typing in another application drives the
    // game (e.g. Space/Enter confirming the main menu unseen).
    if (s_hwnd == nullptr || ::GetForegroundWindow() != s_hwnd) {
        return FALSE;
    }
    return (GetAsyncKeyState(GLFWKeyToVK(key)) & 0x8000) ? TRUE : FALSE;
#else
    (void)key;
    return FALSE;
#endif
}

RwBool RwPlatformSetMouseCapture(RwBool capture) {
#ifdef RW_GL3
    if (s_glfwWindow == nullptr) {
        return FALSE;
    }
    glfwSetInputMode(s_glfwWindow, GLFW_CURSOR,
                     (capture == TRUE) ? GLFW_CURSOR_DISABLED
                                       : GLFW_CURSOR_NORMAL);
    s_mouseCalibrated = FALSE; // recalibrate (zero the delta)
    return TRUE;
#elif defined(RW_D3D9)
    if (s_hwnd == nullptr) {
        return FALSE;
    }
    if (capture == TRUE) {
        // clip the cursor to the client rectangle (counterpart of GL3 CURSOR_DISABLED)
        RECT rc;
        GetClientRect(s_hwnd, &rc);
        POINT tl = {rc.left, rc.top};
        POINT br = {rc.right, rc.bottom};
        ClientToScreen(s_hwnd, &tl);
        ClientToScreen(s_hwnd, &br);
        RECT clip = {tl.x, tl.y, br.x, br.y};
        ClipCursor(&clip);
    } else {
        ClipCursor(nullptr);
    }
    s_mouseCalibrated = FALSE; // recalibrate (zero the delta)
    return TRUE;
#else
    (void)capture;
    return FALSE;
#endif
}

RwBool RwPlatformMouseDelta(int *pDX, int *pDY) {
#ifdef RW_GL3
    if (s_glfwWindow == nullptr || pDX == nullptr || pDY == nullptr) {
        return FALSE;
    }
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(s_glfwWindow, &x, &y);
    if (s_mouseCalibrated == FALSE) {
        s_lastMouseX = x;
        s_lastMouseY = y;
        s_mouseCalibrated = TRUE;
        *pDX = 0;
        *pDY = 0;
        return FALSE; // first frame: zero delta
    }
    *pDX = (int)(x - s_lastMouseX);
    *pDY = (int)(y - s_lastMouseY);
    s_lastMouseX = x;
    s_lastMouseY = y;
    return (*pDX != 0 || *pDY != 0) ? TRUE : FALSE;
#elif defined(RW_D3D9)
    if (s_hwnd == nullptr || pDX == nullptr || pDY == nullptr) {
        return FALSE;
    }
    POINT pt;
    if (!GetCursorPos(&pt)) {
        return FALSE;
    }
    ScreenToClient(s_hwnd, &pt);
    double x = (double)pt.x;
    double y = (double)pt.y;
    if (s_mouseCalibrated == FALSE) {
        s_lastMouseX = x;
        s_lastMouseY = y;
        s_mouseCalibrated = TRUE;
        *pDX = 0;
        *pDY = 0;
        return FALSE; // first frame: zero delta
    }
    *pDX = (int)(x - s_lastMouseX);
    *pDY = (int)(y - s_lastMouseY);
    s_lastMouseX = x;
    s_lastMouseY = y;
    return (*pDX != 0 || *pDY != 0) ? TRUE : FALSE;
#else
    (void)pDX;
    (void)pDY;
    return FALSE;
#endif
}

RwBool RwEngineStart(void) {
    if (!rw::Engine::start()) {
        return FALSE;
    }
    s_engineRunning = TRUE;
    return TRUE;
}

RwBool RwEngineStop(void) {
    rw::Engine::stop();
    s_engineRunning = FALSE;
    return TRUE;
}

RwBool RwEngineClose(void) {
    rw::Engine::close();
    return TRUE;
}

RwBool RwEngineTerm(void) {
    rw::Engine::term();
    return TRUE;
}

RwBool RwEngineIsRunning(void) { return s_engineRunning; }

RwBool RwEngineSetConfig(const RwEngineConfig *pConfig) {
    // librw has no near/fog globals — startup values ({1e-5,1e-5,0.01f})
    // will be moved to the camera setup later (documented no-op).
    (void)pConfig;
    return TRUE;
}

// ---------------------------------------------------------------------------
// Streams — rw::StreamFile
// ---------------------------------------------------------------------------
RwStream *RwStreamOpen(RwInt32 type, RwInt32 accessType, const void *pData) {
    if (type != rwSTREAMFILENAME || !pData) {
        return 0; // file path only (rwSTREAMMEMORY not used yet)
    }
    ShimStream *p = new ShimStream();
    const char *pMode = (accessType == rwSTREAMREAD) ? "rb" : "wb";
    if (!p->file.open(static_cast<const char *>(pData), pMode)) {
        delete p;
        return 0;
    }
    return reinterpret_cast<RwStream *>(p);
}

RwBool RwStreamClose(RwStream *stream, void *pData) {
    (void)pData;
    if (!stream) {
        return FALSE;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    p->file.close();
    delete p;
    return TRUE;
}

RwUInt32 RwStreamRead(RwStream *stream, void *buffer, RwUInt32 length) {
    if (!stream || !buffer) {
        return 0;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    return p->file.read8(buffer, length);
}

RwStream *RwStreamWrite(RwStream *stream, const void *buffer,
                        RwUInt32 length) {
    if (!stream || !buffer) {
        return 0;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    p->file.write8(buffer, length);
    return stream;
}

RwUInt32 RwStreamSkip(RwStream *stream, RwUInt32 length) {
    if (!stream) {
        return 0;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    p->file.seek(static_cast<rw::int32>(length), SEEK_CUR);
    return length;
}

RwUInt32 RwStreamTell(RwStream *stream) {
    if (!stream) {
        return 0;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    return static_cast<RwUInt32>(p->file.tell());
}

RwBool RwStreamSeekAbs(RwStream *stream, RwUInt32 position) {
    if (!stream) {
        return FALSE;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    p->file.seek(static_cast<rw::int32>(position), SEEK_SET);
    return TRUE;
}

RwBool RwStreamFindChunk(RwStream *stream, RwUInt32 type, RwUInt32 *lengthOut,
                         RwUInt32 *versionOut) {
    if (!stream) {
        return FALSE;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    rw::uint32 length = 0;
    rw::uint32 version = 0;
    if (!rw::findChunk(&p->file, type, &length, &version)) {
        return FALSE;
    }
    if (lengthOut) {
        *lengthOut = length;
    }
    if (versionOut) {
        *versionOut = version;
    }
    return TRUE;
}

RwBool RwStreamReadChunkHeaderInfo(RwStream *stream,
                                   RwChunkHeaderInfo *chunkHeader) {
    if (!stream || !chunkHeader) {
        return FALSE;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    rw::ChunkHeaderInfo header;
    if (!rw::readChunkHeaderInfo(&p->file, &header)) {
        return FALSE;
    }
    chunkHeader->type = header.type;
    chunkHeader->length = header.length;
    chunkHeader->version = header.version;
    chunkHeader->buildVersion = header.build;
    chunkHeader->isComplex = FALSE; // librw has no field for this
    return TRUE;
}

// ---------------------------------------------------------------------------
// Asset reading — clump/txd (world: not in librw — stub)
// ---------------------------------------------------------------------------
RpClump *RpClumpStreamRead(RwStream *stream) {
    if (!stream) {
        return 0;
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    return reinterpret_cast<RpClump *>(rw::Clump::streamRead(&p->file));
}

// Clump from an in-memory chunk payload (after the 12-byte rwID_CLUMP
// header), e.g. a model embedded in a Streams/*.RWS asset record.
RpClump *RpClumpStreamReadMemory(const void *pData, RwUInt32 size) {
    if (!pData || size == 0) {
        return 0;
    }
    rw::StreamMemory mem;
    mem.open(static_cast<rw::uint8 *>(const_cast<void *>(pData)), size, size);
    rw::Clump *pClump = rw::Clump::streamRead(&mem);
    mem.close();
    return reinterpret_cast<RpClump *>(pClump);
}

RwBool RpClumpDestroy(RpClump *clump) {
    if (!clump) {
        return FALSE;
    }
    reinterpret_cast<rw::Clump *>(clump)->destroy();
    return TRUE;
}

// Port helpers working on the librw clump directly (the RwFrame shim in
// rwstubs.cpp does not share librw's frame layout).
RwBool RpClumpPortSetMatrix(RpClump *clump, const RwReal m[16]) {
    if (!clump || !m) {
        return FALSE;
    }
    rw::Frame *pFrame = reinterpret_cast<rw::Clump *>(clump)->getFrame();
    if (!pFrame) {
        return FALSE;
    }
    rw::Matrix mat;
    mat.right = rw::makeV3d(m[0], m[1], m[2]);
    mat.up = rw::makeV3d(m[4], m[5], m[6]);
    mat.at = rw::makeV3d(m[8], m[9], m[10]);
    mat.pos = rw::makeV3d(m[12], m[13], m[14]);
    mat.optimize();
    pFrame->transform(&mat, rw::COMBINEREPLACE);
    return TRUE;
}

RwBool RpClumpPortGetPosition(RpClump *clump, RwV3d *pPos) {
    if (!clump || !pPos) {
        return FALSE;
    }
    rw::Frame *pFrame = reinterpret_cast<rw::Clump *>(clump)->getFrame();
    if (!pFrame) {
        return FALSE;
    }
    const rw::Matrix *pLtm = pFrame->getLTM();
    pPos->x = pLtm->pos.x;
    pPos->y = pLtm->pos.y;
    pPos->z = pLtm->pos.z;
    return TRUE;
}

RwInt32 RpClumpPortCountAtomics(RpClump *clump) {
    if (!clump) {
        return 0;
    }
    return reinterpret_cast<rw::Clump *>(clump)->countAtomics();
}

RwTexDictionary *RwTexDictionaryStreamRead(RwStream *stream) {
    if (!stream) {
        return 0;
    }
    // Ensure librw engine & native raster readers are registered if a test
    // calls RwTexDictionaryStreamRead before RwEngineInit.
    if (rw::engine == nullptr) {
        RwEngineInit(nullptr, 0, 0);
    }
    ShimStream *p = reinterpret_cast<ShimStream *>(stream);
    // Support both raw .txd files (cursor at outer rwID_TEXDICTIONARY header)
    // and streams where RwStreamFindChunk(..., rwID_TEXDICTIONARY) already
    // consumed the 12-byte chunk header.
    const rw::int32 pos = p->file.tell();
    rw::uint32 chunkType = 0;
    if (p->file.read8(&chunkType, sizeof(chunkType)) == sizeof(chunkType)) {
        p->file.seek(pos, SEEK_SET);
        if (chunkType == rw::ID_TEXDICTIONARY) {
            rw::findChunk(&p->file, rw::ID_TEXDICTIONARY, nullptr, nullptr);
        }
    }
    return reinterpret_cast<RwTexDictionary *>(
        rw::TexDictionary::streamRead(&p->file));
}

// ---------------------------------------------------------------------------
// Image texture dictionary (Localiza\Win\win_<lang>.txd, the font atlas).
// Not a native txd, so rw::TexDictionary::streamRead fails on it:
//   PITEXDICTIONARY (0x23) header, u32 0x00010001, u32 1,
//   IMAGE (0x18) { STRUCT { width, height, depth, stride } pixels },
//   TEXTURE (0x06) { STRUCT flags, STRING name ("brady48.png"), ... }.
// Builds a dictionary holding one texture made from the image.
// ---------------------------------------------------------------------------
RwTexDictionary *RwTexDictionaryReadImageFile(const char *path) {
    if (!path) {
        return 0;
    }
    FILE *pFile = std::fopen(path, "rb");
    if (!pFile) {
        return 0;
    }
    std::vector<RwUInt8> buf;
    std::fseek(pFile, 0, SEEK_END);
    const long fileSize = std::ftell(pFile);
    std::fseek(pFile, 0, SEEK_SET);
    if (fileSize > 0) {
        buf.resize((size_t)fileSize);
        if (std::fread(&buf[0], 1, buf.size(), pFile) != buf.size()) {
            buf.clear();
        }
    }
    std::fclose(pFile);
    const RwUInt32 n = (RwUInt32)buf.size();
    RwUInt32 u[3];
    if (n < 12) {
        return 0;
    }
    std::memcpy(u, &buf[0], 4);
    // 0x23 = rwID_PITEXDICTIONARY (platform-independent dictionary of
    // images), not the native rw::ID_TEXDICTIONARY (0x16).
    const RwUInt32 kPiTexDictionary = 0x23;
    if (u[0] != kPiTexDictionary) {
        return 0;
    }
    // Find IMAGE chunk and its STRUCT (4-byte aligned scan after the header).
    RwUInt32 imgOff = 0;
    for (RwUInt32 off = 12; off + 40 <= n; off += 4) {
        std::memcpy(u, &buf[off], 12);
        if (u[0] == rw::ID_IMAGE && u[1] <= n - off - 12) {
            RwUInt32 s[2];
            std::memcpy(s, &buf[off + 12], 8);
            if (s[0] == rw::ID_STRUCT && s[1] == 16) {
                imgOff = off;
                break;
            }
        }
    }
    if (imgOff == 0) {
        return 0;
    }
    RwInt32 info[4]; // width, height, depth, stride
    std::memcpy(info, &buf[imgOff + 24], sizeof(info));
    const RwInt32 w = info[0], h = info[1], depth = info[2], stride = info[3];
    const RwUInt32 pixOff = imgOff + 40;
    if (w <= 0 || h <= 0 || depth != 32 || stride < w * 4 ||
        (unsigned long long)stride * (unsigned long long)h > (unsigned long long)(n - pixOff)) {
        return 0;
    }
    // Texture name: first STRING chunk after the pixels.
    char name[32] = "win_font";
    for (RwUInt32 off = pixOff + (RwUInt32)(stride * h); off + 12 <= n;
         off += 4) {
        std::memcpy(u, &buf[off], 12);
        if (u[0] == rw::ID_STRING && u[1] > 0 && u[1] <= n - off - 12) {
            const char *pStr = reinterpret_cast<const char *>(&buf[off + 12]);
            RwUInt32 i = 0;
            for (; i + 1 < sizeof(name) && i < u[1] && pStr[i] != '\0'; ++i) {
                name[i] = pStr[i];
            }
            name[i] = '\0';
            char *pDot = std::strrchr(name, '.');
            if (pDot) {
                *pDot = '\0'; // "brady48.png" -> "brady48"
            }
            break;
        }
    }
    rw::Image *pImg = rw::Image::create(w, h, 32);
    if (!pImg) {
        return 0;
    }
    pImg->allocate();
    for (RwInt32 y = 0; y < h; ++y) {
        std::memcpy(pImg->pixels + (size_t)y * (size_t)pImg->stride,
                    &buf[pixOff + (RwUInt32)(y * stride)], (size_t)w * 4u);
    }
    // The NULL (headless) driver cannot pick a raster format; keep the
    // texture by name without a raster there.
    rw::Raster *pRaster = rw::Raster::createFromImage(pImg);
    pImg->destroy();
    rw::Texture *pTex = rw::Texture::create(pRaster);
    if (!pTex) {
        if (pRaster) {
            pRaster->destroy();
        }
        return 0;
    }
    std::strncpy(pTex->name, name, sizeof(pTex->name) - 1);
    pTex->name[sizeof(pTex->name) - 1] = '\0';
    rw::TexDictionary *pDict = rw::TexDictionary::create();
    if (!pDict) {
        pTex->destroy();
        return 0;
    }
    pDict->add(pTex);
    return reinterpret_cast<RwTexDictionary *>(pDict);
}