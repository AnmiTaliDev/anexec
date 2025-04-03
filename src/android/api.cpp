#include "api.h"
#include <map>
#include <mutex>
#include <sstream>
#include <chrono>

namespace anexec {

class API::Impl {
private:
    struct APIState {
        bool initialized{false};
        std::string package_name;
        std::string version_name;
        int version_code{0};
        APILevel min_sdk_level{APILevel::ANDROID_10};
        APILevel target_sdk_level{APILevel::ANDROID_13};
        std::map<std::string, void*> native_functions;
        mutable std::mutex state_mutex;
    } state;

    std::map<std::string, std::function<void(const APIRequest&)>> handlers;
    std::mutex handlers_mutex;

    static constexpr int MAX_API_CALLS_PER_SECOND = 1000;
    std::chrono::system_clock::time_point last_rate_reset;
    int api_calls_count{0};
    std::mutex rate_limit_mutex;

    void registerDefaultHandlers() {
        registerHandler("getApiLevel", [this](const APIRequest& req) {
            handleGetApiLevel(req);
        });

        registerHandler("checkPermission", [this](const APIRequest& req) {
            handleCheckPermission(req);
        });

        registerHandler("registerNativeMethod", [this](const APIRequest& req) {
            handleRegisterNativeMethod(req);
        });
    }

    void handleGetApiLevel(const APIRequest& req) {
        int api_level = static_cast<int>(state.target_sdk_level);
        req.callback(APIResponse{
            .success = true,
            .data = std::to_string(api_level)
        });
    }

    void handleCheckPermission(const APIRequest& req) {
        const std::string& permission = req.params.at("permission");
        bool granted = false;

        if (permission == "android.permission.INTERNET" ||
            permission == "android.permission.READ_EXTERNAL_STORAGE" ||
            permission == "android.permission.WRITE_EXTERNAL_STORAGE") {
            granted = true;
        }

        req.callback(APIResponse{
            .success = true,
            .data = granted ? "granted" : "denied"
        });
    }

    void handleRegisterNativeMethod(const APIRequest& req) {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        const std::string& method_name = req.params.at("name");
        void* method_ptr = reinterpret_cast<void*>(
            std::stoull(req.params.at("pointer"))
        );
        
        state.native_functions[method_name] = method_ptr;

        req.callback(APIResponse{
            .success = true,
            .data = "Native method registered: " + method_name
        });
    }

    bool checkRateLimit() {
        std::lock_guard<std::mutex> lock(rate_limit_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_rate_reset
        );

        if (elapsed.count() >= 1) {
            api_calls_count = 0;
            last_rate_reset = now;
        }

        if (api_calls_count >= MAX_API_CALLS_PER_SECOND) {
            return false;
        }

        api_calls_count++;
        return true;
    }

public:
    Impl() : last_rate_reset(std::chrono::system_clock::now()) {
        registerDefaultHandlers();
    }

    void initialize(const APIConfig& config) {
        std::lock_guard<std::mutex> lock(state.state_mutex);

        if (state.initialized) {
            throw std::runtime_error("API already initialized");
        }

        state.package_name = config.package_name;
        state.version_name = config.version_name;
        state.version_code = config.version_code;
        state.min_sdk_level = config.min_sdk_level;
        state.target_sdk_level = config.target_sdk_level;
        state.initialized = true;
    }

    void registerHandler(const std::string& name, 
                        std::function<void(const APIRequest&)> handler) {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        handlers[name] = std::move(handler);
    }

    void handleRequest(APIRequest request) {
        if (!state.initialized) {
            request.callback(APIResponse{
                .success = false,
                .error = "API not initialized"
            });
            return;
        }

        if (!checkRateLimit()) {
            request.callback(APIResponse{
                .success = false,
                .error = "Rate limit exceeded"
            });
            return;
        }

        std::lock_guard<std::mutex> lock(handlers_mutex);
        auto it = handlers.find(request.method);
        if (it == handlers.end()) {
            request.callback(APIResponse{
                .success = false,
                .error = "Unknown method: " + request.method
            });
            return;
        }

        try {
            it->second(request);
        } catch (const std::exception& e) {
            request.callback(APIResponse{
                .success = false,
                .error = std::string("Handler error: ") + e.what()
            });
        }
    }

    void* getNativeFunction(const std::string& name) const {
        std::lock_guard<std::mutex> lock(state.state_mutex);
        auto it = state.native_functions.find(name);
        return it != state.native_functions.end() ? it->second : nullptr;
    }

    APILevel getMinSDKLevel() const {
        return state.min_sdk_level;
    }

    APILevel getTargetSDKLevel() const {
        return state.target_sdk_level;
    }

    std::string getPackageName() const {
        return state.package_name;
    }

    std::string getVersionName() const {
        return state.version_name;
    }

    int getVersionCode() const {
        return state.version_code;
    }

    bool isInitialized() const {
        return state.initialized;
    }
};

// Реализация публичного интерфейса
API::API() : impl(new Impl()) {}
API::~API() = default;

void API::initialize(const APIConfig& config) {
    impl->initialize(config);
}

void API::registerHandler(const std::string& name, 
                        std::function<void(const APIRequest&)> handler) {
    impl->registerHandler(name, std::move(handler));
}

void API::handleRequest(APIRequest request) {
    impl->handleRequest(std::move(request));
}

void* API::getNativeFunction(const std::string& name) const {
    return impl->getNativeFunction(name);
}

APILevel API::getMinSDKLevel() const {
    return impl->getMinSDKLevel();
}

APILevel API::getTargetSDKLevel() const {
    return impl->getTargetSDKLevel();
}

std::string API::getPackageName() const {
    return impl->getPackageName();
}

std::string API::getVersionName() const {
    return impl->getVersionName();
}

int API::getVersionCode() const {
    return impl->getVersionCode();
}

bool API::isInitialized() const {
    return impl->isInitialized();
}

} // namespace anexec