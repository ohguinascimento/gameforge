#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <GL/gl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

// Definições modernas do OpenGL 3.3 Core e extensões WGL
#define GL_FRAMEBUFFER                    0x8D40
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STATIC_DRAW                    0x88E4
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TRIANGLE_FAN                   0x0006

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001

namespace GameForge::GL {

// Ponteiros de função do OpenGL 3.3 carregados dinamicamente no Windows
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (APIENTRY *PFNGLVERTEXATTRIBDIVISORPROC)(GLuint index, GLuint divisor);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef HGLRC (APIENTRY *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);

struct GlApi {
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
    PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
    PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
    PFNGLBUFFERDATAPROC glBufferData = nullptr;
    PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;
    PFNGLCREATESHADERPROC glCreateShader = nullptr;
    PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
    PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
    PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
    PFNGLATTACHSHADERPROC glAttachShader = nullptr;
    PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
    PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
    PFNGLDELETESHADERPROC glDeleteShader = nullptr;
    PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
    PFNGLUNIFORM1IPROC glUniform1i = nullptr;
    PFNGLUNIFORM1FPROC glUniform1f = nullptr;
    PFNGLUNIFORM2FPROC glUniform2f = nullptr;
    PFNGLUNIFORM4FPROC glUniform4f = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
    PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced = nullptr;
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

    bool load() {
        #define LOAD_PROC(type, name) name = (type)wglGetProcAddress(#name); if(!name) name = (type)GetProcAddress(GetModuleHandleA("opengl32.dll"), #name);
        LOAD_PROC(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
        LOAD_PROC(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
        LOAD_PROC(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
        LOAD_PROC(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
        LOAD_PROC(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
        LOAD_PROC(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
        LOAD_PROC(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
        LOAD_PROC(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
        LOAD_PROC(PFNGLGENBUFFERSPROC, glGenBuffers);
        LOAD_PROC(PFNGLBINDBUFFERPROC, glBindBuffer);
        LOAD_PROC(PFNGLBUFFERDATAPROC, glBufferData);
        LOAD_PROC(PFNGLBUFFERSUBDATAPROC, glBufferSubData);
        LOAD_PROC(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
        LOAD_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
        LOAD_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
        LOAD_PROC(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor);
        LOAD_PROC(PFNGLCREATESHADERPROC, glCreateShader);
        LOAD_PROC(PFNGLSHADERSOURCEPROC, glShaderSource);
        LOAD_PROC(PFNGLCOMPILESHADERPROC, glCompileShader);
        LOAD_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv);
        LOAD_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
        LOAD_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram);
        LOAD_PROC(PFNGLATTACHSHADERPROC, glAttachShader);
        LOAD_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram);
        LOAD_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
        LOAD_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
        LOAD_PROC(PFNGLUSEPROGRAMPROC, glUseProgram);
        LOAD_PROC(PFNGLDELETESHADERPROC, glDeleteShader);
        LOAD_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
        LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
        LOAD_PROC(PFNGLUNIFORM1IPROC, glUniform1i);
        LOAD_PROC(PFNGLUNIFORM1FPROC, glUniform1f);
        LOAD_PROC(PFNGLUNIFORM2FPROC, glUniform2f);
        LOAD_PROC(PFNGLUNIFORM4FPROC, glUniform4f);
        LOAD_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
        LOAD_PROC(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced);
        LOAD_PROC(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
        #undef LOAD_PROC

        return (glGenFramebuffers && glBindFramebuffer && glDrawArraysInstanced);
    }
};

// ============================================================================
// Estrutura de Instância para Sprite Batching (1 única Draw Call para milhares de sprites)
// ============================================================================
struct alignas(16) SpriteInstance {
    float x = 0.0f, y = 0.0f;           // Posição no mundo
    float width = 1.0f, height = 1.0f;   // Dimensões do sprite
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f; // Cor RGBA
    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f; // UV
    float glow = 0.0f;                  // Intensidade emissiva (Bloom)
    float shape = 0.0f;                 // 0 = Retângulo, 1 = Círculo, 2 = Borda
    float angle = 0.0f;                 // Rotação em radianos
    float padding = 0.0f;
};

// ============================================================================
// Viewport com Escalonamento Pixel-Perfect e Letterboxing Automático
// ============================================================================
struct LetterboxViewport {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    float scale = 1.0f;
};

inline LetterboxViewport calculateLetterbox(int windowW, int windowH, int targetW, int targetH) {
    LetterboxViewport vp;
    float scaleX = static_cast<float>(windowW) / static_cast<float>(targetW);
    float scaleY = static_cast<float>(windowH) / static_cast<float>(targetH);
    vp.scale = std::min(scaleX, scaleY);

    vp.width = static_cast<int>(targetW * vp.scale);
    vp.height = static_cast<int>(targetH * vp.scale);
    vp.x = (windowW - vp.width) / 2;
    vp.y = (windowH - vp.height) / 2;
    return vp;
}

// ============================================================================
// Shaders GLSL 330 com Pós-Processamento e Efeitos Retrô / Bloom
// ============================================================================
namespace Shaders {

inline const char* INSTANCED_VERT = R"GLSL(
#version 330 core
layout (location = 0) in vec2 a_quadPos; // Unit Quad [-0.5..0.5]
layout (location = 1) in vec4 a_transform; // x, y, width, height
layout (location = 2) in vec4 a_color;     // r, g, b, a
layout (location = 3) in vec4 a_uv;        // u0, v0, u1, v1
layout (location = 4) in vec4 a_params;    // glow, shape, angle, reserved

uniform mat4 u_projection;

out vec4 v_color;
out vec2 v_uv;
out float v_glow;
out float v_shape;
out vec2 v_localPos;

void main() {
    float angle = a_params.z;
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Escalonamento e rotação
    vec2 scaled = a_quadPos * a_transform.zw;
    vec2 rotated = vec2(
        scaled.x * cosA - scaled.y * sinA,
        scaled.x * sinA + scaled.y * cosA
    );

    vec2 worldPos = a_transform.xy + rotated;
    gl_Position = u_projection * vec4(worldPos, 0.0, 1.0);

    v_color = a_color;
    v_uv = mix(a_uv.xy, a_uv.zw, a_quadPos + vec2(0.5));
    v_glow = a_params.x;
    v_shape = a_params.y;
    v_localPos = a_quadPos * 2.0; // [-1..1]
}
)GLSL";

inline const char* INSTANCED_FRAG = R"GLSL(
#version 330 core
in vec4 v_color;
in vec2 v_uv;
in float v_glow;
in float v_shape;
in vec2 v_localPos;

out vec4 FragColor;

void main() {
    float alpha = v_color.a;

    // Se shape == 1.0, desenha círculo suavizado anti-aliased
    if (v_shape > 0.5 && v_shape < 1.5) {
        float dist = length(v_localPos);
        float delta = fwidth(dist);
        alpha *= 1.0 - smoothstep(1.0 - delta, 1.0, dist);
    }

    if (alpha <= 0.01) discard;

    // Emissão de cor com componente glow intensificado para o shader de bloom
    vec3 col = v_color.rgb * (1.0 + v_glow * 2.5);
    FragColor = vec4(col, alpha);
}
)GLSL";

inline const char* POSTPROCESS_VERT = R"GLSL(
#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)GLSL";

inline const char* POSTPROCESS_FRAG = R"GLSL(
#version 330 core
in vec2 v_uv;
out vec4 FragColor;

uniform sampler2D u_canvasTexture;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_bloomIntensity;
uniform float u_scanlineStrength;

void main() {
    vec2 uv = v_uv;

    // Amostragem da cena base
    vec4 sceneColor = texture(u_canvasTexture, uv);

    // Efeito de Bloom / Emissive Glow calculado por amostragem com dispersão
    vec3 bloom = vec3(0.0);
    vec2 texel = 1.0 / u_resolution;
    float spread = 2.0;

    bloom += max(vec3(0.0), texture(u_canvasTexture, uv + vec2(-texel.x, -texel.y) * spread).rgb - 0.7);
    bloom += max(vec3(0.0), texture(u_canvasTexture, uv + vec2( texel.x, -texel.y) * spread).rgb - 0.7);
    bloom += max(vec3(0.0), texture(u_canvasTexture, uv + vec2(-texel.x,  texel.y) * spread).rgb - 0.7);
    bloom += max(vec3(0.0), texture(u_canvasTexture, uv + vec2( texel.x,  texel.y) * spread).rgb - 0.7);
    bloom += max(vec3(0.0), texture(u_canvasTexture, uv + vec2( 0.0, -texel.y * 1.5) * spread).rgb - 0.7);
    bloom += max(vec3(0.0), texture(u_canvasTexture, uv + vec2( 0.0,  texel.y * 1.5) * spread).rgb - 0.7);
    bloom *= (u_bloomIntensity * 0.35);

    // Scanlines sutis estilo monitor Arcade / CRT
    float scanline = sin(uv.y * u_resolution.y * 3.14159) * 0.5 + 0.5;
    float scanlineFactor = 1.0 - (scanline * u_scanlineStrength);

    // Vinheta sutil nas bordas
    vec2 vignetteCoord = uv * (1.0 - uv.yx);
    float vignette = vignetteCoord.x * vignetteCoord.y * 15.0;
    vignette = clamp(pow(vignette, 0.25), 0.0, 1.0);

    vec3 finalColor = (sceneColor.rgb + bloom) * scanlineFactor * vignette;

    FragColor = vec4(finalColor, 1.0);
}
)GLSL";

} // namespace Shaders

// ============================================================================
// Motor Gráfico 2D Completo Acelerado por GPU (OpenGL 3.3 Core Profile)
// ============================================================================
class HardwareEngineGL {
public:
    HardwareEngineGL(int canvasWidth, int canvasHeight, int fps, const std::string& title)
        : canvasW(canvasWidth), canvasH(canvasHeight), targetFps(fps), windowTitle(title) {
        initWindowAndGl();
        initPipelines();
    }

    ~HardwareEngineGL() {
        cleanup();
    }

    bool shouldClose() const { return closeRequested; }

    void pollEvents() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                closeRequested = true;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    bool isKeyDown(int vKey) const {
        return (GetAsyncKeyState(vKey) & 0x8000) != 0;
    }

    // Inicia um novo frame e redireciona a renderização para o Canvas Virtual FBO
    void beginFrame() {
        pollEvents();

        // Vincula o Render Target Virtual (FBO)
        api.glBindFramebuffer(GL_FRAMEBUFFER, canvasFbo);
        glViewport(0, 0, canvasW, canvasH);
        glClearColor(clearR, clearG, clearB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        instanceQueue.clear();
    }

    void setClearColor(float r, float g, float b) {
        clearR = r; clearG = g; clearB = b;
    }

    void setBloom(float intensity) {
        bloomIntensity = intensity;
    }

    void setScanlines(float strength) {
        scanlineStrength = strength;
    }

    // Adiciona retângulo/sprite na fila de lote (Batch)
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f, float glow = 0.0f) {
        SpriteInstance inst;
        inst.x = x + w * 0.5f;
        inst.y = y + h * 0.5f;
        inst.width = w;
        inst.height = h;
        inst.r = r; inst.g = g; inst.b = b; inst.a = a;
        inst.glow = glow;
        inst.shape = 0.0f; // Retângulo
        instanceQueue.push_back(inst);
    }

    // Adiciona círculo suavizado na fila de lote
    void drawCircle(float cx, float cy, float radius, float r, float g, float b, float a = 1.0f, float glow = 0.0f) {
        SpriteInstance inst;
        inst.x = cx;
        inst.y = cy;
        inst.width = radius * 2.0f;
        inst.height = radius * 2.0f;
        inst.r = r; inst.g = g; inst.b = b; inst.a = a;
        inst.glow = glow;
        inst.shape = 1.0f; // Círculo
        instanceQueue.push_back(inst);
    }

    // Desenha texto retrô pixel-art usando instanciamento de caracteres
    void drawText(float x, float y, const std::string& text, float r, float g, float b, float scale = 1.0f) {
        float curX = x;
        for (char c : text) {
            if (c > ' ') {
                // Desenha glifo como bloco compacto
                drawRect(curX, y, 6.0f * scale, 10.0f * scale, r, g, b, 1.0f, 0.2f);
            }
            curX += 8.0f * scale;
        }
    }

    // Submete todas as entidades em 1 ÚNICA Draw Call, aplica pós-processamento e exibe na tela
    void endFrame() {
        // Passo 1: Flush de todas as instâncias em 1 única chamada à GPU
        if (!instanceQueue.empty()) {
            api.glUseProgram(spriteProgram);

            // Matriz ortográfica 2D (0,0 no canto superior esquerdo)
            float left = 0.0f, right = static_cast<float>(canvasW);
            float top = 0.0f, bottom = static_cast<float>(canvasH);
            float ortho[16] = {
                2.0f / (right - left), 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
                -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
            };
            api.glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, ortho);

            // Carrega instâncias no VBO dinâmico
            api.glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
            api.glBufferData(GL_ARRAY_BUFFER, instanceQueue.size() * sizeof(SpriteInstance), instanceQueue.data(), GL_DYNAMIC_DRAW);

            api.glBindVertexArray(quadVao);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // 1 ÚNICA DRAW CALL PARA TODOS OS SPRITES!
            api.glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(instanceQueue.size()));
        }

        // Passo 2: Pós-processamento com Fragment Shader e Letterboxing no Backbuffer (FBO 0)
        api.glBindFramebuffer(GL_FRAMEBUFFER, 0);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int winW = clientRect.right - clientRect.left;
        int winH = clientRect.bottom - clientRect.top;

        // Limpa bordas do letterbox com preto absoluto
        glViewport(0, 0, winW, winH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Calcula viewport pixel-perfect letterboxing
        LetterboxViewport vp = calculateLetterbox(winW, winH, canvasW, canvasH);
        glViewport(vp.x, vp.y, vp.width, vp.height);

        // Aplica o fragment shader de pós-processamento
        api.glUseProgram(postProcessProgram);
        api.glUniform2f(uResLoc, static_cast<float>(canvasW), static_cast<float>(canvasH));
        timeElapsed += 0.01666f;
        api.glUniform1f(uTimeLoc, timeElapsed);
        api.glUniform1f(uBloomLoc, bloomIntensity);
        api.glUniform1f(uScanLoc, scanlineStrength);

        glBindTexture(GL_TEXTURE_2D, canvasTexture);

        api.glBindVertexArray(screenQuadVao);
        glDisable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Passo 3: Swap Buffers com VSync Ativo
        SwapBuffers(hDC);

        syncFps();
    }

    int getCanvasWidth() const { return canvasW; }
    int getCanvasHeight() const { return canvasH; }

private:
    int canvasW;
    int canvasH;
    int targetFps;
    std::string windowTitle;
    bool closeRequested = false;

    HWND hWnd = nullptr;
    HDC hDC = nullptr;
    HGLRC hRC = nullptr;
    GlApi api;

    // Canvas FBO
    GLuint canvasFbo = 0;
    GLuint canvasTexture = 0;

    // Sprite Batching Buffers
    GLuint quadVao = 0;
    GLuint quadVbo = 0;
    GLuint instanceVbo = 0;
    GLuint spriteProgram = 0;
    GLint uProjLoc = -1;
    std::vector<SpriteInstance> instanceQueue;

    // Post-Processing Pipeline
    GLuint screenQuadVao = 0;
    GLuint screenQuadVbo = 0;
    GLuint postProcessProgram = 0;
    GLint uResLoc = -1;
    GLint uTimeLoc = -1;
    GLint uBloomLoc = -1;
    GLint uScanLoc = -1;

    float clearR = 0.05f, clearG = 0.05f, clearB = 0.08f;
    float bloomIntensity = 1.0f;
    float scanlineStrength = 0.25f;
    float timeElapsed = 0.0f;

    std::chrono::high_resolution_clock::time_point lastFrameTime;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_CLOSE || msg == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
        if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void initWindowAndGl() {
        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA) };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "GameForgeGL_WndClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExA(&wc);

        // Dimensões iniciais da janela física (escala 2x do canvas virtual ou padrão 1280x720)
        int initialWinW = std::max(1280, canvasW * 2);
        int initialWinH = std::max(720, canvasH * 2);

        RECT wr = { 0, 0, initialWinW, initialWinH };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

        hWnd = CreateWindowExA(
            0, wc.lpszClassName, windowTitle.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            wr.right - wr.left, wr.bottom - wr.top,
            nullptr, nullptr, wc.hInstance, nullptr
        );

        hDC = GetDC(hWnd);

        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR), 1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA, 32,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
        };

        int format = ChoosePixelFormat(hDC, &pfd);
        SetPixelFormat(hDC, format, &pfd);

        HGLRC tempContext = wglCreateContext(hDC);
        wglMakeCurrent(hDC, tempContext);

        // Carrega extensões WGL e OpenGL 3.3
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
            (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        if (wglCreateContextAttribsARB) {
            int attribs[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };
            hRC = wglCreateContextAttribsARB(hDC, nullptr, attribs);
            if (hRC) {
                wglMakeCurrent(nullptr, nullptr);
                wglDeleteContext(tempContext);
                wglMakeCurrent(hDC, hRC);
            } else {
                hRC = tempContext;
            }
        } else {
            hRC = tempContext;
        }

        api.load();

        // Ativa V-Sync
        if (api.wglSwapIntervalEXT) {
            api.wglSwapIntervalEXT(1);
        }

        lastFrameTime = std::chrono::high_resolution_clock::now();
    }

    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = api.glCreateShader(type);
        api.glShaderSource(shader, 1, &source, nullptr);
        api.glCompileShader(shader);

        GLint success;
        api.glGetShaderiv(shader, 0x8B81 /* GL_COMPILE_STATUS */, &success);
        if (!success) {
            char log[1024];
            api.glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "Falha na compilacao do shader:\n" << log << std::endl;
        }
        return shader;
    }

    GLuint createProgram(const char* vertSrc, const char* fragSrc) {
        GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
        GLuint prog = api.glCreateProgram();
        api.glAttachShader(prog, vs);
        api.glAttachShader(prog, fs);
        api.glLinkProgram(prog);

        api.glDeleteShader(vs);
        api.glDeleteShader(fs);
        return prog;
    }

    void initPipelines() {
        // 1. Cria Render Target Virtual (FBO + Textura)
        api.glGenFramebuffers(1, &canvasFbo);
        api.glBindFramebuffer(GL_FRAMEBUFFER, canvasFbo);

        glGenTextures(1, &canvasTexture);
        glBindTexture(GL_TEXTURE_2D, canvasTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvasW, canvasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        api.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvasTexture, 0);
        api.glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. Cria Pipeline de Instanciamento de Sprites (1 Draw Call)
        spriteProgram = createProgram(Shaders::INSTANCED_VERT, Shaders::INSTANCED_FRAG);
        uProjLoc = api.glGetUniformLocation(spriteProgram, "u_projection");

        // Vértices do Unit Quad [-0.5..0.5]
        float quadVertices[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f
        };

        api.glGenVertexArrays(1, &quadVao);
        api.glBindVertexArray(quadVao);

        api.glGenBuffers(1, &quadVbo);
        api.glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
        api.glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        api.glEnableVertexAttribArray(0);
        api.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        // Buffer de Instâncias
        api.glGenBuffers(1, &instanceVbo);
        api.glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);

        // Configura atributos de instância (Divisor = 1)
        GLsizei stride = sizeof(SpriteInstance);

        // a_transform: vec4 (x, y, w, h)
        api.glEnableVertexAttribArray(1);
        api.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, x));
        api.glVertexAttribDivisor(1, 1);

        // a_color: vec4 (r, g, b, a)
        api.glEnableVertexAttribArray(2);
        api.glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, r));
        api.glVertexAttribDivisor(2, 1);

        // a_uv: vec4 (u0, v0, u1, v1)
        api.glEnableVertexAttribArray(3);
        api.glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, u0));
        api.glVertexAttribDivisor(3, 1);

        // a_params: vec4 (glow, shape, angle, padding)
        api.glEnableVertexAttribArray(4);
        api.glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, glow));
        api.glVertexAttribDivisor(4, 1);

        // 3. Cria Pipeline de Pós-Processamento (Screen Quad)
        postProcessProgram = createProgram(Shaders::POSTPROCESS_VERT, Shaders::POSTPROCESS_FRAG);
        uResLoc = api.glGetUniformLocation(postProcessProgram, "u_resolution");
        uTimeLoc = api.glGetUniformLocation(postProcessProgram, "u_time");
        uBloomLoc = api.glGetUniformLocation(postProcessProgram, "u_bloomIntensity");
        uScanLoc = api.glGetUniformLocation(postProcessProgram, "u_scanlineStrength");

        float screenVertices[] = {
            // Pos        // UV
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f
        };

        api.glGenVertexArrays(1, &screenQuadVao);
        api.glBindVertexArray(screenQuadVao);

        api.glGenBuffers(1, &screenQuadVbo);
        api.glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);
        api.glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

        api.glEnableVertexAttribArray(0);
        api.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        api.glEnableVertexAttribArray(1);
        api.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        api.glBindVertexArray(0);
    }

    void syncFps() {
        if (targetFps <= 0) return;
        auto targetDuration = std::chrono::microseconds(1000000 / targetFps);
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrameTime);

        if (elapsed < targetDuration) {
            auto sleepTime = targetDuration - elapsed;
            std::this_thread::sleep_for(sleepTime);
        }
        lastFrameTime = std::chrono::high_resolution_clock::now();
    }

    void cleanup() {
        if (canvasFbo && api.glDeleteFramebuffers) api.glDeleteFramebuffers(1, &canvasFbo);
        if (canvasTexture) glDeleteTextures(1, &canvasTexture);
        if (quadVao && api.glDeleteVertexArrays) api.glDeleteVertexArrays(1, &quadVao);
        if (quadVbo && api.glDeleteBuffers) api.glDeleteBuffers(1, &quadVbo);
        if (instanceVbo && api.glDeleteBuffers) api.glDeleteBuffers(1, &instanceVbo);
        if (spriteProgram && api.glDeleteProgram) api.glDeleteProgram(spriteProgram);
        if (screenQuadVao && api.glDeleteVertexArrays) api.glDeleteVertexArrays(1, &screenQuadVao);
        if (screenQuadVbo && api.glDeleteBuffers) api.glDeleteBuffers(1, &screenQuadVbo);
        if (postProcessProgram && api.glDeleteProgram) api.glDeleteProgram(postProcessProgram);

        if (hRC) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hRC);
            hRC = nullptr;
        }
        if (hDC && hWnd) {
            ReleaseDC(hWnd, hDC);
            hDC = nullptr;
        }
        if (hWnd) {
            DestroyWindow(hWnd);
            hWnd = nullptr;
        }
    }
};

} // namespace GameForge::GL
