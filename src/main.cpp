#include "Common.h"
#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"
#include "VM.h"
#include "Engine.h"
#include "Transpiler.h"
#include "Version.h"

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
    std::cout << "  \033[1;33mtranspile <arquivo>\033[0m    Gera codigo fonte C++ independente (-o <saida.cpp>)\n";
    std::cout << "  \033[1;33mbuild <arquivo>\033[0m        Transpila e compila para executavel nativo .exe (-o <jogo.exe>)\n";
    std::cout << "  \033[1;33mdump-ast <arquivo>\033[0m     Exibe a Abstract Syntax Tree (AST)\n";
    std::cout << "  \033[1;33mdump-bc <arquivo>\033[0m      Exibe o Bytecode descompilado (Disassembly)\n";
    std::cout << "  \033[1;33mversion / -v\033[0m           Exibe a versao do GameForge\n";
    std::cout << "  \033[1;33mhelp\033[0m                   Exibe esta ajuda\n\n";
    std::cout << "Opcoes de Resiliencia / Tolerancia a Falhas:\n";
    std::cout << "  \033[1;32m--safe-mode / -f\033[0m       (Padrao) Isola erros de runtime e parsing para evitar crash\n";
    std::cout << "  \033[1;32m--ignore-errors\033[0m        Continua execucao mesmo com erros pontuais de sintaxe\n";
    std::cout << "  \033[1;32m--strict\033[0m               Modo estrito: interrompe na primeira falha\n\n";
    std::cout << "Exemplos:\n";
    std::cout << "  gamec run games/space_invaders.game\n";
    std::cout << "  gamec run games/rpg_dungeon.game\n";
    std::cout << "  gamec build games/rpg_dungeon.game -o bin/rpg.exe\n";
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
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--strict") {
            strictMode = true;
            safeMode = false;
        } else if (arg == "--safe" || arg == "--safe-mode" || arg == "-f" || arg == "--ignore-errors") {
            safeMode = true;
            strictMode = false;
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

    // Handle dump-ast
    if (cmd == "dump-ast") {
        std::cout << "\033[1;32m=== Abstract Syntax Tree (AST) ===\033[0m\n";
        program->dump(std::cout);
        return 0;
    }

    // Handle transpile
    if (cmd == "transpile") {
        std::string outPath = "output.cpp";
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "-o" && i + 1 < argc) {
                outPath = argv[i + 1];
                break;
            }
        }
        Transpiler transpiler;
        std::string cppCode = transpiler.transpileToCpp(*program);
        std::ofstream outFile(outPath);
        if (!outFile.is_open()) {
            std::cerr << "Erro ao gravar em " << outPath << std::endl;
            return 1;
        }
        outFile << cppCode;
        outFile.close();
        std::cout << "\033[1;32m[Sucesso]\033[0m Codigo C++ transpilado com sucesso em: " << outPath << "\n";
        return 0;
    }

    // Handle build (Transpile -> g++ compile to native .exe)
    if (cmd == "build") {
        std::string exePath = "game.exe";
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "-o" && i + 1 < argc) {
                exePath = argv[i + 1];
                break;
            }
        }
        Transpiler transpiler;
        std::string cppCode = transpiler.transpileToCpp(*program);
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

        std::cout << "\033[1;33mCompilando executavel nativo com g++...\033[0m\n";
        std::string buildCmd = "\"" + gppCmd + "\" -std=c++20 -O2 " + tmpCpp + " -o " + exePath;
        // On Windows cmd, wrap whole line in an extra quote
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
