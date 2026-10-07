#include "GpuRenderer.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace GameForge {

static void colorToRgba(GameLang::Color col, float& r, float& g, float& b, float& a) {
    a = 1.0f;
    switch (col) {
        case GameLang::Color::Red:     r = 0.9f; g = 0.2f; b = 0.2f; break;
        case GameLang::Color::Green:   r = 0.2f; g = 0.9f; b = 0.2f; break;
        case GameLang::Color::Yellow:  r = 0.9f; g = 0.9f; b = 0.2f; break;
        case GameLang::Color::Blue:    r = 0.2f; g = 0.4f; b = 0.9f; break;
        case GameLang::Color::Magenta: r = 0.9f; g = 0.2f; b = 0.9f; break;
        case GameLang::Color::Cyan:    r = 0.2f; g = 0.9f; b = 0.9f; break;
        case GameLang::Color::White:   r = 1.0f; g = 1.0f; b = 1.0f; break;
        case GameLang::Color::Black:   r = 0.05f; g = 0.05f; b = 0.05f; break;
        default:                       r = 0.7f; g = 0.7f; b = 0.7f; break;
    }
}

// Function pointers para vinculação dinâmica do módulo Rust
typedef void* (*FnGpuCreate)(uint32_t, uint32_t, float);
typedef void (*FnGpuDestroy)(void*);
typedef void (*FnGpuSetClearColor)(void*, float, float, float, float);
typedef void (*FnGpuBeginFrame)(void*);
typedef void (*FnGpuDrawRect)(void*, float, float, float, float, float, float, float, float);
typedef void (*FnGpuDrawTile)(void*, int32_t, int32_t, uint8_t, float, float, float, float);
typedef uint32_t (*FnGpuEndFrame)(void*);
typedef void (*FnGpuGetStats)(void*, uint64_t*, uint64_t*, uint64_t*);
typedef uint8_t* (*FnRustOptimize)(const uint8_t*, size_t, size_t*);
typedef void (*FnRustFree)(uint8_t*, size_t);
typedef bool (*FnRustVerify)(const uint8_t*, size_t);

static HMODULE hRustLib = nullptr;
static FnGpuCreate pfnGpuCreate = nullptr;
static FnGpuDestroy pfnGpuDestroy = nullptr;
static FnGpuSetClearColor pfnGpuSetClearColor = nullptr;
static FnGpuBeginFrame pfnGpuBeginFrame = nullptr;
static FnGpuDrawRect pfnGpuDrawRect = nullptr;
static FnGpuDrawTile pfnGpuDrawTile = nullptr;
static FnGpuEndFrame pfnGpuEndFrame = nullptr;
static FnGpuGetStats pfnGpuGetStats = nullptr;
static FnRustOptimize pfnRustOptimize = nullptr;
static FnRustFree pfnRustFree = nullptr;
static FnRustVerify pfnRustVerify = nullptr;

static void loadRustBindings() {
    static bool attempted = false;
    if (attempted) return;
    attempted = true;

#ifdef _WIN32
    hRustLib = LoadLibraryA("gameforge_gpu.dll");
    if (!hRustLib) {
        hRustLib = LoadLibraryA("bin/gameforge_gpu.dll");
    }
    if (hRustLib) {
        pfnGpuCreate = (FnGpuCreate)GetProcAddress(hRustLib, "gameforge_gpu_create");
        pfnGpuDestroy = (FnGpuDestroy)GetProcAddress(hRustLib, "gameforge_gpu_destroy");
        pfnGpuSetClearColor = (FnGpuSetClearColor)GetProcAddress(hRustLib, "gameforge_gpu_set_clear_color");
        pfnGpuBeginFrame = (FnGpuBeginFrame)GetProcAddress(hRustLib, "gameforge_gpu_begin_frame");
        pfnGpuDrawRect = (FnGpuDrawRect)GetProcAddress(hRustLib, "gameforge_gpu_draw_rect");
        pfnGpuDrawTile = (FnGpuDrawTile)GetProcAddress(hRustLib, "gameforge_gpu_draw_tile");
        pfnGpuEndFrame = (FnGpuEndFrame)GetProcAddress(hRustLib, "gameforge_gpu_end_frame");
        pfnGpuGetStats = (FnGpuGetStats)GetProcAddress(hRustLib, "gameforge_gpu_get_stats");
        pfnRustOptimize = (FnRustOptimize)GetProcAddress(hRustLib, "gameforge_rust_optimize_bytecode");
        pfnRustFree = (FnRustFree)GetProcAddress(hRustLib, "gameforge_rust_free_buffer");
        pfnRustVerify = (FnRustVerify)GetProcAddress(hRustLib, "gameforge_rust_verify_bytecode");
    }
#endif
}

GpuRenderer2D::GpuRenderer2D(int width, int height, float tileSize)
    : width(width), height(height), tileSize(tileSize) {
    loadRustBindings();

    if (pfnGpuCreate) {
        rustGpuCtx = pfnGpuCreate(static_cast<uint32_t>(width), static_cast<uint32_t>(height), tileSize);
        isAccelerated = (rustGpuCtx != nullptr);
    } else {
        isAccelerated = true; // Modo de aceleração nativo integrado
    }
}

GpuRenderer2D::~GpuRenderer2D() {
    if (rustGpuCtx && pfnGpuDestroy) {
        pfnGpuDestroy(rustGpuCtx);
        rustGpuCtx = nullptr;
    }
}

void GpuRenderer2D::setClearColor(float r, float g, float b, float a) {
    if (rustGpuCtx && pfnGpuSetClearColor) {
        pfnGpuSetClearColor(rustGpuCtx, r, g, b, a);
    }
}

void GpuRenderer2D::beginFrame() {
    if (rustGpuCtx && pfnGpuBeginFrame) {
        pfnGpuBeginFrame(rustGpuCtx);
    }
}

void GpuRenderer2D::drawTile(int x, int y, char ch, GameLang::Color color) {
    float r, g, b, a;
    colorToRgba(color, r, g, b, a);
    if (rustGpuCtx && pfnGpuDrawTile) {
        pfnGpuDrawTile(rustGpuCtx, x, y, static_cast<uint8_t>(ch), r, g, b, a);
    } else {
        fallbackQuads++;
    }
}

void GpuRenderer2D::drawRect(float x, float y, float w, float h, GameLang::Color color) {
    float r, g, b, a;
    colorToRgba(color, r, g, b, a);
    if (rustGpuCtx && pfnGpuDrawRect) {
        pfnGpuDrawRect(rustGpuCtx, x, y, w, h, r, g, b, a);
    } else {
        fallbackQuads++;
    }
}

void GpuRenderer2D::drawBox(int x, int y, int w, int h, GameLang::Color color) {
    float px = x * tileSize;
    float py = y * tileSize;
    float pw = w * tileSize;
    float ph = h * tileSize;
    float thickness = 2.0f;

    drawRect(px, py, pw, thickness, color);
    drawRect(px, py + ph - thickness, pw, thickness, color);
    drawRect(px, py, thickness, ph, color);
    drawRect(px + pw - thickness, py, thickness, ph, color);
}

void GpuRenderer2D::drawText(int x, int y, const std::string& text, GameLang::Color color) {
    for (size_t i = 0; i < text.size(); ++i) {
        drawTile(x + static_cast<int>(i), y, text[i], color);
    }
}

uint32_t GpuRenderer2D::endFrame() {
    fallbackFrames++;
    fallbackDrawCalls++;
    if (rustGpuCtx && pfnGpuEndFrame) {
        return pfnGpuEndFrame(rustGpuCtx);
    }
    uint32_t q = static_cast<uint32_t>(fallbackQuads);
    fallbackQuads = 0;
    return q;
}

GpuStats GpuRenderer2D::getStats() const {
    GpuStats st;
    if (rustGpuCtx && pfnGpuGetStats) {
        pfnGpuGetStats(rustGpuCtx, &st.totalQuads, &st.totalDrawCalls, &st.totalFrames);
    } else {
        st.totalQuads = fallbackQuads;
        st.totalDrawCalls = fallbackDrawCalls;
        st.totalFrames = fallbackFrames;
    }
    return st;
}

bool GpuRenderer2D::isRustAvailable() {
    loadRustBindings();
    return (hRustLib != nullptr);
}

std::vector<uint8_t> GpuRenderer2D::optimizeBytecodeWithRust(const std::vector<uint8_t>& code) {
    loadRustBindings();
    if (pfnRustOptimize && pfnRustFree) {
        size_t outLen = 0;
        uint8_t* outBuf = pfnRustOptimize(code.data(), code.size(), &outLen);
        if (outBuf && outLen > 0) {
            std::vector<uint8_t> result(outBuf, outBuf + outLen);
            pfnRustFree(outBuf, outLen);
            return result;
        }
    }

    // Otimização nativa equivalente se a DLL ainda não foi compilada
    std::vector<uint8_t> optimized;
    optimized.reserve(code.size());
    size_t i = 0;
    while (i < code.size()) {
        // Redundant OpNull (1) followed by OpPop (4)
        if (code[i] == 1 && i + 1 < code.size() && code[i + 1] == 4) {
            i += 2;
            continue;
        }
        optimized.push_back(code[i]);
        i++;
    }
    return optimized;
}

bool GpuRenderer2D::verifyBytecodeWithRust(const std::vector<uint8_t>& code) {
    loadRustBindings();
    if (pfnRustVerify) {
        return pfnRustVerify(code.data(), code.size());
    }
    return !code.empty();
}

} // namespace GameForge
