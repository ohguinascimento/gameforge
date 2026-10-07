#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <mutex>
#include <new>
#include <algorithm>
#include <utility>

namespace GameForge {

// ============================================================================
// 1. Memory Arena (Linear / Bump Allocator com Reset O(1))
// ============================================================================
class MemoryArena {
public:
    explicit MemoryArena(size_t capacityBytes = 64 * 1024)
        : capacity(capacityBytes), offset(0) {
        buffer = static_cast<uint8_t*>(::operator new(capacity));
    }

    ~MemoryArena() {
        if (buffer) {
            ::operator delete(buffer);
            buffer = nullptr;
        }
    }

    // Não copiável
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;

    // Movível
    MemoryArena(MemoryArena&& other) noexcept
        : buffer(other.buffer), capacity(other.capacity), offset(other.offset) {
        other.buffer = nullptr;
        other.capacity = 0;
        other.offset = 0;
    }

    MemoryArena& operator=(MemoryArena&& other) noexcept {
        if (this != &other) {
            if (buffer) ::operator delete(buffer);
            buffer = other.buffer;
            capacity = other.capacity;
            offset = other.offset;
            other.buffer = nullptr;
            other.capacity = 0;
            other.offset = 0;
        }
        return *this;
    }

    void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t)) {
        size_t currentAddr = reinterpret_cast<size_t>(buffer + offset);
        size_t padding = (alignment - (currentAddr % alignment)) % alignment;

        if (offset + padding + bytes > capacity) {
            // Expansão dinâmica se exceder capacidade inicial
            grow(std::max(capacity * 2, offset + padding + bytes));
            currentAddr = reinterpret_cast<size_t>(buffer + offset);
            padding = (alignment - (currentAddr % alignment)) % alignment;
        }

        offset += padding;
        void* ptr = buffer + offset;
        offset += bytes;
        return ptr;
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // Reseta todo o espaço em O(1) sem desalocar buffer do heap
    void reset() noexcept {
        offset = 0;
    }

    size_t getUsedBytes() const noexcept { return offset; }
    size_t getCapacity() const noexcept { return capacity; }

private:
    void grow(size_t newCapacity) {
        uint8_t* newBuffer = static_cast<uint8_t*>(::operator new(newCapacity));
        if (buffer && offset > 0) {
            std::copy(buffer, buffer + offset, newBuffer);
        }
        if (buffer) {
            ::operator delete(buffer);
        }
        buffer = newBuffer;
        capacity = newCapacity;
    }

    uint8_t* buffer = nullptr;
    size_t capacity = 0;
    size_t offset = 0;
};

// ============================================================================
// 2. Generic Object Pool com Free-List e Reuso de Memória
// ============================================================================
template<typename T, size_t BlockSize = 64>
class ObjectPool {
public:
    ObjectPool() = default;

    ~ObjectPool() {
        clear();
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    template<typename... Args>
    T* acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (freeList.empty()) {
            allocateBlock();
        }

        T* obj = freeList.back();
        freeList.pop_back();

        if (releasedCount > 0) {
            reusedCount++;
            releasedCount--;
        }

        activeCount++;
        return new (obj) T(std::forward<Args>(args)...);
    }

    void release(T* obj) {
        if (!obj) return;
        obj->~T(); // Destrói o objeto no slot

        std::lock_guard<std::mutex> lock(poolMutex);
        freeList.push_back(obj);
        releasedCount++;
        if (activeCount > 0) activeCount--;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(poolMutex);
        freeList.clear();
        for (void* block : allocatedBlocks) {
            ::operator delete(block);
        }
        allocatedBlocks.clear();
        activeCount = 0;
        releasedCount = 0;
        reusedCount = 0;
    }

    size_t getActiveCount() const { 
        std::lock_guard<std::mutex> lock(poolMutex);
        return activeCount; 
    }
    size_t getRecycledCount() const { 
        std::lock_guard<std::mutex> lock(poolMutex);
        return reusedCount; 
    }
    size_t getTotalCapacity() const { 
        std::lock_guard<std::mutex> lock(poolMutex);
        return allocatedBlocks.size() * BlockSize; 
    }

private:
    void allocateBlock() {
        size_t bytes = sizeof(T) * BlockSize;
        uint8_t* block = static_cast<uint8_t*>(::operator new(bytes));
        allocatedBlocks.push_back(block);

        for (size_t i = 0; i < BlockSize; ++i) {
            T* item = reinterpret_cast<T*>(block + i * sizeof(T));
            freeList.push_back(item);
        }
    }

    mutable std::mutex poolMutex;
    std::vector<T*> freeList;
    std::vector<void*> allocatedBlocks;
    size_t activeCount = 0;
    size_t releasedCount = 0;
    size_t reusedCount = 0;
};

} // namespace GameForge
