#include "Lexer.h"
#include <cctype>

namespace GameLang {

Lexer::Lexer(std::string source)
    : source(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;
        start = current;
        startColumn = column;
        tokens.push_back(scanToken());
    }
    tokens.push_back(Token{TokenType::EndOfFile, "", 0.0, {line, column}});
    return tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.size()) return '\0';
    return source[current + 1];
}

char Lexer::advance() {
    char c = source[current++];
    column++;
    return c;
}

bool Lexer::isAtEnd() const {
    return current >= source.size();
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source[current] != expected) return false;
    current++;
    column++;
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') {
            advance();
        } else if (c == '\n') {
            line++;
            column = 1;
            current++;
        } else if (c == '/') {
            if (peekNext() == '/') {
                // Line comment
                while (!isAtEnd() && peek() != '\n') {
                    advance();
                }
            } else if (peekNext() == '*') {
                // Block comment
                advance(); // '/'
                advance(); // '*'
                while (!isAtEnd()) {
                    if (peek() == '*' && peekNext() == '/') {
                        advance(); // '*'
                        advance(); // '/'
                        break;
                    }
                    if (peek() == '\n') {
                        line++;
                        column = 1;
                        current++;
                    } else {
                        advance();
                    }
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    return Token{type, text, 0.0, {line, startColumn}};
}

Token Lexer::makeErrorToken(const std::string& message) {
    errors.push_back("Line " + std::to_string(line) + ":" + std::to_string(startColumn) + ": " + message);
    return Token{TokenType::Error, message, 0.0, {line, startColumn}};
}

Token Lexer::scanIdentifier() {
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }
    std::string text = source.substr(start, current - start);
    TokenType type = identifierType(text);
    return Token{type, text, 0.0, {line, startColumn}};
}

TokenType Lexer::identifierType(const std::string& text) const {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"game",      TokenType::Game},
        {"entity",    TokenType::Entity},
        {"var",       TokenType::Var},
        {"func",      TokenType::Func},
        {"return",    TokenType::Return},
        {"if",        TokenType::If},
        {"else",      TokenType::Else},
        {"while",     TokenType::While},
        {"for",       TokenType::For},
        {"init",      TokenType::Init},
        {"update",    TokenType::Update},
        {"render",    TokenType::Render},
        {"on",        TokenType::On},
        {"collision", TokenType::Collision},
        {"as",        TokenType::As},
        {"spawn",     TokenType::Spawn},
        {"destroy",   TokenType::Destroy},
        {"true",      TokenType::True},
        {"false",     TokenType::False},
        {"null",      TokenType::Null}
    };

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::Identifier;
}

Token Lexer::scanNumber() {
    while (!isAtEnd() && std::isdigit(peek())) {
        advance();
    }
    if (peek() == '.' && std::isdigit(peekNext())) {
        advance(); // consume '.'
        while (!isAtEnd() && std::isdigit(peek())) {
            advance();
        }
    }
    std::string text = source.substr(start, current - start);
    double val = std::stod(text);
    Token t{TokenType::Number, text, val, {line, startColumn}};
    return t;
}

Token Lexer::scanString() {
    std::string val;
    while (!isAtEnd() && peek() != '"') {
        char c = advance();
        if (c == '\\') {
            if (isAtEnd()) break;
            char esc = advance();
            switch (esc) {
                case 'n': val.push_back('\n'); break;
                case 't': val.push_back('\t'); break;
                case 'r': val.push_back('\r'); break;
                case '"': val.push_back('"'); break;
                case '\\': val.push_back('\\'); break;
                default: val.push_back(esc); break;
            }
        } else {
            if (c == '\n') {
                line++;
                column = 1;
            }
            val.push_back(c);
        }
    }

    if (isAtEnd()) {
        return makeErrorToken("Unterminated string literal");
    }
    advance(); // closing quote

    Token t{TokenType::String, val, 0.0, {line, startColumn}};
    return t;
}

Token Lexer::scanToken() {
    char c = advance();

    if (std::isalpha(c) || c == '_') return scanIdentifier();
    if (std::isdigit(c)) return scanNumber();

    switch (c) {
        case '(': return makeToken(TokenType::LeftParen);
        case ')': return makeToken(TokenType::RightParen);
        case '{': return makeToken(TokenType::LeftBrace);
        case '}': return makeToken(TokenType::RightBrace);
        case '[': return makeToken(TokenType::LeftBracket);
        case ']': return makeToken(TokenType::RightBracket);
        case ',': return makeToken(TokenType::Comma);
        case ';': return makeToken(TokenType::Semicolon);
        case ':': return makeToken(TokenType::Colon);
        case '.': return makeToken(TokenType::Dot);
        case '@': return makeToken(TokenType::At);

        case '+':
            return match('=') ? makeToken(TokenType::PlusEqual) : makeToken(TokenType::Plus);
        case '-':
            return match('=') ? makeToken(TokenType::MinusEqual) : makeToken(TokenType::Minus);
        case '*': return makeToken(TokenType::Star);
        case '/': return makeToken(TokenType::Slash);
        case '%': return makeToken(TokenType::Percent);

        case '!':
            return match('=') ? makeToken(TokenType::BangEqual) : makeToken(TokenType::Bang);
        case '=':
            return match('=') ? makeToken(TokenType::EqualEqual) : makeToken(TokenType::Equal);
        case '<':
            return match('=') ? makeToken(TokenType::LessEqual) : makeToken(TokenType::Less);
        case '>':
            return match('=') ? makeToken(TokenType::GreaterEqual) : makeToken(TokenType::Greater);

        case '&':
            if (match('&')) return makeToken(TokenType::AmpAmp);
            return makeErrorToken("Expected '&&'");
        case '|':
            if (match('|')) return makeToken(TokenType::PipePipe);
            return makeErrorToken("Expected '||'");

        case '"':
            return scanString();

        default:
            return makeErrorToken(std::string("Unexpected character '") + c + "'");
    }
}

} // namespace GameLang
