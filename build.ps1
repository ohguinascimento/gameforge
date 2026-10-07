$compiler = "C:\Users\user\w64devkit\bin\g++.exe"
if (-not (Test-Path $compiler)) {
    $compiler = "g++"
}

if (-not (Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" -Force | Out-Null
}

Write-Host "Compilando GameForge Game Compiler..." -ForegroundColor Cyan

$sources = @(
    "src/AST.cpp",
    "src/Bytecode.cpp",
    "src/Compiler.cpp",
    "src/Engine.cpp",
    "src/GpuRenderer.cpp",
    "src/Lexer.cpp",
    "src/Parser.cpp",
    "src/VM.cpp",
    "src/Transpiler.cpp",
    "src/main.cpp"
)

& $compiler -std=c++20 -O2 -Iinclude $sources -o bin/gamec.exe

if ($LASTEXITCODE -eq 0) {
    Write-Host "[Sucesso] GameForge compilado com sucesso em bin/gamec.exe!" -ForegroundColor Green
} else {
    Write-Host "[Erro] Falha ao compilar GameForge." -ForegroundColor Red
    exit 1
}
