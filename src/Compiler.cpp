#include "Compiler.h"

namespace GameLang {

Compiler::Compiler() {}

void Compiler::error(const std::string& message) {
    errors.push_back("[Compiler Error] " + message);
}

void Compiler::emitByte(Chunk& chunk, uint8_t byte, int line) {
    chunk.write(byte, line);
}

void Compiler::emitBytes(Chunk& chunk, uint8_t b1, uint8_t b2, int line) {
    chunk.write(b1, line);
    chunk.write(b2, line);
}

void Compiler::emitOp(Chunk& chunk, OpCode op, int line) {
    emitByte(chunk, static_cast<uint8_t>(op), line);
}

void Compiler::emitConstant(Chunk& chunk, Value value, int line) {
    size_t idx = chunk.addConstant(value);
    if (idx > 0xFFFF) {
        error("Too many constants in chunk");
        return;
    }
    emitOp(chunk, OpCode::OP_CONSTANT, line);
    emitByte(chunk, static_cast<uint8_t>((idx >> 8) & 0xFF), line);
    emitByte(chunk, static_cast<uint8_t>(idx & 0xFF), line);
}

size_t Compiler::emitJump(Chunk& chunk, OpCode op, int line) {
    emitOp(chunk, op, line);
    emitByte(chunk, 0xFF, line);
    emitByte(chunk, 0xFF, line);
    return chunk.code.size() - 2;
}

void Compiler::patchJump(Chunk& chunk, size_t offset) {
    // Jump from the instruction right after the 2-byte operand
    int jump = static_cast<int>(chunk.code.size() - (offset + 2));
    if (jump > 32767 || jump < -32768) {
        error("Jump offset too large");
        return;
    }
    chunk.code[offset] = static_cast<uint8_t>((jump >> 8) & 0xFF);
    chunk.code[offset + 1] = static_cast<uint8_t>(jump & 0xFF);
}

int Compiler::resolveLocal(const ScopeContext& scope, const std::string& name) {
    for (int i = static_cast<int>(scope.locals.size()) - 1; i >= 0; --i) {
        if (scope.locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

void Compiler::beginScope(ScopeContext& scope) {
    scope.scopeDepth++;
}

void Compiler::endScope(ScopeContext& scope, Chunk& chunk) {
    scope.scopeDepth--;
    while (!scope.locals.empty() && scope.locals.back().depth > scope.scopeDepth) {
        emitOp(chunk, OpCode::OP_POP);
        scope.locals.pop_back();
    }
}

void Compiler::addLocal(ScopeContext& scope, const std::string& name) {
    scope.locals.push_back(Local{name, scope.scopeDepth});
}

std::unique_ptr<CompiledGame> Compiler::compile(const Program& program) {
    auto game = std::make_unique<CompiledGame>();
    game->config = program.config;
    compileProgram(program, *game);
    return game;
}

void Compiler::compileProgram(const Program& program, CompiledGame& game) {
    // 1. Entities
    for (const auto& entity : program.entities) {
        compileEntity(*entity, game);
    }

    // 2. Global variables
    ScopeContext globalScope;
    for (const auto& g : program.globals) {
        declaredGlobals[g->name] = true;
        if (g->initializer) {
            compileExpression(*g->initializer, game.globalInitChunk, globalScope);
        } else {
            emitOp(game.globalInitChunk, OpCode::OP_NULL);
        }
        size_t nameIdx = game.globalInitChunk.addConstant(Value(g->name));
        emitOp(game.globalInitChunk, OpCode::OP_SET_GLOBAL);
        emitByte(game.globalInitChunk, (nameIdx >> 8) & 0xFF);
        emitByte(game.globalInitChunk, nameIdx & 0xFF);
        emitOp(game.globalInitChunk, OpCode::OP_POP);
    }
    emitOp(game.globalInitChunk, OpCode::OP_RETURN);

    // 3. Functions
    for (const auto& fn : program.functions) {
        compileFunction(*fn, game);
    }

    // 4. Init Block
    if (program.initBlock) {
        ScopeContext initScope;
        compileBlock(*program.initBlock, game.initChunk, initScope);
    }
    emitOp(game.initChunk, OpCode::OP_RETURN);

    // 5. Update Block
    if (program.updateBlock) {
        ScopeContext updateScope;
        compileBlock(*program.updateBlock, game.updateChunk, updateScope);
    }
    emitOp(game.updateChunk, OpCode::OP_RETURN);

    // 6. Render Block
    if (program.renderBlock) {
        ScopeContext renderScope;
        compileBlock(*program.renderBlock, game.renderChunk, renderScope);
    }
    emitOp(game.renderChunk, OpCode::OP_RETURN);

    // 7. Collision Handlers
    for (const auto& ch : program.collisionHandlers) {
        compileCollisionHandler(*ch, game);
    }
}

void Compiler::compileEntity(const EntityDecl& entity, CompiledGame& game) {
    std::vector<std::string> fieldNames;
    std::unordered_map<std::string, Value> defaults;

    for (const auto& field : entity.fields) {
        fieldNames.push_back(field.name);
        if (field.defaultValue) {
            // Constant evaluation for field defaults if literal
            if (auto lit = dynamic_cast<LiteralExpr*>(field.defaultValue.get())) {
                switch (lit->kind) {
                    case LiteralExpr::Kind::Number: defaults[field.name] = Value(lit->numberVal); break;
                    case LiteralExpr::Kind::String: defaults[field.name] = Value(lit->stringVal); break;
                    case LiteralExpr::Kind::Boolean: defaults[field.name] = Value(lit->boolVal); break;
                    case LiteralExpr::Kind::Null: defaults[field.name] = Value(); break;
                }
            } else {
                defaults[field.name] = Value(0.0);
            }
        } else {
            defaults[field.name] = Value(0.0);
        }
    }

    game.entityFields[entity.name] = fieldNames;
    game.entityDefaultValues[entity.name] = defaults;
}

void Compiler::compileFunction(const FunctionDecl& fn, CompiledGame& game) {
    CompiledFunction compFn;
    compFn.name = fn.name;
    compFn.arity = static_cast<int>(fn.params.size());

    ScopeContext scope;
    // Parameters are slots in the function's stack frame
    for (const auto& param : fn.params) {
        addLocal(scope, param);
    }

    if (fn.body) {
        for (const auto& stmt : fn.body->statements) {
            compileStatement(*stmt, compFn.chunk, scope);
        }
    }
    emitOp(compFn.chunk, OpCode::OP_NULL);
    emitOp(compFn.chunk, OpCode::OP_RETURN);

    game.functions[fn.name] = std::move(compFn);
}

void Compiler::compileCollisionHandler(const CollisionHandlerDecl& ch, CompiledGame& game) {
    CompiledCollisionHandler compCh;
    compCh.entityA = ch.entityA;
    compCh.entityB = ch.entityB;
    compCh.varA = ch.varA;
    compCh.varB = ch.varB;

    ScopeContext scope;
    // Slots 0 and 1 are the two colliding entities
    addLocal(scope, ch.varA);
    addLocal(scope, ch.varB);

    if (ch.body) {
        for (const auto& stmt : ch.body->statements) {
            compileStatement(*stmt, compCh.chunk, scope);
        }
    }
    emitOp(compCh.chunk, OpCode::OP_RETURN);

    game.collisionHandlers.push_back(std::move(compCh));
}

void Compiler::compileBlock(const BlockStmt& block, Chunk& chunk, ScopeContext& scope) {
    beginScope(scope);
    for (const auto& stmt : block.statements) {
        compileStatement(*stmt, chunk, scope);
    }
    endScope(scope, chunk);
}

void Compiler::compileStatement(const Stmt& stmt, Chunk& chunk, ScopeContext& scope) {
    if (auto varDecl = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        compileVarDecl(*varDecl, chunk, scope);
    } else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        compileIf(*ifStmt, chunk, scope);
    } else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        compileWhile(*whileStmt, chunk, scope);
    } else if (auto blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        compileBlock(*blockStmt, chunk, scope);
    } else if (auto returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        compileReturn(*returnStmt, chunk, scope);
    } else if (auto destroyStmt = dynamic_cast<const DestroyStmt*>(&stmt)) {
        compileDestroy(*destroyStmt, chunk, scope);
    } else if (auto exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        compileExpression(*exprStmt->expr, chunk, scope);
        emitOp(chunk, OpCode::OP_POP);
    }
}

void Compiler::compileVarDecl(const VarDeclStmt& stmt, Chunk& chunk, ScopeContext& scope) {
    if (stmt.initializer) {
        compileExpression(*stmt.initializer, chunk, scope);
    } else {
        emitOp(chunk, OpCode::OP_NULL);
    }

    if (scope.scopeDepth > 0) {
        addLocal(scope, stmt.name);
    } else {
        size_t nameIdx = chunk.addConstant(Value(stmt.name));
        emitOp(chunk, OpCode::OP_SET_GLOBAL);
        emitByte(chunk, (nameIdx >> 8) & 0xFF);
        emitByte(chunk, nameIdx & 0xFF);
        emitOp(chunk, OpCode::OP_POP);
    }
}

void Compiler::compileIf(const IfStmt& stmt, Chunk& chunk, ScopeContext& scope) {
    compileExpression(*stmt.condition, chunk, scope);
    size_t thenJump = emitJump(chunk, OpCode::OP_JUMP_IF_FALSE);
    emitOp(chunk, OpCode::OP_POP); // pop condition

    compileStatement(*stmt.thenBranch, chunk, scope);

    if (stmt.elseBranch) {
        size_t elseJump = emitJump(chunk, OpCode::OP_JUMP);
        patchJump(chunk, thenJump);
        emitOp(chunk, OpCode::OP_POP); // pop condition for else branch

        compileStatement(*stmt.elseBranch, chunk, scope);
        patchJump(chunk, elseJump);
    } else {
        patchJump(chunk, thenJump);
        emitOp(chunk, OpCode::OP_POP); // pop condition when false
    }
}

void Compiler::compileWhile(const WhileStmt& stmt, Chunk& chunk, ScopeContext& scope) {
    size_t loopStart = chunk.code.size();
    compileExpression(*stmt.condition, chunk, scope);

    size_t exitJump = emitJump(chunk, OpCode::OP_JUMP_IF_FALSE);
    emitOp(chunk, OpCode::OP_POP); // pop condition

    compileStatement(*stmt.body, chunk, scope);

    // Jump back to loopStart
    int jump = static_cast<int>(chunk.code.size() + 3 - loopStart);
    emitOp(chunk, OpCode::OP_JUMP);
    emitByte(chunk, static_cast<uint8_t>(((-jump) >> 8) & 0xFF));
    emitByte(chunk, static_cast<uint8_t>((-jump) & 0xFF));

    patchJump(chunk, exitJump);
    emitOp(chunk, OpCode::OP_POP); // pop condition
}

void Compiler::compileReturn(const ReturnStmt& stmt, Chunk& chunk, ScopeContext& scope) {
    if (stmt.value) {
        compileExpression(*stmt.value, chunk, scope);
    } else {
        emitOp(chunk, OpCode::OP_NULL);
    }
    emitOp(chunk, OpCode::OP_RETURN);
}

void Compiler::compileDestroy(const DestroyStmt& stmt, Chunk& chunk, ScopeContext& scope) {
    compileExpression(*stmt.target, chunk, scope);
    emitOp(chunk, OpCode::OP_DESTROY);
}

void Compiler::compileExpression(const Expr& expr, Chunk& chunk, ScopeContext& scope) {
    if (auto lit = dynamic_cast<const LiteralExpr*>(&expr)) {
        compileLiteral(*lit, chunk);
    } else if (auto varExpr = dynamic_cast<const VariableExpr*>(&expr)) {
        compileVariable(*varExpr, chunk, scope);
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        compileBinary(*bin, chunk, scope);
    } else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        compileUnary(*un, chunk, scope);
    } else if (auto asgn = dynamic_cast<const AssignExpr*>(&expr)) {
        compileAssign(*asgn, chunk, scope);
    } else if (auto mem = dynamic_cast<const MemberAccessExpr*>(&expr)) {
        compileMemberAccess(*mem, chunk, scope);
    } else if (auto memAsgn = dynamic_cast<const MemberAssignExpr*>(&expr)) {
        compileMemberAssign(*memAsgn, chunk, scope);
    } else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        compileCall(*call, chunk, scope);
    } else if (auto spawn = dynamic_cast<const SpawnExpr*>(&expr)) {
        compileSpawn(*spawn, chunk);
    }
}

void Compiler::compileLiteral(const LiteralExpr& expr, Chunk& chunk) {
    switch (expr.kind) {
        case LiteralExpr::Kind::Number:
            emitConstant(chunk, Value(expr.numberVal));
            break;
        case LiteralExpr::Kind::String:
            emitConstant(chunk, Value(expr.stringVal));
            break;
        case LiteralExpr::Kind::Boolean:
            emitOp(chunk, expr.boolVal ? OpCode::OP_TRUE : OpCode::OP_FALSE);
            break;
        case LiteralExpr::Kind::Null:
            emitOp(chunk, OpCode::OP_NULL);
            break;
    }
}

void Compiler::compileVariable(const VariableExpr& expr, Chunk& chunk, ScopeContext& scope) {
    int localIdx = resolveLocal(scope, expr.name);
    if (localIdx != -1) {
        emitOp(chunk, OpCode::OP_GET_LOCAL);
        emitByte(chunk, (localIdx >> 8) & 0xFF);
        emitByte(chunk, localIdx & 0xFF);
    } else {
        size_t nameIdx = chunk.addConstant(Value(expr.name));
        emitOp(chunk, OpCode::OP_GET_GLOBAL);
        emitByte(chunk, (nameIdx >> 8) & 0xFF);
        emitByte(chunk, nameIdx & 0xFF);
    }
}

void Compiler::compileBinary(const BinaryExpr& expr, Chunk& chunk, ScopeContext& scope) {
    // Short circuit for && and ||
    if (expr.op == TokenType::AmpAmp) {
        compileExpression(*expr.left, chunk, scope);
        size_t endJump = emitJump(chunk, OpCode::OP_JUMP_IF_FALSE);
        emitOp(chunk, OpCode::OP_POP);
        compileExpression(*expr.right, chunk, scope);
        patchJump(chunk, endJump);
        return;
    }
    if (expr.op == TokenType::PipePipe) {
        compileExpression(*expr.left, chunk, scope);
        // If left is true, jump to end
        // Simple trick: negate or check
        size_t elseJump = emitJump(chunk, OpCode::OP_JUMP_IF_FALSE);
        size_t endJump = emitJump(chunk, OpCode::OP_JUMP);
        patchJump(chunk, elseJump);
        emitOp(chunk, OpCode::OP_POP);
        compileExpression(*expr.right, chunk, scope);
        patchJump(chunk, endJump);
        return;
    }

    compileExpression(*expr.left, chunk, scope);
    compileExpression(*expr.right, chunk, scope);

    switch (expr.op) {
        case TokenType::Plus: emitOp(chunk, OpCode::OP_ADD); break;
        case TokenType::Minus: emitOp(chunk, OpCode::OP_SUB); break;
        case TokenType::Star: emitOp(chunk, OpCode::OP_MUL); break;
        case TokenType::Slash: emitOp(chunk, OpCode::OP_DIV); break;
        case TokenType::Percent: emitOp(chunk, OpCode::OP_MOD); break;
        case TokenType::EqualEqual: emitOp(chunk, OpCode::OP_EQUAL); break;
        case TokenType::BangEqual: emitOp(chunk, OpCode::OP_NOT_EQUAL); break;
        case TokenType::Less: emitOp(chunk, OpCode::OP_LESS); break;
        case TokenType::LessEqual: emitOp(chunk, OpCode::OP_LESS_EQUAL); break;
        case TokenType::Greater: emitOp(chunk, OpCode::OP_GREATER); break;
        case TokenType::GreaterEqual: emitOp(chunk, OpCode::OP_GREATER_EQUAL); break;
        default:
            error("Unknown binary operator");
            break;
    }
}

void Compiler::compileUnary(const UnaryExpr& expr, Chunk& chunk, ScopeContext& scope) {
    compileExpression(*expr.right, chunk, scope);
    switch (expr.op) {
        case TokenType::Minus: emitOp(chunk, OpCode::OP_NEGATE); break;
        case TokenType::Bang: emitOp(chunk, OpCode::OP_NOT); break;
        default:
            error("Unknown unary operator");
            break;
    }
}

void Compiler::compileAssign(const AssignExpr& expr, Chunk& chunk, ScopeContext& scope) {
    int localIdx = resolveLocal(scope, expr.name);

    if (expr.op == TokenType::PlusEqual || expr.op == TokenType::MinusEqual) {
        // Load original
        if (localIdx != -1) {
            emitOp(chunk, OpCode::OP_GET_LOCAL);
            emitByte(chunk, (localIdx >> 8) & 0xFF);
            emitByte(chunk, localIdx & 0xFF);
        } else {
            size_t nameIdx = chunk.addConstant(Value(expr.name));
            emitOp(chunk, OpCode::OP_GET_GLOBAL);
            emitByte(chunk, (nameIdx >> 8) & 0xFF);
            emitByte(chunk, nameIdx & 0xFF);
        }
        compileExpression(*expr.value, chunk, scope);
        emitOp(chunk, expr.op == TokenType::PlusEqual ? OpCode::OP_ADD : OpCode::OP_SUB);
    } else {
        compileExpression(*expr.value, chunk, scope);
    }

    if (localIdx != -1) {
        emitOp(chunk, OpCode::OP_SET_LOCAL);
        emitByte(chunk, (localIdx >> 8) & 0xFF);
        emitByte(chunk, localIdx & 0xFF);
    } else {
        size_t nameIdx = chunk.addConstant(Value(expr.name));
        emitOp(chunk, OpCode::OP_SET_GLOBAL);
        emitByte(chunk, (nameIdx >> 8) & 0xFF);
        emitByte(chunk, nameIdx & 0xFF);
    }
}

void Compiler::compileMemberAccess(const MemberAccessExpr& expr, Chunk& chunk, ScopeContext& scope) {
    compileExpression(*expr.object, chunk, scope);
    size_t memberIdx = chunk.addConstant(Value(expr.member));
    emitOp(chunk, OpCode::OP_GET_PROPERTY);
    emitByte(chunk, (memberIdx >> 8) & 0xFF);
    emitByte(chunk, memberIdx & 0xFF);
}

void Compiler::compileMemberAssign(const MemberAssignExpr& expr, Chunk& chunk, ScopeContext& scope) {
    // Member assign: obj.field = val
    // Evaluate object
    compileExpression(*expr.object, chunk, scope);

    if (expr.op == TokenType::PlusEqual || expr.op == TokenType::MinusEqual) {
        // Duplicate object on stack for read then write
        // To simplify without OP_DUP, we can evaluate object, get property, add value, etc.
        // Or compile value:
        size_t memberIdx = chunk.addConstant(Value(expr.member));
        // Push object again
        compileExpression(*expr.object, chunk, scope);
        emitOp(chunk, OpCode::OP_GET_PROPERTY);
        emitByte(chunk, (memberIdx >> 8) & 0xFF);
        emitByte(chunk, memberIdx & 0xFF);

        compileExpression(*expr.value, chunk, scope);
        emitOp(chunk, expr.op == TokenType::PlusEqual ? OpCode::OP_ADD : OpCode::OP_SUB);

        emitOp(chunk, OpCode::OP_SET_PROPERTY);
        emitByte(chunk, (memberIdx >> 8) & 0xFF);
        emitByte(chunk, memberIdx & 0xFF);
    } else {
        compileExpression(*expr.value, chunk, scope);
        size_t memberIdx = chunk.addConstant(Value(expr.member));
        emitOp(chunk, OpCode::OP_SET_PROPERTY);
        emitByte(chunk, (memberIdx >> 8) & 0xFF);
        emitByte(chunk, memberIdx & 0xFF);
    }
}

void Compiler::compileCall(const CallExpr& expr, Chunk& chunk, ScopeContext& scope) {
    // Check built-in functions first
    if (expr.callee == "beep") {
        for (const auto& arg : expr.arguments) compileExpression(*arg, chunk, scope);
        emitOp(chunk, OpCode::OP_BEEP);
        return;
    }
    if (expr.callee == "random") {
        for (const auto& arg : expr.arguments) compileExpression(*arg, chunk, scope);
        emitOp(chunk, OpCode::OP_RANDOM);
        return;
    }
    if (expr.callee == "key" || expr.callee == "key_down") {
        if (!expr.arguments.empty()) compileExpression(*expr.arguments[0], chunk, scope);
        emitOp(chunk, OpCode::OP_KEY_DOWN);
        return;
    }
    if (expr.callee == "key_pressed") {
        if (!expr.arguments.empty()) compileExpression(*expr.arguments[0], chunk, scope);
        emitOp(chunk, OpCode::OP_KEY_PRESSED);
        return;
    }
    if (expr.callee == "print_at") {
        for (const auto& arg : expr.arguments) compileExpression(*arg, chunk, scope);
        emitOp(chunk, OpCode::OP_PRINT_AT);
        return;
    }
    if (expr.callee == "count") {
        if (!expr.arguments.empty()) compileExpression(*expr.arguments[0], chunk, scope);
        emitOp(chunk, OpCode::OP_COUNT_ENTITIES);
        return;
    }
    if (expr.callee == "destroy") {
        if (!expr.arguments.empty()) compileExpression(*expr.arguments[0], chunk, scope);
        emitOp(chunk, OpCode::OP_DESTROY);
        emitOp(chunk, OpCode::OP_NULL); // push result
        return;
    }

    // User-defined function call
    // Push callee name
    size_t fnNameIdx = chunk.addConstant(Value(expr.callee));
    emitConstant(chunk, Value(expr.callee));

    for (const auto& arg : expr.arguments) {
        compileExpression(*arg, chunk, scope);
    }

    emitOp(chunk, OpCode::OP_CALL);
    emitByte(chunk, static_cast<uint8_t>(expr.arguments.size()));
}

void Compiler::compileSpawn(const SpawnExpr& expr, Chunk& chunk) {
    size_t nameIdx = chunk.addConstant(Value(expr.entityType));
    emitOp(chunk, OpCode::OP_SPAWN);
    emitByte(chunk, (nameIdx >> 8) & 0xFF);
    emitByte(chunk, nameIdx & 0xFF);
}

} // namespace GameLang
