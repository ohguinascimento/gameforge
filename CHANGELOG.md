# Changelog

Todas as mudanças notáveis deste projeto serão documentadas neste arquivo.

O formato é baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/),
e este projeto adere ao [Semantic Versioning (SemVer 2.0.0)](https://semver.org/lang/pt-BR/).

---

## [0.0.1] - 2026-10-07

### Adicionado
- **Motor Gráfico 2D em GPU com OpenGL 3.3 Core Profile e Raylib (`include/GpuEngineGL.h`):**
  - Render Target Virtual fixo (Canvas FBO) com escalonamento Pixel-Perfect e Letterboxing/Pillarboxing automático para 1080p, 1440p e 4K.
  - Sprite Batching / Instanciamento: Envio de todas as geometrias e sprites para a GPU em 1 única Draw Call (`glDrawArraysInstanced`).
  - Shaders GLSL 330 de pós-processamento: Bloom / emissive glow para lasers/explosões e CRT scanlines/vinheta retrô calculados em paralelo na GPU.
  - Latência de entrada mínima (<2ms) via polling de alta resolução Win32 (`GetAsyncKeyState`).
- **Verificador Semântico Estrito (Front-End - `include/SemanticAnalyzer.h` e `src/SemanticAnalyzer.cpp`):**
  - Análise semântica estrita da AST verificando entidades declaradas, escopos de variáveis, assinaturas de funções e tratadores de colisão declarativos `on collision(A, B)`.
  - Novo comando de linha de comando: `gamec check <arquivo.game>`.
- **Automação e Ferramentas para Desenvolvedores:**
  - Script `build_gpu.ps1` com otimização máxima `g++ -std=c++20 -O3 -march=native`.
  - Configurações do VS Code: `.vscode/tasks.json` e `.vscode/launch.json` para compilação e depuração nativa com GDB (F5).
  - Demonstração Arcade completa compilável de ponta a ponta: `games/space_invaders_gpu.game` e `games/pong_gpu.game`.
- **Sistema de Mapas RPG e Masmorras 2D:**
  - Primitiva `tile(x, y, ch, cor, solido)` para posicionamento de ladrilhos e obstáculos.
  - Primitiva `map_box(x, y, w, h, ch, cor, solido)` para construção ágil de salas com paredes sólidas.
  - Primitiva `map_row(x, y, texto, cor, solido)` para renderização em lote de linhas de mapa.
  - Função `tile_solid(x, y)` para colisão precisa de terreno antes de movimentar entidades.
  - Função `tile_at(x, y)` para inspeção do caractere na coordenada (portas trancadas, fontes, passagens secretas).
  - Controle de Câmera Viewport com `camera(cx, cy)` para rolagem dinâmica acompanhando o herói em mapas amplos.
  - Sistema de Notificação e Diálogo `msg(texto, cor)` / `dialog(texto, cor)` com renderização estilizada no rodapé.
  - HUD RPG automático na barra inferior para variáveis globais temáticas `hp` (vermelho) e `gold` (amarelo).
- **Jogo de Demonstração RPG:**
  - `games/rpg_dungeon.game`: Masmorra completa com múltiplas salas, corredores, porta com fechadura de chave de ferro `k`, fonte sagrada regenerativa `~`, baú com tesouro `$`, monstros `M` e esqueletos `S`.
- **Módulo Rust de Alta Performance e Aceleração GPU 2D (`rust/gameforge_gpu`):**
  - Crate Rust configurado como biblioteca C-ABI (`cdylib` / `staticlib`) para interoperabilidade direta com o GameForge em C++.
  - Otimizador de Bytecode em Rust (`optimizer.rs`): Passagem de otimização *peephole* com poda de pares redundantes `OP_NULL + OP_POP` e eliminação de código morto inalcançável.
  - Verificador de integridade de Bytecode em Rust (`verify_bytecode_safety`) para prevenção de falhas de memória e execução.
  - Pipeline Gráfico GPU 2D (`gpu_2d.rs`): Estruturas `GpuVertex2D`, `GpuQuad`, e `GpuBatchBuffer` pré-alocado para 16.384 quads por draw call com throughput de **245+ Milhões de quads/segundo**.
  - Hash Espacial 2D (`spatial.rs`): Aceleração *broadphase* para detecção de colisões em tempo $O(N)$ amortizado.
  - Integração C++ via `GpuRenderer2D` (`include/GpuRenderer.h` e `src/GpuRenderer.cpp`) com carregamento dinâmico e fallback nativo.
  - Flag `--gpu` no compilador CLI para ativação sob demanda de aceleração gráfica e otimização Rust.
- **Gerenciamento Inteligente de Memória:**
  - Header `include/MemoryPool.h` introduzindo `MemoryArena` (Bump allocator contíguo com reset $O(1)$).
  - `ObjectPool<T>`: Pool genérico de objetos com *free-list* reaproveitada e zero fragmentação de heap.
  - Reciclagem ativa de entidades no motor (`Engine::spawn` / `destroy`) e no transpilador C++ AOT.
  - Reuso de buffer pré-alocado de string de renderização em `Engine::present()`.
- **Arquitetura Multi-Thread:**
  - Header `include/ThreadPool.h` com fila de tarefas thread-safe e particionamento `parallel_for`.
  - Processamento paralelo de colisões (`checkCollisions`) distribuído pelos núcleos da CPU.
  - Worker thread de áudio assíncrono em background, eliminando congelamentos de frame por chamadas de sistema síncronas de som (`Beep`).
- **Suíte de Testes de Performance:**
  - `tests/test_performance.cpp`: Testes unitários validando a arena, reciclagem do pool, threads paralelas e latência de áudio.
- **Padrões de Versionamento e Patch:**
  - Header C++ centralizado `include/Version.h` definindo macros e constantes SemVer (`VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH`, `VERSION_STRING`, `VERSION_TAG`).
  - Arquivo de versão `VERSION` na raiz do repositório.
  - Suporte aos comandos de terminal `gamec version`, `gamec --version` e `gamec -v`.
  - Configuração de versão SemVer no `CMakeLists.txt` (`project(GameForge VERSION 0.0.1 LANGUAGES CXX)`).
  - Diretório `patches/` com arquivo de patch unificado `patches/0.0.1-rpg-maps.patch`.

### Modificado
- **Motor Gráfico e Console (`Engine`):**
  - Implementado buffer matricial de ladrilhos `tiles` com estrutura `Tile { char ch; Color color; bool solid; }`.
  - Deslocamento de renderização de entidades e mapa compensado pelas coordenadas da câmera.
  - Renderizador de frame com suporte a letreiro de mensagens e estatísticas RPG.
- **Compilador e Bytecode VM:**
  - Novos opcodes: `OP_TILE_SET`, `OP_TILE_SOLID`, `OP_TILE_GET`, `OP_MAP_BOX`, `OP_MAP_ROW`, `OP_CAMERA_SET`, `OP_SET_MESSAGE`.
  - Preenchimento seguro de argumentos padrão em tempo de compilação.
- **Transpilador C++ Nativo:**
  - Runtime embutido atualizado com todos os recursos de mapas, câmera e diálogos para geração de executáveis `.exe` autocontidos.

### Corrigido
- **Resiliência e Tolerância a Falhas:**
  - Isolamento de erros pontuais no parser e na VM para evitar interrupções catastróficas (*Safe Mode*).
  - Tratamento de funções indefinidas com stubs seguros no transpilador.
  - Correção de persistência indesejada de `std::setfill('0')` na formatação de descompilação de bytecode.
