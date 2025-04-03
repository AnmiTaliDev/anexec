#ifndef ANEXEC_EXECUTOR_H
#define ANEXEC_EXECUTOR_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <filesystem>
#include <chrono>

namespace anexec {

// Информация об APK файле
struct ApkInfo {
    std::string package_name;        // Имя пакета
    std::string version_name;        // Версия (строка)
    uint32_t version_code{0};        // Версия (код)
    std::string min_sdk;            // Минимальная версия SDK
    std::string target_sdk;         // Целевая версия SDK
    std::vector<std::string> permissions;  // Разрешения
    std::string main_activity;      // Главная активность
    std::filesystem::path apk_path; // Путь к APK файлу
    std::chrono::system_clock::time_point load_time; // Время загрузки
};

// Состояние выполнения
enum class ExecutionState {
    NotStarted,
    Loading,
    Running,
    Paused,
    Stopped,
    Error
};

// Результат операций
enum class Result {
    Success,
    InvalidApk,
    RuntimeError,
    SecurityError,
    GraphicsError,
    PermissionDenied,
    UnsupportedApi,
    MemoryError,
    NetworkError
};

// Конфигурация исполнителя
struct ExecutorConfig {
    bool enable_graphics{true};     // Включить графический режим
    bool enable_sound{true};        // Включить звук
    bool enable_network{true};      // Включить сеть
    bool sandbox_mode{true};        // Режим песочницы
    std::string data_dir;           // Директория для данных
    std::vector<std::string> allowed_permissions; // Разрешенные права
};

class Executor {
public:
    // Конструкторы
    Executor();
    explicit Executor(const ExecutorConfig& config);
    ~Executor();

    // Запрещаем копирование
    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    // Разрешаем перемещение
    Executor(Executor&&) noexcept;
    Executor& operator=(Executor&&) noexcept;

    // Основные операции
    Result loadApk(const std::string& path);
    Result execute();
    void pause();
    void resume();
    void stop();

    // Получение информации
    ApkInfo getInfo() const;
    ExecutionState getState() const;
    std::string getLastError() const;

    // Управление конфигурацией
    void setConfig(const ExecutorConfig& config);
    ExecutorConfig getConfig() const;

    // Обработчики событий
    using EventCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(Result, const std::string&)>;

    void setEventCallback(EventCallback callback);
    void setErrorCallback(ErrorCallback callback);

    // Управление разрешениями
    bool hasPermission(const std::string& permission) const;
    Result grantPermission(const std::string& permission);
    Result revokePermission(const std::string& permission);

    // Статистика
    struct Statistics {
        size_t memory_used;         // Использованная память
        size_t peak_memory;         // Пиковое использование памяти
        double cpu_usage;           // Использование CPU
        uint64_t network_rx;        // Принятые данные
        uint64_t network_tx;        // Отправленные данные
        std::chrono::milliseconds uptime; // Время работы
    };
    
    Statistics getStatistics() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

// Вспомогательные функции
namespace utils {
    // Проверка совместимости APK
    bool isApkCompatible(const ApkInfo& info);
    
    // Получение информации о системе
    std::string getArchitecture();
    std::string getAndroidApiLevel();
    
    // Форматирование ошибок
    std::string formatError(Result result);
}

} // namespace anexec

#endif // ANEXEC_EXECUTOR_H