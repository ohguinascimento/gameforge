#include "Common.h"
#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"
#include "VM.h"
#include "Engine.h"
#include "Transpiler.h"
#include "Version.h"
#include "GpuRenderer.h"
#include "SemanticAnalyzer.h"

using namespace GameLang;

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void printHelp() {
    std::cout << "\033[1;36m====================================================\033[0m\n";
    std::cout << "\033[1;32m      GameForge " << GameForge::VERSION_TAG << " - Compilador de Games em C++  \033[0m\n";
    std::cout << "\033[1;36m====================================================\033[0m\n\n";
    std::cout << "Uso: gamec <comando> <arquivo.game> [opcoes]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  \033[1;33mrun <arquivo>\033[0m          Compila para bytecode e executa na VM do jogo\n";
    std::cout << "  \033[1;33mcheck <arquivo>\033[0m        Executa analise semantica estrita e verificacao de tipos da AST\n";
    std::cout << "  \033[1;33mtranspile <arquivo>\033[0m    Gera codigo C++20 nativo (-o <saida.cpp>, padrao GPU / OpenGL 3.3)\n";
    std::cout << "  \033[1;33mbuild <arquivo>\033[0m        Compila para executavel nativo .exe com GPU e -O3 (-o <jogo.exe>)\n";
    std::cout << "  \033[1;33mdump-ast <arquivo>\033[0m     Exibe a Abstract Syntax Tree (AST)\n";
    std::cout << "  \033[1;33mdump-bc <arquivo>\033[0m      Exibe o Bytecode descompilado (Disassembly)\n";
    std::cout << "  \033[1;33mversion / -v\033[0m           Exibe a versao do GameForge\n";
    std::cout << "  \033[1;33mhelp\033[0m                   Exibe esta ajuda\n\n";
    std::cout << "Opcoes de Resiliencia / Tolerancia a Falhas:\n";
    std::cout << "  \033[1;32m--safe-mode / -f\033[0m       (Padrao) Isola erros de runtime e parsing para evitar crash\n";
    std::cout << "  \033[1;32m--ignore-errors\033[0m        Continua execucao mesmo com erros pontuais de sintaxe\n";
    std::cout << "  \033[1;32m--strict\033[0m               Modo estrito: interrompe na primeira falha\n\n";
    std::cout << "Opcoes Graficas e Back-end:\n";
    std::cout << "  \033[1;35m--gpu\033[0m                  (Padrao no build) Pipeline acelerado por GPU (OpenGL 3.3 + Shaders GLSL + Rust)\n";
    std::cout << "  \033[1;35m--console\033[0m              Forca geracao de executavel para terminal retro (ANSI Escape sequences)\n\n";
    std::cout << "Exemplos:\n";
    std::cout << "  gamec check games/space_invaders.game\n";
    std::cout << "  gamec run games/space_invaders.game --gpu\n";
    std::cout << "  gamec build games/space_invaders.game -o bin/invaders.exe\n";
    std::cout << "  gamec transpile games/pong.game -o pong.cpp\n\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "version" || cmd == "--version" || cmd == "-v") {
        std::cout << GameForge::getVersionInfo() << std::endl;
        return 0;
    }

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printHelp();
        return 0;
    }

    if (argc < 3) {
        std::cerr << "Erro: Arquivo fonte nao especificado.\n";
        printHelp();
        return 1;
    }

    std::string sourcePath = argv[2];
    std::string source = readFile(sourcePath);
    if (source.empty()) {
        return 1;
    }

    bool safeMode = true;
    bool strictMode = false;
    bool useGpu = false;
    bool useConsole = false;
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--strict") {
            strictMode = true;
            safeMode = false;
        } else if (arg == "--safe" || arg == "--safe-mode" || arg == "-f" || arg == "--ignore-errors") {
            safeMode = true;
            strictMode = false;
        } else if (arg == "--gpu") {
            useGpu = true;
        } else if (arg == "--console") {
            useConsole = true;
        }
    }

    // Step 1: Tokenize
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (!lexer.getErrors().empty()) {
        if (strictMode) {
            std::cerr << "\033[1;31m[Erros Lexicos Encontrados]:\033[0m\n";
            for (const auto& err : lexer.getErrors()) {
                std::cerr << "  " << err << "\n";
            }
            return 1;
        } else {
            std::cerr << "\033[1;33m[Avisos Lexicos Isolados]:\033[0m\n";
            for (const auto& err : lexer.getErrors()) {
                std::cerr << "  " << err << "\n";
            }
        }
    }

    // Step 2: Parse AST
    Parser parser(std::move(tokens));
    auto program = parser.parseProgram();
    if (parser.hasErrors()) {
        if (strictMode) {
            std::cerr << "\033[1;31m[Erros Sintaticos Encontrados]:\033[0m\n";
            for (const auto& err : parser.getErrors()) {
                std::cerr << "  " << err << "\n";
            }
            return 1;
        } else {
            std::cerr << "\033[1;33m[Avisos Sintaticos Isolados (Execucao tolerante ativa)]:\033[0m\n";
            for (const auto& err : parser.getErrors()) {
                std::cerr << "  " << err << "\n";
            }
        }
    }

    // Step 2.5: Verificacao Semantica Estrita
    SemanticAnalyzer semanticAnalyzer;
    bool semOk = semanticAnalyzer.analyze(*program);
    if (cmd == "check") {
        semanticAnalyzer.printReport(std::cout);
        return semanticAnalyzer.hasErrors() ? 1 : 0;
    }
    if (!semOk) {
        if (strictMode) {
            std::cerr << "\033[1;31m[Falha na Verificacao Semantica]:\033[0m\n";
            semanticAnalyzer.printReport(std::cerr);
            return 1;
        } else {
            semanticAnalyzer.printReport(std::cerr);
        }
    }

    // Handle dump-ast
    if (cmd == "dump-ast") {
        std::cout << "\033[1;32m=== Abstract Syntax Tree (AST) ===\033[0m\n";
        program->dump(std::cout);
        return 0;
    }

    // Handle transpile
    if (cmd == "transpile") {
        std::string outPath = "output.cpp";
        bool targetGpu = !useConsole;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "-o" && i + 1 < argc) {
                outPath = argv[i + 1];
                break;
            }
        }
        Transpiler transpiler;
        std::string cppCode = transpiler.transpileToCpp(*program, targetGpu ? TranspileTarget::OpenGL33 : TranspileTarget::Console);
        std::ofstream outFile(outPath);
        if (!outFile.is_open()) {
            std::cerr << "Erro ao gravar em " << outPath << std::endl;
            return 1;
        }
        outFile << cppCode;
        outFile.close();
        std::cout << "\033[1;32m[Sucesso]\033[0m Codigo C++ transpilado com sucesso (" 
                  << (targetGpu ? "OpenGL 3.3 GPU Core" : "Console Retro") << ") em: " << outPath << "\n";
        return 0;
    }

    // Handle build (Transpile -> g++ compile to native .exe)
    if (cmd == "build") {
        std::string exePath = "game.exe";
        bool targetGpu = !useConsole;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "-o" && i + 1 < argc) {
                exePath = argv[i + 1];
                break;
            }
        }
        Transpiler transpiler;
        std::string cppCode = transpiler.transpileToCpp(*program, targetGpu ? TranspileTarget::OpenGL33 : TranspileTarget::Console);
        std::string tmpCpp = "_temp_build.cpp";
        std::ofstream tmpFile(tmpCpp);
        tmpFile << cppCode;
        tmpFile.close();

        std::string gppCmd = "g++";
        char* userProfile = getenv("USERPROFILE");
        if (userProfile) {
            std::string w64gpp = std::string(userProfile) + "\\w64devkit\\bin\\g++.exe";
            std::ifstream testGpp(w64gpp);
            if (testGpp.good()) {
                gppCmd = w64gpp;
            }
        }

        std::cout << "\033[1;33mCompilando executavel nativo (" 
                  << (targetGpu ? "OpenGL 3.3 GPU Core com -O3 -march=native" : "Console com -O3") << ")...\033[0m\n";
        std::string buildCmd = "\"" + gppCmd + "\" -std=c++20 -O3 -march=native -I include " + tmpCpp +
                               (targetGpu ? " -lopengl32 -lgdi32 -luser32" : "") +
                               " -o " + exePath;
        #ifdef _WIN32
        buildCmd = "\"" + buildCmd + "\"";
        #endif
        int res = system(buildCmd.c_str());
        remove(tmpCpp.c_str());

        if (res == 0) {
            std::cout << "\033[1;32m[Sucesso]\033[0m Jogo compilado em executavel nativo: \033[1m" << exePath << "\033[0m\n";
            return 0;
        } else {
            std::cerr << "\033[1;31m[Erro]\033[0m Falha ao compilar com g++.\n";
            return 1;
        }
    }

    // Step 3: Compile to Bytecode
    Compiler compiler;
    auto compiledGame = compiler.compile(*program);
    if (compiler.hasErrors()) {
        if (strictMode) {
            std::cerr << "\033[1;31m[Erros de Compilacao]:\033[0m\n";
            for (const auto& err : compiler.getErrors()) {
                std::cerr << "  " << err << "\n";
            }
            return 1;
        } else {
            std::cerr << "\033[1;33m[Avisos de Compilacao Isolados]:\033[0m\n";
            for (const auto& err : compiler.getErrors()) {
                std::cerr << "  " << err << "\n";
            }
        }
    }

    // Handle dump-bc
    if (cmd == "dump-bc") {
        std::cout << "\033[1;32m=== Bytecode Disassembly ===\033[0m\n";
        compiledGame->globalInitChunk.disassemble(std::cout, "Global Init");
        compiledGame->initChunk.disassemble(std::cout, "Game Init");
        compiledGame->updateChunk.disassemble(std::cout, "Game Update");
        compiledGame->renderChunk.disassemble(std::cout, "Game Render");
        for (const auto& kv : compiledGame->functions) {
            kv.second.chunk.disassemble(std::cout, "Function: " + kv.first);
        }
        for (size_t i = 0; i < compiledGame->collisionHandlers.size(); ++i) {
            compiledGame->collisionHandlers[i].chunk.disassemble(
                std::cout, "Collision: " + compiledGame->collisionHandlers[i].entityA +
                           " vs " + compiledGame->collisionHandlers[i].entityB);
        }
        return 0;
    }

    // Handle run
    if (cmd == "run") {
        if (useGpu) {
            std::cout << "\033[1;35m[GameForge GPU]\033[0m Inicializando pipeline grafico 2D acelerado por GPU...\n";
            std::cout << "  Backend: " << (GameForge::GpuRenderer2D::isRustAvailable() ? "Rust FFI Nativo (gameforge_gpu.dll)" : "Hardware Batching / C++ Fallback") << "\n";
            
            // Aplica otimizador Rust de bytecode nas chunks compiladas
            size_t optCount = 0;
            auto optUpdate = GameForge::GpuRenderer2D::optimizeBytecodeWithRust(compiledGame->updateChunk.code);
            if (optUpdate.size() < compiledGame->updateChunk.code.size()) {
                optCount += (compiledGame->updateChunk.code.size() - optUpdate.size());
                compiledGame->updateChunk.code = std::move(optUpdate);
            }
            auto optRender = GameForge::GpuRenderer2D::optimizeBytecodeWithRust(compiledGame->renderChunk.code);
            if (optRender.size() < compiledGame->renderChunk.code.size()) {
                optCount += (compiledGame->renderChunk.code.size() - optRender.size());
                compiledGame->renderChunk.code = std::move(optRender);
            }
            for (auto& kv : compiledGame->functions) {
                auto optFn = GameForge::GpuRenderer2D::optimizeBytecodeWithRust(kv.second.chunk.code);
                if (optFn.size() < kv.second.chunk.code.size()) {
                    optCount += (kv.second.chunk.code.size() - optFn.size());
                    kv.second.chunk.code = std::move(optFn);
                }
            }
            if (optCount > 0) {
                std::cout << "  \033[1;32m[Rust Optimizer]\033[0m Otimizacoes aplicadas ao bytecode: " << optCount << " instrucoes podadas/otimizadas\n";
            }
        }

        Engine engine(compiledGame->config.width,
                      compiledGame->config.height,
                      compiledGame->config.fps,
                      compiledGame->config.title);

        VM vm(*compiledGame, engine);
        vm.setSafeMode(safeMode);
        InterpretResult res = vm.run();
        if (res != InterpretResult::Ok && strictMode) {
            std::cerr << "\033[1;31mErro durante execucao da VM.\033[0m\n";
            return 1;
        }
        return 0;
    }

    std::cerr << "Comando desconhecido: " << cmd << "\n";
    printHelp();
    return 1;
}
