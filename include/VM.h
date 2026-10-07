#pragma once

#include "Common.h"
#include "Bytecode.h"
#include "Engine.h"

namespace GameLang {

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

class VM {
public:
    VM(CompiledGame& game, Engine& engine);

    InterpretResult run();

    // Direct chunk execution
    InterpretResult executeChunk(const Chunk& chunk, size_t baseOffset = 0);

    const std::unordered_map<std::string, Value>& getGlobals() const { return globals; }

    void setSafeMode(bool enabled) { safeMode = enabled; }
    bool isSafeMode() const { return safeMode; }
    int getSuppressedErrorCount() const { return suppressedErrorCount; }
    const std::vector<std::string>& getErrorLog() const { return errorLog; }
    const std::string& getLastIsolatedError() const { return lastIsolatedError; }

private:
    struct CallFrame {
        const Chunk* chunk = nullptr;
        size_t ip = 0;
        size_t slotOffset = 0;
    };

    void push(Value value);
    Value pop();
    Value peek(int distance = 0) const;

    void runtimeError(const std::string& message);

    void runInit();
    void runUpdate();
    void runRender();
    void runCollisions();

    CompiledGame& game;
    Engine& engine;

    std::vector<Value> stack;
    std::vector<CallFrame> frames;
    std::unordered_map<std::string, Value> globals;

    bool hasError = false;
    bool safeMode = true;
    int suppressedErrorCount = 0;
    std::string errorMessage;
    std::string lastIsolatedError;
    std::vector<std::string> errorLog;
};

} // namespace GameLang
