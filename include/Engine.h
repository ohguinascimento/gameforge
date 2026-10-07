#pragma once

#include "Common.h"
#include "Bytecode.h"

namespace GameLang {

struct Pixel {
    char ch = ' ';
    Color color = Color::White;
};

struct Entity {
    uint32_t id = 0;
    std::string type;
    bool active = true;
    std::unordered_map<std::string, Value> fields;

    double getX() const {
        auto it = fields.find("x");
        return (it != fields.end() && it->second.isNumber()) ? it->second.asNumber() : 0.0;
    }
    double getY() const {
        auto it = fields.find("y");
        return (it != fields.end() && it->second.isNumber()) ? it->second.asNumber() : 0.0;
    }
    char getSymbol() const {
        auto it = fields.find("symbol");
        if (it != fields.end() && it->second.isString() && !it->second.asString().empty()) {
            return it->second.asString()[0];
        }
        return '?';
    }
    Color getColor() const {
        auto it = fields.find("color");
        if (it != fields.end() && it->second.isString()) {
            return parseColor(it->second.asString());
        }
        return Color::White;
    }
};

class Engine {
public:
    Engine(int width, int height, int fps, const std::string& title);
    ~Engine();

    void initTerminal();
    void resetTerminal();

    void clearBuffer();
    void setPixel(int x, int y, char ch, Color color);
    void drawText(int x, int y, const std::string& text, Color color);
    void drawBox(int x, int y, int w, int h, Color color);
    void present();

    void pollInput();
    bool isKeyDown(const std::string& key) const;
    bool isKeyPressed(const std::string& key) const;

    // Entity manager
    uint32_t spawn(const std::string& type, const std::unordered_map<std::string, Value>& defaults);
    void destroy(uint32_t id);
    Entity* getEntity(uint32_t id);
    std::vector<uint32_t> getActiveEntitiesByType(const std::string& type) const;
    const std::unordered_map<uint32_t, Entity>& getAllEntities() const { return entities; }

    // Collision detection
    std::vector<std::pair<uint32_t, uint32_t>> checkCollisions(const std::string& typeA, const std::string& typeB);

    // Audio & Utilities
    void playBeep(int freq, int durationMs);
    int getRandomInt(int minVal, int maxVal);

    bool shouldClose() const { return exitRequested; }
    void requestExit() { exitRequested = true; }

    void syncFrame();

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getFPS() const { return targetFps; }

private:
    int width;
    int height;
    int targetFps;
    std::string title;

    std::vector<Pixel> frontBuffer;
    std::vector<Pixel> backBuffer;

    uint32_t nextEntityId = 1;
    std::unordered_map<uint32_t, Entity> entities;

    std::unordered_map<std::string, bool> keysDown;
    std::unordered_map<std::string, bool> keysPressed;

    bool exitRequested = false;
    std::chrono::steady_clock::time_point lastFrameTime;

#ifdef _WIN32
    HANDLE hConsole = nullptr;
    DWORD originalConsoleMode = 0;
#endif
};

} // namespace GameLang
