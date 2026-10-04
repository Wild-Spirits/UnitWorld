# Рендерер

## Абстракция

`Vega/Source/Vega/Renderer` содержит только интерфейсы и платформонезависимые типы. Конкретная реализация
выбирается в рантайме через `RendererBackendApi` (`kVulkan` по умолчанию, `kOpenGL`, `kDirectX`).

`RendererBackend` (`RendererBackend.hpp`) отвечает за:

- жизненный цикл: `Init`, `Shutdown`, `WaitIdle` (ждать завершения всей отправленной на GPU работы),
  `OnWindowCreate`, `OnWindowDestroy`, `OnResize`;
- кадр: `FramePrepareWindowSurface` → `FrameCommandListBegin` → … → `FrameCommandListEnd` → `FrameSubmit` →
  `FramePresent`; внутри — `BeginRendering(offset, size, framebuffer)` / `DrawIndexed` / `EndRendering`;
- фабрики ресурсов: `CreateShader`, `CreateTexture` (в т.ч. из файла через stb_image), `CreateSampler`,
  `CreateFrameBuffer`, `CreateRenderBuffer`, `CreateImGuiImpl`, `CreateImGuiTextureWrapper`,
  `CreateImGuiFrameBufferWrapper`.

Код вне бэкенда создаёт ресурсы только через эти фабрики и работает с ними через абстрактные интерфейсы
(`Ref<Texture>`, `Ref<Shader>`, …). Доступ к бэкенду: `Application::Get().GetRendererBackend()`.

| Ресурс | Создание | Освобождение |
| --- | --- | --- |
| `Texture` | `CreateTexture(name, props[, data])`, `CreateTexture(name, path, props)` | `OnDetach()` |
| `Sampler` | `CreateSampler(name, props)` | `OnDetach()` |
| `Shader` | `CreateShader(config, { stages… })` | `OnDetach()` |
| `FrameBuffer` | `CreateFrameBuffer(props)` | `Destroy()` |
| `RenderBuffer` | `CreateRenderBuffer(props)` — vertex/index/uniform/staging/read/storage | см. интерфейс |
| ImGui-обёртки | `CreateImGuiTextureWrapper`, `CreateImGuiFrameBufferWrapper` | `OnDetach()` |

GPU-ресурсы **освобождаются явно**, деструкторы `Ref` этого не делают. Освобождайте их в `OnDetach` слоя/менеджера
или в `Destroy` системы сцены — до `RendererBackend::Shutdown`.

## Бэкенды

### Vulkan (`VegaPlugins/VegaVulkanRenderer`) — основной

- Собирается в `VegaVulkanRenderer.dll`. `RendererBackend::CreateVulkanRendererBackend()` грузит DLL через
  `PluginLibrary`, вызывает `SetPluginGlobals(PluginData*)` (передаёт `Application*` и логгеры в копию статиков
  внутри DLL), затем `CreateRendererBackend(Ref<RendererBackend>*)`. Обе функции объявлены `extern "C"` в
  `VegaVulkanPlugin.hpp` с макросом экспорта `VEGA_VULKAN_RENDERER_API`.
- Использует **dynamic rendering** (`vkCmdBeginRendering`), без `VkRenderPass`.
- Ресурсы на кадр (семафоры, фенсы, staging-буферы, command buffers) — по `VulkanSwapchain::GetMaxFramesInFlight()`.
- Внутренние классы: `VulkanDeviceWrapper` (instance/physical/logical device, очереди), `VulkanSwapchain`,
  `VulkanTexture` (хранит текущий layout, переходы через `TransitionImageLayout`), `VulkanSampler`,
  `VulkanFrameBuffer`, `VulkanRenderBuffer`, `VulkanShader` (пайплайн + дескрипторы), `ImGui/*`.
- Доступ к Vulkan-контексту внутри плагина: `VulkanRendererBackend::GetVkRendererBackend()` →
  `GetVkContext()`, `GetVkDeviceWrapper()`, `GetCurrentGraphicsCommandBuffer()`, `GetCurrentFrameIndex()`.
- Проверка результатов Vulkan-вызовов — макрос `VK_CHECK(expr)` из `Renderer/VulkanBase.hpp`.

### OpenGL (`Vega/Source/Platform/OpenGL`) — заглушка

Встроен в статическую библиотеку Vega, не плагин. Реализованы инициализация GLEW и ImGui; большинство методов
`RendererBackend` пустые. Не тратьте время на поддержку паритета, если задача явно этого не требует.

### DirectX — не реализован

`RendererBackend::Create` возвращает `nullptr`.

## Шейдеры

- Исходники GLSL (`#version 450`) лежат в `Assets/Shaders/Source/`. Путь к стадии задаётся в `ShaderStageConfig::Path`
  относительно корня репозитория.
- `VulkanShader` компилирует GLSL **в рантайме** через `shaderc` при каждом создании шейдера и записывает результат
  рядом с исходником как `<file>.spv`. Поэтому `*.spv` в репозитории — сгенерированные артефакты, которые меняются
  после запуска редактора.
- Ошибки компиляции выводятся через `VEGA_CORE_ERROR` в консоль.

### Описание шейдера (`ShaderConfig`)

```cpp
rendererBackend->CreateShader(
    ShaderConfig {
        .Name = "SceneSystemStaticMeshDraw",
        .Attributes = { ShaderAttributeType::kFloat3, ShaderAttributeType::kFloat2 },   // = layout(location=N) in
        .UniformsPerFrame = { { .Name = "view", .Size = sizeof(glm::mat4), .Type = ShaderUniformType::kMatrix4 },
                              { .Name = "proj", .Size = sizeof(glm::mat4), .Type = ShaderUniformType::kMatrix4 } },
        .UniformsPerGroup = { { .Name = "albedoTexture", .Type = ShaderUniformType::kTexture2d },
                              { .Name = "albedoSampler", .Type = ShaderUniformType::kSampler2d } },
        .CullMode = FaceCullMode::kNone,
    },
    { ShaderStageConfig { .Type = ShaderStageConfig::ShaderStageType::kVertex,   .Path = "Assets/Shaders/Source/test.vert" },
      ShaderStageConfig { .Type = ShaderStageConfig::ShaderStageType::kFragment, .Path = "Assets/Shaders/Source/test.frag" } });
```

Атрибуты и uniform-ы в `ShaderConfig` должны совпадать с объявлениями в GLSL — рефлексии нет.

### Частоты обновления (`ShaderUpdateFrequency`)

| Частота | Хранение в Vulkan | Применение |
| --- | --- | --- |
| `kPerFrame` | отдельный descriptor set (UBO/текстуры/сэмплеры) | один набор на кадр |
| `kPerGroup` | descriptor set, до `MaxGroups` наборов на кадр | материалы/группы объектов |
| `kPerDraw` | push constants (гарантировано ≤ 128 байт) | данные конкретного вызова отрисовки |

Номера descriptor set назначаются по порядку **присутствующих** частот: если per-frame uniform-ов нет, per-group
окажется в `set = 0`. В `test.vert` per-frame UBO `{ view, proj }` — `set = 0`, per-group текстура/сэмплер — `set = 1`.

Внутри set частоты все не-текстурные uniform-ы упакованы в **один** UBO на `binding = 0`, в порядке объявления в
`ShaderConfig`, со смещениями по `Size * max(1, ArrayLength)`. Раскладка должна совпадать с `std140` в GLSL
(выравнивание `vec3`/массивов — на совести автора конфига). Текстуры и сэмплеры идут следом с `binding = 1, 2, …`
в порядке объявления.

UBO per-frame и per-group лежат в одном host-coherent буфере шейдера, постоянно замапленном через
`VulkanRenderBuffer::MapMemory`. Буфер разбит на слоты по frames-in-flight (`[кадр 0: perFrame|perGroup][кадр 1: …]`),
descriptor set-ы тоже выделены по одному на frame-in-flight. `SetUniformBufferData` пишет в CPU-копию блока, а
`ApplyFrequency` копирует её в слот текущего кадра и биндит set, поэтому **`Set*` должны идти до `ApplyFrequency`**.
Группа пока одна на шейдер (`MaxGroups` влияет только на размер пула): разные данные per-group в пределах одного кадра
перезапишут друг друга.

Типичная последовательность на вызов отрисовки:

```cpp
shader->Bind();
// Once per frame
shader->SetUniformBufferData("view", view, ShaderUpdateFrequency::kPerFrame);
shader->SetUniformBufferData("proj", projection, ShaderUpdateFrequency::kPerFrame);
shader->ApplyFrequency(ShaderUpdateFrequency::kPerFrame);
// Per draw
shader->SetUniformTexture("albedoTexture", texture, ShaderUpdateFrequency::kPerGroup);
shader->SetUniformSampler("albedoSampler", sampler, ShaderUpdateFrequency::kPerGroup);
shader->ApplyFrequency(ShaderUpdateFrequency::kPerGroup);
shader->SetUniformBufferData("perDrawUbo.model", transform.GetTransformMatrix(), ShaderUpdateFrequency::kPerDraw);
staticMeshManager->BindMesh(meshName);
rendererBackend->DrawIndexed(indexCount);
```

Вся система uniform-ов в активной разработке (`SetUniformBufferData` помечен WIP).

## RenderGraph

`Renderer/RenderGraph/` — заготовка: `RenderGraph` (слои параллельных узлов + `m_MainSink`), пустой `RenderGraphNode`,
`MaterialRenderGraphNode`. Сейчас его роль временно выполняют `RendererBackend::TmpRendergraphExecute()` и прямые
вызовы из `SceneSystemStaticMeshDraw::OnRender`. Набросок задуманной схемы — `Docs/rendergraph.drawio`.

## Добавление нового метода/ресурса в бэкенд

1. Добавить чистый виртуальный метод/интерфейс в `Vega/Source/Vega/Renderer/`.
2. Реализовать в `VegaPlugins/VegaVulkanRenderer/Renderer/` (новые файлы — в `SOURCES` плагина).
3. Добавить пустую реализацию в `OpenGlRendererBackend`, иначе Vega не скомпилируется (класс станет абстрактным).
4. Пересобрать плагин и Editor — интерфейс общий, ABI DLL должен совпадать с Vega.
