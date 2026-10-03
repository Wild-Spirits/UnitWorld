# Стиль кода

Стандарт — C++20 (designated initializers, `std::format`, `std::filesystem` используются повсеместно).

## Форматирование

Источник правды — `.clang-format` в корне. Основное:

- ширина строки 120, отступ 4 пробела, табы не используются;
- фигурные скобки на новой строке после классов, структур, enum, функций, namespace и управляющих конструкций;
  `InsertBraces: true` — тело `if`/`for` всегда в скобках;
- содержимое `namespace` **с отступом** (`NamespaceIndentation: All`), закрывающий комментарий `}    // namespace Vega`;
- `Type* ptr`, `Type& ref` (указатель/ссылка прилипают к типу);
- порядок квалификаторов: `static inline const constexpr volatile Type`;
- `{ }` для пустых тел, `Type { … }` — пробел перед braced-init.

Форматируйте только изменённые фрагменты, не переформатируйте файлы целиком — это засоряет diff.

## Именование

| Сущность | Стиль | Пример |
| --- | --- | --- |
| Классы, структуры, enum, алиасы | PascalCase | `StaticMeshManager`, `TextureProps` |
| Методы и свободные функции | PascalCase | `CreateFrameBuffer`, `GetShaderAttributeTypeSize` |
| Поля класса | `m_` + PascalCase | `m_RendererBackend` |
| Статические поля | `s_` + PascalCase | `s_Instance`, `s_API` |
| Параметры функций | `_` + PascalCase | `_Name`, `_ShaderConfig` |
| Локальные переменные | camelCase | `rendererBackend`, `meshInfo` |
| Публичные поля POD-структур (props/config/компоненты) | PascalCase без префикса | `Width`, `IsUsedInFlight` |
| Значения `enum class`, константы | `k` + PascalCase | `kVulkan`, `kPerGroup`, `kMainMenuFramePadding` |
| Макросы | `VEGA_` + UPPER_SNAKE | `VEGA_CORE_ASSERT`, `VEGA_BIND_EVENT_FN` |
| Булевы геттеры/поля | `Is…`, `IsHas…`, `GetIs…` | `IsHasManager`, `IsNeedSetPluginGlobals` |

Платформенные/бэкенд-классы получают префикс API: `VulkanTexture`, `OpenGlShader`, `GLFWWindow`, `WinPluginLibrary.cpp`.

## Файлы и include-ы

- Заголовки `.hpp`, реализация `.cpp`, встраиваемые таблицы `.inl`; в начале заголовка `#pragma once`.
- Пути include от `Vega/Source`: `"Vega/Renderer/Shader.hpp"`, `"Platform/Platform.hpp"`. Внутри одного каталога
  допустимы короткие `"Shader.hpp"`.
- Группы через пустую строку: собственный заголовок → заголовки Vega → сторонние библиотеки (`"glm/…"`, `<entt/…>`,
  `"imgui.h"`) → стандартная библиотека.
- Платформенный код оборачивается в `#ifdef VEGA_PLATFORM_WINDOWS` и т.п. из `Platform/Platform.hpp`.

## Идиомы

- Владение: `Ref<T>` (= `std::shared_ptr`) / `CreateRef<T>(…)`, `Scope<T>` (= `std::unique_ptr`) / `CreateScope<T>(…)`,
  приведение — `StaticRefCast<T>(ref)`. Сырой `new` — только в `CreateApplication`.
- Параметры-описания передаются структурами `…Props` / `…Config` с designated initializers:
  `CreateFrameBuffer(FrameBufferProps { .Name = "…", .Width = w, .Height = h })`.
- Интерфейс в `Vega/Renderer`, реализация в бэкенде; конкретные типы в общем коде не используются.
- Флаги — `typedef uint32_t XxxFlags` + `namespace XxxFlagBits { enum … : XxxFlags { kA = BIT(0), … }; }`.
- Строковые ключи принимаются как `std::string_view`.
- Проверки: `VEGA_CORE_ASSERT` в коде движка, `VEGA_ASSERT` в клиентском коде (только Debug). Ошибки, которые
  нужно видеть в Release, дополнительно логируйте `VEGA_CORE_ERROR`/`VEGA_CORE_CRITICAL`.
- Логи: `VEGA_CORE_*` в Vega и плагинах, `VEGA_*` в Editor.
- Незавершённое помечается `// TODO: …`, временный код — префиксом `Tmp`/комментарием `// TMP:`.
- Комментарии и строки логов — на английском. Doxygen-комментарии (`@brief`, `@return`) встречаются в публичных
  методах Vulkan-бэкенда; в остальном комментарии короткие и объясняют «почему».

## Коммиты

Сообщения на английском, одной строкой; несколько изменений перечисляются через `; `, незавершённые помечаются `(WIP)`:

```
RenderBuffer class for buffers like: Vertex, Index, Uniform; Scene systems; StaticMesh manager (WIP);
```
