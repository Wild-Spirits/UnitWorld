# AGENTS.md

Инструкции для ИИ-агентов (Claude Code, Codex, Qwen Code, Cursor и др.), работающих с репозиторием **UnitWorld**.

UnitWorld — игровой движок **Vega** (C++20, статическая библиотека) + редактор **Editor** (ImGui) + рендер-бэкенд
**VegaVulkanRenderer** (Vulkan, подгружается как DLL-плагин). Проект в ранней стадии разработки: многие подсистемы
помечены `WIP`/`TODO`/`Tmp`, API регулярно меняется. Основная платформа — Windows x64.

## Самое важное

- Сборка: `cmake --preset windows-ninja-msvc-x64` → `cmake --build --preset windows-ninja-msvc-x64-debug`
  (из Developer Command Prompt / с настроенным окружением MSVC). Подробно — [Docs/build.md](Docs/build.md).
- Тестов нет. Изменения проверяются сборкой; запуск редактора требует GPU и окна — его делает пользователь.
- Исходники перечисляются в `SOURCES` соответствующего `CMakeLists.txt` **вручную**. Новый файл, не добавленный туда,
  не компилируется.
- Стиль: `.clang-format` в корне, именование `m_Member`, `s_Static`, `_Param`, `kEnumValue`, методы в PascalCase.
  Подробно — [Docs/code-style.md](Docs/code-style.md).
- Не коммитить и не редактировать `imgui.ini` — его перезаписывает редактор при каждом запуске.

## Карта документации

Читай нужный файл перед работой в соответствующей области.

| Файл | Когда читать |
| --- | --- |
| [Docs/README.md](Docs/README.md) | Оглавление всей документации проекта |
| [Docs/build.md](Docs/build.md) | Сборка, пресеты CMake, зависимости, запуск, флаги командной строки |
| [Docs/architecture.md](Docs/architecture.md) | Модули, жизненный цикл приложения, кадр, слои, события, менеджеры, сцена/ECS |
| [Docs/renderer.md](Docs/renderer.md) | Абстракция `RendererBackend`, Vulkan-плагин, шейдеры, ресурсы GPU, RenderGraph |
| [Docs/code-style.md](Docs/code-style.md) | Форматирование, именование, идиомы кода (Ref/Scope, ассерты, логирование) |
| [Docs/howto.md](Docs/howto.md) | Пошагово: добавить файл, компонент, систему сцены, менеджер, метод бэкенда |
| [Docs/ai/workflow.md](Docs/ai/workflow.md) | Правила работы ИИ-агента: проверка изменений, границы, коммиты, ответы |
| [Docs/ai/pitfalls.md](Docs/ai/pitfalls.md) | Неочевидные ловушки кодовой базы, на которых агенты ошибаются |
