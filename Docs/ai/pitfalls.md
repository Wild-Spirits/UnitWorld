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
- **Две копии GLFW: соединение с оконной системой не шарится.** `Editor` и `VegaVulkanRenderer` линкуют GLFW статически
  каждый своей копией, поэтому GLFW-состояние (в т.ч. `wl_display`/X-соединение) у них разное. Плагин не вызывает GLFW
  вообще: Vulkan-поверхность, WSI-расширения и presentation support делаются через нативные хэндлы окна
  (`Window::GetNativeWindowInfo` → `Platform/VulkanPlatform*.cpp`), которые принадлежат соединению редактора. Если
  создавать поверхность `glfwCreateWindowSurface` из плагина, драйвер получит `wl_display` плагина вместе с `wl_surface`
  редактора: `vkGetPhysicalDeviceSurfaceFormatsKHR` вернёт 0 форматов (`VK_ERROR_SURFACE_LOST_KHR`), а на Windows это
  незаметно, т.к. HWND валиден во всём процессе. Исключение — поверхности вторичных viewport'ов ImGui: их создаёт
  GLFW-бэкенд редактора (`Platform_CreateVkSurface`) с инстансом плагина, это корректно (соединение редактора).
  Но инстанс обязан включать и `VK_KHR_xlib_surface`, и `VK_KHR_xcb_surface` (GLFW предпочитает XCB): иначе
  `VK_ERROR_EXTENSION_NOT_PRESENT`, и окно viewport'а молча остаётся чёрным (`ImGui_ImplVulkan_CreateWindow`
  выходит без `RendererUserData`).
- **Путь к плагину.** DLL ищется в `BIN_FOLDER/VegaPlugins/VegaVulkanRenderer/`; с многоконфигурационным генератором
  (Visual Studio) путь не совпадает.

## Рантайм

- **Рабочий каталог = корень репозитория** (`RES_FOLDER`). Пути к ассетам и шейдерам пиши относительно корня:
  `"Assets/Shaders/Source/test.vert"`.
- **Явное освобождение GPU-ресурсов.** `Texture`/`Sampler`/`Shader`/ImGui-обёртки требуют `OnDetach()`,
  `FrameBuffer` — `Destroy()`. Делай это в `OnDetach` слоя, `OnDetach` менеджера или `Destroy` системы сцены.
  Порядок уничтожения в `Application::~Application`: слои → менеджеры → бэкенд.
- **Отложенное удаление в Vulkan.** Деструкторы в очереди удаления захватывают Vulkan-хэндлы по значению, а не
  объект-владелец. Удаление ImGui-текстур тоже отложено, поэтому `VulkanImGuiImpl::Shutdown` вызывает `WaitIdle`
  (он очищает очередь) до `ImGui_ImplVulkan_Shutdown`.
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
- **Ресайз не пропускает кадр**: swapchain пересоздаётся в `FramePrepareWindowSurface`, и кадр рендерится сразу;
  `VK_SUBOPTIMAL_KHR` на present терпится, пересоздание — только на `OUT_OF_DATE` или реальную смену размера
  framebuffer. Пропуск кадра при ресайзе вызывал лаг содержимого за рамкой окна — не возвращай его.
- **`VulkanSwapchain::Destroy` требует idle от вызывающей стороны.** Внутри него больше нет `vkDeviceWaitIdle`:
  `RecreateSwapchain` ждёт idle перед пересозданием, а `OnWindowDestroy` вызывается после `Application::WaitIdle`.
  Не добавляй второй idle — он удваивал остановку устройства на каждый кадр ресайза. Пересоздание передаёт старый
  swapchain через `oldSwapchain` и уничтожает его только после создания нового (без разрыва презентации).
- **Логгер не фильтрует уровни.** `Logger::Trace/Info/…` пишут в консоль безусловно (`Vega/Utils/Logger.hpp`),
  фильтра нет ни compile-time, ни runtime. Понижение INFO→TRACE само по себе спам не убирает: детальные логи
  пересоздания swapchain гейтятся по всплеску ресайза (`m_IsResizeRecreationBurst`, параметр `_LogDetails`) —
  печатаются только в первом кадре серии пересозданий, а не каждый кадр.
- **Размер окна логический, framebuffer — физический.** На Wayland с масштабированием `Window::GetWidth/GetHeight`
  возвращают логические координаты, а `GetFramebufferSize()` — физические пиксели (могут отличаться, например, в 1.6
  раза). Swapchain, `renderArea` и depth-буфер создаются по физическому размеру (`VulkanSwapchain::Create`,
  `VulkanImGuiImpl::NewFrame`), иначе композитор растягивает картинку и она не совпадает с курсором. ImGui масштабирует
  вершины сам через `DisplayFramebufferScale`. `FramePrepareWindowSurface` сверяет extent с framebuffer каждый кадр:
  масштаб может измениться без события ресайза (перенос окна на другой вывод). На Windows/X11 размеры совпадают.
- **`Window::Maximize/Minimize/Restore` отложенные** — применяются в `Window::OnUpdate`, `IsWindowMaximized()` сразу
  после вызова возвращает старое значение.
- **Заголовок окна.** При кастомном заголовке клиент обязан каждый кадр вызывать `Window::SetTitleBarLayout`, добавляя
  все интерактивные элементы заголовка: иначе клик по ним перетаскивает окно. Правило относится к главному окну
  (`TitleBarMode::kFull`); тайтлбарам вторичных viewport'ов (`kResizeOnly`) layout не нужен — они обрабатывают только
  ресайз-границы. Win32-код в `Platform/` вызывает wide-char функции явно (`SetWindowLongPtrW`, `CallWindowProcW`):
  `UNICODE` для Vega не определён, а `#define UNICODE` после `glfw3native.h` уже ничего не меняет.
- **Вынесенные окна ImGui.** Обёртки `Platform_CreateWindow/DestroyWindow` в `ImGuiLayer` обязаны вызывать оригиналы
  и не подменять `PlatformUserData`/`PlatformHandle` (на них завязаны все `Platform_*` колбэки GLFW-бэкенда);
  тайтлбар уничтожается до `glfwDestroyWindow` (WinTitleBar восстанавливает wndproc). Caption-drag вторичных окон
  должен оставаться за ImGui — иначе ломается docking. Вынесенные панели: `NoResize | NoCollapse` (double-click по
  заголовку иначе сворачивает окно вместо maximize), `p_open` в `Begin` не передавать (ImGui рисует второй close),
  `PlatformRequestClose` потребляет `DrawViewportTitleBarButtons`. На Windows у undecorated-окна клиент должен
  покрывать весь rect окна: GLFW считает его геометрию без рамки (`AdjustWindowRectEx` для `WS_POPUP`), нативные
  инсеты рассинхронизируют ImGui с реальным клиентом (дрейф позиции, обрезка контента).
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
