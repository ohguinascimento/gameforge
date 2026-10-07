#pragma once

#include "Common.h"
#include "Bytecode.h"
#include "Engine.h"
#include "TimeEngine.h"
#include "DebugInspector.h"

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

    GameForge::Time::TemporalBuffer<300>& getTemporalBuffer() { return temporalBuffer; }
    const GameForge::Time::TemporalBuffer<300>& getTemporalBuffer() const { return temporalBuffer; }

    GameForge::Debug::DebugInspector& getInspector() { return inspector; }
    const GameForge::Debug::DebugInspector& getInspector() const { return inspector; }

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

    // Temporal Spectrum & Live Debugging Subsystems
    GameForge::Time::TemporalBuffer<300> temporalBuffer;
    std::vector<GameForge::Time::TemporalEcho> temporalEchoes;
    GameForge::Debug::DebugInspector inspector;
    std::unordered_map<std::string, int> frozenTypes;

    bool hasError = false;
    bool safeMode = true;
    int suppressedErrorCount = 0;
    std::string errorMessage;
    std::string lastIsolatedError;
    std::vector<std::string> errorLog;
};

} // namespace GameLang
