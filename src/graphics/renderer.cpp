#include "renderer.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace anexec {

class Renderer::Impl {
private:
    struct RenderState {
        bool initialized{false};
        bool surface_created{false};
        int surface_width{0};
        int surface_height{0};
        float scale_factor{1.0f};
        GLuint program_id{0};
        GLuint vertex_buffer{0};
        GLuint texture_id{0};
        std::mutex state_mutex;
    } state;

    struct RenderThread {
        std::thread thread;
        bool running{false};
        std::mutex mutex;
        std::condition_variable cv;
    } render_thread;

    RenderConfig config;
    std::vector<RenderCommand> command_queue;
    std::mutex queue_mutex;

    bool initGLContext() {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        
        if (state.initialized) {
            return true;
        }

        // Basic vertex shader
        const char* vertex_shader_source = R"(
            attribute vec4 a_position;
            attribute vec2 a_texCoord;
            varying vec2 v_texCoord;
            uniform mat4 u_mvpMatrix;
            
            void main() {
                gl_Position = u_mvpMatrix * a_position;
                v_texCoord = a_texCoord;
            }
        )";

        // Basic fragment shader
        const char* fragment_shader_source = R"(
            precision mediump float;
            varying vec2 v_texCoord;
            uniform sampler2D u_texture;
            
            void main() {
                gl_FragColor = texture2D(u_texture, v_texCoord);
            }
        )";

        // Create and compile shaders
        GLuint vertex_shader = compileShader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint fragment_shader = compileShader(GL_FRAGMENT_SHADER, fragment_shader_source);
        
        if (!vertex_shader || !fragment_shader) {
            return false;
        }

        // Create and link program
        state.program_id = glCreateProgram();
        glAttachShader(state.program_id, vertex_shader);
        glAttachShader(state.program_id, fragment_shader);
        glLinkProgram(state.program_id);

        // Check link status
        GLint link_status;
        glGetProgramiv(state.program_id, GL_LINK_STATUS, &link_status);
        if (!link_status) {
            GLint log_length;
            glGetProgramiv(state.program_id, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> log(log_length);
            glGetProgramInfoLog(state.program_id, log_length, nullptr, log.data());
            std::cerr << "Program linking failed: " << log.data() << std::endl;
            return false;
        }

        // Cleanup shaders
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        // Create vertex buffer
        glGenBuffers(1, &state.vertex_buffer);
        
        // Create texture
        glGenTextures(1, &state.texture_id);
        
        state.initialized = true;
        return true;
    }

    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compile_status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (!compile_status) {
            GLint log_length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> log(log_length);
            glGetShaderInfoLog(shader, log_length, nullptr, log.data());
            std::cerr << "Shader compilation failed: " << log.data() << std::endl;
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    void renderFrame() {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        
        if (!state.surface_created || !state.initialized) {
            return;
        }

        glViewport(0, 0, state.surface_width, state.surface_height);
        glClear(GL_COLOR_BUFFER_BIT);

        std::vector<RenderCommand> current_commands;
        {
            std::lock_guard<std::mutex> queue_lock(queue_mutex);
            current_commands.swap(command_queue);
        }

        glUseProgram(state.program_id);

        for (const auto& cmd : current_commands) {
            switch (cmd.type) {
                case RenderCommand::Type::DrawRect:
                    drawRect(cmd);
                    break;
                case RenderCommand::Type::DrawTexture:
                    drawTexture(cmd);
                    break;
                case RenderCommand::Type::Clear:
                    glClear(GL_COLOR_BUFFER_BIT);
                    break;
            }
        }

        glFinish();
    }

    void drawRect(const RenderCommand& cmd) {
        const float vertices[] = {
            cmd.x, cmd.y,
            cmd.x + cmd.width, cmd.y,
            cmd.x, cmd.y + cmd.height,
            cmd.x + cmd.width, cmd.y + cmd.height
        };

        glBindBuffer(GL_ARRAY_BUFFER, state.vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        GLint pos_attrib = glGetAttribLocation(state.program_id, "a_position");
        glEnableVertexAttribArray(pos_attrib);
        glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void drawTexture(const RenderCommand& cmd) {
        glBindTexture(GL_TEXTURE_2D, state.texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cmd.width, cmd.height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, cmd.texture_data);
        
        drawRect(cmd);
    }

public:
    Impl() {
        render_thread.running = true;
        render_thread.thread = std::thread([this]() {
            renderLoop();
        });
    }

    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(render_thread.mutex);
            render_thread.running = false;
        }
        render_thread.cv.notify_one();
        if (render_thread.thread.joinable()) {
            render_thread.thread.join();
        }

        cleanup();
    }

    void renderLoop() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(render_thread.mutex);
                render_thread.cv.wait(lock, [this]() {
                    return !render_thread.running || !command_queue.empty();
                });

                if (!render_thread.running) {
                    break;
                }
            }

            renderFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }

    void initialize(const RenderConfig& cfg) {
        config = cfg;
        if (!initGLContext()) {
            throw std::runtime_error("Failed to initialize GL context");
        }
    }

    void submitCommand(const RenderCommand& cmd) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            command_queue.push_back(cmd);
        }
        render_thread.cv.notify_one();
    }

    void onSurfaceCreated() {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        state.surface_created = true;
    }

    void onSurfaceChanged(int width, int height) {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        state.surface_width = width;
        state.surface_height = height;
        state.scale_factor = static_cast<float>(width) / config.design_width;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        if (state.initialized) {
            glDeleteProgram(state.program_id);
            glDeleteBuffers(1, &state.vertex_buffer);
            glDeleteTextures(1, &state.texture_id);
            state.initialized = false;
        }
    }

    bool isInitialized() const {
        return state.initialized;
    }

    float getScaleFactor() const {
        return state.scale_factor;
    }
};

// Реализация публичного интерфейса
Renderer::Renderer() : impl(new Impl()) {}
Renderer::~Renderer() = default;

void Renderer::initialize(const RenderConfig& config) {
    impl->initialize(config);
}

void Renderer::submitCommand(const RenderCommand& command) {
    impl->submitCommand(command);
}

void Renderer::onSurfaceCreated() {
    impl->onSurfaceCreated();
}

void Renderer::onSurfaceChanged(int width, int height) {
    impl->onSurfaceChanged(width, height);
}

bool Renderer::isInitialized() const {
    return impl->isInitialized();
}

float Renderer::getScaleFactor() const {
    return impl->getScaleFactor();
}

} // namespace anexec