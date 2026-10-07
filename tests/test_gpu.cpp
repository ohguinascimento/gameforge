#include "GpuRenderer.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>

int main() {
    std::cout << "\033[1;36m====================================================\033[0m\n";
    std::cout << "\033[1;32m      GameForge: Testes de GPU 2D e Modulo Rust     \033[0m\n";
    std::cout << "\033[1;36m====================================================\033[0m\n\n";

    // ---------------------------------------------------------
    // Teste 1: Inicialização do Pipeline GPU 2D
    // ---------------------------------------------------------
    std::cout << "[1/4] Testando Inicializacao e Pipeline Grafico 2D...\n";
    {
        GameForge::GpuRenderer2D renderer(80, 25, 16.0f);
        assert(renderer.isHardwareAccelerated());
        
        renderer.setClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        renderer.beginFrame();
        
        // Desenha terreno e ladrilhos
        for (int y = 0; y < 10; ++y) {
            for (int x = 0; x < 20; ++x) {
                renderer.drawTile(x, y, '#', GameLang::Color::White);
            }
        }
        
        // Desenha caixas, retângulos e texto
        renderer.drawRect(50.0f, 50.0f, 100.0f, 60.0f, GameLang::Color::Cyan);
        renderer.drawBox(2, 2, 10, 5, GameLang::Color::Yellow);
        renderer.drawText(3, 3, "GPU Active", GameLang::Color::Green);
        
        uint32_t quadsRendered = renderer.endFrame();
        assert(quadsRendered > 0);
        
        GameForge::GpuStats stats = renderer.getStats();
        assert(stats.totalFrames == 1);
        std::cout << "  -> Frame 1 renderizado com sucesso: " << quadsRendered 
                  << " quads submetidos na GPU!\n";
    }

    // ---------------------------------------------------------
    // Teste 2: Otimizador de Bytecode (Rust Peephole Pass)
    // ---------------------------------------------------------
    std::cout << "\n[2/4] Testando Otimizador de Bytecode Rust...\n";
    {
        // Bytecode sintético: OP_CONSTANT (2), '0', OP_SET_GLOBAL (12), 'hero', 
        // seguido por padrão redundante OP_NULL (1) + OP_POP (4), e OP_RETURN (24)
        std::vector<uint8_t> testCode = {
            2, 0, 0, 0, 0,     // OP_CONSTANT 0
            12, 1, 0, 0, 0,    // OP_SET_GLOBAL 'hero'
            1, 4,              // OP_NULL + OP_POP (Redundante! Deve ser podado)
            1, 4,              // OP_NULL + OP_POP (Segundo redundante)
            24                 // OP_RETURN
        };

        size_t initialLen = testCode.size();
        std::vector<uint8_t> optimized = GameForge::GpuRenderer2D::optimizeBytecodeWithRust(testCode);

        std::cout << "  -> Tamanho inicial: " << initialLen << " bytes\n";
        std::cout << "  -> Tamanho otimizado: " << optimized.size() << " bytes\n";
        assert(optimized.size() < initialLen);
        assert(optimized.size() == initialLen - 4); // Exatamente 4 bytes podados (dois pares null+pop)
        
        // Verifica que o OP_RETURN continua no final
        assert(optimized.back() == 24);
        std::cout << "  -> Poda de instrucoes redundantes realizada com sucesso!\n";
    }

    // ---------------------------------------------------------
    // Teste 3: Verificador de Segurança de Bytecode
    // ---------------------------------------------------------
    std::cout << "\n[3/4] Testando Validador de Seguranca de Bytecode...\n";
    {
        std::vector<uint8_t> validCode = { 2, 0, 0, 0, 0, 24 };
        bool isValid = GameForge::GpuRenderer2D::verifyBytecodeWithRust(validCode);
        assert(isValid);

        std::vector<uint8_t> emptyCode = {};
        bool isInvalid = GameForge::GpuRenderer2D::verifyBytecodeWithRust(emptyCode);
        assert(!isInvalid);
        std::cout << "  -> Verificador de integridade validou os chunks com exito!\n";
    }

    // ---------------------------------------------------------
    // Teste 4: Benchmark de Vazão do Pipeline GPU 2D
    // ---------------------------------------------------------
    std::cout << "\n[4/4] Benchmark de Rendimento (10.000 Quads / Batch GPU)...\n";
    {
        GameForge::GpuRenderer2D renderer(100, 50, 16.0f);
        auto tStart = std::chrono::high_resolution_clock::now();

        const int NUM_FRAMES = 100;
        const int QUADS_PER_FRAME = 5000;

        for (int f = 0; f < NUM_FRAMES; ++f) {
            renderer.beginFrame();
            for (int q = 0; q < QUADS_PER_FRAME; ++q) {
                renderer.drawRect((float)(q % 100), (float)(q / 100), 1.0f, 1.0f, GameLang::Color::Cyan);
            }
            renderer.endFrame();
        }

        auto tEnd = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        double totalQuads = (double)(NUM_FRAMES * QUADS_PER_FRAME);
        double mquadsPerSec = (totalQuads / (elapsedMs / 1000.0)) / 1000000.0;

        std::cout << "  -> Renderizados " << (int)totalQuads << " quads em " 
                  << elapsedMs << " ms (" << mquadsPerSec << " Milhoes de quads/segundo)\n";
    }

    std::cout << "\n\033[1;32m[TODOS OS TESTES DE GPU E MODULO RUST PASSARAM COM SUCESSO!]\033[0m\n";
    return 0;
}
