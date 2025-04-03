#include "activity.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <ctime>

namespace anexec {

class Activity::Impl {
private:
    struct LifecycleState {
        enum Value {
            Created,
            Started,
            Resumed,
            Paused,
            Stopped,
            Destroyed
        } value;

        std::chrono::system_clock::time_point timestamp;
        
        LifecycleState(Value v = Created) 
            : value(v), timestamp(std::chrono::system_clock::now()) {}

        std::string toString() const {
            static const std::map<Value, std::string> states = {
                {Created, "Created"},
                {Started, "Started"},
                {Resumed, "Resumed"},
                {Paused, "Paused"},
                {Stopped, "Stopped"},
                {Destroyed, "Destroyed"}
            };
            return states.at(value);
        }
    };

    std::string package_name;
    std::string activity_name;
    LifecycleState state;
    std::map<std::string, void*> system_services;
    bool is_finishing;
    std::vector<std::string> lifecycle_log;
    std::chrono::system_clock::time_point create_time;
    bool has_window_focus;
    SavedState saved_state;

    void logLifecycleEvent(const std::string& event) {
        std::stringstream ss;
        ss << "Activity " << activity_name << ": " << event;
        lifecycle_log.push_back(ss.str());
        std::cout << ss.str() << std::endl;
    }

public:
    Impl() 
        : package_name("com.example.app")
        , activity_name("MainActivity")
        , state(LifecycleState::Created)
        , is_finishing(false)
        , create_time(std::chrono::system_clock::now())
        , has_window_focus(false) {
        
        logLifecycleEvent("Constructed");
    }

    void onCreate() {
        if (state.value != LifecycleState::Created) {
            return;
        }

        try {
            logLifecycleEvent("onCreate called");
            initializeSystemServices();
            state = LifecycleState::Created;
            
        } catch (const std::exception& e) {
            logLifecycleEvent("onCreate failed: " + std::string(e.what()));
            throw;
        }
    }

    void onStart() {
        if (state.value != LifecycleState::Created && 
            state.value != LifecycleState::Stopped) {
            return;
        }

        try {
            logLifecycleEvent("onStart called");
            state = LifecycleState::Started;
            
        } catch (const std::exception& e) {
            logLifecycleEvent("onStart failed: " + std::string(e.what()));
            throw;
        }
    }

    void onResume() {
        if (state.value != LifecycleState::Started && 
            state.value != LifecycleState::Paused) {
            return;
        }

        try {
            logLifecycleEvent("onResume called");
            state = LifecycleState::Resumed;
            
        } catch (const std::exception& e) {
            logLifecycleEvent("onResume failed: " + std::string(e.what()));
            throw;
        }
    }

    void onPause() {
        if (state.value != LifecycleState::Resumed) {
            return;
        }

        try {
            logLifecycleEvent("onPause called");
            state = LifecycleState::Paused;
            
        } catch (const std::exception& e) {
            logLifecycleEvent("onPause failed: " + std::string(e.what()));
            throw;
        }
    }

    void onStop() {
        if (state.value != LifecycleState::Paused) {
            return;
        }

        try {
            logLifecycleEvent("onStop called");
            state = LifecycleState::Stopped;
            
        } catch (const std::exception& e) {
            logLifecycleEvent("onStop failed: " + std::string(e.what()));
            throw;
        }
    }

    void onDestroy() {
        if (state.value == LifecycleState::Destroyed) {
            return;
        }

        try {
            logLifecycleEvent("onDestroy called");
            cleanupSystemServices();
            state = LifecycleState::Destroyed;
            
        } catch (const std::exception& e) {
            logLifecycleEvent("onDestroy failed: " + std::string(e.what()));
            throw;
        }
    }

    void onSaveInstanceState(SavedState& outState) {
        logLifecycleEvent("Saving instance state");
        outState = saved_state;
        outState.timestamp = std::chrono::system_clock::now();
        outState.is_finishing = is_finishing;
        outState.has_focus = has_window_focus;
    }

    void onRestoreInstanceState(const SavedState& savedState) {
        logLifecycleEvent("Restoring instance state");
        saved_state = savedState;
        has_window_focus = savedState.has_focus;
    }

    void onWindowFocusChanged(bool hasFocus) {
        has_window_focus = hasFocus;
        logLifecycleEvent(std::string("Window focus changed: ") + 
                         (hasFocus ? "gained focus" : "lost focus"));
    }

    void startActivity(const Intent& intent) {
        logLifecycleEvent("Starting new activity: " + intent.action);
    }

    void finish() {
        if (!is_finishing) {
            is_finishing = true;
            logLifecycleEvent("Activity finishing");
            onPause();
            onStop();
            onDestroy();
        }
    }

    std::string getPackageName() const {
        return package_name;
    }

    void* getSystemService(const std::string& name) {
        auto it = system_services.find(name);
        if (it != system_services.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool isFinishing() const {
        return is_finishing;
    }

    bool hasWindowFocus() const {
        return has_window_focus;
    }

    bool isDestroyed() const {
        return state.value == LifecycleState::Destroyed;
    }

    std::chrono::system_clock::time_point getCreateTime() const {
        return create_time;
    }

private:
    void initializeSystemServices() {
        system_services["window"] = nullptr;
        system_services["layout_inflater"] = nullptr;
        system_services["activity"] = nullptr;
        system_services["input_method"] = nullptr;
        system_services["location"] = nullptr;
        
        logLifecycleEvent("System services initialized");
    }

    void cleanupSystemServices() {
        system_services.clear();
        logLifecycleEvent("System services cleaned up");
    }
};

Activity::Activity() : impl(new Impl()) {}
Activity::~Activity() = default;

void Activity::onCreate() {
    impl->onCreate();
}

void Activity::onStart() {
    impl->onStart();
}

void Activity::onResume() {
    impl->onResume();
}

void Activity::onPause() {
    impl->onPause();
}

void Activity::onStop() {
    impl->onStop();
}

void Activity::onDestroy() {
    impl->onDestroy();
}

void Activity::onSaveInstanceState(SavedState& outState) {
    impl->onSaveInstanceState(outState);
}

void Activity::onRestoreInstanceState(const SavedState& savedState) {
    impl->onRestoreInstanceState(savedState);
}

void Activity::onWindowFocusChanged(bool hasFocus) {
    impl->onWindowFocusChanged(hasFocus);
}

void Activity::onActivityResult(int requestCode, int resultCode, const Intent* data) {
}

void Activity::onNewIntent(const Intent& intent) {
}

void Activity::onConfigurationChanged() {
}

bool Activity::onBackPressed() {
    return true;
}

void Activity::startActivity(const Intent& intent) {
    impl->startActivity(intent);
}

void Activity::finish() {
    impl->finish();
}

std::string Activity::getPackageName() const {
    return impl->getPackageName();
}

void* Activity::getSystemService(const std::string& name) {
    return impl->getSystemService(name);
}

bool Activity::isFinishing() const {
    return impl->isFinishing();
}

bool Activity::hasWindowFocus() const {
    return impl->hasWindowFocus();
}

bool Activity::isDestroyed() const {
    return impl->isDestroyed();
}

std::chrono::system_clock::time_point Activity::getCreateTime() const {
    return impl->getCreateTime();
}

} // namespace anexec