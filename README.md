# 🕹️ GameForge — Compilador e Engine de Jogos Arcade em C++

**GameForge** é uma linguagem de domínio específico (**DSL**) e compilador completo desenvolvido em C++20 para criação, prototipagem rápida e distribuição de jogos retrô em terminal.

O projeto oferece um **Dual-Target Execution Pipeline**: você pode tanto rodar o jogo instantaneamente através de uma **Máquina Virtual de Bytecode** com pilha protegida, quanto compilar diretamente para um **executável nativo `.exe` independente** via transpilador C++ integrado.

---

## 🚀 Sumário

1. [Arquitetura Geral](#-arquitetura-geral)
2. [Sintaxe da Linguagem (.game)](#-sintaxe-da-linguagem-game)
3. [Funções Nativas e Recursos de Jogo](#-funções-nativas-e-recursos-de-jogo)
4. [CLI e Comandos do Compilador (gamec)](#-cli-e-comandos-do-compilador-gamec)
5. [Tolerância a Falhas e Isolamento de Erros (Anti-Crash)](#-tolerância-a-falhas-e-isolamento-de-erros-anti-crash)
6. [Jogos de Exemplo Incluídos](#-jogos-de-exemplo-incluídos)
7. [Como Compilar o GameForge](#-como-compilar-o-gameforge)

---

## 🏗️ Arquitetura Geral

```
                    Código Fonte (.game)
                             │
                       [ 1. Lexer ]
                             │ (Tokens com Linha/Coluna)
                      [ 2. Parser ]
                             │ (Árvore Sintática AST)
              ┌──────────────┴──────────────┐
              ▼                             ▼
     [ Pipeline Bytecode ]         [ Pipeline Nativo AOT ]
              │                             │
      [ 3A. Compiler ]             [ 3B. Transpiler C++20 ]
              │ (Bytecode Chunks)           │ (Código Fonte Standalone)
       [ 4A. VM + Engine ]          [ 4B. g++ -O2 ]
              │                             │
      Execução Imediata             Executável Nativo (.exe)
```

### Componentes Principais
* **`Lexer`**: Analisador léxico que reconhece palavras-chave, literais numéricos/strings, operadores lógicos/aritméticos e símbolos de pontuação com localização de linha e coluna.
* **`Parser`**: Analisador sintático descendente recursivo com sincronização e recuperação de erros sintáticos.
* **`AST`**: Representação em árvore de nós fortemente tipados (`Program`, `EntityDecl`, `CollisionHandlerDecl`, etc.).
* **`Compiler`**: Compilador de Bytecode baseado em máquina de pilha com alocação de escopos locais, resolução de saltos (*backpatching*) e tabela de constantes.
* **`VM (Virtual Machine)`**: Interpretador de pilha de alta velocidade com gerenciamento de entidades, despacho de colisões e isolamento de falhas por frame.
* **`Engine`**: Motor gráfico para console com *double-buffering*, sequências de escape ANSI com cores, sincronização de taxa de quadros (FPS), som de hardware (`Beep`) e teclado assíncrono.
* **`Transpiler`**: Gerador de código C++20 autossuficiente que embute o runtime do jogo (`GameRuntime`) e compila para `.exe` com zero dependências externas.

---

## 📜 Sintaxe da Linguagem (.game)

### 1. Configuração do Jogo (`game`)
Define as dimensões do terminal, taxa de atualização por segundo e título:
```game
game "Meu Jogo Arcade" {
    width: 60,
    height: 20,
    fps: 30
}
```

### 2. Variáveis Globais e Locais (`var`)
Tipagem dinâmica inteligente que suporta números, strings, booleanos e referências a entidades:
```game
var score = 0;
var vidas = 3;
var nome = "Jogador 1";
var jogador = 0;
```

### 3. Declaração de Entidades (`entity`)
Entidades possuem campos padrão (`x`, `y`, `vx`, `vy`, `symbol`, `color`) e campos customizados arbitrários:
```game
entity Nave {
    var x = 30;
    var y = 18;
    var symbol = "A";
    var color = "cyan";
    var escudo = 100;
}
```
> **Cores suportadas:** `"black"`, `"red"`, `"green"`, `"yellow"`, `"blue"`, `"magenta"`, `"cyan"`, `"white"`.

### 4. Ciclo de Vida do Jogo

#### Bloco `init`
Executado uma única vez ao iniciar o jogo. Ideal para instanciar entidades:
```game
init {
    jogador = spawn Nave;
    jogador.x = 25;
    jogador.y = 15;
}
```

#### Bloco `update`
Executado a cada frame antes da renderização:
```game
update {
    if (key("LEFT") && jogador.x > 2) {
        jogador.x = jogador.x - 1;
    }
    if (key("RIGHT") && jogador.x < 58) {
        jogador.x = jogador.x + 1;
    }
}
```

#### Bloco `render` (Opcional)
Customização extra de desenho que executa por cima do grid de entidades:
```game
render {
    // Exibe textos ou elementos decorativos customizados
}
```

### 5. Tratador de Colisões (`on collision`)
O motor detecta automaticamente o contato entre entidades de tipos específicos e dispara o evento:
```game
on collision(Projetil, Inimigo) as (proj, inim) {
    destroy proj;
    destroy inim;
    score = score + 100;
    beep(800, 20);
}
```

### 6. Estruturas de Controle
Suporta condicionais e laços de repetição:
```game
if (score > 1000) {
    vidas = vidas + 1;
} else {
    // ...
}

var i = 0;
while (i < 10) {
    var e = spawn Inimigo;
    e.x = i * 5;
    i = i + 1;
}
```

---

## 🛠️ Funções Nativas e Recursos de Jogo

| Função / Primitiva | Descrição | Exemplo |
| :--- | :--- | :--- |
| `spawn <Tipo>` | Cria e ativa uma nova entidade no cenário | `var b = spawn Bola;` |
| `destroy <entidade>` | Desativa e remove uma entidade do cenário | `destroy projetil;` |
| `key(nome)` | Retorna `true` se a tecla estiver pressionada | `if (key("SPACE")) { ... }` |
| `key_down(nome)` | Alias para estado contínuo da tecla | `key_down("UP")` |
| `key_pressed(nome)` | Disparo único por clique da tecla | `key_pressed("ENTER")` |
| `beep(freq, durMs)` | Toca um som retrô no alto-falante interno | `beep(440, 50);` |
| `random(min, max)` | Gera um número inteiro pseudoaleatório | `var r = random(2, 50);` |
| `count("Tipo")` | Retorna o total de entidades ativas desse tipo | `if (count("Alien") == 0) { ... }` |
| `tile(x, y, ch, col, solid)` | Define um ladrilho no mapa com colisão opcional | `tile(10, 5, ".", "white", 0);` |
| `tile_solid(x, y)` | Retorna se o ladrilho na posição é uma parede sólida | `if (!tile_solid(x, y - 1)) { ... }` |
| `tile_at(x, y)` | Retorna o caractere do ladrilho na posição dada | `if (tile_at(x, y) == "+") { ... }` |
| `map_box(x, y, w, h, ch, col, s)` | Constrói salas ou contornos com paredes sólidas | `map_box(2, 2, 14, 8, "#", "blue", 1);` |
| `map_row(x, y, str, col, solid)` | Imprime uma linha inteira de ladrilhos no mapa | `map_row(5, 10, "###...###", "blue", 1);` |
| `camera(cx, cy)` | Move a câmera para seguir o herói pelo mundo | `camera(hero.x - 30, hero.y - 10);` |
| `msg(texto, col)` | Exibe um banner de diálogo/notificação RPG na tela | `msg("Porta destrancada!", "green");` |

> **Teclas suportadas:** `"UP"`, `"DOWN"`, `"LEFT"`, `"RIGHT"`, `"SPACE"`, `"ENTER"`, `"ESC"`, `"W"`, `"A"`, `"S"`, `"D"` e letras `"A"` a `"Z"`.

---

## 🗺️ Criação de Mapas RPG e Masmorras

O GameForge inclui suporte nativo a jogos de aventura, masmorras e RPGs com grid 2D:

### 1. Construção de Salas e Portas
Você pode construir cômodos inteiros com `map_box` e abrir passagens ou portas com `tile`:
```game
// Constrói sala de 14x8 com paredes sólidas
map_box(2, 2, 14, 8, "#", "blue", 1);

// Abre uma passagem não sólida (caminhável)
tile(15, 5, ".", "white", 0);

// Coloca uma porta trancada sólida
tile(7, 12, "+", "yellow", 1);
```

### 2. Movimentação com Paredes Sólidas
Utilize `!tile_solid(x, y)` para impedir que o herói atravesse paredes ou obstáculos:
```game
if (key_pressed("UP")) {
    if (!tile_solid(hero.x, hero.y - 1)) {
        hero.y = hero.y - 1;
    } else {
        msg("Caminho bloqueado por parede de pedra.", "white");
    }
}
```

### 3. Câmera Dinâmica que Segue o Herói
Com mapas maiores que a tela do console, a câmera acompanha o jogador suavemente:
```game
update {
    camera(hero.x - 30, hero.y - 10);
}
```

### 4. Caixas de Diálogo e HUD RPG Automático
- `msg("Texto...", "cor")`: Exibe um letreiro elegante de mensagem/diálogo no rodapé.
- Variáveis globais como `var hp = 100;` e `var gold = 0;` são reconhecidas pelo motor gráfico e exibidas automaticamente no painel de status inferior em cores temáticas (Vermelho para HP, Amarelo para Ouro).

---

## 💻 CLI e Comandos do Compilador (`gamec`)

O compilador aceita comandos diretamente no terminal:

### 1. Execução Imediata na VM
Interpreta o código em bytecode na hora (ideal durante o desenvolvimento):
```powershell
.\bin\gamec.exe run games/space_invaders.game
```

### 2. Compilação para Executável Nativo (.exe)
Gera um binário C++ otimizado (`-O2`), autônomo, sem dependências:
```powershell
.\bin\gamec.exe build games/snake.game -o bin/snake.exe
```

### 3. Transpilação para Código C++
Exporta o código fonte C++ puro:
```powershell
.\bin\gamec.exe transpile games/pong.game -o pong.cpp
```

### 4. Despejo da Árvore Sintática (AST)
Exibe a estrutura hierárquica analisada:
```powershell
.\bin\gamec.exe dump-ast games/pong.game
```

### 5. Despejo do Bytecode (Disassembly)
Exibe as instruções em bytecode geradas para a máquina de pilha:
```powershell
.\bin\gamec.exe dump-bc games/snake.game
```

---

## 🛡️ Tolerância a Falhas e Isolamento de Erros (Anti-Crash)

O GameForge foi projetado para **nunca crashar o jogo** devido a pequenos erros de script em tempo de execução:

### Como funciona o Isolamento de Falhas:
1. **Modo Seguro Automático (`--safe-mode` / `-f`)**:
   * **Entidades Nulas/Destruídas**: Ler ou escrever em entidades mortas (ex: `e.x = 10` quando `e` é `null`) não causa *Access Violation* ou falha de segmentação.
   * **Propriedades Dinâmicas Inexistentes**: Acessar um atributo não declarado retorna `0.0` com segurança.
   * **Funções Não Declaradas**: Se o jogo chamar uma função inexistente, os argumentos são limpos e o jogo continua rodando.
   * **Sanitização de Pilha por Frame**: Se ocorrer um erro durante uma colisão ou atualização, a pilha é recuperada para o estado do início do frame, evitando vazamentos e travamentos nos frames seguintes.
   * **Contador de Erros Isolados**: A engine contabiliza falhas suprimidas e exibe de forma sutil no rodapé: `[Safe: X err isolados]`.
2. **Ponteiro Dummy no Nativo**:
   * No código C++ gerado, referências nulas apontam para uma instância sentinela segura (`getSafeDummy()`), eliminando crashes de ponteiro nulo no `.exe`.
3. **Opções da Linha de Comando**:
   * `--safe-mode` ou `-f` *(Padrão)*: Tolerância ativa e isolamento total.
   * `--ignore-errors`: Permite rodar o código mesmo se o parser encontrar erros sintáticos pontuais recuperáveis.
   * `--strict`: Modo estrito para depuração profunda (interrompe na primeira falha léxica/sintática/runtime).

---

## 🎮 Jogos de Exemplo Incluídos

Na pasta `games/` você encontra implementações completas:

1. **[pong.game](file:///d:/Projetos/Compilador%20game/games/pong.game)**:
   * Jogo Pong arcade com raquete do jogador e raquete da IA.
   * Rebatimento nas bordas superior e inferior.
   * Efeitos sonoros (`beep`) diferentes para rebatimento e pontuação.
2. **[snake.game](file:///d:/Projetos/Compilador%20game/games/snake.game)**:
   * Jogo da cobrinha clássica com controle em 4 direções.
   * Geração randômica de comida e pontuação dinâmica.
   * Teletransporte (*wrap-around*) ao colidir com as bordas da tela.
3. **[space_invaders.game](file:///d:/Projetos/Compilador%20game/games/space_invaders.game)**:
   * Canhão móvel do jogador com controle horizontal e disparo de laser com a barra de espaço.
   * Esquadrão completo de alienígenas em formação matricial.
   * Céu estrelado gerado proceduralmente no fundo.
   * Detecção de vitória ao eliminar todos os alienígenas (`count("Alien") == 0`).
4. **[rpg_dungeon.game](file:///d:/Projetos/Compilador%20game/games/rpg_dungeon.game)**:
   * Masmorra RPG completa com múltiplas salas, corredores e paredes sólidas (`map_box`, `tile`).
   * Portas trancadas que exigem coletar a chave de ferro `k` para abrir.
   * Fonte sagrada que regenera pontos de vida (`hp`).
   * Câmera dinâmica de rolagem centralizada no herói (`camera`).
   * Monstros (Orc, Esqueleto) com pontos de vida individuais e combate.
   * Baú com tesouro em moedas de ouro (`gold`) e poções de cura.
   * Sistema de diálogo narrativo e notificações temáticas (`msg`).

---

## 🔧 Como Compilar o GameForge

Caso altere o código-fonte em `src/` ou `include/`, você pode recompilar o compilador de três formas:

### Opção 1: Via Script PowerShell (Recomendado)
```powershell
.\build.ps1
```

### Opção 2: Via CMake
```powershell
cmake -B build
cmake --build build --config Release
```

### Opção 3: Via g++ direto
```powershell
g++ -std=c++20 -O2 -Iinclude src/*.cpp -o bin/gamec.exe
```
