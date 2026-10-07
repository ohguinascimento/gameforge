#include "AST.h"

namespace GameLang {

static void printIndent(std::ostream& os, int indent) {
    for (int i = 0; i < indent; ++i) os << "  ";
}

void LiteralExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    switch (kind) {
        case Kind::Number: os << "Literal(Number: " << numberVal << ")\n"; break;
        case Kind::String: os << "Literal(String: \"" << stringVal << "\")\n"; break;
        case Kind::Boolean: os << "Literal(Bool: " << (boolVal ? "true" : "false") << ")\n"; break;
        case Kind::Null: os << "Literal(Null)\n"; break;
    }
}

void VariableExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "Variable(" << name << ")\n";
}

void BinaryExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "BinaryExpr(Op: " << Token::tokenTypeName(op) << ")\n";
    if (left) left->dump(os, indent + 1);
    if (right) right->dump(os, indent + 1);
}

void UnaryExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "UnaryExpr(Op: " << Token::tokenTypeName(op) << ")\n";
    if (right) right->dump(os, indent + 1);
}

void AssignExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "AssignExpr(Var: " << name << " Op: " << Token::tokenTypeName(op) << ")\n";
    if (value) value->dump(os, indent + 1);
}

void MemberAccessExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "MemberAccess(." << member << ")\n";
    if (object) object->dump(os, indent + 1);
}

void MemberAssignExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "MemberAssign(." << member << " Op: " << Token::tokenTypeName(op) << ")\n";
    printIndent(os, indent + 1); os << "Object:\n";
    if (object) object->dump(os, indent + 2);
    printIndent(os, indent + 1); os << "Value:\n";
    if (value) value->dump(os, indent + 2);
}

void CallExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "CallExpr(" << callee << ")\n";
    for (const auto& arg : arguments) {
        arg->dump(os, indent + 1);
    }
}

void SpawnExpr::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "SpawnExpr(" << entityType << ")\n";
}

void ExprStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "ExprStmt:\n";
    if (expr) expr->dump(os, indent + 1);
}

void VarDeclStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "VarDecl(" << name << ")\n";
    if (initializer) initializer->dump(os, indent + 1);
}

void BlockStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "BlockStmt (" << statements.size() << " statements):\n";
    for (const auto& stmt : statements) {
        stmt->dump(os, indent + 1);
    }
}

void IfStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "IfStmt:\n";
    printIndent(os, indent + 1); os << "Condition:\n";
    if (condition) condition->dump(os, indent + 2);
    printIndent(os, indent + 1); os << "Then:\n";
    if (thenBranch) thenBranch->dump(os, indent + 2);
    if (elseBranch) {
        printIndent(os, indent + 1); os << "Else:\n";
        elseBranch->dump(os, indent + 2);
    }
}

void WhileStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "WhileStmt:\n";
    printIndent(os, indent + 1); os << "Condition:\n";
    if (condition) condition->dump(os, indent + 2);
    printIndent(os, indent + 1); os << "Body:\n";
    if (body) body->dump(os, indent + 2);
}

void ReturnStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "ReturnStmt\n";
    if (value) value->dump(os, indent + 1);
}

void DestroyStmt::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "DestroyStmt\n";
    if (target) target->dump(os, indent + 1);
}

void EntityDecl::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "EntityDecl(" << name << "):\n";
    for (const auto& field : fields) {
        printIndent(os, indent + 1);
        os << "Field(" << field.name << ")\n";
        if (field.defaultValue) field.defaultValue->dump(os, indent + 2);
    }
}

void FunctionDecl::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "FunctionDecl(" << name << ") Params: [";
    for (size_t i = 0; i < params.size(); ++i) {
        os << params[i] << (i + 1 < params.size() ? ", " : "");
    }
    os << "]\n";
    if (body) body->dump(os, indent + 1);
}

void CollisionHandlerDecl::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "CollisionHandler(" << entityA << " as " << varA << ", "
       << entityB << " as " << varB << ")\n";
    if (body) body->dump(os, indent + 1);
}

void Program::dump(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "Program [Title: \"" << config.title << "\", Size: "
       << config.width << "x" << config.height << ", FPS: " << config.fps << "]\n";

    if (!globals.empty()) {
        printIndent(os, indent + 1); os << "Globals:\n";
        for (const auto& g : globals) g->dump(os, indent + 2);
    }

    if (!entities.empty()) {
        printIndent(os, indent + 1); os << "Entities:\n";
        for (const auto& e : entities) e->dump(os, indent + 2);
    }

    if (!functions.empty()) {
        printIndent(os, indent + 1); os << "Functions:\n";
        for (const auto& f : functions) f->dump(os, indent + 2);
    }

    if (initBlock) {
        printIndent(os, indent + 1); os << "InitBlock:\n";
        initBlock->dump(os, indent + 2);
    }

    if (updateBlock) {
        printIndent(os, indent + 1); os << "UpdateBlock:\n";
        updateBlock->dump(os, indent + 2);
    }

    if (renderBlock) {
        printIndent(os, indent + 1); os << "RenderBlock:\n";
        renderBlock->dump(os, indent + 2);
    }

    if (!collisionHandlers.empty()) {
        printIndent(os, indent + 1); os << "CollisionHandlers:\n";
        for (const auto& c : collisionHandlers) c->dump(os, indent + 2);
    }
}

} // namespace GameLang
