# ANEXEC

ANEXEC is a high-performance Android executable environment for Linux that allows running Android applications without emulation.

> КОД НИФИГА НЕ РАБОТАЕТ, Я УЖЕ ЗАДОЛБАЛСЯ С НИМ ВОЗИТСЯ

## Features

- **Native Performance**: Direct execution of Android apps without emulation overhead
- **OpenGL ES Support**: Hardware-accelerated graphics rendering
- **Activity Lifecycle**: Full Android activity lifecycle management
- **API Compatibility**: Support for Android API levels 29-34 (Android 10-14)
- **Permission System**: Android-compatible permission management
- **Native Methods**: JNI-compatible native method registration and execution

## Requirements

- Linux (x86_64)
- OpenGL ES 2.0 compatible GPU
- CMake 3.15+
- C++17 compatible compiler
- Required libraries:
  - zlib
  - OpenGL ES 2.0
  - EGL
  - dl (dynamic linking)

## Building

```bash
# Clone the repository
git clone https://github.com/AnmiTaliDev/anexec.git
cd anexec

# Create build directory
mkdir build && cd build

bash ../build.sh
```

## Project Structure

```
anexec/
├── src/
│   ├── android/         # Android compatibility layer
│   ├── core/           # Core runtime components
│   ├── graphics/       # Graphics and rendering
│   └── main.cpp        # Entry point
└── build.sh
```

## Performance

- Near-native execution speed
- Minimal memory overhead
- Hardware-accelerated graphics
- Efficient resource management

## License

Copyright © 2025 AnmiTaliDev. All rights reserved.
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## Contact

- Developer: AnmiTaliDev
- Repository: https://github.com/AnmiTaliDev/anexec
- Issues: https://github.com/AnmiTaliDev/anexec/issues