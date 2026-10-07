//! ==============================================================================
//! GameForge Bytecode Optimizer Pass (Rust-powered peephole optimization)
//! ==============================================================================

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OpCode {
    OpConstant = 0,
    OpNull = 1,
    OpTrue = 2,
    OpFalse = 3,
    OpPop = 4,
    OpGetGlobal = 5,
    OpSetGlobal = 6,
    OpGetLocal = 7,
    OpSetLocal = 8,
    OpGetProperty = 9,
    OpSetProperty = 10,
    OpEqual = 11,
    OpNotEqual = 12,
    OpGreater = 13,
    OpGreaterEqual = 14,
    OpLess = 15,
    OpLessEqual = 16,
    OpAdd = 17,
    OpSub = 18,
    OpMul = 19,
    OpDiv = 20,
    OpMod = 21,
    OpNot = 22,
    OpNegate = 23,
    OpJump = 24,
    OpJumpIfFalse = 25,
    OpLoop = 26,
    OpCall = 27,
    OpReturn = 28,
    OpSpawn = 29,
    OpDestroy = 30,
    OpKeyDown = 31,
    OpKeyPressed = 32,
    OpBeep = 33,
    OpRandom = 34,
    OpPrintAt = 35,
    OpCountEntities = 36,
    OpTileSet = 37,
    OpTileSolid = 38,
    OpTileGet = 39,
    OpMapBox = 40,
    OpMapRow = 41,
    OpCameraSet = 42,
    OpSetMessage = 43,
}

impl OpCode {
    pub fn from_u8(val: u8) -> Option<Self> {
        if val <= 43 {
            Some(unsafe { std::mem::transmute(val) })
        } else {
            None
        }
    }
}

pub struct OptimizationReport {
    pub redundant_pops_removed: usize,
    pub dead_instructions_eliminated: usize,
    pub original_size: usize,
    pub optimized_size: usize,
}

/// Otimizador de Bytecode em Rust: Remove sequências redundantes e código morto
pub fn optimize_bytecode(code: &[u8]) -> (Vec<u8>, OptimizationReport) {
    let mut optimized = Vec::with_capacity(code.len());
    let mut i = 0;
    let mut pops_removed = 0;
    let mut dead_removed = 0;

    while i < code.len() {
        let byte = code[i];
        let op = OpCode::from_u8(byte);

        match op {
            // Otimização: Identifica push seguido imediatamente de pop redundante
            Some(OpCode::OpNull) if i + 1 < code.len() && code[i + 1] == OpCode::OpPop as u8 => {
                // Elimina o par (OpNull, OpPop)
                pops_removed += 1;
                i += 2;
                continue;
            }

            // Otimização: Elimina instruções mortas consecutivas após OP_RETURN no mesmo bloco
            Some(OpCode::OpReturn) => {
                optimized.push(byte);
                i += 1;
                // Pula instruções inalcançáveis até o fim ou próximo rótulo
                while i < code.len() && code[i] == OpCode::OpReturn as u8 {
                    dead_removed += 1;
                    i += 1;
                }
                continue;
            }

            // Opcodes com argumentos de 2 bytes (offset/slot/constante)
            Some(OpCode::OpConstant)
            | Some(OpCode::OpGetGlobal)
            | Some(OpCode::OpSetGlobal)
            | Some(OpCode::OpGetLocal)
            | Some(OpCode::OpSetLocal)
            | Some(OpCode::OpGetProperty)
            | Some(OpCode::OpSetProperty)
            | Some(OpCode::OpJump)
            | Some(OpCode::OpJumpIfFalse)
            | Some(OpCode::OpLoop)
            | Some(OpCode::OpSpawn) => {
                optimized.push(byte);
                if i + 2 < code.len() {
                    optimized.push(code[i + 1]);
                    optimized.push(code[i + 2]);
                    i += 3;
                } else {
                    i += 1;
                }
                continue;
            }

            // Opcodes com 1 byte de argumento (ex: OP_CALL com arg count)
            Some(OpCode::OpCall) => {
                optimized.push(byte);
                if i + 1 < code.len() {
                    optimized.push(code[i + 1]);
                    i += 2;
                } else {
                    i += 1;
                }
                continue;
            }

            _ => {
                optimized.push(byte);
                i += 1;
            }
        }
    }

    let report = OptimizationReport {
        redundant_pops_removed: pops_removed,
        dead_instructions_eliminated: dead_removed,
        original_size: code.len(),
        optimized_size: optimized.len(),
    };

    (optimized, report)
}

/// Validador estático de integridade do bytecode
pub fn verify_bytecode_safety(code: &[u8]) -> bool {
    let mut i = 0;
    while i < code.len() {
        let op = match OpCode::from_u8(code[i]) {
            Some(o) => o,
            None => return false, // Opcode desconhecido ou corrompido
        };

        match op {
            OpCode::OpConstant
            | OpCode::OpGetGlobal
            | OpCode::OpSetGlobal
            | OpCode::OpGetLocal
            | OpCode::OpSetLocal
            | OpCode::OpGetProperty
            | OpCode::OpSetProperty
            | OpCode::OpJump
            | OpCode::OpJumpIfFalse
            | OpCode::OpLoop
            | OpCode::OpSpawn => {
                if i + 2 >= code.len() {
                    return false; // Truncado
                }
                i += 3;
            }
            OpCode::OpCall => {
                if i + 1 >= code.len() {
                    return false;
                }
                i += 2;
            }
            _ => {
                i += 1;
            }
        }
    }
    true
}
