# Архитектура

## Модули

| Каталог | Что внутри |
| --- | --- |
| `Vega/Source/Vega/Core` | `Application`, `Window`, `EntryPoint.hpp` (функция `main`), `Base.hpp` (`Ref`/`Scope`, макросы), `Assert.hpp`, коды клавиш |
| `Vega/Source/Vega/Events` | Очередь событий `EventManager`, типы событий окна, клавиатуры и мыши |
| `Vega/Source/Vega/Layers` | `Layer`, `LayerStack`, `GuiLayer` (абстрактный), `ImGuiLayer` |
| `Vega/Source/Vega/Renderer` | Абстрактный рендер-API: `RendererBackend`, `Texture`, `Sampler`, `Shader`, `FrameBuffer`, `RenderBuffer`, `Material`, `RenderGraph/` |
| `Vega/Source/Vega/Managers` | Глобальные реестры ресурсов (`StaticMeshManager`, `TextureManager`, `MaterialManager`, `ShaderManager`, …) |
| `Vega/Source/Vega/Scene` | `Scene`/`Entity` поверх EnTT, `Components/`, `Systems/` |
| `Vega/Source/Vega/Plugins` | `PluginLibrary` — загрузка DLL и поиск экспортированных функций |
| `Vega/Source/Vega/ImGui` | Интерфейсы `ImGuiImpl`, обёртки текстур/фреймбуферов для ImGui, кнопки заголовка вынесенных окон (`ImGuiViewportTitleBar`), иконочные шрифты Font Awesome |
| `Vega/Source/Vega/Utils` | Собственный `Logger`/`Log`, `PluginData`, `magic_enum.hpp`, `utf8.hpp` |
| `Vega/Source/Platform` | Платформенный код: `Platform.hpp` (макросы `VEGA_PLATFORM_*`), GLFW-окно/ввод, кастомный заголовок окна (`Desktop`/`Windows`/`Linux`/`MacOS`), OpenGL-бэкенд (заглушка), загрузчик плагинов (WinAPI / `dlopen`) |
| `VegaPlugins/VegaVulkanRenderer` | Реализация `RendererBackend` на Vulkan, собирается в DLL — см. [renderer.md](renderer.md) |
| `Editor/Source` | Приложение-редактор: `main.cpp`, `EditorLayer`, панели `SceneHierarchyPanel`, `EntityPropsPanel` |
| `Assets` | Шрифты, текстуры, GLSL-шейдеры (`Shaders/Source/*.vert|frag` + сгенерированные `*.spv`), `AppConfig.json` |
| `Docs` | Документация; `rendergraph.drawio` — набросок схемы render graph |

Всё находится в пространстве имён `Vega` (вложенные: `Vega::Components`, `Vega::SceneSystems`).
Include-пути: корнем служит `Vega/Source`, поэтому включения выглядят как `#include "Vega/Core/Application.hpp"`
и `#include "Platform/Platform.hpp"`.

## Точка входа и жизненный цикл

1. `main` определён в `Vega/Core/EntryPoint.hpp`. Клиент (Editor) включает этот заголовок ровно в одном
   `.cpp` и реализует `Vega::CreateApplication(ApplicationCommandLineArgs)`.
2. `CreateApplication` в Editor парсит аргументы (`args`), заполняет `ApplicationProps` и создаёт наследника
   `Application`, который в конструкторе кладёт `ImGuiLayer` в `m_GuiLayer` + `PushOverlay` и `PushLayer(EditorLayer)`.
3. Конструктор `Application`: `Log::Init()` → синглтон `s_Instance` → `NFD::Init()` → смена рабочего каталога →
   `EventManager` → `Window::Create` → `RendererBackend::Create(api)` (для Vulkan — загрузка DLL) →
   `Init()` → `OnWindowCreate(window)` → подписка на `WindowResizeEvent`/`WindowCloseEvent`.
4. `Run()` — главный цикл (см. ниже).
5. Деструктор: `WaitIdle` бэкенда (дождаться последнего отправленного кадра) → `LayerStack::Clear()` (вызывает
   `OnDetach` слоёв) → `OnDetach` всех менеджеров → `OnWindowDestroy` → `Shutdown` бэкенда → `NFD::Quit()`.
   Порядок важен: GPU-ресурсы слоёв и менеджеров освобождаются, когда GPU уже их не использует, и до уничтожения
   устройства.

## Кадр (`Application::Run`)

```
timestep = Time::GetTime() - lastFrameTime
for layer: OnUpdate(timestep)
if not minimized:
    if resizing: backend.OnResize(); backend.FramePrepareWindowSurface()     # кадр пропускается
    elif backend.FramePrepareWindowSurface():
        backend.FrameCommandListBegin()
        for layer: OnRender()                       # запись команд отрисовки
        if GuiLayer.BeginGuiFrame():
            for layer: OnGuiRender()                # ImGui
            GuiLayer.EndGuiFrame()
        backend.TmpRendergraphExecute()             # временно, до появления RenderGraph
        backend.FrameCommandListEnd(); FrameSubmit(); FramePresent()
        GuiLayer.OnExternalViewportsRender()        # окна ImGui multi-viewport, только после GUI-кадра
window.OnUpdate()                                   # отложенные Maximize/Minimize/Restore, тайтлбар, опрос GLFW
eventManager.DispatchEvents()
```

`OnExternalViewportsRender` вызывается только в кадре с отрисованным GUI: `ImGui::UpdatePlatformWindows` требует
парного `NewFrame`/`Render` и ассертит, если кадр GUI был пропущен (например, при пересоздании swapchain).

`Window::Maximize/Minimize/Restore` не меняют состояние сразу, а применяются в `Window::OnUpdate`: смена состояния
синхронно рассылает события ресайза, а вызывается обычно из GUI посреди записи кадра.

Vulkan-бэкенд пересоздаёт swapchain вместе с depth-буферами и command buffer-ами только в начале кадра
(`FramePrepareWindowSurface` при `m_IsNeedRecreateSwapchain`); `VK_ERROR_OUT_OF_DATE_KHR`/`VK_SUBOPTIMAL_KHR` лишь
выставляют этот флаг. Размер depth-буферов берётся из extent swapchain, а не из размера окна.

`Time::GetTime()` (`Vega/Core/Time.hpp`) — монотонное время в секундах на `std::chrono::steady_clock` с
неопределённым началом отсчёта, имеет смысл только разность. `Timestep` — длительность кадра в секундах (`float`),
неявно приводится к `float`; передаётся в `Layer::OnUpdate(Timestep)`, а слой сам передаёт его в `Scene::OnUpdate`.
Длительность кадра не ограничивается: после паузы в отладчике первый шаг будет большим.

В Editor сцена рисуется в собственный `FrameBuffer` (`EditorLayer::OnRender`: `BindAndClearColorDepthStencil` →
`BeginRendering` → `Scene::OnRender(view)` → `EndRendering`), а затем выводится в окно-вьюпорт ImGui через
`ImGuiFrameBufferWrapper`. Размер `FrameBuffer` следует за окном Viewport: новый размер запоминается в
`OnGuiRender`, а ресайз (с пересозданием `ImGuiFrameBufferWrapper`) выполняется в начале следующего `OnUpdate`.

### Камеры и режимы редактора

Сцена камеру не выбирает: `Scene::OnRender(const RenderView&)` получает вид от владельца сцены.

- `EditorLayer` хранит `SceneState` (`kEdit`/`kPlay`, кнопка Play/Stop в заголовке). В `kEdit` рисует
  `EditorCamera`, в `kPlay` — `Scene::GetPrimaryCameraView` (если основной камеры нет — остаётся камера редактора
  и во вьюпорте выводится предупреждение). Play пока только переключает камеру: сцена не копируется, логика не
  запускается.
- `EditorCamera` (`Editor/Source`) — не сущность, в сцену не попадает. Модель «точка фокуса + дистанция + yaw/pitch»:
  fly (ПКМ + мышь, WASD/QE, Shift, колесо — скорость) и orbit (Alt + ЛКМ — вращение, СКМ — панорама,
  колесо / Alt + ПКМ — зум, F — фокус на начало координат). Ввод читается через ImGui IO в `OnGuiRender`, только
  при наведении на вьюпорт; поверх картинки лежит `InvisibleButton`, чтобы перетаскивание не двигало окно.
- Будущий runtime-слой (игра без редактора) всегда использует `GetPrimaryCameraView`.

## Окно и кастомный заголовок

`Window` (реализация `GLFWWindow`) при `WindowProps::IsUseCustomTitlebar` создаёт платформенный `GLFWTitleBar`
(`Platform/Desktop/Core/GLFWTitleBar.*`). Окно создаётся скрытым и показывается после настройки заголовка.
Сам заголовок рисует клиент (`EditorLayer::DrawGuiTitlebar`); платформа отвечает за перетаскивание, ресайз и
двойной клик. Каждый кадр клиент передаёт `Window::SetTitleBarLayout`: высоту заголовка и прямоугольники
интерактивных элементов (меню, кнопки) в координатах окна — всё остальное в пределах высоты перетаскивает окно.

На Linux GLFW принудительно переводится на X11 (`glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11)` в
`GLFWWindow::Init`): multi-viewport ImGui не умеет размещать окна вне основного окна на Wayland. Под Wayland-сессией
приложение работает через XWayland; выбранная оконная система выводится в лог при старте (`Window system: …`).

| Платформа | Класс | Как устроено |
| --- | --- | --- |
| Windows | `Platform/Windows/Core/WinTitleBar` | Сабкласс оконной процедуры (wide-char API). `WM_NCCALCSIZE` убирает только заголовок и верхнюю рамку: левая, правая и нижняя остаются системными (невидимые рамки ресайза, тень, Snap). Верхний ресайз и caption эмулируются в `WM_NCHITTEST`; у развёрнутого окна отступ по толщине рамки для DPI окна и зазор для автоскрываемой панели задач. |
| Linux (X11) | `Platform/Linux/Core/LinuxTitleBar` | Окно без декораций; перемещение и ресайз отдаются оконному менеджеру через `_NET_WM_MOVERESIZE`, после чего GLFW досылается синтетический `ButtonRelease`. На Wayland не поддерживается — используются системные декорации (`IsCustomTitleBar() == false`), но из-за принудительного X11 (см. выше) этот режим не наступает. |
| macOS | `Platform/MacOS/Core/MacOSTitleBar.mm` | `NSWindowStyleMaskFullSizeContentView` + прозрачный системный заголовок; «светофоры» остаются родными (`IsTitleBarHasNativeButtons`, клиент оставляет под них `GetTitleBarNativeButtonsWidth`). Перетаскивание — `performWindowDragWithEvent`, ресайз системный. |

`Application::GetIsHasCutsomTitleBar()` возвращает фактическое состояние окна. Если кастомного заголовка нет,
Editor рисует меню без собственных кнопок окна.

### Вынесенные окна (вторичные viewport'ы ImGui)

Перетаскивание заголовка и docking остаются за ImGui: только ImGui-driven move возвращает окно в главный
dockspace или объединяет с другим вынесенным окном. Платформа (`TitleBarMode::kResizeOnly`) делает только ресайз.

- `ImGuiLayer` оборачивает `Platform_CreateWindow`/`Platform_DestroyWindow` GLFW-бэкенда (оригиналы вызываются,
  `PlatformUserData`/`PlatformHandle` не подменяются) и ведёт реестр `GLFWwindow* → GLFWTitleBar` в режиме
  `kResizeOnly`: только ресайз-границы и курсоры, caption-клики платформа не обрабатывает. Layout
  (`SetTitleBarLayout`) таким тайтлбарам не нужен. Там же окну задаётся минимальный размер
  (`glfwSetWindowSizeLimits`, 200×100): WM-ресайз не ограничен минимумом ImGui (`style.WindowMinSize`),
  без limits окно сжимается до пикселя.
- Драйвинг в `BeginGuiFrame`: до `ImGui_ImplGlfw_NewFrame` — `OnUpdate` тайтлбаров (курсоры, синтетический
  `ButtonRelease`); после `ImGui::NewFrame` — клик по ресайз-границе hovered-окна стартует WM-ресайз и потребляется
  (`FocusWindow` + `SetActiveID(MoveId)`), чтобы ImGui не начал move поверх WM (верхняя граница лежит внутри
  заголовка). `io.ConfigWindowsMoveFromTitleBarOnly = true` (клик по фону окно не двигает).
- Maximized-окно не «ездит» в развёрнутом состоянии: начатый ImGui-move заголовка до `ImGui::NewFrame`
  передаётся оконному менеджеру (`GLFWTitleBar::StartCaptionMove`: X11 — `_NET_WM_MOVERESIZE`, Windows —
  `SC_MOVE | HTCAPTION`), WM сам разворачивает окно обратно и продолжает перетаскивание — как у главного окна.
  Клик потребляется (`SetActiveID(MoveId)`); docking в этом drag не участвует (окно развёрнуто).
- Кнопки min/max/close и double-click maximize рисует `DrawViewportTitleBarButtons`
  (`Vega/Source/Vega/ImGui/ImGuiViewportTitleBar.*`) реальными айтемами внутри заголовка ImGui — айтемы блокируют
  старт перетаскивания. Double-click дополнительно потребляет клик (`SetActiveID(MoveId)`): иначе ImGui в конце
  кадра начинает move и стягивает окно обратно из maximized. Editor вызывает её после `Begin` каждой панели; закрытие скрывает панель (флаги
  `m_IsDraw…Panel`, возврат — меню Window). Вынесенные панели получают `NoResize | NoCollapse` (ресайз — WM,
  double-click — maximize, а не collapse); `p_open` в `Begin` не передаётся (иначе ImGui рисует второй close),
  `PlatformRequestClose` (Alt+F4) потребляет хелпер.
- На Windows в `kResizeOnly` клиент покрывает весь rect окна (`WM_NCCALCSIZE`), все 8 границ эмулируются в
  `WM_NCHITTEST`, окну добавляется `WS_THICKFRAME`: GLFW считает геометрию undecorated-окон без рамки, нативные
  инсеты рассинхронизировали бы ImGui с реальным клиентом.
- Объединённые floating dock-ноды и демо-окно сохраняют стандартный заголовок ImGui (кнопок хелпера нет);
  ресайз-границы WM у них те же, клик по границе потребляется драйвингом и не конфликтует с грипами ImGui.

## Слои

`Layer` — виртуальные `OnAttach(Ref<EventManager>)`, `OnDetach`, `OnUpdate(Timestep)`, `OnRender`, `OnGuiRender`.
`PushLayer` вставляет слой перед оверлеями, `PushOverlay` — в конец. `OnAttach` вызывается сразу при добавлении,
поэтому в нём уже доступны `Application::Get().GetRendererBackend()` и окно.

## События

`EventManager` — отложенная очередь. `QueueEvent(Scope<Event>)` кладёт событие, `DispatchEvents()` в конце кадра
раздаёт его подписчикам. Подписка типизирована:

```cpp
m_EventManager->Subscribe(EventHandler<WindowResizeEvent>(VEGA_BIND_EVENT_FN(OnWindowResize)));
```

Опционально можно подписаться на конкретный `EventId` и указать отписку после первого срабатывания.
Отписка ищет обработчик по `target_type().name()` — для лямбд это ненадёжно.

## Менеджеры

`Manager` — интерфейс с единственным `OnDetach()`. Менеджеры регистрируются в приложении по строковому имени
и достаются с приведением типа:

```cpp
Application::Get().AddManager("StaticMeshManager", CreateRef<StaticMeshManager>());
auto meshes = StaticRefCast<StaticMeshManager>(Application::Get().GetManager("StaticMeshManager"));
```

Сейчас менеджеры создаёт `EditorLayer::OnAttach`. `StaticMeshManager` хранит все меши в общих вершинном и индексном
`RenderBuffer` и выдаёт смещения (`StaticMeshManagerMeshInfo`); `BindMesh(name)` биндит буферы для отрисовки.

## Сцена (EnTT)

- `Scene` владеет `entt::registry` и списком `SceneSystems::SceneSystem`.
- `Entity` — лёгкий хэндл `{entt::entity, Scene*}`, копируется по значению. API: `AddComponent`, `GetComponent`,
  `GetConstComponent`, `HasComponent`, `RemoveComponent`.
- `CreateEntity(name, parent)` добавляет `NameComponent` и `HierarchyComponent`; `CreateActor` дополнительно добавляет
  `TransformComponent`.
- Иерархия — интрузивный список в `HierarchyComponent` (`Parent`, `FirstChild`, `PrevSibling`, `NextSibling`,
  `ChildCount`); новый ребёнок вставляется в начало списка.
- **`GetComponent<TransformComponent>()` запрещён (`= delete`).** Трансформ читается через `GetTransform()`,
  а меняется только через `SetTransform*`, чтобы помечать сущность и её потомков `TransformDirtyComponent`.
- `TransformComponent::GetTransformMatrix()` возвращает локальную матрицу. Мировая матрица хранится в
  `WorldTransformComponent::Matrix`: он добавляется и удаляется вместе с `TransformComponent` (сигналы EnTT в
  конструкторе `Scene`) и пересчитывается в конце `Scene::OnUpdate` — после `OnUpdate` систем — для сущностей с
  `TransformDirtyComponent`, сверху вниз по иерархии; затем все dirty-флаги снимаются. Родитель без
  `TransformComponent` считается единичной матрицей, и через него пометка dirty не распространяется. Рендер читает
  только `WorldTransformComponent`.
- Системы реализуют `Destroy`, `OnUpdate(Scene*, Timestep)`, `OnRender(Scene*, const RenderView&)`.
  `Scene::OnUpdate` и `Scene::OnRender` вызывают соответствующий метод у всех систем в порядке добавления.
  `Destroy` вызывается из деструктора сцены.
- `CameraComponent { CameraProjection Projection; bool IsPrimary; }` — игровая камера, положение берётся из
  `WorldTransformComponent` (смотрит вдоль локальной −Z, масштаб игнорируется). `Scene::GetPrimaryCameraEntity`
  возвращает первую камеру с `IsPrimary`, `Scene::SetPrimaryCamera(entity)` оставляет основной только одну.
- `DestroyEntity` удаляет сущность вместе со всем поддеревом: корень отвязывается от родителя и соседей
  (`ChildCount` родителя уменьшается), затем потомки уничтожаются раньше предков. Хэндлы удалённых сущностей,
  сохранённые снаружи (например, выделение в редакторе), становятся невалидными — проверяй `registry.valid`.

## Плагины

`PluginLibrary` грузит `<PluginPath>/<PluginName>.dll` (только Windows). Если `IsNeedSetPluginGlobals = true`,
после загрузки вызывается экспортируемая функция `SetPluginGlobals(PluginData*)`: DLL получает собственную копию
статических переменных Vega, поэтому ей передаются указатель на `Application` и логгеры. Подробнее о Vulkan-плагине —
в [renderer.md](renderer.md).

## Логирование и ассерты

- Макросы ядра `VEGA_CORE_TRACE/INFO/WARN/ERROR/CRITICAL`, клиента — `VEGA_TRACE/…`. Форматирование в стиле
  `std::format` (`"{}"`); векторы и матрицы GLM печатаются через `operator<<`.
- `VEGA_CORE_ASSERT(cond)` / `VEGA_CORE_ASSERT(cond, msg)` и `VEGA_ASSERT` работают только при `_DEBUG`
  (логируют и вызывают `__debugbreak()`); в Release они пустые — не кладите внутрь выражения с побочными эффектами.

## Редактор

- `EditorLayer` — кастомный заголовок окна (`DrawGuiTitlebar`, см. «Окно и кастомный заголовок»), главное меню, вьюпорт игры,
  тестовая сцена с иерархией и одним мешем. Панели можно скрывать (меню Window) и выносить за пределы главного окна
  (см. «Вынесенные окна»).
- `SceneHierarchyPanel` — дерево сущностей.
- `EntityPropsPanel` — свойства выбранной сущности; отрисовщики компонентов регистрируются через
  `EntityPropsPanel::RegisterComponentDescription<T>({ .Name, .DrawFunc })`.
