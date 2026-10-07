#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#endif

namespace GameLang {

struct SourceLocation {
    int line = 1;
    int column = 1;
};

struct GameConfig {
    std::string title = "My Arcade Game";
    int width = 50;
    int height = 20;
    int fps = 30;
};

enum class Color {
    Default,
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
};

inline std::string colorToAnsi(Color c) {
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

} // namespace GameLang
