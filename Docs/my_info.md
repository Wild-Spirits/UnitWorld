### Почему один буфер со слотами, а не по буферу на слот

По корректности варианты равны, разница в практических деталях. Почему я выбрал один буфер:

- **Одна аллокация `VkDeviceMemory` на шейдер вместо N.** Лимит `maxMemoryAllocationCount` бывает около 4096, а своего суб-аллокатора памяти у нас пока нет.
- **Один `vkMapMemory` на весь срок жизни шейдера.**
- **Задел на `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`.** Когда появятся group id, можно перейти на один descriptor set на группу вместо N, а слот кадра и группы передавать dynamic offset-ом в `vkCmdBindDescriptorSets`. Для этого все слоты должны лежать в одном `VkBuffer`.

Отдельные буферы, как у Kohi, немного проще по коду: смещение внутри буфера не зависит от номера кадра, а в отладчике (RenderDoc) буфер каждого кадра виден отдельно. Если тебе такой вариант ближе, переделать легко: `m_UniformBuffer` станет вектором на `framesInFlight` элементов с маппингом каждого, а в `SetupFrequencyState` и `ApplyFrequency` вместо `i * m_UniformBufferFrameStride` будет индекс буфера.





Я ничего не менял: прочитал документацию, ключевые исходники и собрал все `TODO`. Ниже предложения, самое полезное сначала.





## 1\. Навести порядок, пока проект маленький (дёшево, польза сразу)

* **Убрать сгенерированные файлы из git.** `\\\\\\\\\\\\\\\*.spv` и `imgui.ini` лежат в репозитории, а в [.gitignore](.gitignore) их нет. Поэтому после каждого запуска рабочее дерево «грязное», и в [pitfalls.md](Docs/ai/pitfalls.md) уже пришлось писать об этом отдельное предупреждение. Есть два варианта:

  * класть `.spv` в `build/ShaderCache/` вместо `Assets/`;
  * или добавить их в `.gitignore` и выполнить `git rm --cached`.
* **Включить предупреждения.** Флаги `/Wall /WX` объявлены в [CMakeLists.txt:37](CMakeLists.txt#L37), но не подключены. Полный `/Wall` у MSVC слишком шумный, лучше `/W4` (без `/WX` на первое время). Сейчас тестов нет и единственная проверка — сборка, поэтому предупреждения дадут хоть какой-то контроль. Пример того, что они ловят: неиспользуемая `const float model = 10;` в [Application.cpp:79](Vega/Source/Vega/Core/Application.cpp#L79).
* **Файлы вне сборки.** `SamplerManager.cpp` и `FrameBuffer.cpp` не входят в `SOURCES`. Их нужно либо подключить, либо удалить, чтобы не висели ловушкой.
* **Мелкая ошибка в** [**Application.cpp:153**](Vega/Source/Vega/Core/Application.cpp#L153)**.** `m\\\\\\\\\\\\\\\_GuiLayer->OnExternalViewportsRender()` вызывается без проверки на `nullptr`, хотя выше по коду `m\\\\\\\\\\\\\\\_GuiLayer` проверяется.







## 2\. Закрыть дыры в базовом цикле

* **Таймстеп.** Сейчас в `Application::Run` всегда `time = 0`. Нужен `Time::GetTime()` (через `glfwGetTime` или `std::chrono`) и передача `Timestep` в `Layer::OnUpdate` и `Scene::OnUpdate`. Без этого не сделать ни камеру, ни анимации.
* **`Scene::OnUpdate`** пустой и не вызывает системы. Это пара строк, по аналогии с `OnRender`.
* **`DestroyEntity`.** Удаление из интрузивного списка иерархии с рекурсивным удалением детей. Без этого редактор не может удалять объекты.
* **Мировые трансформы.** `TransformDirtyComponent` уже ставится, но его никто не обрабатывает. Нужна система, которая пересчитывает `WorldTransform` для «грязных» сущностей сверху вниз по иерархии.







## 3\. Камера и per-frame uniform-ы (главный следующий шаг рендера)

В [test.vert](Assets/Shaders/Source/test.vert) сейчас `gl\\\\\\\\\\\\\\\_Position = model \\\\\\\\\\\\\\\* pos`: нет ни view, ни projection, то есть 3D фактически нет. При этом запись данных в UBO в `VulkanShader::SetUniformBufferData` не реализована ([VulkanShader.cpp:312](VegaPlugins/VegaVulkanRenderer/Renderer/VulkanShader.cpp#L312)). Предлагаю такой порядок:

1. Uniform-буферы для частот per-frame и per-group в `VulkanShader`: кольцо по frames-in-flight и запись по смещению.
2. Per-frame UBO `{ view, proj }`.
3. `CameraComponent` или отдельная камера редактора: fly/orbit-управление во вьюпорте, aspect ratio от размера FrameBuffer вьюпорта.
4. Включить отсечение граней. Сейчас стоит `CullMode = kNone` из-за перевёрнутого viewport. Это решается через `frontFace = COUNTER\\\\\\\\\\\\\\\_CLOCKWISE` при отрицательной высоте viewport (у метода уже есть TODO «Add arg to select winding mode»).

Это даст первую настоящую 3D-сцену, и от неё будет понятно, что нужно RenderGraph.









## 4\. Технический долг в Vulkan-плагине

* **`vkQueueWaitIdle` и `vkDeviceWaitIdle` в горячих путях**: [VulkanRenderBuffer.cpp:115](VegaPlugins/VegaVulkanRenderer/Renderer/VulkanRenderBuffer.cpp#L115) при загрузке данных и [VulkanFrameBuffer.cpp:42](VegaPlugins/VegaVulkanRenderer/Renderer/VulkanFrameBuffer.cpp#L42) при уничтожении. Лучше сделать очередь отложенного удаления (deletion queue) по номеру кадра. Это же упростит ресайз вьюпорта: FrameBuffer можно будет пересоздавать без остановки GPU.
* **`VulkanSampler` игнорирует `SamplerProps`** ([VulkanSampler.cpp:15](VegaPlugins/VegaVulkanRenderer/Renderer/VulkanSampler.cpp#L15)). Задача маленькая и рядом с тем, что вы сейчас правите.
* **Пул дескрипторов без обработки переполнения** ([VulkanShader.cpp:78](VegaPlugins/VegaVulkanRenderer/Renderer/VulkanShader.cpp#L78)). Упадёт, когда материалов станет больше.
* **`VEGA\\\\\\\\\\\\\\\_CORE\\\\\\\\\\\\\\\_INFO` на каждое копирование буфера** ([VulkanRenderBuffer.cpp:129](VegaPlugins/VegaVulkanRenderer/Renderer/VulkanRenderBuffer.cpp#L129)). Его стоит понизить до `TRACE`.







## 5\. Ассеты и сериализация (средний срок)

* **Загрузка мешей из файлов.** Сейчас меш один и тестовый; подойдут `tinygltf` или `cgltf`, но это новая зависимость и решать вам. `StaticMeshManager` уже готов принимать данные.
* **Сериализация сцены** (YAML или JSON) и загрузчик `AppConfig.json`, который сейчас не читается.
* **`AssetManager`** с хэндлами вместо строковых имён. Параллельно можно уйти от `GetManager("строка")` к `GetManager<T>()` по типу: опечатка станет ошибкой компиляции, а не исключением в Release.







## 6\. Редактор

Gizmo для трансформов (ImGuizmo), выделение кликом во вьюпорте, контекстное меню «Create / Delete» в иерархии. Всё это опирается на пункты 2 и 3.

\---

**Что я бы сделал в первую очередь:** пункт 1 (час работы, убирает постоянный шум), затем таймстеп, `Scene::OnUpdate` и `DestroyEntity`, затем UBO и камеру. RenderGraph я бы отложил, пока не появятся хотя бы два прохода (например, depth prepass или shadow и основной): сейчас проектировать его не на чем, и [RenderGraph.hpp](Vega/Source/Vega/Renderer/RenderGraph/RenderGraph.hpp) рискует остаться абстракцией без пользователей.

Могу расписать подробный план по любому пункту.

