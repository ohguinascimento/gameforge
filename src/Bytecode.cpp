#include "Bytecode.h"

namespace GameLang {

const char* opCodeName(OpCode op) {
    switch (op) {
        case OpCode::OP_CONSTANT: return "OP_CONSTANT";
        case OpCode::OP_NULL: return "OP_NULL";
        case OpCode::OP_TRUE: return "OP_TRUE";
        case OpCode::OP_FALSE: return "OP_FALSE";
        case OpCode::OP_POP: return "OP_POP";
        case OpCode::OP_ADD: return "OP_ADD";
        case OpCode::OP_SUB: return "OP_SUB";
        case OpCode::OP_MUL: return "OP_MUL";
        case OpCode::OP_DIV: return "OP_DIV";
        case OpCode::OP_MOD: return "OP_MOD";
        case OpCode::OP_NEGATE: return "OP_NEGATE";
        case OpCode::OP_NOT: return "OP_NOT";
        case OpCode::OP_EQUAL: return "OP_EQUAL";
        case OpCode::OP_NOT_EQUAL: return "OP_NOT_EQUAL";
        case OpCode::OP_LESS: return "OP_LESS";
        case OpCode::OP_LESS_EQUAL: return "OP_LESS_EQUAL";
        case OpCode::OP_GREATER: return "OP_GREATER";
        case OpCode::OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";
        case OpCode::OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OpCode::OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OpCode::OP_GET_LOCAL: return "OP_GET_LOCAL";
        case OpCode::OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OpCode::OP_GET_PROPERTY: return "OP_GET_PROPERTY";
        case OpCode::OP_SET_PROPERTY: return "OP_SET_PROPERTY";
        case OpCode::OP_JUMP: return "OP_JUMP";
        case OpCode::OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OpCode::OP_CALL: return "OP_CALL";
        case OpCode::OP_RETURN: return "OP_RETURN";
        case OpCode::OP_SPAWN: return "OP_SPAWN";
        case OpCode::OP_DESTROY: return "OP_DESTROY";
        case OpCode::OP_KEY_DOWN: return "OP_KEY_DOWN";
        case OpCode::OP_KEY_PRESSED: return "OP_KEY_PRESSED";
        case OpCode::OP_BEEP: return "OP_BEEP";
        case OpCode::OP_RANDOM: return "OP_RANDOM";
        case OpCode::OP_PRINT_AT: return "OP_PRINT_AT";
        case OpCode::OP_COUNT_ENTITIES: return "OP_COUNT_ENTITIES";
        default: return "OP_UNKNOWN";
    }
}

void Chunk::disassemble(std::ostream& os, const std::string& name) const {
    os << "== Disassembly: " << name << " ==\n";
    for (size_t offset = 0; offset < code.size();) {
        offset = disassembleInstruction(os, offset);
    }
}

size_t Chunk::disassembleInstruction(std::ostream& os, size_t offset) const {
    os << std::right << std::setw(4) << std::setfill('0') << offset << " ";

    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        os << "   | ";
    } else {
        os << std::right << std::setw(4) << std::setfill(' ') << lines[offset] << " ";
    }
    os << std::setfill(' ');

    uint8_t instruction = code[offset];
    OpCode op = static_cast<OpCode>(instruction);
    os << std::left << std::setw(18) << opCodeName(op);

    switch (op) {
        case OpCode::OP_CONSTANT: {
            uint16_t constantIdx = (code[offset + 1] << 8) | code[offset + 2];
            os << " " << std::setw(4) << constantIdx << " '" << constants[constantIdx].toString() << "'\n";
            return offset + 3;
        }
        case OpCode::OP_GET_GLOBAL:
        case OpCode::OP_SET_GLOBAL:
        case OpCode::OP_GET_PROPERTY:
        case OpCode::OP_SET_PROPERTY:
        case OpCode::OP_SPAWN: {
            uint16_t constantIdx = (code[offset + 1] << 8) | code[offset + 2];
            os << " " << std::setw(4) << constantIdx << " '" << constants[constantIdx].toString() << "'\n";
            return offset + 3;
        }
        case OpCode::OP_GET_LOCAL:
        case OpCode::OP_SET_LOCAL: {
            uint16_t slot = (code[offset + 1] << 8) | code[offset + 2];
            os << " slot " << slot << "\n";
            return offset + 3;
        }
        case OpCode::OP_JUMP:
        case OpCode::OP_JUMP_IF_FALSE: {
            int16_t jumpOffset = static_cast<int16_t>((code[offset + 1] << 8) | code[offset + 2]);
            os << " -> " << (offset + 3 + jumpOffset) << "\n";
            return offset + 3;
        }
        case OpCode::OP_CALL: {
            uint8_t argCount = code[offset + 1];
            os << " (args: " << (int)argCount << ")\n";
            return offset + 2;
        }
        default:
            os << "\n";
            return offset + 1;
    }
}

} // namespace GameLang
