#include "Transpiler.h"

namespace GameLang {

static void collectExpr(const Expr* expr, std::set<std::string>& members, std::set<std::string>& calls) {
    if (!expr) return;
    if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        collectExpr(bin->left.get(), members, calls);
        collectExpr(bin->right.get(), members, calls);
    } else if (auto un = dynamic_cast<const UnaryExpr*>(expr)) {
        collectExpr(un->right.get(), members, calls);
    } else if (auto asgn = dynamic_cast<const AssignExpr*>(expr)) {
        collectExpr(asgn->value.get(), members, calls);
    } else if (auto mem = dynamic_cast<const MemberAccessExpr*>(expr)) {
        members.insert(mem->member);
        collectExpr(mem->object.get(), members, calls);
    } else if (auto memAsgn = dynamic_cast<const MemberAssignExpr*>(expr)) {
        members.insert(memAsgn->member);
        collectExpr(memAsgn->object.get(), members, calls);
        collectExpr(memAsgn->value.get(), members, calls);
    } else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        calls.insert(call->callee);
        for (const auto& arg : call->arguments) {
            collectExpr(arg.get(), members, calls);
        }
    }
}

static void collectStmt(const Stmt* stmt, std::set<std::string>& members, std::set<std::string>& calls) {
    if (!stmt) return;
    if (auto exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        collectExpr(exprStmt->expr.get(), members, calls);
    } else if (auto varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        if (varDecl->initializer) collectExpr(varDecl->initializer.get(), members, calls);
    } else if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        collectExpr(ifStmt->condition.get(), members, calls);
        collectStmt(ifStmt->thenBranch.get(), members, calls);
        collectStmt(ifStmt->elseBranch.get(), members, calls);
    } else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        collectExpr(whileStmt->condition.get(), members, calls);
        collectStmt(whileStmt->body.get(), members, calls);
    } else if (auto blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        for (const auto& s : blockStmt->statements) collectStmt(s.get(), members, calls);
    } else if (auto returnStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        if (returnStmt->value) collectExpr(returnStmt->value.get(), members, calls);
    } else if (auto destroyStmt = dynamic_cast<const DestroyStmt*>(stmt)) {
        collectExpr(destroyStmt->target.get(), members, calls);
    }
}

void Transpiler::emitIndent(std::ostringstream& ss, int indent) {
    for (int i = 0; i < indent; ++i) ss << "    ";
}

std::string Transpiler::transpileToCpp(const Program& program, TranspileTarget target) {
    if (target == TranspileTarget::OpenGL33) {
        return transpileOpenGL(program);
    }
    return transpileConsole(program);
}

// ============================================================================
// Back-End OpenGL 3.3 Core Profile (Hardware Accelerated GPU)
// ============================================================================
std::string Transpiler::transpileOpenGL(const Program& program) {
    std::ostringstream ss;

    std::set<std::string> accessedMembers;
    std::set<std::string> calledFunctions;
    for (const auto& g : program.globals) {
        if (g->initializer) collectExpr(g->initializer.get(), accessedMembers, calledFunctions);
    }
    for (const auto& ent : program.entities) {
        for (const auto& f : ent->fields) {
            accessedMembers.insert(f.name);
            if (f.defaultValue) collectExpr(f.defaultValue.get(), accessedMembers, calledFunctions);
        }
    }
    for (const auto& fn : program.functions) {
        if (fn->body) collectStmt(fn->body.get(), accessedMembers, calledFunctions);
    }
    if (program.initBlock) collectStmt(program.initBlock.get(), accessedMembers, calledFunctions);
    if (program.updateBlock) collectStmt(program.updateBlock.get(), accessedMembers, calledFunctions);
    if (program.renderBlock) collectStmt(program.renderBlock.get(), accessedMembers, calledFunctions);
    for (const auto& ch : program.collisionHandlers) {
        if (ch->body) collectStmt(ch->body.get(), accessedMembers, calledFunctions);
    }

    ss << "// ============================================================================\n";
    ss << "// GameForge: Jogo Acelerado por GPU em C++20 Nativo (OpenGL 3.3 Core Profile)\n";
    ss << "// Pipeline: 1 Draw Call Instanciado, Virtual Canvas FBO, Shaders GLSL de Bloom/CRT\n";
    ss << "// ============================================================================\n";
    ss << "#include \"GpuEngineGL.h\"\n";
    ss << "#include \"TimeEngine.h\"\n";
    ss << "#include \"DebugInspector.h\"\n";
    ss << "#include <iostream>\n";
    ss << "#include <vector>\n";
    ss << "#include <string>\n";
    ss << "#include <unordered_map>\n";
    ss << "#include <memory>\n";
    ss << "#include <chrono>\n";
    ss << "#include <thread>\n";
    ss << "#include <cmath>\n";
    ss << "#include <random>\n";
    ss << "#include <algorithm>\n\n";

    ss << "namespace GameRuntime {\n";

    ss << R"RAW(
    static void parseColorRgb(const std::string& name, float& r, float& g, float& b) {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "red")     { r = 1.0f; g = 0.2f; b = 0.2f; }
        else if (s == "green")   { r = 0.2f; g = 1.0f; b = 0.2f; }
        else if (s == "yellow")  { r = 1.0f; g = 0.95f; b = 0.2f; }
        else if (s == "blue")    { r = 0.2f; g = 0.4f; b = 1.0f; }
        else if (s == "magenta") { r = 1.0f; g = 0.2f; b = 1.0f; }
        else if (s == "cyan")    { r = 0.2f; g = 0.95f; b = 1.0f; }
        else if (s == "white")   { r = 1.0f; g = 1.0f; b = 1.0f; }
        else if (s == "black")   { r = 0.05f; g = 0.05f; b = 0.08f; }
        else { r = 0.8f; g = 0.8f; b = 0.8f; }
    }
)RAW";

    ss << "    struct BaseEntity {\n";
    ss << "        uint32_t id = 0;\n";
    ss << "        std::string type;\n";
    ss << "        bool active = true;\n";
    ss << "        double x = 0;\n";
    ss << "        double y = 0;\n";
    ss << "        double vx = 0;\n";
    ss << "        double vy = 0;\n";
    ss << "        double hp = 0;\n";
    ss << "        double alive = 1;\n";
    ss << "        double width = 16;\n";
    ss << "        double height = 16;\n";
    ss << "        double glow = 0.2;\n";
    ss << "        std::string symbol = \"?\";\n";
    ss << "        std::string color = \"white\";\n";
    for (const auto& mem : accessedMembers) {
        if (mem != "x" && mem != "y" && mem != "vx" && mem != "vy" &&
            mem != "hp" && mem != "alive" && mem != "symbol" && mem != "color" &&
            mem != "id" && mem != "type" && mem != "active" &&
            mem != "width" && mem != "height" && mem != "glow") {
            ss << "        double " << mem << " = 0.0;\n";
        }
    }
    ss << "        virtual ~BaseEntity() = default;\n";
    ss << "    };\n\n";

    ss << R"RAW(
    struct Value {
        double num = 0.0;
        std::string str;
        std::shared_ptr<BaseEntity> entity = nullptr;

        Value() = default;
        Value(double n) : num(n) {}
        Value(int n) : num(static_cast<double>(n)) {}
        Value(const char* s) : str(s) {}
        Value(std::string s) : str(std::move(s)) {}
        template<typename T>
        Value(std::shared_ptr<T> e) : entity(std::static_pointer_cast<BaseEntity>(e)) {}

        static BaseEntity* getSafeDummy() {
            static BaseEntity dummy;
            dummy.active = false;
            return &dummy;
        }
        BaseEntity* operator->() const { return entity ? entity.get() : getSafeDummy(); }
        explicit operator double() const { return num; }
        explicit operator int() const { return static_cast<int>(num); }
        operator bool() const { return entity != nullptr || num != 0.0; }

        Value& operator=(double n) { num = n; entity = nullptr; return *this; }
        Value& operator=(int n) { num = static_cast<double>(n); entity = nullptr; return *this; }
        Value& operator=(const std::string& s) { str = s; return *this; }
        template<typename T>
        Value& operator=(std::shared_ptr<T> e) { entity = std::static_pointer_cast<BaseEntity>(e); return *this; }

        Value operator+(const Value& o) const {
            if (!str.empty() || !o.str.empty()) {
                std::string s1 = str.empty() ? std::to_string(static_cast<int>(num)) : str;
                std::string s2 = o.str.empty() ? std::to_string(static_cast<int>(o.num)) : o.str;
                return Value(s1 + s2);
            }
            return Value(num + o.num);
        }
        Value operator-(const Value& o) const { return Value(num - o.num); }
        Value operator*(const Value& o) const { return Value(num * o.num); }
        Value operator/(const Value& o) const { return Value(o.num != 0 ? num / o.num : 0); }
        bool operator==(const Value& o) const { return num == o.num && entity == o.entity; }
        bool operator!=(const Value& o) const { return !(*this == o); }
        bool operator<(const Value& o) const { return num < o.num; }
        bool operator<=(const Value& o) const { return num <= o.num; }
        bool operator>(const Value& o) const { return num > o.num; }
        bool operator>=(const Value& o) const { return num >= o.num; }

        Value operator+(double d) const { return Value(num + d); }
        Value operator-(double d) const { return Value(num - d); }
        Value operator*(double d) const { return Value(num * d); }
        Value operator/(double d) const { return Value(d != 0 ? num / d : 0); }
        Value operator+(int i) const { return Value(num + i); }
        Value operator-(int i) const { return Value(num - i); }
        Value operator*(int i) const { return Value(num * i); }
        Value operator/(int i) const { return Value(i != 0 ? num / i : 0); }

        bool operator==(double d) const { return num == d; }
        bool operator!=(double d) const { return num != d; }
        bool operator<(double d) const { return num < d; }
        bool operator<=(double d) const { return num <= d; }
        bool operator>(double d) const { return num > d; }
        bool operator>=(double d) const { return num >= d; }

        bool operator==(int i) const { return num == static_cast<double>(i); }
        bool operator!=(int i) const { return num != static_cast<double>(i); }
        bool operator<(int i) const { return num < static_cast<double>(i); }
        bool operator<=(int i) const { return num <= static_cast<double>(i); }
        bool operator>(int i) const { return num > static_cast<double>(i); }
        bool operator>=(int i) const { return num >= static_cast<double>(i); }

        bool operator==(const char* s) const { return str == s; }
        bool operator!=(const char* s) const { return str != s; }
        bool operator==(const std::string& s) const { return str == s; }
        bool operator!=(const std::string& s) const { return str != s; }
        Value operator+(const char* s) const {
            std::string s1 = str.empty() ? std::to_string(static_cast<int>(num)) : str;
            return Value(s1 + s);
        }
        Value operator+(const std::string& s) const {
            std::string s1 = str.empty() ? std::to_string(static_cast<int>(num)) : str;
            return Value(s1 + s);
        }
    };

    inline Value operator+(const char* s, const Value& v) {
        std::string s2 = v.str.empty() ? std::to_string(static_cast<int>(v.num)) : v.str;
        return Value(std::string(s) + s2);
    }
    inline Value operator+(const std::string& s, const Value& v) {
        std::string s2 = v.str.empty() ? std::to_string(static_cast<int>(v.num)) : v.str;
        return Value(s + s2);
    }
)RAW";

    ss << "    static GameForge::GL::HardwareEngineGL* gEngine = nullptr;\n";
    ss << "    static std::vector<std::shared_ptr<BaseEntity>> entities;\n";
    ss << "    static uint32_t nextEntityId = 1;\n";
    ss << "    static std::string hudMessage = \"\";\n";
    ss << "    static double coordScale = 16.0;\n";
    ss << "    static GameForge::Time::TemporalBuffer<300> gTimeBuffer;\n";
    ss << "    static std::vector<GameForge::Time::TemporalEcho> gEchoes;\n";
    ss << "    static GameForge::Debug::DebugInspector gInspector;\n";
    ss << "    static std::unordered_map<std::string, int> gFrozenTypes;\n\n";

    ss << R"RAW(
    inline bool key(const std::string& k) {
        if (!gEngine) return false;
        if (k == "left" || k == "LEFT" || k == "a" || k == "A") return gEngine->isKeyDown(VK_LEFT) || gEngine->isKeyDown('A');
        if (k == "right" || k == "RIGHT" || k == "d" || k == "D") return gEngine->isKeyDown(VK_RIGHT) || gEngine->isKeyDown('D');
        if (k == "up" || k == "UP" || k == "w" || k == "W") return gEngine->isKeyDown(VK_UP) || gEngine->isKeyDown('W');
        if (k == "down" || k == "DOWN" || k == "s" || k == "S") return gEngine->isKeyDown(VK_DOWN) || gEngine->isKeyDown('S');
        if (k == "space" || k == "SPACE") return gEngine->isKeyDown(VK_SPACE);
        if (k == "q" || k == "Q" || k == "esc" || k == "escape") return gEngine->isKeyDown('Q') || gEngine->isKeyDown(VK_ESCAPE);
        if (k == "enter" || k == "return") return gEngine->isKeyDown(VK_RETURN);
        return false;
    }
    inline bool key_down(const std::string& k) { return key(k); }
    inline bool key_pressed(const std::string& k) { return key(k); }

    inline void beep(int freq = 440, int duration = 50) {
        std::thread([=]() { Beep(freq, duration); }).detach();
    }

    inline double random(double min, double max) {
        static std::mt19937 rng(1337);
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }
    inline double rnd(double min, double max) { return random(min, max); }

    inline void msg(const Value& text, const std::string& col = "yellow") {
        hudMessage = text.str.empty() ? std::to_string(static_cast<int>(text.num)) : text.str;
    }
    inline void dialog(const Value& text, const std::string& col = "white") { msg(text, col); }
    inline void set_bloom(double intensity) { if (gEngine) gEngine->setBloom(static_cast<float>(intensity)); }
    inline void set_scanlines(double strength) { if (gEngine) gEngine->setScanlines(static_cast<float>(strength)); }

    inline void tile(double x, double y, const std::string& ch, const std::string& col, bool solid = false) {
        if (!gEngine) return;
        float r, g, b;
        parseColorRgb(col, r, g, b);
        gEngine->drawRect(static_cast<float>(x * coordScale), static_cast<float>(y * coordScale),
                          static_cast<float>(coordScale), static_cast<float>(coordScale), r, g, b, 1.0f, 0.1f);
    }
    inline bool tile_solid(double x, double y) { return false; }
    inline std::string tile_at(double x, double y) { return " "; }
    inline void map_box(double x, double y, double w, double h, const std::string& ch, const std::string& col, bool solid = true) {
        for (double ix = x; ix < x + w; ++ix) {
            tile(ix, y, ch, col, solid);
            tile(ix, y + h - 1, ch, col, solid);
        }
        for (double iy = y; iy < y + h; ++iy) {
            tile(x, iy, ch, col, solid);
            tile(x + w - 1, iy, ch, col, solid);
        }
    }
    inline void map_row(double x, double y, const std::string& text, const std::string& col, bool solid = true) {
        for (size_t i = 0; i < text.size(); ++i) {
            std::string c(1, text[i]);
            tile(x + i, y, c, col, solid);
        }
    }
    inline void camera(double cx, double cy) {}

    inline void destroy(Value v) {
        if (v.entity) v.entity->active = false;
    }

    inline int count(const std::string& type) {
        int c = 0;
        for (const auto& e : entities) {
            if (e->active && e->type == type) c++;
        }
        return c;
    }

    inline void time_rewind(double frames = 60.0) {
        GameForge::Time::WorldSnapshot snap;
        if (gTimeBuffer.rewindWorld(static_cast<size_t>(frames), snap)) {
            for (uint32_t i = 0; i < snap.entityCount; ++i) {
                const auto& es = snap.entities[i];
                for (auto& e : entities) {
                    if (e->id == es.id) {
                        e->active = es.active;
                        e->x = es.x;
                        e->y = es.y;
                        e->vx = es.vx;
                        e->vy = es.vy;
                        e->hp = es.hp;
                        break;
                    }
                }
            }
            beep(gTimeBuffer.getPitchShiftedFrequency(880), 40);
        }
    }

    inline void time_scale(double s = 1.0) {
        gTimeBuffer.setTimeScale(static_cast<float>(s));
    }

    inline void spawn_echo(const Value& entVal, double frames = 60.0) {
        uint32_t entId = entVal.entity ? entVal.entity->id : static_cast<uint32_t>(entVal.num);
        auto history = gTimeBuffer.getEntityHistory(entId, static_cast<size_t>(frames));
        if (!history.empty()) {
            GameForge::Time::TemporalEcho echo;
            echo.targetEntityId = entId;
            echo.trajectory = std::vector<GameForge::Time::EntitySnapshot>(history.rbegin(), history.rend());
            echo.playbackIndex = 0;
            echo.active = true;
            gEchoes.push_back(std::move(echo));
        }
    }

    inline void freeze_type(const std::string& type, double duration = 60.0) {
        gFrozenTypes[type] = static_cast<int>(duration);
    }

    inline void god_mode(bool enabled = true) {
        gInspector.setGodMode(enabled);
    }

    inline void tweak_var(const std::string& name, double minVal, double maxVal, double step = 1.0) {}
)RAW";

    // Subclasses de Entidades
    for (const auto& ent : program.entities) {
        ss << "    struct Entity_" << ent->name << " : public BaseEntity {\n";
        ss << "        Entity_" << ent->name << "() {\n";
        ss << "            type = \"" << ent->name << "\";\n";
        for (const auto& f : ent->fields) {
            ss << "            " << f.name << " = ";
            if (f.defaultValue) transpileExpression(*f.defaultValue, ss);
            else ss << "0";
            ss << ";\n";
        }
        ss << "        }\n";
        ss << "    };\n";

        ss << "    inline std::shared_ptr<BaseEntity> spawn_" << ent->name << "() {\n";
        ss << "        auto e = std::make_shared<Entity_" << ent->name << ">();\n";
        ss << "        e->id = nextEntityId++;\n";
        ss << "        entities.push_back(e);\n";
        ss << "        return e;\n";
        ss << "    }\n\n";
    }

    // Variáveis Globais
    for (const auto& g : program.globals) {
        ss << "    static Value " << g->name << " = ";
        if (g->initializer) transpileExpression(*g->initializer, ss);
        else ss << "0";
        ss << ";\n";
    }
    ss << "\n";

    // Funções de Usuário
    for (const auto& fn : program.functions) {
        ss << "    Value fn_" << fn->name << "(";
        for (size_t i = 0; i < fn->params.size(); ++i) {
            ss << "Value " << fn->params[i];
            if (i + 1 < fn->params.size()) ss << ", ";
        }
        ss << ") {\n";
        if (fn->body) transpileBlock(*fn->body, ss, 2);
        ss << "        return Value();\n";
        ss << "    }\n\n";
    }

    // Tratadores de Colisão
    for (size_t i = 0; i < program.collisionHandlers.size(); ++i) {
        const auto& ch = program.collisionHandlers[i];
        ss << "    void col_handler_" << i << "(std::shared_ptr<BaseEntity> " << ch->varA
           << ", std::shared_ptr<BaseEntity> " << ch->varB << ") {\n";
        if (ch->body) transpileBlock(*ch->body, ss, 2);
        ss << "    }\n\n";
    }

    // Despacho de Colisão
    ss << "    void checkCollisions() {\n";
    ss << "        for (size_t i = 0; i < entities.size(); ++i) {\n";
    ss << "            auto& a = entities[i];\n";
    ss << "            if (!a->active) continue;\n";
    ss << "            for (size_t j = i + 1; j < entities.size(); ++j) {\n";
    ss << "                auto& b = entities[j];\n";
    ss << "                if (!b->active) continue;\n";
    ss << "                double dx = std::abs(a->x - b->x);\n";
    ss << "                double dy = std::abs(a->y - b->y);\n";
    ss << "                double limit = (coordScale == 1.0) ? 14.0 : 1.1;\n";
    ss << "                if (dx <= limit && dy <= limit) {\n";
    for (size_t k = 0; k < program.collisionHandlers.size(); ++k) {
        const auto& ch = program.collisionHandlers[k];
        ss << "                    if (a->type == \"" << ch->entityA << "\" && b->type == \"" << ch->entityB << "\") {\n";
        ss << "                        col_handler_" << k << "(a, b);\n";
        ss << "                    } else if (a->type == \"" << ch->entityB << "\" && b->type == \"" << ch->entityA << "\") {\n";
        ss << "                        col_handler_" << k << "(b, a);\n";
        ss << "                    }\n";
    }
    ss << "                }\n";
    ss << "            }\n";
    ss << "        }\n";
    ss << "    }\n\n";

    // Hooks do Ciclo de Vida
    ss << "    void game_init() {\n";
    for (const auto& g : program.globals) {
        if (g->tweak.hasTweak) {
            ss << "        GameRuntime::gInspector.registerVar(\"" << g->name << "\", &GameRuntime::" << g->name
               << ".num, " << g->tweak.minVal << ", " << g->tweak.maxVal << ", " << g->tweak.step << ");\n";
        }
        if (g->name == "hp" || g->name == "player_hp") {
            ss << "        GameRuntime::gInspector.registerPlayerHp(&GameRuntime::" << g->name << ".num, 100.0);\n";
        }
    }
    if (program.initBlock) transpileBlock(*program.initBlock, ss, 2);
    ss << "    }\n\n";

    ss << "    void game_update() {\n";
    if (program.updateBlock) transpileBlock(*program.updateBlock, ss, 2);
    ss << "    }\n\n";

    ss << "    void game_render() {\n";
    if (program.renderBlock) transpileBlock(*program.renderBlock, ss, 2);
    ss << "    }\n\n";

    ss << "} // namespace GameRuntime\n\n";

    // Função main() Nativa
    int baseW = program.config.width > 0 ? program.config.width : 60;
    int baseH = program.config.height > 0 ? program.config.height : 22;
    int baseFps = program.config.fps > 0 ? program.config.fps : 60;

    ss << "int main() {\n";
    ss << "    int configW = " << baseW << ";\n";
    ss << "    int configH = " << baseH << ";\n";
    ss << "    int fps = " << baseFps << ";\n";
    ss << "    std::string title = \"" << program.config.title << "\";\n\n";

    ss << "    int canvasW = (configW < 120) ? (configW * 16) : configW;\n";
    ss << "    int canvasH = (configH < 80) ? (configH * 16) : configH;\n";
    ss << "    GameRuntime::coordScale = (configW < 120) ? 16.0 : 1.0;\n\n";

    ss << "    GameForge::GL::HardwareEngineGL engine(canvasW, canvasH, fps, title);\n";
    ss << "    GameRuntime::gEngine = &engine;\n";
    ss << "    engine.setBloom(1.2f);\n";
    ss << "    engine.setScanlines(0.25f);\n\n";

    ss << "    GameRuntime::game_init();\n\n";

    ss << "    while (!engine.shouldClose()) {\n";
    ss << "        engine.beginFrame();\n\n";

    ss << "        // Input para Live-Tuning Inspector, God Mode e atalhos\n";
    ss << "        static bool lastTab = false;\n";
    ss << "        bool curTab = engine.isKeyDown(VK_TAB);\n";
    ss << "        bool tabPressed = curTab && !lastTab;\n";
    ss << "        lastTab = curTab;\n";
    ss << "        static bool lastG = false;\n";
    ss << "        bool curG = engine.isKeyDown('G');\n";
    ss << "        bool gPressed = curG && !lastG;\n";
    ss << "        lastG = curG;\n";
    ss << "        static bool lastLBracket = false;\n";
    ss << "        bool curLBracket = engine.isKeyDown(VK_OEM_4);\n";
    ss << "        bool lBracketPressed = curLBracket && !lastLBracket;\n";
    ss << "        lastLBracket = curLBracket;\n";
    ss << "        static bool lastRBracket = false;\n";
    ss << "        bool curRBracket = engine.isKeyDown(VK_OEM_6);\n";
    ss << "        bool rBracketPressed = curRBracket && !lastRBracket;\n";
    ss << "        lastRBracket = curRBracket;\n";
    ss << "        GameRuntime::gInspector.handleInput(tabPressed, gPressed, lBracketPressed, rBracketPressed);\n\n";

    ss << "        // God Mode ativo\n";
    ss << "        if (GameRuntime::gInspector.isGodMode()) {\n";
    ss << "            for (auto& e : GameRuntime::entities) {\n";
    ss << "                if (e->type == \"Player\") e->hp = 100.0;\n";
    ss << "            }\n";
    ss << "        }\n\n";

    ss << "        // Atualiza contadores de tipos congelados\n";
    ss << "        for (auto it = GameRuntime::gFrozenTypes.begin(); it != GameRuntime::gFrozenTypes.end();) {\n";
    ss << "            if (it->second > 0) { it->second--; ++it; } else { it = GameRuntime::gFrozenTypes.erase(it); }\n";
    ss << "        }\n\n";

    ss << "        // 1. Atualizacao de fisica e movimento das entidades com escala temporal\n";
    ss << "        float dtScale = GameRuntime::gTimeBuffer.getTimeScale();\n";
    ss << "        for (auto& e : GameRuntime::entities) {\n";
    ss << "            if (e->active) {\n";
    ss << "                if (GameRuntime::gFrozenTypes.find(e->type) != GameRuntime::gFrozenTypes.end()) continue;\n";
    ss << "                e->x += e->vx * dtScale;\n";
    ss << "                e->y += e->vy * dtScale;\n";
    ss << "            }\n";
    ss << "        }\n\n";

    ss << "        // 2. Deteccao e despacho de colisoes\n";
    ss << "        GameRuntime::checkCollisions();\n\n";

    ss << "        // 3. Hook de logica update do jogo\n";
    ss << "        GameRuntime::game_update();\n\n";

    ss << "        // Gravacao O(1) de Snapshot Temporal no Ring Buffer\n";
    ss << "        GameForge::Time::WorldSnapshot snap;\n";
    ss << "        snap.timeScale = dtScale;\n";
    ss << "        uint32_t snapIdx = 0;\n";
    ss << "        for (const auto& e : GameRuntime::entities) {\n";
    ss << "            if (snapIdx >= GameForge::Time::WorldSnapshot::MAX_SNAPSHOT_ENTITIES) break;\n";
    ss << "            if (!e->active) continue;\n";
    ss << "            auto& es = snap.entities[snapIdx++];\n";
    ss << "            es.id = e->id;\n";
    ss << "            es.x = static_cast<float>(e->x);\n";
    ss << "            es.y = static_cast<float>(e->y);\n";
    ss << "            es.vx = static_cast<float>(e->vx);\n";
    ss << "            es.vy = static_cast<float>(e->vy);\n";
    ss << "            es.hp = static_cast<float>(e->hp);\n";
    ss << "            es.active = e->active;\n";
    ss << "        }\n";
    ss << "        snap.entityCount = snapIdx;\n";
    ss << "        GameRuntime::gTimeBuffer.captureFrame(snap);\n\n";

    ss << "        // 4. Renderizacao em lote de todas as entidades (1 Draw Call para a GPU)\n";
    ss << "        for (auto& e : GameRuntime::entities) {\n";
    ss << "            if (!e->active) continue;\n";
    ss << "            float r, g, b;\n";
    ss << "            GameRuntime::parseColorRgb(e->color, r, g, b);\n";
    ss << "            float px = static_cast<float>(e->x * GameRuntime::coordScale);\n";
    ss << "            float py = static_cast<float>(e->y * GameRuntime::coordScale);\n";
    ss << "            float size = static_cast<float>(GameRuntime::coordScale);\n";
    ss << "            float glow = (e->type == \"Laser\" || e->type == \"Bullet\" || e->type == \"Ball\") ? 2.5f : 0.2f;\n";
    ss << "            if (e->symbol == \"@\" || e->symbol == \"O\" || e->symbol == \"o\") {\n";
    ss << "                engine.drawCircle(px + size * 0.5f, py + size * 0.5f, size * 0.5f, r, g, b, 1.0f, glow);\n";
    ss << "            } else {\n";
    ss << "                engine.drawRect(px, py, size, size, r, g, b, 1.0f, glow);\n";
    ss << "            }\n";
    ss << "        }\n\n";

    ss << "        // 5. Hook de renderizacao customizada do jogo\n";
    ss << "        GameRuntime::game_render();\n\n";

    ss << "        // Rastro Espectral (Ghost Trails / After-Images translucidas na GPU)\n";
    ss << "        for (const auto& e : GameRuntime::entities) {\n";
    ss << "            if (e->active && e->type == \"Player\") {\n";
    ss << "                auto hist = GameRuntime::gTimeBuffer.getEntityHistory(e->id, 8);\n";
    ss << "                for (size_t h = 0; h < hist.size(); ++h) {\n";
    ss << "                    float alpha = 0.35f * (1.0f - static_cast<float>(h) / 8.0f);\n";
    ss << "                    float hpx = static_cast<float>(hist[h].x * GameRuntime::coordScale);\n";
    ss << "                    float hpy = static_cast<float>(hist[h].y * GameRuntime::coordScale);\n";
    ss << "                    float size = static_cast<float>(GameRuntime::coordScale);\n";
    ss << "                    engine.drawRect(hpx, hpy, size, size, 0.2f, 0.8f, 1.0f, alpha, 0.8f);\n";
    ss << "                }\n";
    ss << "            }\n";
    ss << "        }\n\n";

    ss << "        // Renderiza Ecos Temporais\n";
    ss << "        for (auto it = GameRuntime::gEchoes.begin(); it != GameRuntime::gEchoes.end();) {\n";
    ss << "            GameForge::Time::EntitySnapshot echoFrame;\n";
    ss << "            if (it->updateNextFrame(echoFrame)) {\n";
    ss << "                float px = static_cast<float>(echoFrame.x * GameRuntime::coordScale);\n";
    ss << "                float py = static_cast<float>(echoFrame.y * GameRuntime::coordScale);\n";
    ss << "                float size = static_cast<float>(GameRuntime::coordScale);\n";
    ss << "                engine.drawCircle(px + size * 0.5f, py + size * 0.5f, size * 0.6f, 0.2f, 0.95f, 1.0f, 0.6f, 1.5f);\n";
    ss << "                ++it;\n";
    ss << "            } else {\n";
    ss << "                it = GameRuntime::gEchoes.erase(it);\n";
    ss << "            }\n";
    ss << "        }\n\n";

    ss << "        // Live-Tuning Inspector HUD Overlay\n";
    ss << "        if (GameRuntime::gInspector.isVisible()) {\n";
    ss << "            engine.drawRect(16.0f, 16.0f, 320.0f, 190.0f, 0.04f, 0.05f, 0.12f, 0.85f, 0.0f);\n";
    ss << "            engine.drawRect(14.0f, 14.0f, 324.0f, 194.0f, 0.2f, 0.6f, 1.0f, 0.4f, 0.6f);\n";
    ss << "            engine.drawText(28.0f, 26.0f, \"LIVE-TUNING INSPECTOR\", 0.2f, 0.95f, 1.0f, 1.1f);\n";
    ss << "            if (GameRuntime::gInspector.isGodMode()) {\n";
    ss << "                engine.drawText(28.0f, 48.0f, \"[G] GOD MODE: ATIVO\", 0.2f, 1.0f, 0.2f, 1.0f);\n";
    ss << "            } else {\n";
    ss << "                engine.drawText(28.0f, 48.0f, \"[G] GOD MODE: DESATIVADO\", 0.8f, 0.8f, 0.8f, 1.0f);\n";
    ss << "            }\n";
    ss << "            float hr, hg, hb;\n";
    ss << "            GameRuntime::gInspector.getHpColor(hr, hg, hb);\n";
    ss << "            engine.drawText(28.0f, 68.0f, \"VIDA:\", 1.0f, 1.0f, 1.0f, 1.0f);\n";
    ss << "            engine.drawRect(80.0f, 68.0f, 150.0f, 12.0f, 0.2f, 0.2f, 0.2f, 0.8f, 0.0f);\n";
    ss << "            double hpRatio = GameRuntime::gInspector.getHpRatio();\n";
    ss << "            engine.drawRect(80.0f, 68.0f, static_cast<float>(150.0 * hpRatio), 12.0f, hr, hg, hb, 0.95f, 1.5f);\n";
    ss << "            const auto& vars = GameRuntime::gInspector.getVariables();\n";
    ss << "            if (!vars.empty()) {\n";
    ss << "                size_t sel = GameRuntime::gInspector.getSelectedIndex();\n";
    ss << "                const auto& v = vars[sel];\n";
    ss << "                std::string varStr = \"> \" + v.name + \": \" + std::to_string(static_cast<int>(v.ptr ? *v.ptr : 0));\n";
    ss << "                engine.drawText(28.0f, 96.0f, varStr, 1.0f, 0.95f, 0.2f, 1.0f);\n";
    ss << "                engine.drawText(28.0f, 116.0f, \"Ajuste: [ - ]  e  [ + ]\", 0.4f, 0.8f, 1.0f, 0.9f);\n";
    ss << "            }\n";
    ss << "        }\n\n";

    ss << "        // 6. Mensagens de HUD\n";
    ss << "        if (!GameRuntime::hudMessage.empty()) {\n";
    ss << "            engine.drawText(20.0f, static_cast<float>(canvasH - 24), GameRuntime::hudMessage, 1.0f, 0.95f, 0.2f, 1.2f);\n";
    ss << "        }\n\n";

    ss << "        // 7. Submissao de 1 Draw Call, Pos-processamento GLSL e VSync Swap\n";
    ss << "        engine.endFrame();\n\n";

    ss << "        // Compactacao de entidades inativas\n";
    ss << "        GameRuntime::entities.erase(\n";
    ss << "            std::remove_if(GameRuntime::entities.begin(), GameRuntime::entities.end(),\n";
    ss << "                           [](const auto& e) { return !e->active; }),\n";
    ss << "            GameRuntime::entities.end()\n";
    ss << "        );\n";
    ss << "    }\n\n";

    ss << "    return 0;\n";
    ss << "}\n";

    return ss.str();
}

// ============================================================================
// Back-End Terminal Console Clássico (ANSI Escape Sequences)
// ============================================================================
std::string Transpiler::transpileConsole(const Program& program) {
    std::ostringstream ss;

    std::set<std::string> accessedMembers;
    std::set<std::string> calledFunctions;
    for (const auto& g : program.globals) {
        if (g->initializer) collectExpr(g->initializer.get(), accessedMembers, calledFunctions);
    }
    for (const auto& ent : program.entities) {
        for (const auto& f : ent->fields) {
            accessedMembers.insert(f.name);
            if (f.defaultValue) collectExpr(f.defaultValue.get(), accessedMembers, calledFunctions);
        }
    }
    for (const auto& fn : program.functions) {
        if (fn->body) collectStmt(fn->body.get(), accessedMembers, calledFunctions);
    }
    if (program.initBlock) collectStmt(program.initBlock.get(), accessedMembers, calledFunctions);
    if (program.updateBlock) collectStmt(program.updateBlock.get(), accessedMembers, calledFunctions);
    if (program.renderBlock) collectStmt(program.renderBlock.get(), accessedMembers, calledFunctions);
    for (const auto& ch : program.collisionHandlers) {
        if (ch->body) collectStmt(ch->body.get(), accessedMembers, calledFunctions);
    }

    ss << "// Generated by GameForge C++ Game Compiler\n";
    ss << "#include <iostream>\n";
    ss << "#include <vector>\n";
    ss << "#include <string>\n";
    ss << "#include <unordered_map>\n";
    ss << "#include <memory>\n";
    ss << "#include <chrono>\n";
    ss << "#include <thread>\n";
    ss << "#include <queue>\n";
    ss << "#include <mutex>\n";
    ss << "#include <condition_variable>\n";
    ss << "#include <atomic>\n";
    ss << "#include <cmath>\n";
    ss << "#include <random>\n";
    ss << "#include <algorithm>\n";
    ss << "#ifdef _WIN32\n";
    ss << "#define WIN32_LEAN_AND_MEAN\n";
    ss << "#include <windows.h>\n";
    ss << "#include <conio.h>\n";
    ss << "#endif\n";
    ss << "#include \"TimeEngine.h\"\n";
    ss << "#include \"DebugInspector.h\"\n\n";

    ss << R"RAW(
namespace GameRuntime {
    enum class Color { Default, Black, Red, Green, Yellow, Blue, Magenta, Cyan, White };

    inline const char* colorToAnsi(Color c) {
        switch (c) {
            case Color::Black:   return "\033[30m";
            case Color::Red:     return "\033[91m";
            case Color::Green:   return "\033[92m";
            case Color::Yellow:  return "\033[93m";
            case Color::Blue:    return "\033[94m";
            case Color::Magenta: return "\033[95m";
            case Color::Cyan:    return "\033[96m";
            case Color::White:   return "\033[97m";
            default:             return "\033[0m";
        }
    }

    inline Color parseColor(const std::string& name) {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "red") return Color::Red;
        if (s == "green") return Color::Green;
        if (s == "yellow") return Color::Yellow;
        if (s == "blue") return Color::Blue;
        if (s == "magenta") return Color::Magenta;
        if (s == "cyan") return Color::Cyan;
        if (s == "white") return Color::White;
        if (s == "black") return Color::Black;
        return Color::Default;
    }

    struct Pixel { char ch = ' '; Color color = Color::White; };
    struct Tile { char ch = ' '; Color color = Color::White; bool solid = false; };
)RAW";

    ss << "    struct BaseEntity {\n";
    ss << "        uint32_t id = 0;\n";
    ss << "        std::string type;\n";
    ss << "        bool active = true;\n";
    ss << "        double x = 0;\n";
    ss << "        double y = 0;\n";
    ss << "        double vx = 0;\n";
    ss << "        double vy = 0;\n";
    ss << "        double hp = 0;\n";
    ss << "        double alive = 1;\n";
    ss << "        std::string symbol = \"?\";\n";
    ss << "        std::string color = \"white\";\n";
    for (const auto& mem : accessedMembers) {
        if (mem != "x" && mem != "y" && mem != "vx" && mem != "vy" &&
            mem != "hp" && mem != "alive" && mem != "symbol" && mem != "color" &&
            mem != "id" && mem != "type" && mem != "active") {
            ss << "        double " << mem << " = 0.0;\n";
        }
    }
    ss << "        virtual ~BaseEntity() = default;\n";
    ss << "    };\n";

    ss << R"RAW(
    struct Value {
        double num = 0.0;
        std::string str;
        std::shared_ptr<BaseEntity> entity = nullptr;

        Value() = default;
        Value(double n) : num(n) {}
        Value(int n) : num(static_cast<double>(n)) {}
        Value(const char* s) : str(s) {}
        Value(std::string s) : str(std::move(s)) {}
        template<typename T>
        Value(std::shared_ptr<T> e) : entity(std::static_pointer_cast<BaseEntity>(e)) {}

        static BaseEntity* getSafeDummy() {
            static BaseEntity dummy;
            dummy.active = false;
            dummy.x = 0; dummy.y = 0; dummy.vx = 0; dummy.vy = 0;
            return &dummy;
        }
        BaseEntity* operator->() const { return entity ? entity.get() : getSafeDummy(); }
        explicit operator double() const { return num; }
        explicit operator int() const { return static_cast<int>(num); }
        operator bool() const { return entity != nullptr || num != 0.0; }

        Value& operator=(double n) { num = n; entity = nullptr; return *this; }
        Value& operator=(int n) { num = static_cast<double>(n); entity = nullptr; return *this; }
        Value& operator=(const std::string& s) { str = s; return *this; }
        template<typename T>
        Value& operator=(std::shared_ptr<T> e) { entity = std::static_pointer_cast<BaseEntity>(e); return *this; }

        Value operator+(const Value& o) const { return Value(num + o.num); }
        Value operator-(const Value& o) const { return Value(num - o.num); }
        Value operator*(const Value& o) const { return Value(num * o.num); }
        Value operator/(const Value& o) const { return Value(o.num != 0 ? num / o.num : 0); }
        bool operator==(const Value& o) const { return num == o.num && entity == o.entity; }
        bool operator!=(const Value& o) const { return !(*this == o); }
        bool operator<(const Value& o) const { return num < o.num; }
        bool operator<=(const Value& o) const { return num <= o.num; }
        bool operator>(const Value& o) const { return num > o.num; }
        bool operator>=(const Value& o) const { return num >= o.num; }

        Value operator+(double d) const { return Value(num + d); }
        Value operator-(double d) const { return Value(num - d); }
        Value operator*(double d) const { return Value(num * d); }
        Value operator/(double d) const { return Value(d != 0 ? num / d : 0); }
        Value operator+(int i) const { return Value(num + i); }
        Value operator-(int i) const { return Value(num - i); }
        Value operator*(int i) const { return Value(num * i); }
        Value operator/(int i) const { return Value(i != 0 ? num / i : 0); }

        bool operator==(double d) const { return num == d; }
        bool operator!=(double d) const { return num != d; }
        bool operator<(double d) const { return num < d; }
        bool operator<=(double d) const { return num <= d; }
        bool operator>(double d) const { return num > d; }
        bool operator>=(double d) const { return num >= d; }

        bool operator==(int i) const { return num == static_cast<double>(i); }
        bool operator!=(int i) const { return num != static_cast<double>(i); }
        bool operator<(int i) const { return num < static_cast<double>(i); }
        bool operator<=(int i) const { return num <= static_cast<double>(i); }
        bool operator>(int i) const { return num > static_cast<double>(i); }
        bool operator>=(int i) const { return num >= static_cast<double>(i); }

        bool operator==(const char* s) const { return str == s; }
        bool operator!=(const char* s) const { return str != s; }
        bool operator==(const std::string& s) const { return str == s; }
        bool operator!=(const std::string& s) const { return str != s; }
        Value operator+(const char* s) const { return Value(str + s); }
        Value operator+(const std::string& s) const { return Value(str + s); }
    };

    static std::vector<std::shared_ptr<BaseEntity>> entities;
    static uint32_t nextEntityId = 1;
    static int screenWidth = 80;
    static int screenHeight = 25;
    static int targetFps = 30;
    static std::string gameTitle = "Game";
    static std::vector<std::vector<Pixel>> backBuffer;
    static std::vector<std::vector<Pixel>> frontBuffer;
    static std::vector<std::vector<Tile>> tiles;
    static double camX = 0, camY = 0;
    static std::string currentMessage = "";
    static Color messageColor = Color::Yellow;
    static std::string renderCache;

    struct AudioRequest { int freq; int duration; };
    static std::queue<AudioRequest> audioQueue;
    static std::mutex audioMutex;
    static std::condition_variable audioCv;
    static std::atomic<bool> audioRunning{true};
    static std::thread audioWorker;

    inline void startAudioWorker() {
        audioWorker = std::thread([]() {
            while (audioRunning) {
                AudioRequest req{0, 0};
                {
                    std::unique_lock<std::mutex> lock(audioMutex);
                    audioCv.wait(lock, []() { return !audioQueue.empty() || !audioRunning; });
                    if (!audioRunning && audioQueue.empty()) break;
                    req = audioQueue.front();
                    audioQueue.pop();
                }
#ifdef _WIN32
                if (req.freq > 0 && req.duration > 0) {
                    Beep(req.freq, req.duration);
                }
#endif
            }
        });
    }

    inline void stopAudioWorker() {
        audioRunning = false;
        audioCv.notify_all();
        if (audioWorker.joinable()) {
            audioWorker.join();
        }
    }

    inline void beep(int freq = 440, int duration = 50) {
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            audioQueue.push({freq, duration});
        }
        audioCv.notify_one();
    }

    inline void initEngine(int w, int h, int fps, const std::string& title) {
        screenWidth = w; screenHeight = h; targetFps = fps; gameTitle = title;
        backBuffer.assign(h, std::vector<Pixel>(w, Pixel{' ', Color::Default}));
        frontBuffer.assign(h, std::vector<Pixel>(w, Pixel{' ', Color::Default}));
        tiles.assign(100, std::vector<Tile>(200, Tile{' ', Color::Default, false}));
        renderCache.reserve(w * h * 16);
        startAudioWorker();
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hOut, &mode);
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        CONSOLE_CURSOR_INFO ci;
        GetConsoleCursorInfo(hOut, &ci);
        ci.bVisible = FALSE;
        SetConsoleCursorInfo(hOut, &ci);
        SetConsoleTitleA(title.c_str());
#endif
        std::cout << "\033[2J\033[H";
    }

    inline void clearBuffer() {
        for (int y = 0; y < screenHeight; ++y)
            for (int x = 0; x < screenWidth; ++x)
                backBuffer[y][x] = Pixel{' ', Color::Default};
    }

    inline void setPixel(int x, int y, char ch, Color color) {
        if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight)
            backBuffer[y][x] = Pixel{ch, color};
    }

    inline void tile(double x, double y, const std::string& ch, const std::string& colorName, bool solid = false) {
        int ix = static_cast<int>(x), iy = static_cast<int>(y);
        if (iy >= 0 && iy < (int)tiles.size() && ix >= 0 && ix < (int)tiles[0].size()) {
            char c = ch.empty() ? ' ' : ch[0];
            tiles[iy][ix] = Tile{c, parseColor(colorName), solid};
        }
    }

    inline bool tile_solid(double x, double y) {
        int ix = static_cast<int>(x), iy = static_cast<int>(y);
        if (iy >= 0 && iy < (int)tiles.size() && ix >= 0 && ix < (int)tiles[0].size())
            return tiles[iy][ix].solid;
        return false;
    }

    inline std::string tile_at(double x, double y) {
        int ix = static_cast<int>(x), iy = static_cast<int>(y);
        if (iy >= 0 && iy < (int)tiles.size() && ix >= 0 && ix < (int)tiles[0].size())
            return std::string(1, tiles[iy][ix].ch);
        return " ";
    }

    inline void map_box(double x, double y, double w, double h, const std::string& ch, const std::string& col, bool solid = true) {
        int ix = (int)x, iy = (int)y, iw = (int)w, ih = (int)h;
        for (int r = iy; r < iy + ih; ++r) {
            for (int c = ix; c < ix + iw; ++c) {
                if (r == iy || r == iy + ih - 1 || c == ix || c == ix + iw - 1)
                    tile(c, r, ch, col, solid);
            }
        }
    }

    inline void map_row(double x, double y, const std::string& str, const std::string& col, bool solid = false) {
        int ix = (int)x, iy = (int)y;
        for (size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (c != ' ') tile(ix + (int)i, iy, std::string(1, c), col, solid);
        }
    }

    inline void camera(double cx, double cy) { camX = cx; camY = cy; }
    inline void msg(const std::string& text, const std::string& col = "yellow") {
        currentMessage = text;
        messageColor = parseColor(col);
    }
    inline void dialog(const std::string& text, const std::string& col = "white") { msg(text, col); }
    inline void time_rewind(double frames = 60.0) {}
    inline void time_scale(double s = 1.0) {}
    inline void spawn_echo(const Value& entVal, double frames = 60.0) {}
    inline void freeze_type(const std::string& type, double duration = 60.0) {}
    inline void god_mode(bool enabled = true) {}
    inline void tweak_var(const std::string& name, double minVal, double maxVal, double step = 1.0) {}

    inline void present() {
        int viewStartX = static_cast<int>(camX) - screenWidth / 2;
        int viewStartY = static_cast<int>(camY) - screenHeight / 2;
        int maxTileY = (int)tiles.size();
        int maxTileX = (int)tiles[0].size();
        for (int y = 0; y < screenHeight; ++y) {
            int ty = viewStartY + y;
            for (int x = 0; x < screenWidth; ++x) {
                int tx = viewStartX + x;
                if (ty >= 0 && ty < maxTileY && tx >= 0 && tx < maxTileX) {
                    const auto& t = tiles[ty][tx];
                    if (t.ch != ' ') backBuffer[y][x] = Pixel{t.ch, t.color};
                }
            }
        }
        for (const auto& ent : entities) {
            if (!ent->active) continue;
            int rx = static_cast<int>(ent->x) - viewStartX;
            int ry = static_cast<int>(ent->y) - viewStartY;
            if (rx >= 0 && rx < screenWidth && ry >= 0 && ry < screenHeight) {
                char ch = ent->symbol.empty() ? '?' : ent->symbol[0];
                backBuffer[ry][rx] = Pixel{ch, parseColor(ent->color)};
            }
        }
        if (!currentMessage.empty()) {
            int msgY = screenHeight - 1;
            int msgX = 1;
            for (size_t i = 0; i < currentMessage.size() && (msgX + (int)i) < screenWidth - 1; ++i) {
                backBuffer[msgY][msgX + (int)i] = Pixel{currentMessage[i], messageColor};
            }
        }
        renderCache.clear();
        renderCache.append("\033[H");
        Color lastColor = Color::Default;
        for (int y = 0; y < screenHeight; ++y) {
            for (int x = 0; x < screenWidth; ++x) {
                const auto& p = backBuffer[y][x];
                if (p.color != lastColor) {
                    renderCache.append(colorToAnsi(p.color));
                    lastColor = p.color;
                }
                renderCache.push_back(p.ch);
            }
            if (y < screenHeight - 1) renderCache.push_back('\n');
        }
        renderCache.append("\033[0m");
        std::cout << renderCache << std::flush;
        frontBuffer = backBuffer;
    }

    inline bool key(const std::string& k) {
#ifdef _WIN32
        if (k == "left") return (GetAsyncKeyState(VK_LEFT) & 0x8000) || (GetAsyncKeyState('A') & 0x8000);
        if (k == "right") return (GetAsyncKeyState(VK_RIGHT) & 0x8000) || (GetAsyncKeyState('D') & 0x8000);
        if (k == "up") return (GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState('W') & 0x8000);
        if (k == "down") return (GetAsyncKeyState(VK_DOWN) & 0x8000) || (GetAsyncKeyState('S') & 0x8000);
        if (k == "space") return (GetAsyncKeyState(VK_SPACE) & 0x8000);
        if (k == "q") return (GetAsyncKeyState('Q') & 0x8000) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000);
        if (k == "enter") return (GetAsyncKeyState(VK_RETURN) & 0x8000);
#endif
        return false;
    }
    inline bool key_down(const std::string& k) { return key(k); }
    inline bool key_pressed(const std::string& k) { return key(k); }

    inline double random(double min, double max) {
        static std::mt19937 rng(1337);
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }
    inline double rnd(double min, double max) { return random(min, max); }

    inline void destroy(Value v) {
        if (v.entity) v.entity->active = false;
    }

    inline int count(const std::string& type) {
        int c = 0;
        for (const auto& e : entities) {
            if (e->active && e->type == type) c++;
        }
        return c;
    }
)RAW";

    for (const auto& ent : program.entities) {
        ss << "    struct Entity_" << ent->name << " : public BaseEntity {\n";
        ss << "        Entity_" << ent->name << "() {\n";
        ss << "            type = \"" << ent->name << "\";\n";
        for (const auto& f : ent->fields) {
            ss << "            " << f.name << " = ";
            if (f.defaultValue) transpileExpression(*f.defaultValue, ss);
            else ss << "0";
            ss << ";\n";
        }
        ss << "        }\n";
        ss << "    };\n";

        ss << "    inline std::shared_ptr<BaseEntity> spawn_" << ent->name << "() {\n";
        ss << "        auto e = std::make_shared<Entity_" << ent->name << ">();\n";
        ss << "        e->id = nextEntityId++;\n";
        ss << "        entities.push_back(e);\n";
        ss << "        return e;\n";
        ss << "    }\n\n";
    }

    for (const auto& g : program.globals) {
        ss << "    static Value " << g->name << " = ";
        if (g->initializer) transpileExpression(*g->initializer, ss);
        else ss << "0";
        ss << ";\n";
    }
    ss << "\n";

    for (const auto& fn : program.functions) {
        ss << "    Value fn_" << fn->name << "(";
        for (size_t i = 0; i < fn->params.size(); ++i) {
            ss << "Value " << fn->params[i];
            if (i + 1 < fn->params.size()) ss << ", ";
        }
        ss << ") {\n";
        if (fn->body) transpileBlock(*fn->body, ss, 2);
        ss << "        return Value();\n";
        ss << "    }\n\n";
    }

    for (size_t i = 0; i < program.collisionHandlers.size(); ++i) {
        const auto& ch = program.collisionHandlers[i];
        ss << "    void col_handler_" << i << "(std::shared_ptr<BaseEntity> " << ch->varA
           << ", std::shared_ptr<BaseEntity> " << ch->varB << ") {\n";
        if (ch->body) transpileBlock(*ch->body, ss, 2);
        ss << "    }\n\n";
    }

    ss << "    void checkCollisions() {\n";
    ss << "        for (size_t i = 0; i < entities.size(); ++i) {\n";
    ss << "            auto& a = entities[i];\n";
    ss << "            if (!a->active) continue;\n";
    ss << "            for (size_t j = i + 1; j < entities.size(); ++j) {\n";
    ss << "                auto& b = entities[j];\n";
    ss << "                if (!b->active) continue;\n";
    ss << "                if (static_cast<int>(a->x) == static_cast<int>(b->x) &&\n";
    ss << "                    static_cast<int>(a->y) == static_cast<int>(b->y)) {\n";
    for (size_t k = 0; k < program.collisionHandlers.size(); ++k) {
        const auto& ch = program.collisionHandlers[k];
        ss << "                    if (a->type == \"" << ch->entityA << "\" && b->type == \"" << ch->entityB << "\") {\n";
        ss << "                        col_handler_" << k << "(a, b);\n";
        ss << "                    } else if (a->type == \"" << ch->entityB << "\" && b->type == \"" << ch->entityA << "\") {\n";
        ss << "                        col_handler_" << k << "(b, a);\n";
        ss << "                    }\n";
    }
    ss << "                }\n";
    ss << "            }\n";
    ss << "        }\n";
    ss << "    }\n\n";

    ss << "    void game_init() {\n";
    if (program.initBlock) transpileBlock(*program.initBlock, ss, 2);
    ss << "    }\n\n";

    ss << "    void game_update() {\n";
    if (program.updateBlock) transpileBlock(*program.updateBlock, ss, 2);
    ss << "    }\n\n";

    ss << "    void game_render() {\n";
    if (program.renderBlock) transpileBlock(*program.renderBlock, ss, 2);
    ss << "    }\n\n";

    ss << "} // namespace GameRuntime\n\n";

    ss << "int main() {\n";
    ss << "    GameRuntime::initEngine(" << (program.config.width > 0 ? program.config.width : 60)
       << ", " << (program.config.height > 0 ? program.config.height : 22)
       << ", " << (program.config.fps > 0 ? program.config.fps : 30)
       << ", \"" << program.config.title << "\");\n\n";

    ss << "    GameRuntime::game_init();\n\n";

    ss << "    auto targetFrameTime = std::chrono::milliseconds(1000 / GameRuntime::targetFps);\n";
    ss << "    bool running = true;\n";
    ss << "    while (running) {\n";
    ss << "        auto start = std::chrono::high_resolution_clock::now();\n";
    ss << "        if (GameRuntime::key(\"q\")) break;\n\n";

    ss << "        for (auto& e : GameRuntime::entities) {\n";
    ss << "            if (e->active) { e->x += e->vx; e->y += e->vy; }\n";
    ss << "        }\n\n";

    ss << "        GameRuntime::checkCollisions();\n";
    ss << "        GameRuntime::game_update();\n\n";

    ss << "        GameRuntime::clearBuffer();\n";
    ss << "        GameRuntime::game_render();\n";
    ss << "        GameRuntime::present();\n\n";

    ss << "        GameRuntime::entities.erase(\n";
    ss << "            std::remove_if(GameRuntime::entities.begin(), GameRuntime::entities.end(),\n";
    ss << "                           [](const auto& e) { return !e->active; }),\n";
    ss << "            GameRuntime::entities.end()\n";
    ss << "        );\n\n";

    ss << "        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(\n";
    ss << "            std::chrono::high_resolution_clock::now() - start);\n";
    ss << "        if (elapsed < targetFrameTime) {\n";
    ss << "            std::this_thread::sleep_for(targetFrameTime - elapsed);\n";
    ss << "        }\n";
    ss << "    }\n\n";

    ss << "    GameRuntime::stopAudioWorker();\n";
    ss << "    std::cout << \"\\033[2J\\033[H\\033[0mGame Over! Obrigado por jogar.\\n\";\n";
    ss << "    return 0;\n";
    ss << "}\n";

    return ss.str();
}

void Transpiler::transpileBlock(const BlockStmt& block, std::ostringstream& ss, int indent) {
    for (const auto& stmt : block.statements) {
        transpileStatement(*stmt, ss, indent);
    }
}

void Transpiler::transpileStatement(const Stmt& stmt, std::ostringstream& ss, int indent) {
    if (auto varDecl = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        emitIndent(ss, indent);
        ss << "Value " << varDecl->name << " = ";
        if (varDecl->initializer) {
            transpileExpression(*varDecl->initializer, ss);
        } else {
            ss << "0";
        }
        ss << ";\n";
    } else if (auto ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        emitIndent(ss, indent);
        ss << "if (static_cast<bool>(";
        transpileExpression(*ifStmt->condition, ss);
        ss << ")) {\n";
        if (ifStmt->thenBranch) transpileStatement(*ifStmt->thenBranch, ss, indent + 1);
        emitIndent(ss, indent);
        ss << "}";
        if (ifStmt->elseBranch) {
            ss << " else {\n";
            transpileStatement(*ifStmt->elseBranch, ss, indent + 1);
            emitIndent(ss, indent);
            ss << "}";
        }
        ss << "\n";
    } else if (auto whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        emitIndent(ss, indent);
        ss << "while (static_cast<bool>(";
        transpileExpression(*whileStmt->condition, ss);
        ss << ")) {\n";
        if (whileStmt->body) transpileStatement(*whileStmt->body, ss, indent + 1);
        emitIndent(ss, indent);
        ss << "}\n";
    } else if (auto blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        emitIndent(ss, indent);
        ss << "{\n";
        transpileBlock(*blockStmt, ss, indent + 1);
        emitIndent(ss, indent);
        ss << "}\n";
    } else if (auto returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        emitIndent(ss, indent);
        if (returnStmt->value) {
            ss << "return ";
            transpileExpression(*returnStmt->value, ss);
            ss << ";\n";
        } else {
            ss << "return;\n";
        }
    } else if (auto destroyStmt = dynamic_cast<const DestroyStmt*>(&stmt)) {
        emitIndent(ss, indent);
        ss << "destroy(";
        transpileExpression(*destroyStmt->target, ss);
        ss << ");\n";
    } else if (auto exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        emitIndent(ss, indent);
        transpileExpression(*exprStmt->expr, ss);
        ss << ";\n";
    }
}

void Transpiler::transpileExpression(const Expr& expr, std::ostringstream& ss) {
    if (auto lit = dynamic_cast<const LiteralExpr*>(&expr)) {
        switch (lit->kind) {
            case LiteralExpr::Kind::Number: ss << lit->numberVal; break;
            case LiteralExpr::Kind::String: ss << "\"" << lit->stringVal << "\""; break;
            case LiteralExpr::Kind::Boolean: ss << (lit->boolVal ? "true" : "false"); break;
            case LiteralExpr::Kind::Null: ss << "nullptr"; break;
        }
    } else if (auto varExpr = dynamic_cast<const VariableExpr*>(&expr)) {
        ss << varExpr->name;
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        ss << "(";
        transpileExpression(*bin->left, ss);
        ss << " " << Token::tokenTypeName(bin->op) << " ";
        transpileExpression(*bin->right, ss);
        ss << ")";
    } else if (auto un = dynamic_cast<const UnaryExpr*>(&expr)) {
        ss << Token::tokenTypeName(un->op);
        transpileExpression(*un->right, ss);
    } else if (auto asgn = dynamic_cast<const AssignExpr*>(&expr)) {
        ss << asgn->name << " " << Token::tokenTypeName(asgn->op) << " ";
        transpileExpression(*asgn->value, ss);
    } else if (auto mem = dynamic_cast<const MemberAccessExpr*>(&expr)) {
        transpileExpression(*mem->object, ss);
        ss << "->" << mem->member;
    } else if (auto memAsgn = dynamic_cast<const MemberAssignExpr*>(&expr)) {
        transpileExpression(*memAsgn->object, ss);
        ss << "->" << memAsgn->member << " " << Token::tokenTypeName(memAsgn->op) << " ";
        transpileExpression(*memAsgn->value, ss);
    } else if (auto call = dynamic_cast<const CallExpr*>(&expr)) {
        if (call->callee == "key" || call->callee == "key_down" || call->callee == "key_pressed" ||
            call->callee == "beep" || call->callee == "random" || call->callee == "rnd" ||
            call->callee == "destroy" || call->callee == "count" || call->callee == "tile" ||
            call->callee == "tile_solid" || call->callee == "tile_at" || call->callee == "map_box" ||
            call->callee == "map_row" || call->callee == "camera" || call->callee == "msg" ||
            call->callee == "dialog" || call->callee == "set_bloom" || call->callee == "set_scanlines" ||
            call->callee == "time_rewind" || call->callee == "time_scale" || call->callee == "spawn_echo" ||
            call->callee == "freeze_type" || call->callee == "god_mode" || call->callee == "tweak_var") {
            ss << call->callee << "(";
        } else {
            ss << "fn_" << call->callee << "(";
        }
        for (size_t i = 0; i < call->arguments.size(); ++i) {
            transpileExpression(*call->arguments[i], ss);
            if (i + 1 < call->arguments.size()) ss << ", ";
        }
        ss << ")";
    } else if (auto spawn = dynamic_cast<const SpawnExpr*>(&expr)) {
        ss << "spawn_" << spawn->entityType << "()";
    }
}

} // namespace GameLang
