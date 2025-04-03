#ifndef ANEXEC_RUNTIME_H
#define ANEXEC_RUNTIME_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace anexec {

enum class Result {
    Success,
    RuntimeError,
    ClassNotFound,
    MethodNotFound,
    OutOfMemory,
    SecurityException
};

enum class RuntimeState {
    NotInitialized,
    Ready,
    Running,
    Stopped,
    Error
};

struct RuntimeConfig {
    std::string system_dir;        // Путь к системным файлам Android
    std::string data_dir;          // Путь к данным приложения
    size_t heap_size{256 * 1024 * 1024}; // Размер кучи (256MB по умолчанию)
    bool debug_mode{false};        // Режим отладки
    std::vector<std::string> classpath; // Дополнительные пути для классов
};

struct RuntimeStats {
    std::chrono::seconds uptime;   // Время работы
    size_t loaded_classes_count;    // Количество загруженных классов
    size_t native_methods_count;    // Количество нативных методов
    RuntimeState current_state;     // Текущее состояние
    std::chrono::system_clock::time_point start_time; // Время запуска
    std::string user;              // Текущий пользователь
};

class Runtime {
public:
    using EventCallback = std::function<void(const std::string&)>;

    Runtime();
    ~Runtime();

    // Запрещаем копирование
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    bool initialize(const RuntimeConfig& config);
    RuntimeState getState() const;
    Result startActivity(const std::string& activity_name, void* savedInstanceState = nullptr);
    void setEventCallback(EventCallback callback);
    RuntimeStats getStats() const;
    void shutdown();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace anexec

#endif // ANEXEC_RUNTIME_H