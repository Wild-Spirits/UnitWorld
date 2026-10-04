# Типовые задачи

## Добавить исходный файл

1. Создать `.hpp`/`.cpp` в нужном каталоге.
2. Добавить их в список `SOURCES` соответствующего `CMakeLists.txt` (`Vega/`, `Editor/` или
   `VegaPlugins/VegaVulkanRenderer/`). Пара заголовок + реализация пишется в одну строку, выровненную по колонке:

   ```cmake
   Source/Vega/Managers/SamplerManager.hpp                                 Source/Vega/Managers/SamplerManager.cpp
   ```

3. Перезапустить configure (`cmake --preset …`), если сборка не подхватила изменения сама.

Glob-ов нет: `.cpp`, отсутствующий в `SOURCES`, молча не компилируется — ошибка всплывёт только на линковке, когда
кто-то начнёт использовать его символы.
Заголовки вне `SOURCES` компилируются через `#include`, но не видны в проекте IDE.

## Добавить компонент сцены

1. `Vega/Source/Vega/Scene/Components/<Name>Component.hpp`, структура в `namespace Vega::Components`, поля в PascalCase:

   ```cpp
   namespace Vega::Components
   {
       struct StaticMeshComponent
       {
           std::string MeshName;
       };
   }    // namespace Vega::Components
   ```

2. Добавить заголовок в `SOURCES` Vega.
3. Использовать: `entity.AddComponent<Components::XxxComponent>(args…)`.
4. Чтобы компонент отображался в редакторе, зарегистрировать отрисовщик (сейчас это делается в
   `EditorLayer::OnAttach`):

   ```cpp
   EntityPropsPanel::RegisterComponentDescription<Components::XxxComponent>(
       EntityPropsPanelComponentDescription { .Name = "Xxx", .DrawFunc = DrawXxx });
   ```

Если компоненту нужна реакция на изменение трансформа — опирайтесь на `TransformDirtyComponent`, а не на прямой доступ
к `TransformComponent`.

## Добавить систему сцены

1. `Vega/Source/Vega/Scene/Systems/SceneSystem<Name>.hpp/.cpp`, класс в `namespace Vega::SceneSystems`,
   наследник `SceneSystem`, реализует `Destroy`, `OnUpdate(Scene*)`, `OnRender(Scene*)`.
2. GPU-ресурсы создавать в конструкторе через `Application::Get().GetRendererBackend()`, освобождать в `Destroy()`.
3. Обход сущностей — через `_Scene->GetRegistry().view<A, B>().each(...)`.
4. Подключить: `scene->AddSceneSystem(CreateRef<SceneSystems::SceneSystemXxx>())`.
5. Учтите: `Scene::OnUpdate(Timestep)` сейчас не вызывает `OnUpdate` систем.

## Добавить менеджер

1. Наследник `Manager` в `Vega/Source/Vega/Managers/`, реализует `OnDetach()` (освобождение GPU-ресурсов).
2. Добавить в `SOURCES` Vega.
3. Зарегистрировать: `Application::Get().AddManager("XxxManager", CreateRef<XxxManager>())`; имя должно быть
   уникальным (иначе ассерт).
4. Получать: `StaticRefCast<XxxManager>(Application::Get().GetManager("XxxManager"))`.

## Добавить слой

Наследник `Layer` с переопределением нужных `On*`; подключение — `PushLayer`/`PushOverlay` в конструкторе приложения.
Ресурсы, созданные в `OnAttach`, освобождаются в `OnDetach`.

## Добавить шейдер

1. GLSL в `Assets/Shaders/Source/` (`#version 450`).
2. Описать атрибуты и uniform-ы в `ShaderConfig`, совпадающие с GLSL; per-draw данные — через push constants
   (≤ 128 байт). Подробности — [renderer.md](renderer.md#шейдеры).
3. Сгенерированный `.spv` появится рядом при первом запуске.

## Добавить метод в RendererBackend

См. [renderer.md](renderer.md#добавление-нового-методаресурса-в-бэкенд): интерфейс → реализация в Vulkan-плагине →
заглушка в OpenGL-бэкенде.
