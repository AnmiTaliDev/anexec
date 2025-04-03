#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <csignal>
#include <thread>

#include "core/executor.h"
#include "android/api.h"
#include "android/activity.h"
#include "graphics/renderer.h"

namespace {

const char* LOGO = R"(
    ▄▄▄       ███▄    █ ▓█████ ▒██   ██▒▓█████  ▄████▄  
    ▒████▄     ██ ▀█   █ ▓█   ▀ ▒▒ █ █ ▒░▓█   ▀ ▒██▀ ▀█  
    ▒██  ▀█▄  ▓██  ▀█ ██▒▒███   ░░  █   ░▒███   ▒▓█    ▄ 
    ░██▄▄▄▄██ ▓██▒  ▐▌██▒▒▓█  ▄  ░ █ █ ▒ ▒▓█  ▄ ▒▓▓▄ ▄██▒
     ▓█   ▓██▒▒██░   ▓██░░▒████▒▒██▒ ▒██▒░▒████▒▒ ▓███▀ ░
     ▒▒   ▓▒█░░ ▒░   ▒ ▒ ░░ ▒░ ░▒▒ ░ ░▓ ░░░ ▒░ ░░ ░▒ ▒  ░
      ▒   ▒▒ ░░ ░░   ░ ▒░ ░ ░  ░░░   ░▒ ░ ░ ░  ░  ░  ▒   
      ░   ▒      ░   ░ ░    ░    ░    ░     ░   ░        
          ░  ░         ░    ░  ░ ░    ░     ░  ░░ ░      
)";

volatile std::sig_atomic_t g_running = 1;

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_running = 0;
    }
}

void setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

void printHeader() {
    std::cout << LOGO << std::endl;
    std::cout << "Native Android APK Executor v1.0.0" << std::endl;
    std::cout << "Started by AnmiTaliDev at 2025-04-03 05:07:43 UTC" << std::endl << std::endl;
}

void printApkInfo(const anexec::ApkMetadata& metadata) {
    std::cout << "APK Information:" << std::endl;
    std::cout << "  Package:       " << metadata.packageName << std::endl;
    std::cout << "  Version:       " << metadata.versionName
              << " (" << metadata.versionCode << ")" << std::endl;
    std::cout << "  Min SDK:       " << metadata.minSdkVersion << std::endl;
    std::cout << "  Target SDK:    " << metadata.targetSdkVersion << std::endl;
    std::cout << "  Main Activity: " << metadata.mainActivity << std::endl;
    
    if (!metadata.permissions.empty()) {
        std::cout << "  Permissions:" << std::endl;
        for (const auto& permission : metadata.permissions) {
            std::cout << "    - " << permission << std::endl;
        }
    }
    std::cout << std::endl;
}

class ExecutionManager {
public:
    ExecutionManager() {
        setupSignalHandlers();
    }

    int run(const char* apk_path) {
        try {
            anexec::Executor executor;
            
            std::cout << "Loading APK: " << apk_path << "..." << std::endl << std::endl;
            
            auto [result, metadata] = executor.loadApk(apk_path);
            if (result != anexec::Result::OK) {
                std::cerr << "Failed to load APK: " << static_cast<int>(result) << std::endl;
                return 1;
            }

            printApkInfo(metadata);
            if (!initializeComponents(metadata)) {
                return 1;
            }

            std::cout << "Starting execution..." << std::endl;
            std::cout << "Press Ctrl+C to stop" << std::endl << std::endl;

            return executeMainLoop(executor);

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

private:
    bool initializeComponents(const anexec::ApkMetadata& metadata) {
        try {
            // Инициализация рендерера
            anexec::RenderConfig render_config{
                .width = 1080,
                .height = 1920,
                .vsync = true,
                .msaa = 4,
                .surface_type = anexec::SurfaceType::PBUFFER
            };
            
            if (!renderer_.initialize(render_config)) {
                std::cerr << "Failed to initialize renderer" << std::endl;
                return false;
            }

            // Инициализация Android API
            anexec::APIConfig api_config{
                .package_name = metadata.packageName,
                .version_name = metadata.versionName,
                .version_code = metadata.versionCode,
                .min_sdk = metadata.minSdkVersion,
                .target_sdk = metadata.targetSdkVersion,
                .data_dir = "/data/data/" + metadata.packageName,
                .native_lib_dir = "/data/app/" + metadata.packageName + "/lib"
            };
            
            if (!api_.initialize(api_config)) {
                std::cerr << "Failed to initialize Android API" << std::endl;
                return false;
            }

            // Создание и инициализация Activity
            activity_ = std::make_unique<anexec::Activity>(metadata.mainActivity);
            if (!activity_->onCreate(api_.getContext())) {
                std::cerr << "Failed to create main activity" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialization error: " << e.what() << std::endl;
            return false;
        }
    }

    int executeMainLoop(anexec::Executor& executor) {
        try {
            if (!activity_->onStart()) {
                return 1;
            }

            anexec::RenderCommand cmd;
            while (g_running) {
                // Обработка событий Activity
                if (!activity_->onResume()) {
                    break;
                }

                // Обновление состояния приложения
                executor.update();

                // Отрисовка кадра
                while (renderer_.pollCommand(cmd)) {
                    renderer_.executeCommand(cmd);
                }
                
                renderer_.swapBuffers();

                // Поддержание стабильного FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }

            // Корректное завершение
            activity_->onPause();
            activity_->onStop();
            activity_->onDestroy();

            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Runtime error: " << e.what() << std::endl;
            return 1;
        }
    }

private:
    anexec::Renderer renderer_;
    anexec::API api_;
    std::unique_ptr<anexec::Activity> activity_;
};

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Error: Please provide APK file path" << std::endl;
        std::cerr << "Usage: " << argv[0] << " <apk_file>" << std::endl;
        return 1;
    }

    printHeader();

    ExecutionManager manager;
    return manager.run(argv[1]);
}