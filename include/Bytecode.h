#pragma once

#include "Common.h"

namespace GameLang {

enum class ValueType {
    Null,
    Number,
    Boolean,
    String,
    EntityId
};

struct Value {
    ValueType type = ValueType::Null;
    double numberVal = 0.0;
    bool boolVal = false;
    std::string stringVal;
    uint32_t entityId = 0;

    Value() : type(ValueType::Null) {}
    Value(double n) : type(ValueType::Number), numberVal(n) {}
    Value(bool b) : type(ValueType::Boolean), boolVal(b) {}
    Value(std::string s) : type(ValueType::String), stringVal(std::move(s)) {}
    static Value makeEntity(uint32_t id) {
        Value v;
        v.type = ValueType::EntityId;
        v.entityId = id;
        return v;
    }

    bool isNull() const { return type == ValueType::Null; }
    bool isNumber() const { return type == ValueType::Number; }
    bool isBool() const { return type == ValueType::Boolean; }
    bool isString() const { return type == ValueType::String; }
    bool isEntity() const { return type == ValueType::EntityId; }

    double asNumber() const { return numberVal; }
    bool asBool() const {
        if (type == ValueType::Boolean) return boolVal;
        if (type == ValueType::Number) return numberVal != 0.0;
        if (type == ValueType::String) return !stringVal.empty();
        if (type == ValueType::EntityId) return entityId != 0;
        return false;
    }
    const std::string& asString() const { return stringVal; }
    uint32_t asEntity() const { return entityId; }

    std::string toString() const {
        switch (type) {
            case ValueType::Null: return "null";
            case ValueType::Boolean: return boolVal ? "true" : "false";
            case ValueType::Number: {
                if (std::floor(numberVal) == numberVal) {
                    return std::to_string(static_cast<long long>(numberVal));
                }
                std::ostringstream ss;
                ss << numberVal;
                return ss.str();
            }
            case ValueType::String: return stringVal;
            case ValueType::EntityId: return "<Entity #" + std::to_string(entityId) + ">";
        }
        return "unknown";
    }

    bool operator==(const Value& other) const {
        if (type != other.type) return false;
        switch (type) {
            case ValueType::Null: return true;
            case ValueType::Boolean: return boolVal == other.boolVal;
            case ValueType::Number: return numberVal == other.numberVal;
            case ValueType::String: return stringVal == other.stringVal;
            case ValueType::EntityId: return entityId == other.entityId;
        }
        return false;
    }
};

enum class OpCode : uint8_t {
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEGATE,
    OP_NOT,

    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,

    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_GET_PROPERTY,
    OP_SET_PROPERTY,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_CALL,
    OP_RETURN,

    // Game Primitives
    OP_SPAWN,
    OP_DESTROY,
    OP_KEY_DOWN,
    OP_KEY_PRESSED,
    OP_BEEP,
    OP_RANDOM,
    OP_PRINT_AT,
    OP_COUNT_ENTITIES,

    // RPG Map Operations
    OP_TILE_SET,
    OP_TILE_SOLID,
    OP_TILE_GET,
    OP_MAP_BOX,
    OP_MAP_ROW,
    OP_CAMERA_SET,
    OP_SET_MESSAGE,

    // Temporal Spectrum & Live-Tuning Operations
    OP_TIME_REWIND,
    OP_SET_TIMESCALE,
    OP_SPAWN_ECHO,
    OP_FREEZE_TYPE,
    OP_SET_GODMODE
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;

    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    size_t addConstant(Value value) {
        constants.push_back(value);
        return constants.size() - 1;
    }

    void disassemble(std::ostream& os, const std::string& name) const;
    size_t disassembleInstruction(std::ostream& os, size_t offset) const;
};

struct CompiledFunction {
    std::string name;
    int arity = 0;
    Chunk chunk;
};

struct CompiledCollisionHandler {
    std::string entityA;
    std::string entityB;
    std::string varA;
    std::string varB;
    Chunk chunk;
};

struct CompiledGame {
    GameConfig config;
    std::unordered_map<std::string, std::vector<std::string>> entityFields;
    std::unordered_map<std::string, std::unordered_map<std::string, Value>> entityDefaultValues;

    Chunk globalInitChunk;
    Chunk initChunk;
    Chunk updateChunk;
    Chunk renderChunk;
    std::unordered_map<std::string, CompiledFunction> functions;
    std::vector<CompiledCollisionHandler> collisionHandlers;
};

const char* opCodeName(OpCode op);

} // namespace GameLang
