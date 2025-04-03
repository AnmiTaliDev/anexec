#ifndef ANEXEC_API_H
#define ANEXEC_API_H

#include <string>
#include <memory>
#include <functional>
#include <map>

namespace anexec {

enum class APILevel {
    ANDROID_10 = 29,
    ANDROID_11 = 30,
    ANDROID_12 = 31,
    ANDROID_12L = 32,
    ANDROID_13 = 33,
    ANDROID_14 = 34
};

struct APIConfig {
    std::string package_name;
    std::string version_name;
    int version_code;
    APILevel min_sdk_level;
    APILevel target_sdk_level;
};

struct APIResponse {
    bool success;
    std::string data;
    std::string error;
};

struct APIRequest {
    std::string method;
    std::map<std::string, std::string> params;
    std::function<void(APIResponse)> callback;
};

class API {
public:
    API();
    ~API();

    // Запрещаем копирование
    API(const API&) = delete;
    API& operator=(const API&) = delete;

    void initialize(const APIConfig& config);
    void registerHandler(const std::string& name, 
                        std::function<void(const APIRequest&)> handler);
    void handleRequest(APIRequest request);
    void* getNativeFunction(const std::string& name) const;

    APILevel getMinSDKLevel() const;
    APILevel getTargetSDKLevel() const;
    std::string getPackageName() const;
    std::string getVersionName() const;
    int getVersionCode() const;
    bool isInitialized() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace anexec

#endif // ANEXEC_API_H