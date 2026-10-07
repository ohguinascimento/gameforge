#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace GameForge::Debug {

// ============================================================================
// Variável Ajustável em Tempo de Execução (Live-Tuning Attribute)
// ============================================================================
struct TweakableVar {
    std::string name;
    double* ptr = nullptr;
    double minVal = 0.0;
    double maxVal = 100.0;
    double step = 1.0;
    int keyUp = 0;
    int keyDown = 0;
};

// ============================================================================
// Inspetor e HUD de Ajustes ao Vivo (Live-Tuning Inspector & God Mode)
// ============================================================================
class DebugInspector {
public:
    DebugInspector() = default;

    bool isVisible() const noexcept { return visible; }
    void toggleVisible() noexcept { visible = !visible; }
    void setVisible(bool v) noexcept { visible = v; }

    bool isGodMode() const noexcept { return godMode; }
    void setGodMode(bool g) noexcept { godMode = g; }
    void toggleGodMode() noexcept { godMode = !godMode; }

    void registerVar(const std::string& name, double* ptr, double minVal = 0.0, double maxVal = 100.0, double step = 1.0) {
        if (!ptr) return;
        for (auto& v : variables) {
            if (v.name == name) {
                v.ptr = ptr;
                v.minVal = minVal;
                v.maxVal = maxVal;
                v.step = step;
                return;
            }
        }
        variables.push_back({ name, ptr, minVal, maxVal, step, 0, 0 });
    }

    void registerPlayerHp(double* hpPtr, double maxHp = 100.0) {
        playerHpPtr = hpPtr;
        playerMaxHp = maxHp;
        registerVar("Player HP", hpPtr, 0.0, maxHp, 5.0);
    }

    // Processamento de teclas de atalho (TAB para HUD, G para God Mode, [ / ] para valores)
    void handleInput(bool tabPressed, bool gPressed, bool leftBracketPressed, bool rightBracketPressed) {
        if (tabPressed) {
            toggleVisible();
        }
        if (gPressed) {
            toggleGodMode();
        }

        if (!variables.empty()) {
            if (leftBracketPressed) {
                decreaseCurrentVar();
            }
            if (rightBracketPressed) {
                increaseCurrentVar();
            }
        }
    }

    void selectNextVar() {
        if (!variables.empty()) {
            selectedIndex = (selectedIndex + 1) % variables.size();
        }
    }

    void selectPrevVar() {
        if (!variables.empty()) {
            selectedIndex = (selectedIndex + variables.size() - 1) % variables.size();
        }
    }

    void increaseCurrentVar() {
        if (variables.empty() || selectedIndex >= variables.size()) return;
        auto& v = variables[selectedIndex];
        if (v.ptr) {
            *v.ptr = std::min(v.maxVal, *v.ptr + v.step);
        }
    }

    void decreaseCurrentVar() {
        if (variables.empty() || selectedIndex >= variables.size()) return;
        auto& v = variables[selectedIndex];
        if (v.ptr) {
            *v.ptr = std::max(v.minVal, *v.ptr - v.step);
        }
    }

    // Retorna a cor da barra de vida com base nos limiares:
    // Verde > 50%, Amarelo > 25%, Vermelho <= 25%
    void getHpColor(float& r, float& g, float& b) const {
        double ratio = getHpRatio();
        if (ratio > 0.50) {
            r = 0.2f; g = 1.0f; b = 0.2f; // Verde
        } else if (ratio > 0.25) {
            r = 1.0f; g = 0.95f; b = 0.2f; // Amarelo
        } else {
            r = 1.0f; g = 0.2f; b = 0.2f; // Vermelho
        }
    }

    double getHpRatio() const {
        if (!playerHpPtr || playerMaxHp <= 0.0) return 1.0;
        return std::clamp(*playerHpPtr / playerMaxHp, 0.0, 1.0);
    }

    const std::vector<TweakableVar>& getVariables() const { return variables; }
    size_t getSelectedIndex() const { return selectedIndex; }
    double getPlayerHp() const { return playerHpPtr ? *playerHpPtr : 0.0; }
    double getPlayerMaxHp() const { return playerMaxHp; }

private:
    bool visible = false;
    bool godMode = false;
    double* playerHpPtr = nullptr;
    double playerMaxHp = 100.0;
    std::vector<TweakableVar> variables;
    size_t selectedIndex = 0;
};

} // namespace GameForge::Debug
