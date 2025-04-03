#ifndef ANEXEC_RENDERER_H
#define ANEXEC_RENDERER_H

#include <memory>
#include <string>

namespace anexec {

struct RenderConfig {
    int design_width{1080};    // Базовая ширина для расчета масштаба
    int design_height{1920};   // Базовая высота для расчета масштаба
    bool vsync_enabled{true};  // Включена ли вертикальная синхронизация
    int msaa_samples{4};       // Количество сэмплов для MSAA
};

struct RenderCommand {
    enum class Type {
        DrawRect,
        DrawTexture,
        Clear
    } type;

    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    void* texture_data{nullptr};
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Запрещаем копирование
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void initialize(const RenderConfig& config);
    void submitCommand(const RenderCommand& command);
    void onSurfaceCreated();
    void onSurfaceChanged(int width, int height);
    bool isInitialized() const;
    float getScaleFactor() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace anexec

#endif // ANEXEC_RENDERER_H