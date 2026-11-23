1. Ensure you have the prerequisites installed:
    - [CMake](https://cmake.org/) (version 3.15 or higher is required)
    - Windows:
        - Generator: Visual Studio 17 2022
        - Windows SDK: Windows 10.0.19044 (Targeted)
        - C compiler: MSVC 19.44.35233.0
    - Linux:
        - Generator: GNU Make 4.4.1
        - C compiler: GNU GCC 15.2.0
2. In a terminal, navigate to `giflib-6.1.1/build` and run `cmake ..`
3. Build the library:
    - Windows:
        1. Open the generated Visual Studio solution
        2. Switch configuration to Release
        3. Rebuild all
    - Linux:
        1. Run `make` in the terminal