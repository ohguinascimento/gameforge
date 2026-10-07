#pragma once

#include "Common.h"
#include "Bytecode.h"
#include "MemoryPool.h"
#include "ThreadPool.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace GameLang {

struct Pixel {
    char ch = ' ';
    Color color = Color::White;
};

struct Tile {
    char ch = ' ';
    Color color = Color::Default;
    bool solid = false;
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

    // Entity manager com Reciclagem Inteligente
    uint32_t spawn(const std::string& type, const std::unordered_map<std::string, Value>& defaults);
    void destroy(uint32_t id);
    Entity* getEntity(uint32_t id);
    std::vector<uint32_t> getActiveEntitiesByType(const std::string& type) const;
    const std::unordered_map<uint32_t, Entity>& getAllEntities() const { return entities; }
    size_t getRecycledEntityCount() const { return recycledEntityCount; }

    // Collision detection (Paralelo via ThreadPool)
    std::vector<std::pair<uint32_t, uint32_t>> checkCollisions(const std::string& typeA, const std::string& typeB);

    // Audio & Utilities (Audio assíncrono não bloqueante)
    void playBeep(int freq, int durationMs);
    int getRandomInt(int minVal, int maxVal);

    // RPG Map & Tilemap
    void setMapSize(int w, int h);
    void setTile(int x, int y, char ch, Color color, bool solid);
    bool isTileSolid(int x, int y) const;
    char getTileChar(int x, int y) const;
    void fillMapBox(int x, int y, int w, int h, char ch, Color color, bool solid);
    void setMapRow(int x, int y, const std::string& row, Color color, bool solid);
    void setCamera(int cx, int cy);
    int getCameraX() const { return cameraX; }
    int getCameraY() const { return cameraY; }
    void setMessage(const std::string& msg, Color color = Color::Yellow);
    const std::string& getMessage() const { return currentMessage; }
    Color getMessageColor() const { return messageColor; }
    const std::vector<Tile>& getTiles() const { return tiles; }
    int getMapWidth() const { return mapWidth; }
    int getMapHeight() const { return mapHeight; }

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

    // Tilemap & Camera state
    int mapWidth = 0;
    int mapHeight = 0;
    std::vector<Tile> tiles;
    int cameraX = 0;
    int cameraY = 0;
    std::string currentMessage;
    Color messageColor = Color::Yellow;

    std::vector<Pixel> frontBuffer;
    std::vector<Pixel> backBuffer;

    // Gerenciamento e reciclagem inteligente de entidades
    uint32_t nextEntityId = 1;
    std::unordered_map<uint32_t, Entity> entities;
    std::vector<uint32_t> freeEntityIds;
    size_t recycledEntityCount = 0;

    // Reuso de buffer de string de renderizacao
    std::string frameBuffer;

    // Arquitetura Multi-thread
    GameForge::ThreadPool threadPool;

    // Background Audio Worker (Non-blocking)
    struct AudioRequest { int freq; int durationMs; };
    std::queue<AudioRequest> audioQueue;
    std::mutex audioMutex;
    std::condition_variable audioCv;
    std::thread audioThread;
    std::atomic<bool> audioRunning{true};

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
