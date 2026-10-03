# Сборка и запуск

## Требования

- Windows 10/11 x64 (другие платформы объявлены в `Platform/Platform.hpp`, но загрузчик плагинов реализован только
  для Windows — `Platform/Windows/Plugins/WinPluginLibrary.cpp`).
- CMake ≥ 3.12 (пресеты используют схему версии 3, на практике нужен CMake ≥ 3.21).
- MSVC (Visual Studio 2022, toolset v143) или `clang-cl`.
- Ninja (для пресетов `windows-ninja-*`).
- **Vulkan SDK** с переменной окружения `VULKAN_SDK`. Плагин линкуется с `Vulkan::Vulkan` и `shaderc_shared`
  из `$VULKAN_SDK/Lib`, `shaderc_shared.dll` должна находиться в `PATH` при запуске.
- Доступ в интернет при первой конфигурации — зависимости тянутся через `FetchContent`.

## Пресеты

Конфигурация (`CMakePresets.json`, каталог сборки — `build/<preset>`):

| Configure preset | Генератор / компилятор |
| --- | --- |
| `windows-ninja-msvc-x64` | Ninja + `cl.exe` (основной; на него смотрит `.clangd`) |
| `windows-ninja-clang-x64` | Ninja + `clang-cl.exe` |
| `windows-vs-msvc-x64` | Visual Studio 17 2022 (см. ограничение ниже) |

Build-пресеты: `<configure-preset>-debug`, `-release`, `-RelWithDebInfo`.

```powershell
# из "Developer PowerShell for VS 2022" (нужны cl.exe и окружение MSVC)
cmake --preset windows-ninja-msvc-x64
cmake --build --preset windows-ninja-msvc-x64-debug
```

Ninja — одноконфигурационный генератор: тип сборки фактически задаётся на этапе configure (`CMAKE_BUILD_TYPE`),
`configuration` в build-пресете на него не влияет.

## Структура целей

```
UnitWorld (корневой CMakeLists.txt)
├── Vega                 STATIC  — ядро движка (Vega/)
├── VegaVulkanRenderer   SHARED  — Vulkan-бэкенд, линкуется с Vega (VegaPlugins/VegaVulkanRenderer/)
└── Editor               EXE     — редактор, линкуется с Vega; add_dependencies на VegaVulkanRenderer
```

`Runtime/` — пустая заготовка, закомментирована в корневом `CMakeLists.txt`.

## Зависимости (FetchContent в `Vega/CMakeLists.txt`)

| Библиотека | Версия | Назначение |
| --- | --- | --- |
| GLFW | 3.4 | окно и ввод |
| GLEW | форк `mihaillatyshov/glew` | OpenGL-бэкенд |
| GLM | 1.0.2 | математика |
| args (Taywee) | 6.4.7 | парсинг командной строки (Editor) |
| Dear ImGui | v1.92.4-docking | GUI; собирается как отдельная static-цель `imgui` с бэкендами glfw + opengl3 |
| stb | фиксированный коммит | загрузка изображений (`stb_image`, реализация в `RendererBackend.cpp`) |
| EnTT | v3.16.0 | ECS для сцены |
| nativefiledialog-extended | v1.2.1 | системные диалоги файлов |

Vulkan-бэкенд ImGui подключается в самом плагине (`ImGui/VulkanImGuiBackend.cpp`).

## Глобальные определения

Корневой `CMakeLists.txt` задаёт для всех целей:

- `RES_FOLDER="<корень репозитория>"` — Editor делает его рабочим каталогом, поэтому пути к ассетам пишутся
  относительно корня: `"Assets/Textures/logo_ws.png"`.
- `BIN_FOLDER="<каталог сборки>"` — отсюда грузятся плагины: `BIN_FOLDER/VegaPlugins/VegaVulkanRenderer/VegaVulkanRenderer.dll`.
- `OPENGL`.

Строгие флаги предупреждений (`/Wall /WX`) объявлены, но **не применяются** (строка с `ERROR_FLAGS` закомментирована).
Опция `SANITIZE` (ON) реально влияет только на не-MSVC компиляторы.

## Запуск

```powershell
build\windows-ninja-msvc-x64\Editor\Editor.exe [--vulkan | --opengl | --directx] [-h]
```

- По умолчанию используется Vulkan.
- `--opengl` — бэкенд-заглушка внутри Vega, большая часть методов не реализована.
- `--directx` — не реализован (`RendererBackend::Create` вернёт `nullptr`).
- Рабочий каталог переключается на корень репозитория, поэтому `imgui.ini` пишется в корень.
- `Assets/AppConfig.json` описывает настройки рендера (`vsync`, `power_saving`, `enable_validation`), но кодом
  **пока не читается**.

### Ограничение VS-генератора

`windows-vs-msvc-x64` — многоконфигурационный генератор, он кладёт DLL в подкаталог `Debug/`/`Release/`,
а `PluginLibrary::GetDefaultPluginPath()` ищет плагин без него. Для запуска используйте Ninja-пресеты.

## Инструменты

- `.clang-format` — форматирование, см. [code-style.md](code-style.md).
- `.clangd` — берёт `compile_commands.json` из `build/windows-ninja-msvc-x64` (`CMAKE_EXPORT_COMPILE_COMMANDS ON`).
