# Working with the project in VS Code

This project is a template for the Raspberry Pi Pico 2/2W microcontroller with support for:

- Pico SDK
- FreeRTOS
- two architecture variants: ARM and RISC-V
- working with VS Code through the GUI: configuration, build, debugging, and flashing

Key project files:

- [CMakeLists.txt](CMakeLists.txt) – project-level configuration, selection of the ARM/RISC-V platform, and SDK/FreeRTOS paths
- [CMakePresets.json](CMakePresets.json) – CMake presets for debug/release builds and both architectures
- [src/CMakeLists.txt](src/CMakeLists.txt) – build target definition and library linking
- [.vscode/tasks.json](.vscode/tasks.json) – VS Code tasks for configuration, building, cleaning, flashing, and static analysis
- [.vscode/launch.json](.vscode/launch.json) – debugging configurations for GDB/Cortex-Debug
- [.vscode/c_cpp_properties.json](.vscode/c_cpp_properties.json) – IntelliSense settings and include paths
- [.vscode/settings.json](.vscode/settings.json) – CMake settings for VS Code

## 1. Opening the project in VS Code

1. Open the project folder in VS Code.
2. Make sure the following extensions are installed:
   - CMake Tools
   - C/C++
   - Cortex-Debug (optional, if you want to use this debugging configuration)
3. Open the Command Palette and use the available CMake commands if needed.

The project is configured to use CMake presets instead of manually invoking cmake from the terminal.

## 2. Choosing the build variant

The project supports four main presets:

- ARM Debug
- ARM Release
- RISC-V Debug
- RISC-V Release

These are defined in [CMakePresets.json](CMakePresets.json). Each preset sets:

- the build directory:
  - build/arm/debug
  - build/arm/release
  - build/riscv/debug
  - build/riscv/release
- the appropriate CMake values:
  - BUILD_FOR_ARM or BUILD_FOR_RISCV
  - PICO_BOARD=pico2_w
  - the appropriate PICO_PLATFORM for the selected architecture

## 3. Configuring and building through the VS Code GUI

The easiest workflow is to use the tasks defined in [.vscode/tasks.json](.vscode/tasks.json).

### Quick workflow

1. Open Terminal > Run Task.
2. Choose one of the tasks:
   - (ARM) [DEBUG] Configure + Build
   - (ARM) [RELEASE] Configure + Build
   - (RISC-V) [DEBUG] Configure + Build
   - (RISC-V) [RELEASE] Configure + Build
3. VS Code will run configuration and build in sequence.

### Purpose of the individual tasks

- Configure – runs cmake --preset ...
- Build – runs cmake --build --preset ...
- Configure + Build – runs both steps in sequence
- Clean Build Directory – removes the selected build directory

For GUI-based usage, the main advantage is that no manual commands are required. Everything is already prepared as tasks in the Task panel.

## 4. Debugging in VS Code

The project includes debugging configurations in [.vscode/launch.json](.vscode/launch.json).

### Typical debugging workflow

1. Start the task:
   - (ARM) [DEBUG] GDB Server via OpenOCD
   - or (RISC-V) [DEBUG] GDB Server via OpenOCD
2. Then, in the Run and Debug panel, select the appropriate configuration:
   - (ARM) RP2350 - GDB
   - (RISC-V) Hazard3 - GDB
3. Press F5.

The debugging setup:

- connects to OpenOCD on localhost:3333
- launches the program from the ELF file:
  - build/arm/debug/src/hkRTOS.elf
  - build/riscv/debug/src/hkRTOS.elf
- uses the appropriate debugger:
  - arm-none-eabi-gdb for ARM
  - riscv32-unknown-elf-gdb for RISC-V

### Cortex-Debug

The project also includes the following configuration:

- (ARM) RP2350 - Cortex-Debug external OpenOCD

You can use it if you have the Cortex-Debug extension installed and prefer a more integrated debugging experience with OpenOCD.

## 5. Flashing the program to the device

To program the board, use the flashing task from [.vscode/tasks.json](.vscode/tasks.json):

- (ARM) [DEBUG] Flash RP2350 via OpenOCD
- (RISC-V) [DEBUG] Flash RP2350 via OpenOCD

These tasks run OpenOCD with the appropriate RP2350 configuration and flash the ELF file from the build directory.

## 6. IntelliSense and code navigation

The project is configured so that VS Code has the correct build information:

- [CMakeLists.txt](CMakeLists.txt) sets CMAKE_EXPORT_COMPILE_COMMANDS=ON
- after CMake configuration, the compile_commands.json file is copied to [.vscode/compile_commands.json](.vscode/compile_commands.json)
- [.vscode/c_cpp_properties.json](.vscode/c_cpp_properties.json) defines the include paths for ARM and RISC-V

This allows VS Code to provide correct symbol suggestions, header resolution, and project structure navigation.

## 7. Static analysis

A task is available for this:

- (ARM) [DEBUG] Static Analysis with Cppcheck

It runs cppcheck over the src directory with the appropriate definitions for the ARM platform.

## 8. Common issues and how to resolve them

### CMake does not configure correctly

- check whether the correct preset has been selected
- check the Output panel for CMake Tools
- make sure the cross-compiler tools are available in PATH

### The debugger does not connect to the target

- make sure the GDB Server via OpenOCD task has been started
- verify that the correct debugger is being used for the selected architecture
- check whether the board is connected to the debug probe

### The build does not produce an ELF file

- check whether the build completed successfully
- inspect the build/arm/debug/src or build/riscv/debug/src directory
- if necessary, run Clean Build Directory and then Configure + Build again

## 9. Recommended workflow

1. Open the project in VS Code.
2. Select the ARM or RISC-V preset.
3. Run Configure + Build.
4. If you want to debug, start the OpenOCD server.
5. Start debugging from the Run and Debug panel.
6. If you want to program the board, run the flashing task.

This is the simplest and most GUI-friendly workflow for this project.
