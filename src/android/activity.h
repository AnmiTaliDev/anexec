#ifndef ANEXEC_ACTIVITY_H
#define ANEXEC_ACTIVITY_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace anexec {

/**
 * @brief Базовый класс для Android Activity
 * 
 * Реализует основной жизненный цикл Activity и базовый функционал
 * Android-компонента. Поддерживает управление состоянием, системные
 * сервисы и Intent-based навигацию.
 */
class Activity {
public:
    /**
     * @brief Структура для передачи данных между Activity
     */
    struct Intent {
        std::string action;              // Действие Intent
        std::string data;                // URI или другие данные
        std::vector<std::string> categories;  // Категории Intent
        std::string type;                // MIME тип данных
        std::string package;             // Целевой пакет
        std::string component;           // Имя компонента
        bool is_explicit{false};         // Явный или неявный Intent
    };

    /**
     * @brief Структура для сохранения состояния Activity
     */
    struct SavedState {
        std::chrono::system_clock::time_point timestamp;
        std::vector<std::pair<std::string, std::string>> data;
        bool is_finishing{false};
        bool has_focus{false};
    };

    /**
     * @brief Конструктор Activity
     */
    Activity();

    /**
     * @brief Виртуальный деструктор
     */
    virtual ~Activity();

    // Запрещаем копирование
    Activity(const Activity&) = delete;
    Activity& operator=(const Activity&) = delete;

    // Разрешаем перемещение
    Activity(Activity&&) noexcept = default;
    Activity& operator=(Activity&&) noexcept = default;

    /**
     * @brief Методы жизненного цикла Activity
     * 
     * Вызываются системой в определенном порядке при изменении
     * состояния Activity. Могут быть переопределены в производных классах.
     */
    virtual void onCreate();
    virtual void onStart();
    virtual void onResume();
    virtual void onPause();
    virtual void onStop();
    virtual void onDestroy();

    /**
     * @brief Методы для управления состоянием
     */
    virtual void onSaveInstanceState(SavedState& state);
    virtual void onRestoreInstanceState(const SavedState& state);
    virtual void onWindowFocusChanged(bool has_focus);
    
    /**
     * @brief Запуск новой Activity
     * @param intent Информация о запускаемой Activity
     */
    void startActivity(const Intent& intent);

    /**
     * @brief Завершение текущей Activity
     */
    void finish();

    /**
     * @brief Получение имени пакета
     * @return Имя пакета приложения
     */
    std::string getPackageName() const;

    /**
     * @brief Получение системного сервиса
     * @param name Имя сервиса
     * @return Указатель на системный сервис или nullptr
     */
    void* getSystemService(const std::string& name);

    /**
     * @brief Проверка состояния Activity
     */
    bool isFinishing() const;
    bool hasWindowFocus() const;
    bool isDestroyed() const;

    /**
     * @brief Получение информации о времени создания
     */
    std::chrono::system_clock::time_point getCreateTime() const;

protected:
    /**
     * @brief Защищенные методы для производных классов
     */
    virtual void onActivityResult(int requestCode, int resultCode, const Intent* data);
    virtual void onNewIntent(const Intent& intent);
    virtual void onConfigurationChanged();
    virtual bool onBackPressed();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Вспомогательные функции и константы
 */
namespace activity {
    // Стандартные действия Intent
    constexpr const char* ACTION_MAIN = "android.intent.action.MAIN";
    constexpr const char* ACTION_VIEW = "android.intent.action.VIEW";
    constexpr const char* ACTION_EDIT = "android.intent.action.EDIT";
    constexpr const char* ACTION_SEND = "android.intent.action.SEND";

    // Категории Intent
    constexpr const char* CATEGORY_DEFAULT = "android.intent.category.DEFAULT";
    constexpr const char* CATEGORY_LAUNCHER = "android.intent.category.LAUNCHER";
    constexpr const char* CATEGORY_HOME = "android.intent.category.HOME";

    // Коды результатов
    constexpr int RESULT_OK = -1;
    constexpr int RESULT_CANCELED = 0;
    constexpr int RESULT_FIRST_USER = 1;

    // Имена системных сервисов
    constexpr const char* WINDOW_SERVICE = "window";
    constexpr const char* LAYOUT_INFLATER_SERVICE = "layout_inflater";
    constexpr const char* ACTIVITY_SERVICE = "activity";
    constexpr const char* INPUT_METHOD_SERVICE = "input_method";
    constexpr const char* LOCATION_SERVICE = "location";
}

} // namespace anexec

#endif // ANEXEC_ACTIVITY_H