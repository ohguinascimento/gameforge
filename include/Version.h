#pragma once

#include <string>

namespace GameForge {

// Semantic Versioning (SemVer 2.0.0): MAJOR.MINOR.PATCH
constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 1;

constexpr const char* VERSION_STRING = "0.0.1";
constexpr const char* VERSION_TAG = "v0.0.1";
constexpr const char* RELEASE_NAME = "RPG Map Engine & Resilience Alpha";

inline std::string getVersionInfo() {
    return std::string("GameForge version ") + VERSION_STRING + " (" + VERSION_TAG + ") - " + RELEASE_NAME;
}

} // namespace GameForge
