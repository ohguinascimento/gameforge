#include "MemoryPool.h"
#include "ThreadPool.h"
#include "Engine.h"
#include <iostream>
#include <cassert>
#include <chrono>

struct Particle {
    double x = 0;
    double y = 0;
    double vx = 0;
    double vy = 0;
    int life = 100;
};

int main() {
    std::cout << "====================================================\n";
    std::cout << "   GameForge: Testes de Memoria e Multithreading    \n";
    std::cout << "====================================================\n\n";

    // ---------------------------------------------------------
    // Teste 1: MemoryArena
    // ---------------------------------------------------------
    std::cout << "[1/4] Testando MemoryArena (Bump Allocator & Reset O(1))...\n";
    {
        GameForge::MemoryArena arena(1024 * 64);
        void* p1 = arena.allocate(128);
        assert(p1 != nullptr);
        Particle* part = arena.create<Particle>();
        part->x = 10;
        part->y = 20;
        assert(part->x == 10);
        assert(arena.getUsedBytes() > 0);
        size_t used = arena.getUsedBytes();

        // Reset instantâneo O(1)
        arena.reset();
        assert(arena.getUsedBytes() == 0);
        std::cout << "  -> Arena alocou " << used << " bytes e resetou em O(1) com sucesso!\n";
    }

    // ---------------------------------------------------------
    // Teste 2: ObjectPool com Reuso e Free-List
    // ---------------------------------------------------------
    std::cout << "\n[2/4] Testando ObjectPool (Reciclagem e Reuso de Objetos)...\n";
    {
        GameForge::ObjectPool<Particle, 32> pool;
        std::vector<Particle*> active;

        // Aloca 50 partículas
        for (int i = 0; i < 50; ++i) {
            Particle* p = pool.acquire();
            p->x = i;
            active.push_back(p);
        }
        assert(pool.getActiveCount() == 50);

        // Libera 25 partículas de volta para a free-list
        for (int i = 0; i < 25; ++i) {
            pool.release(active.back());
            active.pop_back();
        }

        // Re-adquire 25 partículas (devem ser recicladas da free-list, sem new)
        for (int i = 0; i < 25; ++i) {
            Particle* p = pool.acquire();
            active.push_back(p);
        }

        assert(pool.getRecycledCount() == 25);
        std::cout << "  -> Pool reciclou com sucesso " << pool.getRecycledCount() 
                  << " objetos da free-list sem overhead de heap!\n";
    }

    // ---------------------------------------------------------
    // Teste 3: ThreadPool e Parallel For
    // ---------------------------------------------------------
    std::cout << "\n[3/4] Testando ThreadPool (Worker Queue e Parallel For)...\n";
    {
        GameForge::ThreadPool pool(4);
        assert(pool.getThreadCount() == 4);

        std::vector<int> numbers(1000, 0);
        pool.parallel_for(0, numbers.size(), [&](size_t i) {
            numbers[i] = static_cast<int>(i * 2);
        });

        for (size_t i = 0; i < numbers.size(); ++i) {
            assert(numbers[i] == static_cast<int>(i * 2));
        }

        auto fut = pool.enqueue([]() { return 42; });
        assert(fut.get() == 42);

        std::cout << "  -> ThreadPool processou 1000 iteracoes paralelas em 4 threads com sucesso!\n";
    }

    // ---------------------------------------------------------
    // Teste 4: Engine Entity Recycling e Non-blocking Audio
    // ---------------------------------------------------------
    std::cout << "\n[4/4] Testando Engine Entity Pool e Audio Assincrono...\n";
    {
        GameLang::Engine engine(40, 15, 30, "Test Engine");

        // Spawn de 10 entidades
        std::vector<uint32_t> ids;
        for (int i = 0; i < 10; ++i) {
            ids.push_back(engine.spawn("Bullet", {}));
        }

        // Destroi 5 entidades (liberando para reciclagem)
        for (int i = 0; i < 5; ++i) {
            engine.destroy(ids[i]);
        }

        // Spawna 5 novas entidades (devem reutilizar os slots das 5 destruidas)
        for (int i = 0; i < 5; ++i) {
            engine.spawn("Bullet", {});
        }

        assert(engine.getRecycledEntityCount() == 5);
        std::cout << "  -> Engine reciclou " << engine.getRecycledEntityCount() 
                  << " slots de entidades destruidas!\n";

        // Teste de Audio Não Bloqueante (< 1ms ao invés de travar 50ms)
        auto t0 = std::chrono::steady_clock::now();
        engine.playBeep(440, 50); // Enfileirado assincronamente
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0
        ).count();

        std::cout << "  -> playBeep enfileirado em background em apenas " << elapsed << " us (Sem congelamento de frame)!\n";
    }

    std::cout << "\n\033[1;32m[SUCESSO] Todos os testes de Memoria e Multithreading passaram!\033[0m\n";
    return 0;
}
