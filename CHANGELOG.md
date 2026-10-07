# Changelog

Todas as mudanças notáveis deste projeto serão documentadas neste arquivo.

O formato é baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/),
e este projeto adere ao [Semantic Versioning (SemVer 2.0.0)](https://semver.org/lang/pt-BR/).

---

## [0.0.1] - 2026-10-07

### Adicionado
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
