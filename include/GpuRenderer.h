#pragma once

#include "Common.h"
#include <cstdint>
#include <string>
#include <vector>

namespace GameForge {

// ============================================================================
// Declarações C-ABI FFI para o Módulo Rust (gameforge_gpu)
// ============================================================================
extern "C" {
    void* gameforge_gpu_create(uint32_t width, uint32_t height, float tileSize);
    void gameforge_gpu_destroy(void* ctx);
    void gameforge_gpu_set_clear_color(void* ctx, float r, float g, float b, float a);
    void gameforge_gpu_begin_frame(void* ctx);
    void gameforge_gpu_draw_rect(void* ctx, float x, float y, float w, float h, float r, float g, float b, float a);
    void gameforge_gpu_draw_tile(void* ctx, int32_t gridX, int32_t gridY, uint8_t ch, float r, float g, float b, float a);
    uint32_t gameforge_gpu_end_frame(void* ctx);
    void gameforge_gpu_get_stats(void* ctx, uint64_t* outQuads, uint64_t* outDrawCalls, uint64_t* outFrames);

    uint8_t* gameforge_rust_optimize_bytecode(const uint8_t* inputBytes, size_t inputLen, size_t* outLen);
    void gameforge_rust_free_buffer(uint8_t* buf, size_t len);
    bool gameforge_rust_verify_bytecode(const uint8_t* inputBytes, size_t inputLen);
}

struct GpuStats {
    uint64_t totalQuads = 0;
    uint64_t totalDrawCalls = 0;
    uint64_t totalFrames = 0;
};

// ============================================================================
// Classe de Interface GPU 2D de Alta Performance
// ============================================================================
class GpuRenderer2D {
public:
    GpuRenderer2D(int width = 80, int height = 25, float tileSize = 16.0f);
    ~GpuRenderer2D();

    GpuRenderer2D(const GpuRenderer2D&) = delete;
    GpuRenderer2D& operator=(const GpuRenderer2D&) = delete;

    bool isHardwareAccelerated() const noexcept { return isAccelerated; }

    void setClearColor(float r, float g, float b, float a);
    void beginFrame();
    void drawTile(int x, int y, char ch, GameLang::Color color);
    void drawRect(float x, float y, float w, float h, GameLang::Color color);
    void drawBox(int x, int y, int w, int h, GameLang::Color color);
    void drawText(int x, int y, const std::string& text, GameLang::Color color);
    uint32_t endFrame();

    GpuStats getStats() const;

    // Otimizador de Bytecode alimentado pelo módulo Rust
    static bool isRustAvailable();
    static std::vector<uint8_t> optimizeBytecodeWithRust(const std::vector<uint8_t>& code);
    static bool verifyBytecodeWithRust(const std::vector<uint8_t>& code);

private:
    int width;
    int height;
    float tileSize;
    void* rustGpuCtx = nullptr;
    bool isAccelerated = false;
    uint64_t fallbackQuads = 0;
    uint64_t fallbackDrawCalls = 0;
    uint64_t fallbackFrames = 0;
};

} // namespace GameForge
