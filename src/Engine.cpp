#include "Engine.h"
#include <random>
#include <thread>

namespace GameLang {

Engine::Engine(int width, int height, int fps, const std::string& title)
    : width(width), height(height), targetFps(fps), title(title) {
    frontBuffer.resize(width * height, Pixel{' ', Color::Default});
    backBuffer.resize(width * height, Pixel{' ', Color::Default});
    mapWidth = width;
    mapHeight = height;
    tiles.resize(mapWidth * mapHeight, Tile{' ', Color::Default, false});
    frameBuffer.reserve(width * height * 12);
    lastFrameTime = std::chrono::steady_clock::now();

    // Inicia worker thread de audio em background (nao bloqueante)
    audioThread = std::thread([this]() {
        while (this->audioRunning) {
            AudioRequest req{0, 0};
            {
                std::unique_lock<std::mutex> lock(this->audioMutex);
                this->audioCv.wait(lock, [this]() {
                    return !this->audioRunning || !this->audioQueue.empty();
                });
                if (!this->audioRunning && this->audioQueue.empty()) {
                    break;
                }
                req = this->audioQueue.front();
                this->audioQueue.pop();
            }
#ifdef _WIN32
            if (req.freq > 0 && req.durationMs > 0) {
                Beep(req.freq, std::min(req.durationMs, 50));
            }
#endif
        }
    });
}

Engine::~Engine() {
    // Encerra audio worker e thread pool de forma segura
    audioRunning = false;
    audioCv.notify_all();
    if (audioThread.joinable()) {
        audioThread.join();
    }
    threadPool.shutdown();
    resetTerminal();
}

void Engine::initTerminal() {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hConsole, &originalConsoleMode);
        DWORD mode = originalConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);

        // Hide cursor
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
#endif
    // Set title and clear screen
    std::cout << "\033]0;" << title << "\007";
    std::cout << "\033[2J\033[H" << std::flush;
}

void Engine::resetTerminal() {
#ifdef _WIN32
    if (hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hConsole, originalConsoleMode);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
#endif
    std::cout << "\033[0m\033[?25h\n" << std::flush;
}

void Engine::clearBuffer() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            backBuffer[y * width + x] = Pixel{' ', Color::Default};
        }
    }
}

void Engine::setPixel(int x, int y, char ch, Color color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        backBuffer[y * width + x] = Pixel{ch, color};
    }
}

void Engine::drawText(int x, int y, const std::string& text, Color color) {
    for (size_t i = 0; i < text.size(); ++i) {
        setPixel(x + static_cast<int>(i), y, text[i], color);
    }
}

void Engine::drawBox(int x, int y, int w, int h, Color color) {
    for (int i = 0; i < w; ++i) {
        setPixel(x + i, y, '-', color);
        setPixel(x + i, y + h - 1, '-', color);
    }
    for (int i = 0; i < h; ++i) {
        setPixel(x, y + i, '|', color);
        setPixel(x + w - 1, y + i, '|', color);
    }
    setPixel(x, y, '+', color);
    setPixel(x + w - 1, y, '+', color);
    setPixel(x, y + h - 1, '+', color);
    setPixel(x + w - 1, y + h - 1, '+', color);
}

void Engine::setMapSize(int w, int h) {
    if (w <= 0 || h <= 0) return;
    mapWidth = w;
    mapHeight = h;
    tiles.assign(mapWidth * mapHeight, Tile{' ', Color::Default, false});
}

void Engine::setTile(int x, int y, char ch, Color color, bool solid) {
    if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
        tiles[y * mapWidth + x] = Tile{ch, color, solid};
    }
}

bool Engine::isTileSolid(int x, int y) const {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return true;
    return tiles[y * mapWidth + x].solid;
}

char Engine::getTileChar(int x, int y) const {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return '#';
    return tiles[y * mapWidth + x].ch;
}

void Engine::fillMapBox(int x, int y, int w, int h, char ch, Color color, bool solid) {
    for (int cy = y; cy < y + h; ++cy) {
        for (int cx = x; cx < x + w; ++cx) {
            if (cy == y || cy == y + h - 1 || cx == x || cx == x + w - 1) {
                setTile(cx, cy, ch, color, solid);
            }
        }
    }
}

void Engine::setMapRow(int x, int y, const std::string& row, Color color, bool solid) {
    for (size_t i = 0; i < row.size(); ++i) {
        if (row[i] != ' ') {
            setTile(x + static_cast<int>(i), y, row[i], color, solid);
        }
    }
}

void Engine::setCamera(int cx, int cy) {
    cameraX = cx;
    cameraY = cy;
}

void Engine::setMessage(const std::string& msg, Color color) {
    currentMessage = msg;
    messageColor = color;
}

void Engine::present() {
    frameBuffer.clear();
    frameBuffer += "\033[H"; // Move to home (0,0)

    Color currentColor = Color::Default;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Pixel& p = backBuffer[y * width + x];
            if (p.color != currentColor) {
                frameBuffer += colorToAnsi(p.color);
                currentColor = p.color;
            }
            frameBuffer += p.ch;
        }
        frameBuffer += "\n";
    }

    if (currentColor != Color::Default) {
        frameBuffer += "\033[0m";
    }

    std::cout << frameBuffer << std::flush;
    frontBuffer = backBuffer;
}

void Engine::pollInput() {
    keysPressed.clear();

#ifdef _WIN32
    while (_kbhit()) {
        int ch = _getch();
        std::string keyName;

        if (ch == 224 || ch == 0) {
            // Extended key code (arrow keys, function keys)
            int ext = _getch();
            switch (ext) {
                case 72: keyName = "UP"; break;
                case 80: keyName = "DOWN"; break;
                case 75: keyName = "LEFT"; break;
                case 77: keyName = "RIGHT"; break;
                default: break;
            }
        } else if (ch == 27) {
            keyName = "ESC";
            requestExit();
        } else if (ch == 32) {
            keyName = "SPACE";
        } else if (ch == 13) {
            keyName = "ENTER";
        } else {
            char upper = static_cast<char>(std::toupper(ch));
            keyName = std::string(1, upper);

            // Also map WASD
            if (upper == 'W') keysDown["UP"] = true;
            if (upper == 'S') keysDown["DOWN"] = true;
            if (upper == 'A') keysDown["LEFT"] = true;
            if (upper == 'D') keysDown["RIGHT"] = true;
            if (upper == 'Q') requestExit();
        }

        if (!keyName.empty()) {
            keysDown[keyName] = true;
            keysPressed[keyName] = true;
        }
    }

    // Windows Async Key State for smooth continuous holding
    auto checkAsync = [&](int vKey, const std::string& name) {
        if (GetAsyncKeyState(vKey) & 0x8000) {
            keysDown[name] = true;
        } else {
            keysDown[name] = false;
        }
    };

    checkAsync(VK_LEFT, "LEFT");
    checkAsync(VK_RIGHT, "RIGHT");
    checkAsync(VK_UP, "UP");
    checkAsync(VK_DOWN, "DOWN");
    checkAsync(VK_SPACE, "SPACE");
    checkAsync('A', "A");
    checkAsync('D', "D");
    checkAsync('W', "W");
    checkAsync('S', "S");
    checkAsync('Q', "Q");

    if (keysDown["A"]) keysDown["LEFT"] = true;
    if (keysDown["D"]) keysDown["RIGHT"] = true;
    if (keysDown["W"]) keysDown["UP"] = true;
    if (keysDown["S"]) keysDown["DOWN"] = true;

#endif
}

bool Engine::isKeyDown(const std::string& key) const {
    std::string upper = key;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = keysDown.find(upper);
    return it != keysDown.end() && it->second;
}

bool Engine::isKeyPressed(const std::string& key) const {
    std::string upper = key;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = keysPressed.find(upper);
    return it != keysPressed.end() && it->second;
}

uint32_t Engine::spawn(const std::string& type, const std::unordered_map<std::string, Value>& defaults) {
    uint32_t id;
    if (!freeEntityIds.empty()) {
        id = freeEntityIds.back();
        freeEntityIds.pop_back();
        recycledEntityCount++;

        Entity& ent = entities[id];
        ent.id = id;
        ent.type = type;
        ent.active = true;
        ent.fields = defaults;
    } else {
        id = nextEntityId++;
        Entity ent;
        ent.id = id;
        ent.type = type;
        ent.active = true;
        ent.fields = defaults;
        entities[id] = std::move(ent);
    }
    return id;
}

void Engine::destroy(uint32_t id) {
    auto it = entities.find(id);
    if (it != entities.end() && it->second.active) {
        it->second.active = false;
        it->second.fields.clear();
        freeEntityIds.push_back(id);
    }
}

Entity* Engine::getEntity(uint32_t id) {
    auto it = entities.find(id);
    if (it != entities.end() && it->second.active) {
        return &it->second;
    }
    return nullptr;
}

std::vector<uint32_t> Engine::getActiveEntitiesByType(const std::string& type) const {
    std::vector<uint32_t> list;
    for (const auto& kv : entities) {
        if (kv.second.active && kv.second.type == type) {
            list.push_back(kv.first);
        }
    }
    return list;
}

std::vector<std::pair<uint32_t, uint32_t>> Engine::checkCollisions(const std::string& typeA, const std::string& typeB) {
    std::vector<std::pair<uint32_t, uint32_t>> hits;
    auto listA = getActiveEntitiesByType(typeA);
    auto listB = getActiveEntitiesByType(typeB);
    if (listA.empty() || listB.empty()) return hits;

    size_t totalPairs = listA.size() * listB.size();

    // Se houver volume expressivo de pares e mais de 1 thread disponivel, paraleliza
    if (totalPairs >= 32 && threadPool.getThreadCount() > 1) {
        std::vector<std::vector<std::pair<uint32_t, uint32_t>>> threadHits(listA.size());

        threadPool.parallel_for(0, listA.size(), [&](size_t idx) {
            uint32_t idA = listA[idx];
            Entity* eA = getEntity(idA);
            if (!eA || !eA->active) return;

            int ax = static_cast<int>(std::round(eA->getX()));
            int ay = static_cast<int>(std::round(eA->getY()));

            for (uint32_t idB : listB) {
                if (idA == idB) continue;
                Entity* eB = getEntity(idB);
                if (!eB || !eB->active) continue;

                int bx = static_cast<int>(std::round(eB->getX()));
                int by = static_cast<int>(std::round(eB->getY()));

                if (ax == bx && ay == by) {
                    threadHits[idx].push_back({idA, idB});
                }
            }
        });

        for (const auto& thHit : threadHits) {
            hits.insert(hits.end(), thHit.begin(), thHit.end());
        }
    } else {
        // Fast path sequencial para cenarios pequenos
        for (uint32_t idA : listA) {
            Entity* eA = getEntity(idA);
            if (!eA || !eA->active) continue;

            for (uint32_t idB : listB) {
                if (idA == idB) continue;
                Entity* eB = getEntity(idB);
                if (!eB || !eB->active) continue;

                int ax = static_cast<int>(std::round(eA->getX()));
                int ay = static_cast<int>(std::round(eA->getY()));
                int bx = static_cast<int>(std::round(eB->getX()));
                int by = static_cast<int>(std::round(eB->getY()));

                if (ax == bx && ay == by) {
                    hits.push_back({idA, idB});
                }
            }
        }
    }

    return hits;
}

void Engine::playBeep(int freq, int durationMs) {
    if (freq > 0 && durationMs > 0) {
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            audioQueue.push({freq, durationMs});
        }
        audioCv.notify_one();
    }
}

int Engine::getRandomInt(int minVal, int maxVal) {
    static std::mt19937 rng(1337);
    if (minVal > maxVal) std::swap(minVal, maxVal);
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(rng);
}

void Engine::syncFrame() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
    long long frameDuration = 1000 / targetFps;

    if (elapsed < frameDuration) {
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDuration - elapsed));
    }
    lastFrameTime = std::chrono::steady_clock::now();
}

} // namespace GameLang
