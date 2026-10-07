#include "SemanticAnalyzer.h"
#include <iostream>

namespace GameLang {

bool SemanticAnalyzer::isBuiltinFunction(const std::string& name) const {
    static const std::unordered_set<std::string> builtins = {
        "sin", "cos", "tan", "sqrt", "abs", "rnd", "random", "key", "key_down", "key_pressed", "beep",
        "count", "tile", "tile_at", "tile_solid", "map_box", "map_row",
        "camera", "msg", "dialog", "clear_msg", "color", "print", "len",
        "dist", "clamp", "min", "max", "draw_rect", "draw_box", "draw_text",
        "set_bloom", "set_scanlines", "set_light",
        "time_rewind", "time_scale", "spawn_echo", "freeze_type", "god_mode", "tweak_var"
    };
    return builtins.find(name) != builtins.end();
}

void SemanticAnalyzer::addError(const std::string& msg, int line, int col) {
    diagnostics.push_back({ SemanticDiagnostic::Severity::Error, msg, line, col });
    errorCount++;
}

void SemanticAnalyzer::addWarning(const std::string& msg, int line, int col) {
    diagnostics.push_back({ SemanticDiagnostic::Severity::Warning, msg, line, col });
    warningCount++;
}

bool SemanticAnalyzer::analyze(const Program& program) {
    diagnostics.clear();
    declaredEntities.clear();
    entityFields.clear();
    declaredGlobals.clear();
    declaredFunctions.clear();
    collisionPairs.clear();
    errorCount = 0;
    warningCount = 0;

    // Passo 1: Coleta e validação de declarações de nível superior
    collectDeclarations(program);

    // Passo 2: Validação de entidades e campos
    validateEntities(program);

    // Passo 3: Validação de tratadores de colisão declarativos
    validateCollisionHandlers(program);

    // Passo 4: Validação de funções e hooks de ciclo de vida (init, update, render)
    validateFunctions(program);

    std::unordered_set<std::string> globalScope = declaredGlobals;
    if (program.initBlock) {
        validateBlock(program.initBlock.get(), globalScope);
    }
    if (program.updateBlock) {
        validateBlock(program.updateBlock.get(), globalScope);
    }
    if (program.renderBlock) {
        validateBlock(program.renderBlock.get(), globalScope);
    }

    return errorCount == 0;
}

void SemanticAnalyzer::collectDeclarations(const Program& program) {
    // Valida variáveis globais
    for (const auto& g : program.globals) {
        if (!g) continue;
        if (declaredGlobals.find(g->name) != declaredGlobals.end()) {
            addWarning("Variavel global '" + g->name + "' redeclarada.");
        }
        declaredGlobals.insert(g->name);
    }

    // Valida declarações de entidades
    for (const auto& ent : program.entities) {
        if (!ent) continue;
        if (declaredEntities.find(ent->name) != declaredEntities.end()) {
            addError("Entidade '" + ent->name + "' declarada em duplicidade.");
        }
        declaredEntities.insert(ent->name);

        std::unordered_set<std::string> fields;
        for (const auto& f : ent->fields) {
            if (fields.find(f.name) != fields.end()) {
                addWarning("Campo '" + f.name + "' duplicado na entidade '" + ent->name + "'.");
            }
            fields.insert(f.name);
        }
        entityFields[ent->name] = std::move(fields);
    }

    // Valida funções
    for (const auto& fn : program.functions) {
        if (!fn) continue;
        if (declaredFunctions.find(fn->name) != declaredFunctions.end()) {
            addError("Funcao '" + fn->name + "' declarada em duplicidade.");
        }
        declaredFunctions[fn->name] = fn->params.size();
    }
}

void SemanticAnalyzer::validateEntities(const Program& program) {
    for (const auto& ent : program.entities) {
        if (!ent) continue;
        for (const auto& f : ent->fields) {
            if (f.defaultValue) {
                std::unordered_set<std::string> emptyScope;
                validateExpression(f.defaultValue.get(), emptyScope);
            }
        }
    }
}

void SemanticAnalyzer::validateCollisionHandlers(const Program& program) {
    for (const auto& ch : program.collisionHandlers) {
        if (!ch) continue;

        // Verifica se ambas as entidades existem
        if (declaredEntities.find(ch->entityA) == declaredEntities.end()) {
            addError("Tratador de colisão referencia entidade indefinida: '" + ch->entityA + "'.");
        }
        if (declaredEntities.find(ch->entityB) == declaredEntities.end()) {
            addError("Tratador de colisão referencia entidade indefinida: '" + ch->entityB + "'.");
        }

        // Verifica duplicidade de tratador para o mesmo par
        std::string pairKey = ch->entityA + "::" + ch->entityB;
        if (collisionPairs.find(pairKey) != collisionPairs.end()) {
            addWarning("Tratador de colisão duplicado para o par: " + ch->entityA + " vs " + ch->entityB + ".");
        }
        collisionPairs.insert(pairKey);

        // Escopo do bloco de colisão inclui os parâmetros das entidades
        std::unordered_set<std::string> colScope = declaredGlobals;
        if (!ch->varA.empty()) colScope.insert(ch->varA);
        if (!ch->varB.empty()) colScope.insert(ch->varB);

        if (ch->body) {
            validateBlock(ch->body.get(), colScope);
        }
    }
}

void SemanticAnalyzer::validateFunctions(const Program& program) {
    for (const auto& fn : program.functions) {
        if (!fn) continue;
        std::unordered_set<std::string> fnScope = declaredGlobals;
        for (const auto& param : fn->params) {
            fnScope.insert(param);
        }
        if (fn->body) {
            validateBlock(fn->body.get(), fnScope);
        }
    }
}

void SemanticAnalyzer::validateBlock(const BlockStmt* block, const std::unordered_set<std::string>& localScope) {
    if (!block) return;
    std::unordered_set<std::string> currentScope = localScope;
    for (const auto& stmt : block->statements) {
        if (stmt) {
            validateStatement(stmt.get(), currentScope);
        }
    }
}

void SemanticAnalyzer::validateStatement(const Stmt* stmt, std::unordered_set<std::string>& localScope) {
    if (!stmt) return;

    if (auto exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        validateExpression(exprStmt->expr.get(), localScope);
    } else if (auto varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        if (varDecl->initializer) {
            validateExpression(varDecl->initializer.get(), localScope);
        }
        localScope.insert(varDecl->name);
    } else if (auto blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        validateBlock(blockStmt, localScope);
    } else if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        validateExpression(ifStmt->condition.get(), localScope);
        if (ifStmt->thenBranch) {
            std::unordered_set<std::string> branchScope = localScope;
            validateStatement(ifStmt->thenBranch.get(), branchScope);
        }
        if (ifStmt->elseBranch) {
            std::unordered_set<std::string> branchScope = localScope;
            validateStatement(ifStmt->elseBranch.get(), branchScope);
        }
    } else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        validateExpression(whileStmt->condition.get(), localScope);
        if (whileStmt->body) {
            std::unordered_set<std::string> bodyScope = localScope;
            validateStatement(whileStmt->body.get(), bodyScope);
        }
    } else if (auto returnStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        if (returnStmt->value) {
            validateExpression(returnStmt->value.get(), localScope);
        }
    } else if (auto destroyStmt = dynamic_cast<const DestroyStmt*>(stmt)) {
        if (destroyStmt->target) {
            validateExpression(destroyStmt->target.get(), localScope);
        }
    }
}

void SemanticAnalyzer::validateExpression(const Expr* expr, const std::unordered_set<std::string>& localScope) {
    if (!expr) return;

    if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        validateExpression(bin->left.get(), localScope);
        validateExpression(bin->right.get(), localScope);
    } else if (auto un = dynamic_cast<const UnaryExpr*>(expr)) {
        validateExpression(un->right.get(), localScope);
    } else if (auto asgn = dynamic_cast<const AssignExpr*>(expr)) {
        validateExpression(asgn->value.get(), localScope);
    } else if (auto mem = dynamic_cast<const MemberAccessExpr*>(expr)) {
        validateExpression(mem->object.get(), localScope);
    } else if (auto memAsgn = dynamic_cast<const MemberAssignExpr*>(expr)) {
        validateExpression(memAsgn->object.get(), localScope);
        validateExpression(memAsgn->value.get(), localScope);
    } else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        // Verifica se a função chamada é declarada ou builtin
        if (!isBuiltinFunction(call->callee) && declaredFunctions.find(call->callee) == declaredFunctions.end()) {
            addWarning("Chamada para funcao nao declarada: '" + call->callee + "()'.");
        } else if (declaredFunctions.find(call->callee) != declaredFunctions.end()) {
            size_t expected = declaredFunctions[call->callee];
            if (call->arguments.size() != expected) {
                addWarning("Funcao '" + call->callee + "' chamada com " +
                           std::to_string(call->arguments.size()) + " argumentos, esperava " +
                           std::to_string(expected) + ".");
            }
        }
        for (const auto& arg : call->arguments) {
            validateExpression(arg.get(), localScope);
        }
    } else if (auto spawn = dynamic_cast<const SpawnExpr*>(expr)) {
        if (declaredEntities.find(spawn->entityType) == declaredEntities.end()) {
            addError("Comando 'spawn' referencia entidade inexistente: '" + spawn->entityType + "'.");
        }
    }
}

void SemanticAnalyzer::printReport(std::ostream& os) const {
    if (diagnostics.empty()) {
        os << "\033[1;32m[Analise Semantica]: 0 erros, 0 avisos. AST perfeitamente tipada e valida.\033[0m\n";
        return;
    }

    os << "=== Relatorio de Verificacao Semantica ===\n";
    for (const auto& d : diagnostics) {
        if (d.severity == SemanticDiagnostic::Severity::Error) {
            os << "\033[1;31m[Erro Semantico]\033[0m " << d.message << "\n";
        } else {
            os << "\033[1;33m[Aviso Semantico]\033[0m " << d.message << "\n";
        }
    }
    os << "Total: " << errorCount << " erro(s), " << warningCount << " aviso(s).\n";
}

} // namespace GameLang
