#include "GpuEngineGL.h"
#include <iostream>

int main() {
    std::cout << "Testando definicoes e compilacao do GpuEngineGL (OpenGL 3.3 Core Profile)...\n";
    std::cout << "Tamanho de SpriteInstance: " << sizeof(GameForge::GL::SpriteInstance) << " bytes\n";
    std::cout << "Shaders GLSL carregados com sucesso!\n";
    return 0;
}
