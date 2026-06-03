#include "common.hpp"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <numeric>

namespace {

void fail(const char* message) {
    LOGE("%s", message);
    std::exit(EXIT_FAILURE);
}

void checkGl(const char* where) {
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOGE("GL error at %s: 0x%04x", where, err);
        std::exit(EXIT_FAILURE);
    }
}

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLchar log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        LOGE("Shader compile failed: %s", log);
        std::exit(EXIT_FAILURE);
    }
    return shader;
}

GLuint createProgram() {
    static const char* kVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)";

    static const char* kFragmentShader = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
layout(location = 0) out vec3 outColor;
uniform sampler2D uYTex;
uniform sampler2D uUvTex;
vec3 nv12ToRgb(float y, vec2 uv) {
    float u = uv.r - 0.5;
    float v = uv.g - 0.5;
    return vec3(
        y + 1.402 * v,
        y - 0.344136 * u - 0.714136 * v,
        y + 1.772 * u
    );
}
void main() {
    float y = texture(uYTex, vTexCoord).r;
    vec2 uv = texture(uUvTex, vTexCoord).rg;
    outColor = clamp(nv12ToRgb(y, uv), 0.0, 1.0);
}
)";

    const GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLchar log[1024] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        LOGE("Program link failed: %s", log);
        std::exit(EXIT_FAILURE);
    }
    return program;
}

struct EglContext {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    EglContext() {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            fail("eglGetDisplay failed");
        }
        if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE) {
            fail("eglInitialize failed");
        }

        const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_NONE
        };
        EGLConfig config = nullptr;
        EGLint numConfigs = 0;
        if (eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs < 1) {
            fail("eglChooseConfig failed");
        }

        const EGLint surfaceAttribs[] = {
            EGL_WIDTH, kDstWidth,
            EGL_HEIGHT, kDstHeight,
            EGL_NONE
        };
        surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
        if (surface == EGL_NO_SURFACE) {
            fail("eglCreatePbufferSurface failed");
        }

        const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
        };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        if (context == EGL_NO_CONTEXT) {
            fail("eglCreateContext failed");
        }
        if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) {
            fail("eglMakeCurrent failed");
        }
    }

    ~EglContext() {
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT) {
                eglDestroyContext(display, context);
            }
            if (surface != EGL_NO_SURFACE) {
                eglDestroySurface(display, surface);
            }
            eglTerminate(display);
        }
    }
};

GLuint createTexture(GLenum internalFormat, int width, int height, GLenum format) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
    checkGl("createTexture");
    return texture;
}

}  // namespace

int main(int argc, char** argv) {
    const char* path = inputPath(argc, argv);
    const std::vector<unsigned char> nv12 = readNv12File(path);
    const unsigned char* yPlane = nv12.data();
    const unsigned char* uvPlane = nv12.data() + kSrcWidth * kSrcHeight;

    EglContext egl;
    const GLuint program = createProgram();

    const GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
    };
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          reinterpret_cast<void*>(2 * sizeof(GLfloat)));

    GLuint yTexture = createTexture(GL_R8, kSrcWidth, kSrcHeight, GL_RED);
    GLuint uvTexture = createTexture(GL_RG8, kSrcWidth / 2, kSrcHeight / 2, GL_RG);
    GLuint outTexture = createTexture(GL_RGB8, kDstWidth, kDstHeight, GL_RGB);

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fail("Framebuffer is not complete");
    }

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uYTex"), 0);
    glUniform1i(glGetUniformLocation(program, "uUvTex"), 1);
    glViewport(0, 0, kDstWidth, kDstHeight);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSrcWidth, kSrcHeight, GL_RED,
                    GL_UNSIGNED_BYTE, yPlane);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uvTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSrcWidth / 2, kSrcHeight / 2, GL_RG,
                    GL_UNSIGNED_BYTE, uvPlane);
    glFinish();
    checkGl("initial texture upload");

    std::vector<double> costsMs;
    costsMs.reserve(kIterations);

    for (int i = 0; i < kIterations; ++i) {
        const auto begin = std::chrono::steady_clock::now();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glFinish();

        const auto end = std::chrono::steady_clock::now();
        checkGl("benchmark iteration");
        costsMs.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
    }

    const double total = std::accumulate(costsMs.begin(), costsMs.end(), 0.0);
    const double avg = total / static_cast<double>(costsMs.size());
    LOGI("OpenGL ES3 NV12 %dx%d -> RGB %dx%d", kSrcWidth, kSrcHeight, kDstWidth, kDstHeight);
    LOGI("Iterations: %d", kIterations);
    LOGI("Average GPU draw time, no upload/readback: %.3f ms", avg);
    LOGI("Output RGB texture bytes: %zu", kRgbSize);

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &outTexture);
    glDeleteTextures(1, &uvTexture);
    glDeleteTextures(1, &yTexture);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    return 0;
}
