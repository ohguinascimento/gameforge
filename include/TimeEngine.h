#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>

namespace GameForge::Time {

// ============================================================================
// Snapshot Leve de Entidade para o Histórico Temporal
// ============================================================================
struct alignas(16) EntitySnapshot {
    uint32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float hp = 100.0f;
    bool active = false;
    char symbol = '?';
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    char typeName[32] = {0};
};

// ============================================================================
// Snapshot Completo do Mundo por Tick (Sem Alocação Dinâmica Contínua)
// ============================================================================
struct WorldSnapshot {
    uint64_t frameIndex = 0;
    float timeScale = 1.0f;
    double score = 0.0;
    double mana = 100.0;
    double lives = 3.0;
    uint32_t entityCount = 0;
    static constexpr size_t MAX_SNAPSHOT_ENTITIES = 256;
    std::array<EntitySnapshot, MAX_SNAPSHOT_ENTITIES> entities{};
};

// ============================================================================
// Snapshot Ring Buffer de Alta Performance (Buffer Circular O(1))
// ============================================================================
template<size_t Capacity = 300>
class TemporalBuffer {
public:
    TemporalBuffer()
        : buffer(std::make_unique<std::array<WorldSnapshot, Capacity>>()) {}

    TemporalBuffer(TemporalBuffer&&) noexcept = default;
    TemporalBuffer& operator=(TemporalBuffer&&) noexcept = default;
    TemporalBuffer(const TemporalBuffer&) = delete;
    TemporalBuffer& operator=(const TemporalBuffer&) = delete;

    void reset() {
        head = 0;
        count = 0;
        currentFrame = 0;
        globalTimeScale = 1.0f;
    }

    // Gravação O(1) de frame no buffer circular
    void captureFrame(const WorldSnapshot& snap) {
        if (!buffer) return;
        (*buffer)[head] = snap;
        (*buffer)[head].frameIndex = currentFrame++;
        (*buffer)[head].timeScale = globalTimeScale;

        head = (head + 1) % Capacity;
        if (count < Capacity) {
            count++;
        }
    }

    size_t getAvailableFrames() const noexcept { return count; }
    size_t getCapacity() const noexcept { return Capacity; }
    float getTimeScale() const noexcept { return globalTimeScale; }
    void setTimeScale(float scale) noexcept { globalTimeScale = scale; }

    // Retorna snapshot de N frames atrás (0 = frame mais recente gravado)
    const WorldSnapshot* getFrameAgo(size_t framesAgo) const {
        if (!buffer || count == 0) return nullptr;
        if (framesAgo >= count) framesAgo = count - 1;

        size_t idx = (head + Capacity - 1 - framesAgo) % Capacity;
        return &(*buffer)[idx];
    }

    // Retorna histórico de posições de uma entidade para efeitos de Echo ou Ghost Trail
    std::vector<EntitySnapshot> getEntityHistory(uint32_t entityId, size_t maxFrames) const {
        std::vector<EntitySnapshot> history;
        history.reserve(std::min(maxFrames, count));

        for (size_t i = 0; i < maxFrames && i < count; ++i) {
            const WorldSnapshot* snap = getFrameAgo(i);
            if (!snap) break;

            for (uint32_t e = 0; e < snap->entityCount; ++e) {
                if (snap->entities[e].id == entityId) {
                    history.push_back(snap->entities[e]);
                    break;
                }
            }
        }
        return history;
    }

    // Retrocede o cursor do universo em N frames
    bool rewindWorld(size_t frames, WorldSnapshot& outState) {
        if (!buffer || count == 0) return false;
        size_t actualRewind = std::min(frames, count - 1);
        const WorldSnapshot* past = getFrameAgo(actualRewind);
        if (!past) return false;

        outState = *past;
        // Rebaixa o ponteiro head pelo número de frames retrocedidos
        head = (head + Capacity - actualRewind) % Capacity;
        count -= actualRewind;
        return true;
    }

    // Recall específico de entidade (apenas uma entidade é teletransportada no tempo)
    bool rewindEntity(uint32_t entityId, size_t framesAgo, EntitySnapshot& outEntity) const {
        const WorldSnapshot* past = getFrameAgo(framesAgo);
        if (!past) return false;

        for (uint32_t i = 0; i < past->entityCount; ++i) {
            if (past->entities[i].id == entityId) {
                outEntity = past->entities[i];
                return true;
            }
        }
        return false;
    }

    // Calcula frequência com ajuste dinâmico de pitch temporal (Audio Slow-Mo / Rewind)
    int getPitchShiftedFrequency(int baseFreq) const {
        if (globalTimeScale <= 0.0f) {
            // Frequência invertida / modulada durante rewind
            return std::clamp(static_cast<int>(baseFreq * 1.5f), 100, 2500);
        }
        // Desacelera a frequência proporcionalmente à dilatação temporal
        return std::clamp(static_cast<int>(baseFreq * std::max(0.25f, globalTimeScale)), 80, 2500);
    }

private:
    std::unique_ptr<std::array<WorldSnapshot, Capacity>> buffer;
    size_t head = 0;
    size_t count = 0;
    uint64_t currentFrame = 0;
    float globalTimeScale = 1.0f;
};

// ============================================================================
// Entidade Eco Temporal (Fantasma com Rastro do Passado)
// ============================================================================
struct TemporalEcho {
    uint32_t targetEntityId = 0;
    std::vector<EntitySnapshot> trajectory;
    size_t playbackIndex = 0;
    bool active = true;
    float alphaFade = 0.65f;

    bool updateNextFrame(EntitySnapshot& outFrame) {
        if (!active || trajectory.empty() || playbackIndex >= trajectory.size()) {
            active = false;
            return false;
        }
        outFrame = trajectory[playbackIndex++];
        return true;
    }
};

} // namespace GameForge::Time
