#pragma once

#include "Common.h"

namespace GameLang {

enum class TokenType {
    // Literals & Identifiers
    Identifier,
    Number,
    String,

    // Keywords
    Game,
    Entity,
    Var,
    Func,
    Return,
    If,
    Else,
    While,
    For,
    Init,
    Update,
    Render,
    On,
    Collision,
    As,
    Spawn,
    Destroy,
    True,
    False,
    Null,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equal,
    PlusEqual,
    MinusEqual,
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Bang,
    AmpAmp,
    PipePipe,

    // Delimiters
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Comma,
    Semicolon,
    Colon,
    Dot,
    At,

    // End of file / Error
    EndOfFile,
    Error
};

struct Token {
    TokenType type;
    std::string text;
    double numberValue = 0.0;
    SourceLocation location;

    std::string toString() const {
        std::ostringstream ss;
        ss << "[" << location.line << ":" << location.column << " " << tokenTypeName(type) << " '" << text << "']";
        return ss.str();
    }

    static const char* tokenTypeName(TokenType t);
};

inline const char* Token::tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";
        case TokenType::Game: return "game";
        case TokenType::Entity: return "entity";
        case TokenType::Var: return "var";
        case TokenType::Func: return "func";
        case TokenType::Return: return "return";
        case TokenType::If: return "if";
        case TokenType::Else: return "else";
        case TokenType::While: return "while";
        case TokenType::For: return "for";
        case TokenType::Init: return "init";
        case TokenType::Update: return "update";
        case TokenType::Render: return "render";
        case TokenType::On: return "on";
        case TokenType::Collision: return "collision";
        case TokenType::As: return "as";
        case TokenType::Spawn: return "spawn";
        case TokenType::Destroy: return "destroy";
        case TokenType::True: return "true";
        case TokenType::False: return "false";
        case TokenType::Null: return "null";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Equal: return "=";
        case TokenType::PlusEqual: return "+=";
        case TokenType::MinusEqual: return "-=";
        case TokenType::EqualEqual: return "==";
        case TokenType::BangEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::LessEqual: return "<=";
        case TokenType::Greater: return ">";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::Bang: return "!";
        case TokenType::AmpAmp: return "&&";
        case TokenType::PipePipe: return "||";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::LeftBrace: return "{";
        case TokenType::RightBrace: return "}";
        case TokenType::LeftBracket: return "[";
        case TokenType::RightBracket: return "]";
        case TokenType::Comma: return ",";
        case TokenType::Semicolon: return ";";
        case TokenType::Colon: return ":";
        case TokenType::Dot: return ".";
        case TokenType::At: return "@";
        case TokenType::EndOfFile: return "EOF";
        case TokenType::Error: return "Error";
        default: return "Unknown";
    }
}

} // namespace GameLang
