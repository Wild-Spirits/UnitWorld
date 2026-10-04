# Ловушки кодовой базы

Неочевидные вещи, на которых легко ошибиться при правке кода.

## Сборка

- **Ручной список `SOURCES`.** Новый `.cpp`, не добавленный в `CMakeLists.txt`, не компилируется, и ошибки нет до
  линковки.
- **Предупреждения не являются ошибками.** Для целей проекта включён `/W4` (без `/WX`), поэтому успешная сборка
  ещё не значит, что в твоём коде нет предупреждений: проверь вывод сборки на `warning` в изменённых файлах.
- **Ассерты только в Debug.** `VEGA_CORE_ASSERT`/`VEGA_ASSERT` раскрываются в пустоту без `_DEBUG`. Не помещай в них
  вызовы с побочными эффектами. В `Base.hpp` debug-break для Debug определён только при `WIN32`/`LINUX`.
- **Один плагин — две копии статиков.** `VegaVulkanRenderer.dll` линкуется со статической Vega и ImGui, поэтому у DLL свои
  `Application::s_Instance`, логгеры и контекст ImGui. Они синхронизируются через `SetPluginGlobals(PluginData*)` и
  `ImGui::SetCurrentContext`/`SetAllocatorFunctions` в `VulkanImGuiImpl.cpp`. Новый глобальный/статический объект,
  нужный и в Vega, и в плагине, надо так же передавать через `PluginData`.
- **Путь к плагину.** DLL ищется в `BIN_FOLDER/VegaPlugins/VegaVulkanRenderer/`; с многоконфигурационным генератором
  (Visual Studio) путь не совпадает.

## Рантайм

- **Рабочий каталог = корень репозитория** (`RES_FOLDER`). Пути к ассетам и шейдерам пиши относительно корня:
  `"Assets/Shaders/Source/test.vert"`.
- **Явное освобождение GPU-ресурсов.** `Texture`/`Sampler`/`Shader`/ImGui-обёртки требуют `OnDetach()`,
  `FrameBuffer` — `Destroy()`. Делай это в `OnDetach` слоя, `OnDetach` менеджера или `Destroy` системы сцены.
  Порядок уничтожения в `Application::~Application`: слои → менеджеры → бэкенд.
- **Трансформ только через сеттеры.** `Entity::GetComponent<TransformComponent>()` удалён (`= delete`), а
  `registry.get<TransformComponent>` в обход `Entity` сломает пометку `TransformDirtyComponent`. Используй
  `GetTransform()` + `SetTransform*`.
- **Мировой трансформ обновляется раз в кадр.** `GetTransformMatrix()` — локальная матрица; мировая лежит в
  `WorldTransformComponent` и пересчитывается только в конце `Scene::OnUpdate`. Изменения, сделанные в `OnRender`
  или GUI, попадут в мировую матрицу в следующем кадре. Не снимай `TransformDirtyComponent` вручную — эти сущности не
  пересчитаются.
- **`DestroyEntity` удаляет всё поддерево.** Сохранённые `entt::entity`/`Entity` после этого невалидны, перед
  использованием проверяй `GetRegistry().valid(...)`.
- **Менеджеры по строковому имени.** Опечатка в имени `GetManager("…")` — ассерт в Debug и исключение
  `std::out_of_range` в Release.
- **Ресайз пропускает кадр**: при `m_Resizing` бэкенд пересоздаёт swapchain, слои в этом кадре не рендерятся.
- **`Window::Maximize/Minimize/Restore` отложенные** — применяются в `Window::OnUpdate`, `IsWindowMaximized()` сразу
  после вызова возвращает старое значение.
- **Заголовок окна.** При кастомном заголовке клиент обязан каждый кадр вызывать `Window::SetTitleBarLayout`, добавляя
  все интерактивные элементы заголовка: иначе клик по ним перетаскивает окно. Win32-код в `Platform/` вызывает
  wide-char функции явно (`SetWindowLongPtrW`, `CallWindowProcW`): `UNICODE` для Vega не определён, а `#define UNICODE`
  после `glfw3native.h` уже ничего не меняет.
- **`AppConfig.json` не читается** — менять его бесполезно, пока не написан загрузчик.

## Шейдеры

- **Нет рефлексии.** `ShaderConfig::Attributes` и списки uniform-ов должны вручную совпадать с GLSL (порядок `location`,
  имена, `set`/`binding`).
- **Номер `set` зависит от набора частот.** Descriptor set-ы нумеруются по присутствующим частотам (per-frame →
  per-group → per-draw), поэтому добавление per-frame uniform-ов сдвигает `set` у per-group в GLSL.
- **Per-draw = push constants**, гарантировано только 128 байт.
- **UBO заливается в `ApplyFrequency`.** `SetUniformBufferData` для per-frame/per-group пишет в CPU-копию; значение,
  выставленное после `ApplyFrequency`, попадёт на GPU только при следующем `ApplyFrequency`.
- **Ресурсы шейдера на кадр индексируются `GetCurrentFrameIndex()`** (frame-in-flight, защищён fence), а не
  `GetCurrentImageIndex()` (индекс картинки swapchain).
- **`.spv` перезаписываются при каждом запуске** — изменения в них в `git status` после запуска нормальны, это не правки
  пользователя.
- **Перевёрнутый viewport** в Vulkan инвертирует winding: поэтому у тестового шейдера `CullMode = kNone`.

## OpenGL / DirectX

- OpenGL-бэкенд — заглушка, DirectX отсутствует (`--directx` приводит к `nullptr`-бэкенду и падению). При добавлении
  чистого виртуального метода в `RendererBackend` (или другие интерфейсы рендера с OpenGL-реализацией) обязательно
  добавь пустую реализацию в OpenGL, иначе Vega не соберётся.
