#include "VM.h"

namespace GameLang {

VM::VM(CompiledGame& game, Engine& engine)
    : game(game), engine(engine) {
    stack.reserve(1024);
}

void VM::push(Value value) {
    stack.push_back(std::move(value));
}

Value VM::pop() {
    if (stack.empty()) {
        runtimeError("Stack underflow");
        return Value(0.0);
    }
    Value val = std::move(stack.back());
    stack.pop_back();
    return val;
}

Value VM::peek(int distance) const {
    if (stack.size() <= static_cast<size_t>(distance)) {
        return Value(0.0);
    }
    return stack[stack.size() - 1 - distance];
}

void VM::runtimeError(const std::string& message) {
    errorMessage = "[Runtime Error] " + message;
    lastIsolatedError = message;
    suppressedErrorCount++;
    if (!safeMode) {
        hasError = true;
        std::cerr << errorMessage << std::endl;
    } else {
        if (errorLog.size() < 20) {
            errorLog.push_back(errorMessage);
        }
    }
}

InterpretResult VM::executeChunk(const Chunk& chunk, size_t baseOffset) {
    size_t ip = 0;
    const uint8_t* code = chunk.code.data();
    size_t codeSize = chunk.code.size();

    while (ip < codeSize) {
        OpCode op = static_cast<OpCode>(code[ip++]);
        switch (op) {
            case OpCode::OP_CONSTANT: {
                uint16_t idx = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (idx < chunk.constants.size()) {
                    push(chunk.constants[idx]);
                } else {
                    runtimeError("Constant index out of bounds: " + std::to_string(idx));
                    push(Value(0.0));
                }
                break;
            }
            case OpCode::OP_NULL: push(Value()); break;
            case OpCode::OP_TRUE: push(Value(true)); break;
            case OpCode::OP_FALSE: push(Value(false)); break;
            case OpCode::OP_POP: pop(); break;

            case OpCode::OP_ADD: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_ADD"); push(Value(0.0)); break; }
                Value b = pop();
                Value a = pop();
                if (a.isString() || b.isString()) {
                    push(Value(a.toString() + b.toString()));
                } else {
                    push(Value(a.asNumber() + b.asNumber()));
                }
                break;
            }
            case OpCode::OP_SUB: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_SUB"); push(Value(0.0)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a.asNumber() - b.asNumber()));
                break;
            }
            case OpCode::OP_MUL: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_MUL"); push(Value(0.0)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a.asNumber() * b.asNumber()));
                break;
            }
            case OpCode::OP_DIV: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_DIV"); push(Value(0.0)); break; }
                Value b = pop();
                Value a = pop();
                if (b.asNumber() == 0.0) {
                    push(Value(0.0));
                } else {
                    push(Value(a.asNumber() / b.asNumber()));
                }
                break;
            }
            case OpCode::OP_MOD: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_MOD"); push(Value(0.0)); break; }
                Value b = pop();
                Value a = pop();
                long long ia = static_cast<long long>(a.asNumber());
                long long ib = static_cast<long long>(b.asNumber());
                push(Value(ib != 0 ? static_cast<double>(ia % ib) : 0.0));
                break;
            }
            case OpCode::OP_NEGATE: {
                if (stack.empty()) { runtimeError("Stack underflow in OP_NEGATE"); push(Value(0.0)); break; }
                Value a = pop();
                push(Value(-a.asNumber()));
                break;
            }
            case OpCode::OP_NOT: {
                if (stack.empty()) { runtimeError("Stack underflow in OP_NOT"); push(Value(0.0)); break; }
                Value a = pop();
                push(Value(!a.asBool()));
                break;
            }
            case OpCode::OP_EQUAL: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_EQUAL"); push(Value(false)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a == b));
                break;
            }
            case OpCode::OP_NOT_EQUAL: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_NOT_EQUAL"); push(Value(false)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(!(a == b)));
                break;
            }
            case OpCode::OP_LESS: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_LESS"); push(Value(false)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a.asNumber() < b.asNumber()));
                break;
            }
            case OpCode::OP_LESS_EQUAL: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_LESS_EQUAL"); push(Value(false)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a.asNumber() <= b.asNumber()));
                break;
            }
            case OpCode::OP_GREATER: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_GREATER"); push(Value(false)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a.asNumber() > b.asNumber()));
                break;
            }
            case OpCode::OP_GREATER_EQUAL: {
                if (stack.size() < 2) { runtimeError("Stack underflow in OP_GREATER_EQUAL"); push(Value(false)); break; }
                Value b = pop();
                Value a = pop();
                push(Value(a.asNumber() >= b.asNumber()));
                break;
            }

            case OpCode::OP_GET_GLOBAL: {
                uint16_t idx = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (idx < chunk.constants.size()) {
                    const std::string& name = chunk.constants[idx].asString();
                    auto it = globals.find(name);
                    if (it != globals.end()) {
                        push(it->second);
                    } else {
                        push(Value(0.0));
                    }
                } else {
                    push(Value(0.0));
                }
                break;
            }
            case OpCode::OP_SET_GLOBAL: {
                uint16_t idx = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (idx < chunk.constants.size()) {
                    const std::string& name = chunk.constants[idx].asString();
                    globals[name] = peek(0);
                }
                break;
            }
            case OpCode::OP_GET_LOCAL: {
                uint16_t slot = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (baseOffset + slot < stack.size()) {
                    push(stack[baseOffset + slot]);
                } else {
                    runtimeError("Local variable slot out of bounds: " + std::to_string(slot));
                    push(Value(0.0));
                }
                break;
            }
            case OpCode::OP_SET_LOCAL: {
                uint16_t slot = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (baseOffset + slot < stack.size()) {
                    stack[baseOffset + slot] = peek(0);
                } else {
                    runtimeError("Local variable slot out of bounds: " + std::to_string(slot));
                }
                break;
            }

            case OpCode::OP_GET_PROPERTY: {
                uint16_t idx = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (idx >= chunk.constants.size()) {
                    push(Value(0.0));
                    break;
                }
                const std::string& prop = chunk.constants[idx].asString();
                Value target = pop();
                if (target.isEntity()) {
                    Entity* ent = engine.getEntity(target.asEntity());
                    if (ent) {
                        auto it = ent->fields.find(prop);
                        if (it != ent->fields.end()) {
                            push(it->second);
                        } else {
                            push(Value(0.0));
                        }
                    } else {
                        push(Value(0.0));
                    }
                } else {
                    push(Value(0.0));
                }
                break;
            }
            case OpCode::OP_SET_PROPERTY: {
                uint16_t idx = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (idx >= chunk.constants.size()) {
                    break;
                }
                const std::string& prop = chunk.constants[idx].asString();
                Value val = pop();
                Value target = pop();
                if (target.isEntity()) {
                    Entity* ent = engine.getEntity(target.asEntity());
                    if (ent) {
                        ent->fields[prop] = val;
                    }
                }
                push(val);
                break;
            }

            case OpCode::OP_JUMP: {
                int16_t jump = static_cast<int16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                if (ip + jump <= codeSize) {
                    ip += jump;
                } else {
                    runtimeError("Jump target out of bytecode bounds");
                    break;
                }
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE: {
                int16_t jump = static_cast<int16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                if (!peek(0).asBool()) {
                    if (ip + jump <= codeSize) {
                        ip += jump;
                    } else {
                        runtimeError("Jump target out of bytecode bounds");
                        break;
                    }
                }
                break;
            }

            case OpCode::OP_CALL: {
                uint8_t argCount = code[ip++];
                if (stack.size() < static_cast<size_t>(argCount + 1)) {
                    runtimeError("Stack underflow in function call");
                    push(Value(0.0));
                    break;
                }
                // Callee is right below arguments
                Value calleeVal = stack[stack.size() - argCount - 1];
                std::string fnName = calleeVal.asString();

                auto it = game.functions.find(fnName);
                if (it != game.functions.end()) {
                    size_t fnBase = stack.size() - argCount;
                    executeChunk(it->second.chunk, fnBase);
                    Value retVal = pop();

                    // Clean up arguments and callee
                    while (stack.size() > fnBase - 1) {
                        stack.pop_back();
                    }
                    push(retVal);
                } else {
                    runtimeError("Undefined function '" + fnName + "'");
                    // Fault isolation: clean arguments and callee, return 0.0
                    for (int a = 0; a <= argCount; ++a) {
                        if (!stack.empty()) stack.pop_back();
                    }
                    push(Value(0.0));
                    if (!safeMode) return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::OP_RETURN: {
                return InterpretResult::Ok;
            }

            case OpCode::OP_SPAWN: {
                uint16_t idx = (code[ip] << 8) | code[ip + 1];
                ip += 2;
                if (idx < chunk.constants.size()) {
                    const std::string& typeName = chunk.constants[idx].asString();
                    auto it = game.entityDefaultValues.find(typeName);
                    std::unordered_map<std::string, Value> defaults;
                    if (it != game.entityDefaultValues.end()) {
                        defaults = it->second;
                    }
                    uint32_t id = engine.spawn(typeName, defaults);
                    push(Value::makeEntity(id));
                } else {
                    push(Value::makeEntity(0));
                }
                break;
            }
            case OpCode::OP_DESTROY: {
                Value target = pop();
                if (target.isEntity()) {
                    engine.destroy(target.asEntity());
                }
                break;
            }

            case OpCode::OP_KEY_DOWN: {
                Value k = pop();
                push(Value(engine.isKeyDown(k.asString())));
                break;
            }
            case OpCode::OP_KEY_PRESSED: {
                Value k = pop();
                push(Value(engine.isKeyPressed(k.asString())));
                break;
            }

            case OpCode::OP_BEEP: {
                Value dur = pop();
                Value freq = pop();
                engine.playBeep(static_cast<int>(freq.asNumber()), static_cast<int>(dur.asNumber()));
                break;
            }
            case OpCode::OP_RANDOM: {
                Value maxVal = pop();
                Value minVal = pop();
                int r = engine.getRandomInt(static_cast<int>(minVal.asNumber()), static_cast<int>(maxVal.asNumber()));
                push(Value(static_cast<double>(r)));
                break;
            }
            case OpCode::OP_PRINT_AT: {
                Value col = pop();
                Value txt = pop();
                Value y = pop();
                Value x = pop();
                Color c = col.isString() ? parseColor(col.asString()) : Color::White;
                engine.drawText(static_cast<int>(x.asNumber()), static_cast<int>(y.asNumber()), txt.asString(), c);
                break;
            }
            case OpCode::OP_COUNT_ENTITIES: {
                Value type = pop();
                auto list = engine.getActiveEntitiesByType(type.asString());
                push(Value(static_cast<double>(list.size())));
                break;
            }

            case OpCode::OP_TILE_SET: {
                Value solidVal = pop();
                Value colVal = pop();
                Value chVal = pop();
                Value yVal = pop();
                Value xVal = pop();
                int x = static_cast<int>(xVal.asNumber());
                int y = static_cast<int>(yVal.asNumber());
                char ch = chVal.isString() && !chVal.asString().empty() ? chVal.asString()[0] : ' ';
                Color c = colVal.isString() ? parseColor(colVal.asString()) : Color::White;
                bool solid = solidVal.isBool() ? solidVal.asBool() : (solidVal.asNumber() != 0.0);
                engine.setTile(x, y, ch, c, solid);
                break;
            }
            case OpCode::OP_TILE_SOLID: {
                Value yVal = pop();
                Value xVal = pop();
                int x = static_cast<int>(xVal.asNumber());
                int y = static_cast<int>(yVal.asNumber());
                push(Value(engine.isTileSolid(x, y)));
                break;
            }
            case OpCode::OP_TILE_GET: {
                Value yVal = pop();
                Value xVal = pop();
                int x = static_cast<int>(xVal.asNumber());
                int y = static_cast<int>(yVal.asNumber());
                char ch = engine.getTileChar(x, y);
                push(Value(std::string(1, ch)));
                break;
            }
            case OpCode::OP_MAP_BOX: {
                Value solidVal = pop();
                Value colVal = pop();
                Value chVal = pop();
                Value hVal = pop();
                Value wVal = pop();
                Value yVal = pop();
                Value xVal = pop();
                int x = static_cast<int>(xVal.asNumber());
                int y = static_cast<int>(yVal.asNumber());
                int w = static_cast<int>(wVal.asNumber());
                int h = static_cast<int>(hVal.asNumber());
                char ch = chVal.isString() && !chVal.asString().empty() ? chVal.asString()[0] : '#';
                Color c = colVal.isString() ? parseColor(colVal.asString()) : Color::Default;
                bool solid = solidVal.isBool() ? solidVal.asBool() : (solidVal.asNumber() != 0.0);
                engine.fillMapBox(x, y, w, h, ch, c, solid);
                break;
            }
            case OpCode::OP_MAP_ROW: {
                Value solidVal = pop();
                Value colVal = pop();
                Value rowVal = pop();
                Value yVal = pop();
                Value xVal = pop();
                int x = static_cast<int>(xVal.asNumber());
                int y = static_cast<int>(yVal.asNumber());
                std::string row = rowVal.isString() ? rowVal.asString() : "";
                Color c = colVal.isString() ? parseColor(colVal.asString()) : Color::White;
                bool solid = solidVal.isBool() ? solidVal.asBool() : (solidVal.asNumber() != 0.0);
                engine.setMapRow(x, y, row, c, solid);
                break;
            }
            case OpCode::OP_CAMERA_SET: {
                Value yVal = pop();
                Value xVal = pop();
                engine.setCamera(static_cast<int>(xVal.asNumber()), static_cast<int>(yVal.asNumber()));
                break;
            }
            case OpCode::OP_SET_MESSAGE: {
                Value colVal = pop();
                Value msgVal = pop();
                Color c = colVal.isString() ? parseColor(colVal.asString()) : Color::Yellow;
                engine.setMessage(msgVal.isString() ? msgVal.asString() : msgVal.toString(), c);
                break;
            }

            default:
                runtimeError("Unknown opcode");
                if (!safeMode) return InterpretResult::RuntimeError;
                break;
        }

        if (hasError && !safeMode) return InterpretResult::RuntimeError;
    }

    return InterpretResult::Ok;
}

void VM::runInit() {
    try {
        executeChunk(game.globalInitChunk);
        executeChunk(game.initChunk);
    } catch (const std::exception& ex) {
        runtimeError(std::string("Exception in init: ") + ex.what());
    } catch (...) {
        runtimeError("Unknown exception during game init");
    }
}

void VM::runUpdate() {
    engine.pollInput();

    // Auto-update entities velocity (vx, vy)
    for (const auto& kv : engine.getAllEntities()) {
        Entity* e = engine.getEntity(kv.first);
        if (!e || !e->active) continue;
        auto vxIt = e->fields.find("vx");
        auto vyIt = e->fields.find("vy");
        if (vxIt != e->fields.end() && vxIt->second.isNumber()) {
            e->fields["x"] = Value(e->getX() + vxIt->second.asNumber());
        }
        if (vyIt != e->fields.end() && vyIt->second.isNumber()) {
            e->fields["y"] = Value(e->getY() + vyIt->second.asNumber());
        }
        if (e->type == "Bullet" && (e->getY() < 1 || e->getY() >= engine.getHeight() - 1)) {
            e->active = false;
        }
    }

    size_t stackBase = stack.size();
    try {
        executeChunk(game.updateChunk);
    } catch (const std::exception& ex) {
        runtimeError(std::string("Exception in update: ") + ex.what());
    } catch (...) {
        runtimeError("Unknown exception in update block");
    }
    // Isolate stack leaks
    if (stack.size() > stackBase) {
        stack.resize(stackBase);
    }

    runCollisions();
}

void VM::runCollisions() {
    for (const auto& handler : game.collisionHandlers) {
        try {
            auto pairs = engine.checkCollisions(handler.entityA, handler.entityB);
            for (const auto& pair : pairs) {
                size_t base = stack.size();
                push(Value::makeEntity(pair.first));
                push(Value::makeEntity(pair.second));
                executeChunk(handler.chunk, base);
                if (stack.size() >= base + 2) {
                    pop(); // slot 1
                    pop(); // slot 0
                } else {
                    stack.resize(base);
                }
            }
        } catch (const std::exception& ex) {
            runtimeError(std::string("Exception in collision handler: ") + ex.what());
        } catch (...) {
            runtimeError("Unknown exception in collision handler");
        }
    }
}

void VM::runRender() {
    try {
        engine.clearBuffer();

        int camX = engine.getCameraX();
        int camY = engine.getCameraY();

        // Render tilemap background with camera offset
        const auto& tiles = engine.getTiles();
        int mapW = engine.getMapWidth();
        int mapH = engine.getMapHeight();
        if (!tiles.empty() && mapW > 0) {
            for (int sy = 1; sy < engine.getHeight() - 1; ++sy) {
                int my = sy + camY;
                if (my < 0 || my >= mapH) continue;
                for (int sx = 1; sx < engine.getWidth() - 1; ++sx) {
                    int mx = sx + camX;
                    if (mx < 0 || mx >= mapW) continue;
                    const auto& t = tiles[my * mapW + mx];
                    if (t.ch != ' ' && t.ch != '\0') {
                        engine.setPixel(sx, sy, t.ch, t.color);
                    }
                }
            }
        }

        // Draw game frame box
        engine.drawBox(0, 0, engine.getWidth(), engine.getHeight(), Color::Blue);

        // Header info: Title, Score
        std::string titleText = " " + game.config.title + " ";
        engine.drawText(2, 0, titleText, Color::Cyan);

        // If 'score' global exists, render it
        auto scoreIt = globals.find("score");
        if (scoreIt != globals.end()) {
            std::string scoreStr = "Score: " + scoreIt->second.toString();
            engine.drawText(engine.getWidth() - static_cast<int>(scoreStr.length()) - 3, 0, scoreStr, Color::Yellow);
        }

        // If 'lives' global exists, render it
        auto livesIt = globals.find("lives");
        if (livesIt != globals.end()) {
            std::string livesStr = "Lives: " + livesIt->second.toString();
            engine.drawText(2, engine.getHeight() - 1, livesStr, Color::Green);
        }

        // RPG HUD globals: HP and Gold
        auto hpIt = globals.find("hp");
        if (hpIt != globals.end()) {
            std::string hpStr = "HP: " + hpIt->second.toString();
            engine.drawText(livesIt != globals.end() ? 14 : 2, engine.getHeight() - 1, hpStr, Color::Red);
        }

        auto goldIt = globals.find("gold");
        if (goldIt != globals.end()) {
            std::string goldStr = "Ouro: " + goldIt->second.toString();
            engine.drawText(26, engine.getHeight() - 1, goldStr, Color::Yellow);
        }

        // Render active entities (offset by camera)
        const auto& entities = engine.getAllEntities();
        for (const auto& kv : entities) {
            const Entity& e = kv.second;
            if (!e.active) continue;

            int x = static_cast<int>(std::round(e.getX())) - camX;
            int y = static_cast<int>(std::round(e.getY())) - camY;

            if (x > 0 && x < engine.getWidth() - 1 && y > 0 && y < engine.getHeight() - 1) {
                char sym = e.getSymbol();
                Color col = e.getColor();
                engine.setPixel(x, y, sym, col);
            }
        }

        // Run custom render chunk if defined
        if (!game.renderChunk.code.empty()) {
            size_t renderBase = stack.size();
            executeChunk(game.renderChunk);
            if (stack.size() > renderBase) {
                stack.resize(renderBase);
            }
        }

        // RPG Dialogue / Message Banner
        if (!engine.getMessage().empty()) {
            std::string msgBar = "[ " + engine.getMessage() + " ]";
            int msgX = std::max(2, (engine.getWidth() - static_cast<int>(msgBar.length())) / 2);
            engine.drawText(msgX, engine.getHeight() - 2, msgBar, engine.getMessageColor());
        }

        // Footer prompt with safe mode status if errors occurred
        if (safeMode && suppressedErrorCount > 0) {
            std::string safeTag = "[Safe: " + std::to_string(suppressedErrorCount) + " err isolados]";
            engine.drawText(engine.getWidth() / 2 - static_cast<int>(safeTag.length()) / 2, engine.getHeight() - 1, safeTag, Color::Yellow);
        }

        std::string help = "[ESC/Q: Sair]";
        engine.drawText(engine.getWidth() - static_cast<int>(help.length()) - 2, engine.getHeight() - 1, help, Color::Default);

        engine.present();
    } catch (...) {
        // Suppress render exception so game loop does not crash
    }
}

InterpretResult VM::run() {
    engine.initTerminal();
    runInit();

    while (!engine.shouldClose()) {
        try {
            runUpdate();
            runRender();
        } catch (const std::exception& ex) {
            runtimeError(std::string("Fatal frame exception: ") + ex.what());
            if (!safeMode) break;
        } catch (...) {
            runtimeError("Unknown fatal frame exception");
            if (!safeMode) break;
        }
        engine.syncFrame();
    }

    engine.resetTerminal();
    return InterpretResult::Ok;
}

} // namespace GameLang
