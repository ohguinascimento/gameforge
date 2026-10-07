# ==============================================================================
# GameForge: Script de Compilação Automatizada de Alta Performance
# Flags de Otimização Extrema: -std=c++20 -O3 -march=native
# ==============================================================================

param(
    [switch]$RebuildCompiler,
    [string]$Game = "games/space_invaders_gpu.game",
    [string]$Output = "bin/space_invaders.exe"
)

$compiler = "C:\Users\user\w64devkit\bin\g++.exe"
if (-not (Test-Path $compiler)) {
    $compiler = "g++"
}

if (-not (Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" -Force | Out-Null
}

# 1. Compila o compilador GameForge caso não exista ou se solicitado
if ($RebuildCompiler -or (-not (Test-Path "bin/gamec.exe"))) {
    Write-Host "[1/2] Compilando compilador GameForge com -O3 e -march=native..." -ForegroundColor Cyan

    $compilerSources = @(
        "src/AST.cpp",
        "src/Bytecode.cpp",
        "src/Compiler.cpp",
        "src/Engine.cpp",
        "src/GpuRenderer.cpp",
        "src/Lexer.cpp",
        "src/Parser.cpp",
        "src/SemanticAnalyzer.cpp",
        "src/VM.cpp",
        "src/Transpiler.cpp",
        "src/main.cpp"
    )

    & $compiler -std=c++20 -O3 -march=native -Iinclude $compilerSources -o bin/gamec.exe
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[Erro] Falha ao compilar GameForge Compiler." -ForegroundColor Red
        exit 1
    }
    Write-Host "      -> Compilador GameForge gerado com sucesso em bin/gamec.exe!" -ForegroundColor Green
}

# 2. Compila o jogo arcade selecionado para binário nativo acelerado por GPU
Write-Host "[2/2] Compilando jogo '$Game' para binario nativo acelerado por GPU..." -ForegroundColor Cyan
& .\bin\gamec.exe build $Game -o $Output

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Green
    Write-Host " [SUCESSO] Jogo compilado de ponta a ponta: $Output" -ForegroundColor Green
    Write-Host " Backend: OpenGL 3.3 Core Profile + Shaders GLSL + V-Sync" -ForegroundColor Green
    Write-Host " Pressione F5 no VS Code ou execute: .\$Output" -ForegroundColor Green
    Write-Host "============================================================" -ForegroundColor Green
} else {
    Write-Host "[Erro] Falha ao compilar o jogo arcade." -ForegroundColor Red
    exit 1
}
