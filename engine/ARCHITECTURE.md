# eng 引擎：项目结构、技术与渲染方法

`eng` 是本仓库为《日渐冷清的书店》（[`../emptying-bookshop`](../emptying-bookshop)）编写的离线影片渲染引擎，用 C++20 和 Vulkan 1.2 实现，共约 5,500 行代码：头文件约 950 行，实现约 3,950 行，着色器约 600 行。

它不是实时游戏引擎，而是一条“逐帧拍摄”的流水线：

1. 程序搭建场景，并为每个时刻摆好姿态；
2. 引擎用 Vulkan 在屏幕外渲染这一帧，再读回 CPU；
3. CPU 在图像上叠加 2D 文字和图形；
4. 帧数据经管道交给 ffmpeg，编码成 H.264 MP4；配乐由引擎内置的合成器离线生成。

本文按以下顺序介绍：设计取向、目录结构、技术栈、核心数据模型、渲染器（初始化、资源上传、每帧流程、各通道算法）、2D 叠加层与文字、视频与音频输出、合成器，最后列出局限与注意事项。用法示例和构建依赖见 [`README.md`](README.md)。

---

## 1. 设计取向

- **离线、无窗口。** 不创建交换链（swapchain），也不需要显示器。任何支持 Vulkan 1.2 的设备都能运行，包括 Mesa 的软件光栅器 llvmpipe，所以服务器上也能渲染。
- **确定性。** 引擎内部没有随时间累积的状态（没有物理模拟，也不依赖上一帧的时域累积）。同一场景姿态总是渲染出同一幅画面，所以影片可以从任意时刻开始渲染、只渲染一段，或者只输出几张静帧。
- **同步、一次一帧。** 每帧录制一个命令缓冲区，提交后用 fence 等待 GPU 完成，再把像素读回。这种做法简单可靠，代价是 CPU 和 GPU 不能并行工作。
- **API 仿照 three.js。** 影片原来是用 three.js 写的，所以场景图、材质参数（`roughness`、`metalness`、`emissive`、`side`、`transparent`……）、灯光衰减公式和泛光权重都尽量与 three.js 保持一致，方便移植。
- **对象归引擎所有。** 节点、几何体、材质和纹理都存放在静态对象池里，生命周期与程序相同。工厂函数返回裸指针，调用者不需要、也不应该释放它们。

---

## 2. 目录结构

```
engine/
├── CMakeLists.txt        构建静态库 eng；用 glslc 把着色器编译成 SPIR-V 并嵌入库中
├── README.md             功能概览、用法示例、构建依赖
├── ARCHITECTURE.md       本文
├── include/eng/          公共头文件（eng.h 一次性包含全部）
│   ├── math.h            vec2/3/4、quat、mat3/4、Color（sRGB ↔ 线性）
│   ├── scene.h           场景图：Node、Mesh、InstancedMesh、PointLight、Camera、Scene
│   ├── geometry.h        Geometry 数据结构与 geo:: 形状构建函数
│   ├── curve.h           参数曲线：二次贝塞尔、Catmull-Rom、函数曲线；按弧长采样
│   ├── material.h        Material、Texture；standard()/basic() 工厂
│   ├── renderer.h        Renderer、RendererOptions、PostSettings（后期参数）
│   ├── canvas.h          CPU 2D 光栅器：Canvas、Mask、合成到帧
│   ├── text.h            文字：字体注册、TextStyle、分行排版、绘制
│   ├── video.h           VideoWriter（ffmpeg 管道）、savePng、Audio 缓冲
│   └── synth.h           离线合成器：乐器、和弦、闪避（ducking）、限幅
├── src/
│   ├── math.cpp          矩阵求逆、法线矩阵、四元数转欧拉角
│   ├── scene.cpp         节点、材质、纹理的对象池与工厂；世界矩阵更新；lookAt
│   ├── geometry.cpp      各种形状的构建、耳切三角化、挤出、边线、合并
│   ├── curve.cpp         弧长表、平行移动标架、Catmull-Rom
│   ├── renderer.cpp      全部 Vulkan 代码（约 1,480 行，引擎的核心）
│   ├── paper.cpp/.h      程序化生成可平铺的纸纤维纹理
│   ├── canvas.cpp        覆盖率光栅化、预乘 alpha 混合、高斯模糊、背景模糊
│   ├── text.cpp          FreeType + HarfBuzz：字体实例、回退、塑形、字形缓存、断行
│   ├── video.cpp         ffmpeg 管道、PNG、WAV 读写、重采样、混音
│   └── synth.cpp         乐器合成、Freeverb 混响、和弦解析、闪避曲线、限幅器
└── shaders/              GLSL 450
    ├── common.glsl       场景着色器共用的 UBO、绘制记录 SSBO、标志位定义
    ├── mesh.vert         网格（含实例化）顶点着色器
    ├── points.vert       点云顶点着色器
    ├── sprite.vert       始终朝向相机的公告板四边形
    ├── shadow.vert       阴影深度（仅位置）
    ├── mesh.frag         所有场景物体共用的片元着色器：光照、阴影、纸张、雾
    ├── sky.vert/.frag    天空渐变（全屏三角形 + 射线与球求交）
    ├── fullscreen.vert   后期通道用的全屏三角形
    ├── bright.frag       泛光亮部提取
    ├── blur.frag         可分离高斯模糊（泛光）
    ├── ssao.frag         屏幕空间环境光遮蔽
    ├── aoblur.frag       保边（双边）AO 模糊
    ├── dofpre.frag       景深预处理：半分辨率颜色 + 弥散圆
    ├── dofgather.frag    景深聚集：前景、背景分开收集
    └── final.frag        合成：AO、景深、泛光、曝光、ACES、sRGB、调色、抖动
```

---

## 3. 技术栈

| 方面 | 技术 | 说明 |
|---|---|---|
| 语言 | C++20 | 用指定初始化器写参数结构，如 `standard({.color = 0xc98e6c, .roughness = 0.8f})` |
| 构建 | CMake ≥ 3.20，通常配合 Ninja | 产出静态库 `eng`；影片用 `add_subdirectory` 引入 |
| 图形 API | Vulkan 1.2 | 屏幕外渲染；只用一个图形队列；可选开启 Khronos 校验层 |
| 着色语言 | GLSL 450 → SPIR-V | 构建时由 `glslc -O -mfmt=c` 编译成 C 数组 `*.inc`，再 `#include` 进 `renderer.cpp`。运行时不读任何着色器文件 |
| 字体塑形 | HarfBuzz | 处理字距、连字和组合字符；支持可变字体的字重轴 |
| 字体光栅化 | FreeType | 不做 hinting，8 位抗锯齿，水平方向 1/4 像素定位 |
| 图片 | libpng（simplified API） | 导出静帧 |
| 视频 | ffmpeg 子进程（`popen`） | rawvideo RGBA → BT.709 YUV420 → libx264；可附带 AAC 音轨 |
| 音频 | 自写 | 读写 16 位 PCM WAV、加窗 sinc 重采样、等功率声像、合成器、混响、限幅 |

`CMakeLists.txt` 为每个着色器添加一条自定义命令，依赖它自己和 `common.glsl`。所以修改着色器后，`ninja` 会自动重新编译并重新链接。新增着色器时，要把它加进 `ENG_SHADERS` 列表，并在 `renderer.cpp` 顶部 `#include` 对应的 `.inc`。

---

## 4. 核心数据模型

### 4.1 数学约定（`math.h`）

- 矩阵按列主序存储（`m[col*4 + row]`）。世界坐标系是右手系，+y 向上，相机看向自身的 −z 方向。
- 欧拉角按 XYZ 顺序应用（矩阵为 Rx·Ry·Rz），与 three.js 默认一致。
- 投影矩阵的深度范围是 Vulkan 的 0..1，裁剪空间 y 向上。场景和阴影通道使用高度为负的视口来翻转 y，后期通道不翻转（uv 原点在左上角）。最终读回的图像第一行就是画面顶部。
- `Color` 内部存储线性值。`Color::hex(0xffcf5a)` 把 sRGB 十六进制颜色转换成线性值，所有光照计算都在线性空间进行。

### 4.2 场景图（`scene.h`）

```
Node（位置 / 欧拉旋转 / 缩放、visible、castShadow、receiveShadow、renderOrder、frustumCulled）
 ├── Mesh           几何体 + 材质（三角形）
 │    └── InstancedMesh   同一网格的多个副本：每个实例有自己的矩阵，可选单独颜色
 ├── Mesh(Lines)    线段（lines()）
 ├── Mesh(Points)   点云（pointCloud()）
 ├── Mesh(Sprite)   朝向相机的单位四边形（sprite()），大小由节点缩放决定
 └── PointLight     点光源（颜色、强度、作用距离、衰减指数）
Scene : Node        另含 sky（渐变天空）、fog（雾）、hemi（半球光）、sun（唯一投射阴影的平行光）、
                    fill（无阴影补光）、exposure（曝光）、paper（纸张外观的全局参数）
Camera              position / target / up、fov（垂直视角，单位度）、aspect、near / far
```

每帧开始时 `Renderer::render` 会调用 `scene.updateWorld()`，从根节点递归计算世界矩阵。`Node::computeWorld()` 不依赖缓存，任何时候都能算出当前的世界矩阵，供 `lookAt` 和屏幕投影使用。

### 4.3 几何体（`geometry.h`）

- `Geometry` 保存独立的 `positions`、`normals`、`uvs` 数组，以及可选的 32 位 `indices`；拓扑可以是三角形、线或点。
- **绘制范围** `drawStart` / `drawCount`：只绘制索引（或顶点）的一段。`tube` 和 `ribbon` 生成的索引沿曲线方向排列，所以逐帧增大 `drawCount` 就能做出“沿路径逐渐画出”的动画，不需要重建几何体。
- `computeBounds()` 计算包围球，用于视锥剔除和透明物体排序。
- 构建函数（`geo::`）：box、roundedBox、plane、circle、sphere（可取部分）、cylinder / cone、capsule、torus、icosahedron（可细分）、沿曲线的 tube / ribbon、edges（按二面角阈值提取边线）、lineSegments、points、flat（每个三角形使用独立的面法线，形成纸张折痕）、extrude（对简单多边形做耳切三角化后挤出，正面和切口可以分开取，以便使用不同材质）、merge（把多个几何体按各自的矩阵合并成一个）。

### 4.4 曲线（`curve.h`）

`Curve` 基类用 200 段折线近似建立弧长表，提供 `pointAt(u)`（按弧长比例取点）和 `tangentAt(u)`。`frames()` 用**平行移动**（parallel transport）计算切线、法线、副法线，避免 Frenet 标架在拐点处翻转；闭合曲线会把首尾的扭转均匀分摊到各段。具体曲线有 `QuadraticBezier`、`CatmullRom`（带张力参数，可闭合）和 `FnCurve`（任意 lambda）。

### 4.5 材质与纹理（`material.h`）

| 字段 | 含义 |
|---|---|
| `shading` | `Standard`：有光照（漫反射 + GGX 高光 + 自发光）；`Basic`：不受光照的纯色 |
| `color`、`opacity`、`map` | 基础色、不透明度、可选的 sRGB 纹理（与颜色和 alpha 相乘） |
| `roughness`、`metalness`、`emissive`、`emissiveIntensity` | 与 three.js 的 `MeshStandardMaterial` 含义相同 |
| `transparent`、`depthWrite`、`depthTest`、`side` | 混合方式、深度状态、正面 / 背面 / 双面 |
| `fog` | 是否受雾影响 |
| `pointSize` | 点云的点大小，以 720p 下的像素计，随输出分辨率缩放 |
| `paper` | 0..1，纸张外观的强度，仅对 Standard 材质有效，详见 §5.4 |
| `user[4]` | 留给应用程序自由使用（例如记录淡入淡出前的原始不透明度） |

`Texture` 存储直通 alpha（straight alpha）的 RGBA8 像素，默认按 sRGB 解释。修改像素后要调用 `touch()` 递增版本号，渲染器下次绘制时会重新上传。

---

## 5. 渲染器（`renderer.cpp`）

### 5.1 初始化

1. **实例与设备。** 创建 Vulkan 1.2 实例；开启校验时加载 `VK_LAYER_KHRONOS_validation` 和 debug utils 回调。物理设备按“独立显卡 > 集成显卡 > 虚拟 GPU > CPU”的顺序选择，取第一个支持图形操作的队列族。
2. **多重采样。** 在颜色和深度都支持的采样数里，选不超过 `RendererOptions::samples` 的最大值（8 / 4 / 2，都不支持时退回 1）。
3. **可选特性。** 设备支持时才开启：采样器数组动态索引、各向异性过滤（最高 4×）、大尺寸点、独立混合（independent blend）。
4. **渲染目标**（W×H 为输出尺寸）：

   | 图像 | 格式 | 尺寸 | 用途 |
   |---|---|---|---|
   | `msaaColor` | RGBA16F，多重采样，transient | W×H | 场景 HDR 颜色 |
   | `msaaAux` | RGBA16F，多重采样，transient | W×H | 视空间法线（rgb）+ 沿视线的距离（a） |
   | `depth` | D32F，多重采样，transient | W×H | 深度缓冲 |
   | `hdr` / `gbuf` | RGBA16F | W×H | 上面两者解析（resolve）后的结果，供后期通道采样 |
   | `shadow` | D32F | `shadowMapSize`²（默认 2048²） | 太阳阴影贴图 |
   | `aoA` / `aoB` | RGBA16F | W/2×H/2 | AO 结果及模糊的中间结果 |
   | `dofHalf` / `dofBlur` | RGBA16F | W/2×H/2 | 景深的预处理结果与聚集结果 |
   | `bright` | RGBA16F | W/2×H/2 | 泛光亮部 |
   | `bloomH[5]` / `bloomV[5]` | RGBA16F | W/2 … W/32 | 五级泛光（水平、垂直两步） |
   | `out` | RGBA8 UNORM | W×H | 最终图像，拷贝回 CPU |
   | `paper` | RGBA8 + 完整 mip 链 | 512² | 纸纤维纹理（repeat 采样） |
   | `white` | RGBA8 | 1×1 | 空槽位的占位纹理 |

5. **渲染通道。** 场景通道有两个颜色附件（HDR 颜色和 aux）和一个深度附件，使用多重采样时再加两个解析附件。另有只写深度的阴影通道、输出 RGBA16F 的后期通道，以及输出 RGBA8 的最终通道。每个通道都设置了保守的外部子通道依赖，所以通道之间不需要额外插入屏障。
6. **描述符。** 整帧只用**一个场景描述符集**：

   | binding | 内容 |
   |---|---|
   | 0 | `FrameUBO`：各种矩阵、相机、太阳 / 补光 / 半球光、雾、阴影参数、天空、纸张参数、最多 4 个点光源 |
   | 1 | `Draws[]`（SSBO）：每次绘制一条 176 字节的记录（模型矩阵、法线矩阵、颜色、自发光、参数、标志位、纹理槽、首个实例的下标） |
   | 2 / 3 | 实例矩阵数组、实例颜色数组（SSBO） |
   | 4 | 阴影贴图（`sampler2DShadow`，带硬件比较） |
   | 5 | `textures[128]`：所有材质纹理共用一个大数组 |
   | 6 | 纸纤维纹理 |

   每次绘制只需推送 16 字节的 push constant（绘制记录的下标），并绑定顶点 / 索引缓冲区，不用切换描述符集。后期通道共用一个布局：源图像、5 级泛光、4 个辅助输入，外加最多 128 字节的 push constant。
7. **管线。** 天空、阴影（不剔除 / 剔除背面 / 剔除正面三种）和各个后期管线在初始化时创建。场景物体的管线**按需创建并缓存**，键由程序类型（mesh / points / sprite）、拓扑、剔除模式、是否混合、深度测试和深度写入组成。视口与裁剪矩形是动态状态。

### 5.2 资源上传

- **几何体**在第一次被绘制时上传。顶点被交错打包成 32 字节（位置 12 + 法线 12 + uv 8；缺少法线时默认 (0,1,0)，缺少 uv 时为 0），索引为 uint32。所有几何体放进若干块至少 32 MB 的 device-local 大缓冲区，按顺序分配。**上传之后不会再同步**：之后修改 `positions` 等数组不会生效，只有绘制范围可以逐帧改变。
- **纹理**在首次使用或版本号变化时上传。像素行按自下而上的顺序写入，使 v=0 对应图像底部（与 three.js 的 flipY 约定一致）。mip 链用 `vkCmdBlitImage` 逐级线性缩小生成。最多 128 张，首次上传后不能再改变尺寸或格式。
- 上传命令和本帧的渲染命令录在**同一个**命令缓冲区里，最后插入一个内存屏障，保证上传对顶点输入和着色器读取可见。
- 每帧的数据（UBO、绘制记录、实例矩阵和实例颜色）放在 host-visible、coherent 的缓冲区里，容量不够时按 2 倍扩容，并重写描述符。

### 5.3 每帧流程

```
CPU：scene.updateWorld() → camera.update() → 计算太阳的正交阴影相机 → 提取相机与阴影相机的视锥平面
     遍历场景图：
       · 不可见的节点连同子树一起跳过；最多收集 4 个点光源（多余的忽略）
       · 包围球与相机视锥相交检测（非实例化且 frustumCulled 的节点）；与阴影视锥相交检测（投射阴影的节点）
       · 完全透明、不写深度且不投射阴影的物体直接跳过
       · 为每个物体生成一条 DrawRec，追加实例矩阵和颜色，分入 opaque / transparent / casters 三个列表
     排序：不透明物体按 renderOrder → 管线键 → 由近到远；透明物体按 renderOrder → 由远到近
GPU（单个命令缓冲区）：
  ① 上传新的几何体和纹理
  ② 阴影通道        太阳视角的深度图
  ③ 场景通道        天空 → 不透明物体 → 透明物体；输出 HDR 颜色和法线/深度；MSAA 解析
  ④ 泛光            亮部提取 → 5 级可分离高斯模糊（各级分辨率逐级减半）
  ⑤ SSAO           半分辨率采样 → 水平、垂直两次保边模糊
  ⑥ 景深            半分辨率预处理（颜色 × AO，计算弥散圆）→ 37 个采样点的圆盘聚集
  ⑦ 合成            AO、景深、泛光 → 曝光 → ACES → sRGB → 调色 → 抖动 → RGBA8
  ⑧ 读回            out 图像 → host 缓冲区
提交并等待 fence → memcpy 到 pixels()
```

`Renderer::stats()` 返回上一帧的绘制次数、阴影绘制次数和三角形数，`--still` 输出静帧时会打印这些数字。

### 5.4 场景着色（`mesh.vert` / `mesh.frag`）

**顶点阶段。** 实例化绘制时，先应用实例矩阵，再应用节点的模型矩阵。法线会先除以实例各轴缩放的平方，再乘实例矩阵的 3×3 部分。在没有切变的前提下，这与逆转置矩阵等价。顶点着色器还输出两项供纸纹理使用：按世界尺度缩放的**物体空间坐标** `vObj`，以及物体空间法线。因为是物体空间坐标，纸纹会“贴”在移动的物体上；又因为乘了缩放，同一张纸在不同大小的物体上纤维粗细一致。

**光照模型**（只有 Standard 材质的三角形会计算光照；线、点、公告板永远不受光照）：

- 漫反射：Lambert 漫反射除以 π，并支持**环绕光**（wrap）：`NLd = clamp((N·L + w) / (1 + w))`，让光稍微绕过明暗交界线，模拟光透过薄纸的效果。
- 高光：
  - 法线分布用 GGX：`D = a² / (π((N·H)²(a²−1)+1)²)`，其中 `a = roughness²`；
  - 可见性项用高度相关的 Smith 函数：`V = 0.5 / (NL·√(a²+(1−a²)NV²) + NV·√(a²+(1−a²)NL²))`；
  - 菲涅尔项用 Schlick 近似，指数部分用球面高斯近似 `exp2((−5.55473·VH − 6.98316)·VH)`；
  - 金属度：`f0 = mix(0.04, base, metalness)`，漫反射颜色为 `base·(1−metalness)`。
- **高光抗锯齿**：粗糙度至少为 0.0525，并加上法线在相邻像素间的最大变化量（`dFdx` / `dFdy`），防止细小曲面上的高光闪烁。
- 光源：太阳（乘以阴影系数）、补光（强度为 0 时跳过）、最多 4 个点光源，以及半球光。
  - 点光源衰减为 `1/d^decay`；设置了 `distance` 时，再乘窗口函数 `(1 − (d/distance)⁴)²`。公式与 three.js 相同。
  - 半球光：`mix(ground, sky, 0.5·N.y + 0.5) · diffuse / π`，只影响漫反射。
- 最后加上自发光，再按视线方向的深度用 `smoothstep(near, far)` 混入雾色。
- 第二个输出目标写入视空间法线和沿视线的距离，供 SSAO 和景深使用。

**太阳阴影：**
- 阴影贴图使用正交投影，范围由 `sun.shadow.left/right/top/bottom/near/far` 指定。
- `Side::Front` 材质只把**背面**画进阴影贴图，从而避免受光面出现阴影失真（shadow acne）。`Side::Back` 材质正好相反，只画正面。双面材质不做剔除。
- 采样时，先沿法线偏移 `normalBias`，再加上深度偏移 `bias`。然后在半径为 `radius` 个纹素的圆盘内取 16 个硬件比较采样，采样点按 Vogel 螺旋分布（黄金角 2.39996），整体旋转角由每个像素的交错梯度噪声（interleaved gradient noise）决定，用来打散带状条纹。
- 阴影贴图范围之外按“受光”处理（边框颜色为白色）。

**纸张外观**（`paper > 0`）：
1. **纤维纹理**由 `paper.cpp` 在 CPU 上生成：512×512，四方连续可平铺。
   - R 通道是颜色起伏，由三部分组成：3 个倍频（4 / 9 / 20 格）的周期值噪声，模拟纸浆的“云絮”；约 n²/90 条随机方向、略带弯曲、两端渐细、有明有暗的短纤维；细小颗粒。
   - G 通道是起伏高度，B 通道是颗粒。
   - 生成时所有计算都按环绕边界处理，所以纹理平铺时没有接缝。
2. **三平面映射**：沿物体空间的三个轴分别采样，按 |法线|⁴ 加权混合。一个纹理周期覆盖 `Scene::paper.tile` 个世界单位。
3. **斑驳**：`base *= 1 + (R − 0.5)·2·mottle·paper`。
4. **起伏**：把高度 `G·relief·0.01·paper` 当作凹凸贴图。实现采用基于屏幕空间导数的**表面梯度法**（Mikkelsen 的 “Bump Mapping Unparametrized Surfaces on the GPU”），不需要切线。
5. **环绕光**：`wrap = Scene::paper.wrap · paper`，作用方式见上文的光照模型。

**天空**（`sky.vert` / `sky.frag`）：场景通道第一个绘制的是一个全屏三角形。它不做深度测试，也不写深度，之后绘制的物体都会覆盖在它上面。片元着色器用 `invViewProj` 重建视线，求它与以原点为中心、半径为 `sky.radius` 的球的交点，再按交点高度在地平线色和天顶色之间插值。`paper.sky > 0` 时，天空上也会叠加纸纤维（以视线方向做三平面映射，整个天空没有接缝），看上去像一块手绘的背景布。天空在 aux 中写入一个约定的“无穷远”距离 60000；后续通道遇到距离大于 50000 的像素就当作天空，不对它做 AO，也不让它模糊。

**透明物体。** 透明物体使用直通 alpha 混合（`SRC_ALPHA, ONE_MINUS_SRC_ALPHA`），以包围球中心为准由远到近排序。双面的透明三角形网格要画两遍，先画背面再画正面，这样同一个物体的远侧会先被画出。设备支持独立混合时，透明物体不写入 aux 目标，所以 AO 和景深只看不透明表面。不支持时，透明物体会混合进 aux，只会让透明物周围的 AO 和对焦变柔和一些。

### 5.5 后期处理

**泛光**（`PostSettings::bloom*`；强度为 0 时整段跳过）：
- 在半分辨率下按亮度阈值提取亮部（过渡带宽 0.01）。
- 然后做 5 级可分离高斯模糊，半径依次为 6 / 10 / 14 / 18 / 22 个采样，σ 取半径的 1/3。每一级以上一级的结果为输入，分辨率减半。
- 合成时各级的权重为 `3·strength·lerp(fᵢ, 1.2−fᵢ, radius)`，其中 fᵢ 依次为 1.0、0.8、0.6、0.4、0.2，与 three.js 的 UnrealBloomPass 一致。`radius` 越大，粗糙的级别权重越高。泛光在线性 HDR 空间、曝光之前叠加。

**环境光遮蔽**（`aoStrength`、`aoRadius`、`aoPower`）：
- 在半分辨率下，从 aux 缓冲区重建每个像素的视空间位置。
- 在法线方向的半球内取 14 个采样点：采样方向按黄金角螺旋排列并用噪声旋转，采样距离在半径内按伪随机分布。
- 采样半径随距离缓慢增大（`R·(1 + 0.012·d)`），偏移量也随距离增大（`0.02 + 0.0015·d`）。
- 用 `smoothstep(R / |Δd|)` 做距离检查，避免远处的前景物体在背景上投下 AO 光晕。结果再取 `aoPower` 次幂。
- 接着做两次 9 抽头的双边模糊：深度差超过 `0.03·d + 0.04` 的样本权重迅速下降，所以 AO 不会跨过物体轮廓渗出。
- AO 在合成阶段乘到整个颜色上，但很亮的像素（亮度在 1.5 到 4 之间平滑过渡）会被豁免，让发光的窗户和光线保持明亮。

**景深**（`dofFocus`、`dofAperture`、`dofMaxRadius`；半径参数以 1080p 下的像素为单位，引擎会按实际分辨率缩放）：
- 弥散圆为 `coc = aperture·(d − focus)/d`，限制在 ±maxR 以内。负值表示在焦平面之前。天空的弥散圆为 0。
- 预处理：在半分辨率下输出乘过 AO 的颜色，alpha 通道存弥散圆，取对应的 4 个全分辨率像素里绝对值最大的那个，这样细小的前景物体不会丢失。
- 聚集：在半径为 maxR 的圆盘上，按 1 + 6 + 12 + 18 共 4 圈取 37 个样本，**分开计算前景和背景**。
  - 背景样本的权重用 `min(样本 coc, 中心 coc)`，所以模糊的背景不会盖到更清晰的前景上。
  - 前景样本的权重用 `−coc`，前景覆盖率为 `min(1, fgW·π/样本数)`，所以模糊的前景可以溢出到后面清晰的物体上。
- 合成：混合系数为 `k + a − k·a`，其中 `k = smoothstep(0.1, 1, |coc|)`，a 为前景覆盖率。
- 影片的用法：对焦距离取相机到目标点的距离，光圈随镜头远近从 4 增大到 16。远景因此模糊得很强，看起来像微缩模型（移轴效果）。

**合成与调色**（`final.frag`，写入 RGBA8）：
1. HDR 颜色 × AO → 与景深结果混合 → 加上泛光；
2. × `scene.exposure` → ACES 电影色调映射（Stephen Hill 的 RRT/ODT 拟合，输入先除以 0.6，与 three.js 相同）→ sRGB 编码；
3. 以下步骤在显示空间（sRGB）里依次进行：
   - **聚焦遮罩**：以 `focusCenter` 为圆心、`focusRadius` 为半径（按宽高比修正为正圆）。圆外区域在 r 到 1.35r+0.02 之间平滑过渡，最多去色 85%、压暗 72%，强度乘以 `focusAmount`。影片用它把观众的视线引向画面的某一处。
   - 饱和度；
   - 以 0.5 为中心的对比度；
   - 增益 `gain`；
   - 暗部提升 `c += lift·(1 − c)`；
   - 暗角 `1 − vignette·|uv − 0.5|²·1.5`；
   - 闪白 `flash`；
   - 黑场 `fade`；
4. 加上 ±0.5/255 的哈希噪声作为抖动，防止量化到 8 位时出现色带。

---

## 6. 2D 叠加层与文字（CPU）

3D 画面读回之后，标签、字幕、标题卡等 2D 内容都在 CPU 上画到 RGBA8 帧上。同一套 2D 代码也能画进 `Texture`，做成场景里的招牌。

### 6.1 Canvas（`canvas.h/.cpp`）

- 像素格式为 float 的**预乘** RGBA。混合在 sRGB 空间进行，与浏览器合成 CSS 的方式一致，所以从网页版移植过来的界面观感不变。
- 抗锯齿用解析覆盖率实现：矩形按像素的覆盖面积计算；圆角矩形、圆和环由有向距离场计算覆盖率（`clamp(0.5 − d)`）。
- `Mask` 是一张覆盖率蒙版（例如文字的轮廓），可以做高斯模糊（三次盒式模糊近似），用来生成投影和发光。
- 合成到帧：`compositeOnto` 把一个 Canvas 图层叠加到帧上；`backdropBlur` 在一个圆角矩形内模糊帧本身，相当于 CSS 的 `backdrop-filter: blur()`；`fillOnto` 直接在帧上填色。

### 6.2 文字（`text.h/.cpp`）

- **字体族**：Sans、Serif、SerifItalic（拉丁文字），以及 CJK、CJKBold、CJKSerif、CJKSerifBold（中文）。每个字体族用 `setFontFile` 注册一个字体文件。
- **字体实例**是“字体族 + 字重”的组合，同时持有 FreeType face 和 HarfBuzz font。对可变字体，两者都设置 `wght` 轴，保证塑形和光栅化使用同一字重。中文字重 ≥ 600 时，如果注册了粗体文件就改用粗体文件。
- **逐字回退**：主字体缺字时依次换用对应的字体族，Sans ↔ CJK，Serif → CJKSerif，中文字体缺拉丁字母时用对应的拉丁字体。中英混排因此可以写成一个 span。相邻且使用同一字体的字符会合并成一段，交给 HarfBuzz 塑形。
- **光栅化**：FreeType 不做 hinting，用 8 位灰度抗锯齿。水平方向按 1/4 像素定位（把轮廓平移 0、16、32 或 48 个 26.6 定点单位），垂直方向基线对齐到整像素。字形位图按（实例、字形、字号、亚像素位置）缓存。
- **排版**：
  - 切分记号：拉丁文按“单词 + 后随空格”切分，中文每个字是一个记号。
  - 避头尾规则：，。、；：！？）」』”’》〉…— 以及 `) , . ; : ! ? %` 不能出现在行首；（「『“‘《〈 以及 `(` 不能出现在行尾。
  - 贪心填行，支持左对齐和居中。框宽度取最长一行的宽度，换过行时取最大宽度。
  - 支持字间距、全大写、行高和删除线。

---

## 7. 输出（`video.h/.cpp`）

- **VideoWriter** 启动以下 ffmpeg 子进程，并通过标准输入写入原始 RGBA 帧：
  ```
  ffmpeg -f rawvideo -pix_fmt rgba -s WxH -framerate N -i - [-i 音轨.wav]
         -vf scale=out_color_matrix=bt709:out_range=tv,format=yuv420p
         -c:v libx264 -preset P -crf C
         -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv
         [-c:a aac -b:a 160k -ar 48000 -ac 2] -movflags +faststart out.mp4
  ```
  颜色按 BT.709 有限范围转换，并在流里写入相应标记，使各种播放器解码出的颜色一致。`close()` 会检查 ffmpeg 的退出码。
- **savePng** 用 libpng 的 simplified API 写出 RGBA 静帧。
- **Audio** 以 float 存储样本，多声道交错排列：
  - 读写 16 位 PCM WAV；
  - 重采样使用 Blackman 窗 sinc 插值，截止频率取两个采样率中较低的奈奎斯特频率，窗口覆盖 32 个过零点；
  - `mix` 支持起始时间、增益和等功率声像（单声道混入立体声时）。

---

## 8. 合成器（`synth.h/.cpp`）

配乐在渲染前离线生成：`synth::render(tracks, seconds)` 把若干条音轨逐个音符合成到立体声缓冲区，每条音轨按 `reverb` 参数送一部分信号到共享混响。

| 乐器 | 算法 |
|---|---|
| Piano（毡锤钢琴） | 加法合成：泛音频率含琴弦刚度造成的非谐性；每个泛音由两根略微失谐的弦叠加；两段衰减（短促的初始声 + 长余音）；柔软的击槌噪声；力度越大音色越亮；有制音器释放 |
| Pluck / Bass | Karplus–Strong：延迟线的小数长度用一阶全通滤波器实现；激励为滤波后的噪声，减去一份延迟副本以模拟拨弦位置 |
| Pad（弦乐铺底） | 多个失谐、带限（polyBLEP）的锯齿波，经状态变量滤波器（拓扑保持结构）低通；起音和释音都很慢 |
| Bell（八音盒） | 金属条的非谐泛音 |
| Shaker / Tick / Thud / Shutter / Swell / Page / Flap | 基于噪声和短包络的音效：沙锤、木鱼（钟表滴答）、闷响、快门、渐强气声、翻书（声音从右扫到左）、卡纸弹起 |

- **混响**：Freeverb 结构，每个声道 8 个梳状滤波器 + 4 个全通滤波器，右声道各延迟线加长 23 个采样以展开立体声，可设置房间大小、阻尼、预延迟和宽度。
- **和弦**：`chord("A/C#")`、`chord("Gmaj7")` 等把和弦名解析成音级；`voicing` 从指定音高往上叠放和弦音；`bassNote` 取低音音高。
- **闪避**：`duckingCurve` 用 40 ms 的 RMS 窗口检测人声：电平高于 −36 dBFS 就算有人声，并据此生成增益曲线。其中起音段是在时间上**倒着**计算的，所以音乐会在人声开始之前就提前压低。
- **限幅**：`limit` 用 5 ms 的前视窗口取所需的最小增益（单调队列实现），然后以 80 ms 平滑恢复，保证峰值不超过上限。

---

## 9. 一帧的完整生命周期（以《日渐冷清的书店》为例）

```
film::World::update(t)        影片自己的关键帧系统：灯光、相机、人群、主角、书店状态
各章节的 Update(t)              摆放本章的道具（全部是时间 t 的纯函数）
World::applyFx()               把对焦距离、光圈、饱和度、聚焦遮罩等写进 PostSettings
eng::Renderer::render(...)     §5.3 的全部流程，结果读回 pixels()
Labels::draw / Captions::draw  在 CPU 上用 Canvas 和文字模块把标签、字幕画到帧上
pageTurn(...)                  （仅在章节之间）用 CPU 把两帧合成为翻页画面
eng::VideoWriter::write(...)   通过管道交给 ffmpeg
```

影片设置的参数（`emptying-bookshop/src/world.cpp`）可以作为调参的参考：

- 太阳：强度 2.6；阴影范围 ±44 个单位；`bias = −0.0004`，`normalBias = 0.03`，`radius = 3`。
- 补光：淡蓝色，强度 0.35。
- 后期：
  - 泛光 `strength 0.32 / radius 0.55 / threshold 0.86`；
  - AO `strength 0.9 / radius 0.45`；
  - `dofMaxRadius 16`；
  - `vignette 0.4`，`contrast 0.96`；
  - 轻微偏暖的 `gain` 与 `lift`，模拟印刷品的色调。

---

## 10. 局限与注意事项

- **只有一盏投射阴影的灯**：太阳。阴影贴图是一张固定范围的正交图，没有级联阴影（CSM），点光源也不投射阴影。
- **阴影贴图尺寸由 `RendererOptions::shadowMapSize` 决定**。`DirectionalLight::shadow.mapSize` 字段存在，但渲染器不读取它。
- **点光源最多 4 个**，超出部分按遍历顺序被忽略，不会报错。
- **纹理最多 128 张**，首次上传后不能改变尺寸或格式。
- **几何体上传后不可修改**，只有绘制范围可以变化。要改变形状，请改节点的变换，或新建一个几何体。
- 实例化网格不做视锥剔除。透明物体按物体排序，而不是按三角形排序，所以互相穿插的透明物体可能出现排序错误。
- 没有基于图像的光照（IBL）和环境反射，半球光只影响漫反射。
- 线宽固定为 1 像素。点的大小受设备 `largePoints` 特性限制。
- 每帧都同步等待 GPU，CPU 和 GPU 不能并行工作。在 llvmpipe 上 1080p 每秒只能渲染几帧，一部七分钟的影片要一个多小时；有 GPU 时只需几分钟（见影片的 README）。
- 对象池里的资源在程序退出前从不释放。引擎适合“一次性搭好场景、然后逐帧渲染”的影片程序，不适合需要长时间运行、不断增删物体的程序。
- 调试 Vulkan 问题时，设置 `RendererOptions::validation`（影片里对应 `--validate` 或环境变量 `ENG_VALIDATION=1`）即可开启 Khronos 校验层。需要先安装 `vulkan-validationlayers`。
