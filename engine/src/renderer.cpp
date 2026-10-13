#include "eng/renderer.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace eng {

namespace {

#define VKC(x)                                                                                    \
  do {                                                                                            \
    VkResult r_ = (x);                                                                            \
    if (r_ != VK_SUCCESS) throw std::runtime_error(std::string(#x) + " failed (" + std::to_string((int)r_) + ")"); \
  } while (0)

const uint32_t kMeshVert[] =
#include "mesh.vert.inc"
    ;
const uint32_t kPointsVert[] =
#include "points.vert.inc"
    ;
const uint32_t kSpriteVert[] =
#include "sprite.vert.inc"
    ;
const uint32_t kMeshFrag[] =
#include "mesh.frag.inc"
    ;
const uint32_t kShadowVert[] =
#include "shadow.vert.inc"
    ;
const uint32_t kSkyVert[] =
#include "sky.vert.inc"
    ;
const uint32_t kSkyFrag[] =
#include "sky.frag.inc"
    ;
const uint32_t kFullscreenVert[] =
#include "fullscreen.vert.inc"
    ;
const uint32_t kBrightFrag[] =
#include "bright.frag.inc"
    ;
const uint32_t kBlurFrag[] =
#include "blur.frag.inc"
    ;
const uint32_t kFinalFrag[] =
#include "final.frag.inc"
    ;

constexpr int kMaxTextures = 128;
constexpr int kMaxPointLights = 4;
constexpr int kBloomLevels = 5;
constexpr VkFormat kHdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;
constexpr VkFormat kOutFormat = VK_FORMAT_R8G8B8A8_UNORM;

constexpr uint32_t F_LIT = 1, F_MAP = 2, F_INSTANCED = 4, F_ICOLOR = 8, F_RECEIVE = 16, F_FOG = 32, F_DOUBLE = 64;

struct FrameUBO {
  mat4 viewProj, view, invViewProj, shadowViewProj, shadowMat;
  vec4 camPos, sunDir, sunColor, fillDir, fillColor, hemiSky, hemiGround, fogColor, fogRange, shadowParams, camRight, camUp, viewport, skyTop,
      skyHorizon;
  vec4 pointPos[kMaxPointLights], pointColor[kMaxPointLights], pointParams[kMaxPointLights];
};

struct DrawRec {
  mat4 model;
  vec4 n0, n1, n2;
  vec4 color, emissive, params;
  uint32_t info[4];
};
static_assert(sizeof(DrawRec) == 176, "draw record must match the shader's std430 layout");

struct Buffer {
  VkBuffer buf = VK_NULL_HANDLE;
  VkDeviceMemory mem = VK_NULL_HANDLE;
  VkDeviceSize size = 0;
  void* map = nullptr;
};

struct Image {
  VkImage img = VK_NULL_HANDLE;
  VkDeviceMemory mem = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkFormat format = VK_FORMAT_UNDEFINED;
  uint32_t w = 0, h = 0, mips = 1;
};

enum Program : uint32_t { P_MESH = 0, P_POINTS = 1, P_SPRITE = 2 };

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT sev, VkDebugUtilsMessageTypeFlagsEXT,
                                             const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
  if (sev >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) std::fprintf(stderr, "[vulkan] %s\n", data->pMessage);
  return VK_FALSE;
}

}  // namespace

struct Renderer::Impl {
  RendererOptions opt;
  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
  VkPhysicalDevice phys = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties props{};
  VkPhysicalDeviceMemoryProperties memProps{};
  VkDevice dev = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  uint32_t qfam = 0;
  VkCommandPool cmdPool = VK_NULL_HANDLE;
  VkCommandBuffer cmd = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  bool anisotropy = false, largePoints = false, mipBlit = true;

  // Render targets.
  Image msaaColor, depth, hdr, shadow, bright, bloomH[kBloomLevels], bloomV[kBloomLevels], out, white;
  VkRenderPass scenePass = VK_NULL_HANDLE, shadowPass = VK_NULL_HANDLE, postPass = VK_NULL_HANDLE, outPass = VK_NULL_HANDLE;
  VkFramebuffer sceneFb = VK_NULL_HANDLE, shadowFb = VK_NULL_HANDLE, brightFb = VK_NULL_HANDLE, outFb = VK_NULL_HANDLE;
  VkFramebuffer bloomHFb[kBloomLevels]{}, bloomVFb[kBloomLevels]{};
  VkSampler linearClamp = VK_NULL_HANDLE, shadowSampler = VK_NULL_HANDLE, texSampler = VK_NULL_HANDLE;

  // Descriptors and pipelines.
  VkDescriptorSetLayout sceneLayout = VK_NULL_HANDLE, postLayout = VK_NULL_HANDLE;
  VkPipelineLayout scenePL = VK_NULL_HANDLE, postPL = VK_NULL_HANDLE;
  VkDescriptorPool descPool = VK_NULL_HANDLE;
  VkDescriptorSet sceneSet = VK_NULL_HANDLE, brightSet = VK_NULL_HANDLE, outSet = VK_NULL_HANDLE;
  VkDescriptorSet blurHSet[kBloomLevels]{}, blurVSet[kBloomLevels]{};
  std::map<std::string, VkShaderModule> modules;
  std::map<uint32_t, VkPipeline> scenePipes;
  VkPipeline shadowPipes[3]{}, skyPipe = VK_NULL_HANDLE, brightPipe = VK_NULL_HANDLE, blurPipe = VK_NULL_HANDLE, outPipe = VK_NULL_HANDLE;

  // Per-frame data.
  Buffer ubo, drawBuf, instBuf, icolBuf, readback, staging;

  // Geometry lives in large shared buffers.
  struct Chunk { Buffer buf; VkDeviceSize used = 0; };
  std::vector<Chunk> chunks;
  struct GeoGPU { int chunk; VkDeviceSize vOff, iOff; bool indexed; uint32_t elements; };
  std::vector<GeoGPU> geos;

  struct TexGPU { Image img; uint32_t version = 0; };
  std::vector<TexGPU> texs;

  std::vector<uint8_t> pixels;
  Renderer::Stats stats;

  // ---------- setup ----------
  void init() {
    createInstance();
    pickDevice();
    createDevice();
    createSamplers();
    createTargets();
    createPasses();
    createFramebuffers();
    createDescriptors();
    createPipelines();
    pixels.resize((size_t)opt.width * opt.height * 4);
    readback = makeBuffer((VkDeviceSize)opt.width * opt.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
    initLayouts();
  }

  void createInstance() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "eng";
    app.apiVersion = VK_API_VERSION_1_2;
    std::vector<const char*> layers, exts;
    if (opt.validation) {
      uint32_t n = 0;
      vkEnumerateInstanceLayerProperties(&n, nullptr);
      std::vector<VkLayerProperties> lp(n);
      vkEnumerateInstanceLayerProperties(&n, lp.data());
      for (auto& l : lp)
        if (!std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation")) layers.push_back("VK_LAYER_KHRONOS_validation");
      if (layers.empty()) std::fprintf(stderr, "[vulkan] validation layer not installed\n");
      else exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledLayerCount = (uint32_t)layers.size();
    ci.ppEnabledLayerNames = layers.data();
    ci.enabledExtensionCount = (uint32_t)exts.size();
    ci.ppEnabledExtensionNames = exts.data();
    VKC(vkCreateInstance(&ci, nullptr, &instance));
    if (!layers.empty()) {
      auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
      VkDebugUtilsMessengerCreateInfoEXT mi{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
      mi.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      mi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      mi.pfnUserCallback = debugCallback;
      if (fn) fn(instance, &mi, nullptr, &messenger);
    }
  }

  void pickDevice() {
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(instance, &n, nullptr);
    if (!n) throw std::runtime_error("no Vulkan device found");
    std::vector<VkPhysicalDevice> devs(n);
    vkEnumeratePhysicalDevices(instance, &n, devs.data());
    auto rank = [](VkPhysicalDeviceType t) {
      switch (t) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 4;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 3;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 2;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return 1;
        default: return 0;
      }
    };
    int best = -1;
    for (auto d : devs) {
      VkPhysicalDeviceProperties p;
      vkGetPhysicalDeviceProperties(d, &p);
      uint32_t qn = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(d, &qn, nullptr);
      std::vector<VkQueueFamilyProperties> qs(qn);
      vkGetPhysicalDeviceQueueFamilyProperties(d, &qn, qs.data());
      for (uint32_t i = 0; i < qn; i++)
        if (qs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          if (rank(p.deviceType) > best) { best = rank(p.deviceType); phys = d; qfam = i; }
          break;
        }
    }
    if (!phys) throw std::runtime_error("no Vulkan device with a graphics queue");
    vkGetPhysicalDeviceProperties(phys, &props);
    vkGetPhysicalDeviceMemoryProperties(phys, &memProps);
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
    samples = VK_SAMPLE_COUNT_1_BIT;
    for (int s : {8, 4, 2})
      if (opt.samples >= s && (counts & s)) { samples = (VkSampleCountFlagBits)s; break; }
    VkFormatProperties fp;
    vkGetPhysicalDeviceFormatProperties(phys, VK_FORMAT_R8G8B8A8_SRGB, &fp);
    mipBlit = (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) &&
              (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
  }

  void createDevice() {
    VkPhysicalDeviceFeatures have;
    vkGetPhysicalDeviceFeatures(phys, &have);
    VkPhysicalDeviceFeatures want{};
    want.shaderSampledImageArrayDynamicIndexing = have.shaderSampledImageArrayDynamicIndexing;
    want.samplerAnisotropy = anisotropy = have.samplerAnisotropy;
    want.largePoints = largePoints = have.largePoints;
    float prio = 1;
    VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qi.queueFamilyIndex = qfam;
    qi.queueCount = 1;
    qi.pQueuePriorities = &prio;
    VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    ci.queueCreateInfoCount = 1;
    ci.pQueueCreateInfos = &qi;
    ci.pEnabledFeatures = &want;
    VKC(vkCreateDevice(phys, &ci, nullptr, &dev));
    vkGetDeviceQueue(dev, qfam, 0, &queue);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pi.queueFamilyIndex = qfam;
    VKC(vkCreateCommandPool(dev, &pi, nullptr, &cmdPool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = cmdPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VKC(vkAllocateCommandBuffers(dev, &ai, &cmd));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VKC(vkCreateFence(dev, &fi, nullptr, &fence));
  }

  uint32_t memType(uint32_t bits, VkMemoryPropertyFlags want, VkMemoryPropertyFlags fallback) {
    for (VkMemoryPropertyFlags f : {want, fallback})
      for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & f) == f) return i;
    throw std::runtime_error("no suitable Vulkan memory type");
  }

  Buffer makeBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool host) {
    Buffer b;
    b.size = std::max<VkDeviceSize>(size, 16);
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = b.size;
    bi.usage = usage;
    VKC(vkCreateBuffer(dev, &bi, nullptr, &b.buf));
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev, b.buf, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = host ? memType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                              : memType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
    VKC(vkAllocateMemory(dev, &ai, nullptr, &b.mem));
    VKC(vkBindBufferMemory(dev, b.buf, b.mem, 0));
    if (host) VKC(vkMapMemory(dev, b.mem, 0, VK_WHOLE_SIZE, 0, &b.map));
    return b;
  }

  void destroy(Buffer& b) {
    if (b.map) vkUnmapMemory(dev, b.mem);
    if (b.buf) vkDestroyBuffer(dev, b.buf, nullptr);
    if (b.mem) vkFreeMemory(dev, b.mem, nullptr);
    b = Buffer{};
  }

  Image makeImage(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits s = VK_SAMPLE_COUNT_1_BIT, uint32_t mips = 1) {
    Image im;
    im.w = w; im.h = h; im.format = format; im.mips = mips;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = format;
    ci.extent = {w, h, 1};
    ci.mipLevels = mips;
    ci.arrayLayers = 1;
    ci.samples = s;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = usage;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VKC(vkCreateImage(dev, &ci, nullptr, &im.img));
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(dev, im.img, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    VkMemoryPropertyFlags want = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) want |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    ai.memoryTypeIndex = memType(req.memoryTypeBits, want, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VKC(vkAllocateMemory(dev, &ai, nullptr, &im.mem));
    VKC(vkBindImageMemory(dev, im.img, im.mem, 0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = im.img;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    bool isDepth = format == kDepthFormat;
    vi.subresourceRange = {(VkImageAspectFlags)(isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT), 0, mips, 0, 1};
    VKC(vkCreateImageView(dev, &vi, nullptr, &im.view));
    return im;
  }

  void destroy(Image& im) {
    if (im.view) vkDestroyImageView(dev, im.view, nullptr);
    if (im.img) vkDestroyImage(dev, im.img, nullptr);
    if (im.mem) vkFreeMemory(dev, im.mem, nullptr);
    im = Image{};
  }

  void createSamplers() {
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.maxLod = 0;
    VKC(vkCreateSampler(dev, &si, nullptr, &linearClamp));
    si.maxLod = VK_LOD_CLAMP_NONE;
    si.anisotropyEnable = anisotropy;
    si.maxAnisotropy = anisotropy ? std::min(4.f, props.limits.maxSamplerAnisotropy) : 1.f;
    VKC(vkCreateSampler(dev, &si, nullptr, &texSampler));
    VkSamplerCreateInfo ss{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    ss.magFilter = ss.minFilter = VK_FILTER_LINEAR;
    ss.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    ss.addressModeU = ss.addressModeV = ss.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    ss.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    ss.compareEnable = VK_TRUE;
    ss.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    VKC(vkCreateSampler(dev, &ss, nullptr, &shadowSampler));
  }

  void createTargets() {
    uint32_t W = opt.width, H = opt.height;
    VkImageUsageFlags att = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (samples != VK_SAMPLE_COUNT_1_BIT)
      msaaColor = makeImage(W, H, kHdrFormat, att | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, samples);
    depth = makeImage(W, H, kDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, samples);
    hdr = makeImage(W, H, kHdrFormat, att | VK_IMAGE_USAGE_SAMPLED_BIT);
    uint32_t S = opt.shadowMapSize;
    shadow = makeImage(S, S, kDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    uint32_t bw = (W + 1) / 2, bh = (H + 1) / 2;
    bright = makeImage(bw, bh, kHdrFormat, att | VK_IMAGE_USAGE_SAMPLED_BIT);
    for (int i = 0; i < kBloomLevels; i++) {
      bloomH[i] = makeImage(bw, bh, kHdrFormat, att | VK_IMAGE_USAGE_SAMPLED_BIT);
      bloomV[i] = makeImage(bw, bh, kHdrFormat, att | VK_IMAGE_USAGE_SAMPLED_BIT);
      bw = std::max(1u, (bw + 1) / 2);
      bh = std::max(1u, (bh + 1) / 2);
    }
    out = makeImage(W, H, kOutFormat, att | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    white = makeImage(1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  }

  VkRenderPass colorPass(VkFormat format, VkImageLayout finalLayout) {
    VkAttachmentDescription a{};
    a.format = format;
    a.samples = VK_SAMPLE_COUNT_1_BIT;
    a.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    a.finalLayout = finalLayout;
    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments = &ref;
    return makePass({a}, sp);
  }

  VkRenderPass makePass(const std::vector<VkAttachmentDescription>& atts, const VkSubpassDescription& sp) {
    // Conservative dependencies: earlier passes' writes and reads finish before this pass writes,
    // and this pass's writes are visible to later shader reads and transfers.
    VkSubpassDependency deps[2]{};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    deps[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
                           VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                            VK_ACCESS_INDEX_READ_BIT;
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    ci.attachmentCount = (uint32_t)atts.size();
    ci.pAttachments = atts.data();
    ci.subpassCount = 1;
    ci.pSubpasses = &sp;
    ci.dependencyCount = 2;
    ci.pDependencies = deps;
    VkRenderPass p;
    VKC(vkCreateRenderPass(dev, &ci, nullptr, &p));
    return p;
  }

  void createPasses() {
    // Scene: colour (multisampled, resolved into hdr) and depth.
    std::vector<VkAttachmentDescription> atts;
    VkAttachmentDescription c{};
    c.format = kHdrFormat;
    c.samples = samples;
    c.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    c.storeOp = samples == VK_SAMPLE_COUNT_1_BIT ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    c.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    c.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    c.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    c.finalLayout = samples == VK_SAMPLE_COUNT_1_BIT ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    atts.push_back(c);
    VkAttachmentDescription d = c;
    d.format = kDepthFormat;
    d.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    d.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    atts.push_back(d);
    VkAttachmentReference cref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, dref{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference rref{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments = &cref;
    sp.pDepthStencilAttachment = &dref;
    if (samples != VK_SAMPLE_COUNT_1_BIT) {
      VkAttachmentDescription r = c;
      r.samples = VK_SAMPLE_COUNT_1_BIT;
      r.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      r.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      r.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      atts.push_back(r);
      sp.pResolveAttachments = &rref;
    }
    scenePass = makePass(atts, sp);

    // Shadow: depth only.
    VkAttachmentDescription s{};
    s.format = kDepthFormat;
    s.samples = VK_SAMPLE_COUNT_1_BIT;
    s.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    s.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    s.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    s.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    s.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    s.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference sref{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription ssp{};
    ssp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    ssp.pDepthStencilAttachment = &sref;
    shadowPass = makePass({s}, ssp);

    postPass = colorPass(kHdrFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    outPass = colorPass(kOutFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  }

  VkFramebuffer framebuffer(VkRenderPass pass, std::vector<VkImageView> views, uint32_t w, uint32_t h) {
    VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    ci.renderPass = pass;
    ci.attachmentCount = (uint32_t)views.size();
    ci.pAttachments = views.data();
    ci.width = w;
    ci.height = h;
    ci.layers = 1;
    VkFramebuffer fb;
    VKC(vkCreateFramebuffer(dev, &ci, nullptr, &fb));
    return fb;
  }

  void createFramebuffers() {
    if (samples == VK_SAMPLE_COUNT_1_BIT) sceneFb = framebuffer(scenePass, {hdr.view, depth.view}, hdr.w, hdr.h);
    else sceneFb = framebuffer(scenePass, {msaaColor.view, depth.view, hdr.view}, hdr.w, hdr.h);
    shadowFb = framebuffer(shadowPass, {shadow.view}, shadow.w, shadow.h);
    brightFb = framebuffer(postPass, {bright.view}, bright.w, bright.h);
    for (int i = 0; i < kBloomLevels; i++) {
      bloomHFb[i] = framebuffer(postPass, {bloomH[i].view}, bloomH[i].w, bloomH[i].h);
      bloomVFb[i] = framebuffer(postPass, {bloomV[i].view}, bloomV[i].w, bloomV[i].h);
    }
    outFb = framebuffer(outPass, {out.view}, out.w, out.h);
  }

  void createDescriptors() {
    VkDescriptorSetLayoutBinding b[6]{};
    b[0] = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    b[1] = {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    b[2] = {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    b[3] = {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    b[4] = {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    b[5] = {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxTextures, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    li.bindingCount = 6;
    li.pBindings = b;
    VKC(vkCreateDescriptorSetLayout(dev, &li, nullptr, &sceneLayout));
    VkDescriptorSetLayoutBinding pb[2]{};
    pb[0] = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    pb[1] = {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kBloomLevels, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    li.bindingCount = 2;
    li.pBindings = pb;
    VKC(vkCreateDescriptorSetLayout(dev, &li, nullptr, &postLayout));

    VkPushConstantRange pr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16};
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &sceneLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pr;
    VKC(vkCreatePipelineLayout(dev, &pli, nullptr, &scenePL));
    VkPushConstantRange ppr{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 64};
    pli.pSetLayouts = &postLayout;
    pli.pPushConstantRanges = &ppr;
    VKC(vkCreatePipelineLayout(dev, &pli, nullptr, &postPL));

    VkDescriptorPoolSize sizes[3] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6},
                                     {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxTextures + 1 + (2 * kBloomLevels + 2) * (1 + kBloomLevels)}};
    VkDescriptorPoolCreateInfo dpi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpi.maxSets = 1 + 2 + 2 * kBloomLevels;
    dpi.poolSizeCount = 3;
    dpi.pPoolSizes = sizes;
    VKC(vkCreateDescriptorPool(dev, &dpi, nullptr, &descPool));
    auto alloc = [&](VkDescriptorSetLayout l) {
      VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
      ai.descriptorPool = descPool;
      ai.descriptorSetCount = 1;
      ai.pSetLayouts = &l;
      VkDescriptorSet s;
      VKC(vkAllocateDescriptorSets(dev, &ai, &s));
      return s;
    };
    sceneSet = alloc(sceneLayout);
    brightSet = alloc(postLayout);
    outSet = alloc(postLayout);
    for (int i = 0; i < kBloomLevels; i++) { blurHSet[i] = alloc(postLayout); blurVSet[i] = alloc(postLayout); }

    // Per-frame buffers start small and grow on demand.
    ubo = makeBuffer(sizeof(FrameUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
    drawBuf = makeBuffer(sizeof(DrawRec) * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    instBuf = makeBuffer(sizeof(mat4) * 4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    icolBuf = makeBuffer(sizeof(vec4) * 4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    writeBufferDescriptors();

    std::vector<VkDescriptorImageInfo> imgs(kMaxTextures, {texSampler, white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
    VkDescriptorImageInfo sh{shadowSampler, shadow.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet w[2]{};
    w[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, sceneSet, 4, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &sh, nullptr, nullptr};
    w[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, sceneSet, 5, 0, kMaxTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imgs.data(), nullptr, nullptr};
    vkUpdateDescriptorSets(dev, 2, w, 0, nullptr);

    auto postSet = [&](VkDescriptorSet set, VkImageView src, const Image* levels) {
      VkDescriptorImageInfo s0{linearClamp, src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      std::vector<VkDescriptorImageInfo> s1(kBloomLevels, {linearClamp, white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
      if (levels)
        for (int i = 0; i < kBloomLevels; i++) s1[i].imageView = levels[i].view;
      VkWriteDescriptorSet pw[2]{};
      pw[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &s0, nullptr, nullptr};
      pw[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, 1, 0, kBloomLevels, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, s1.data(), nullptr, nullptr};
      vkUpdateDescriptorSets(dev, 2, pw, 0, nullptr);
    };
    postSet(brightSet, hdr.view, nullptr);
    for (int i = 0; i < kBloomLevels; i++) {
      postSet(blurHSet[i], i == 0 ? bright.view : bloomV[i - 1].view, nullptr);
      postSet(blurVSet[i], bloomH[i].view, nullptr);
    }
    postSet(outSet, hdr.view, bloomV);
  }

  void writeBufferDescriptors() {
    VkDescriptorBufferInfo bi[4] = {{ubo.buf, 0, VK_WHOLE_SIZE}, {drawBuf.buf, 0, VK_WHOLE_SIZE}, {instBuf.buf, 0, VK_WHOLE_SIZE}, {icolBuf.buf, 0, VK_WHOLE_SIZE}};
    VkWriteDescriptorSet w[4]{};
    for (int i = 0; i < 4; i++)
      w[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, sceneSet, (uint32_t)i, 0, 1,
              i == 0 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bi[i], nullptr};
    vkUpdateDescriptorSets(dev, 4, w, 0, nullptr);
  }

  void ensureCapacity(Buffer& b, VkDeviceSize need) {
    if (need <= b.size) return;
    VkDeviceSize size = b.size;
    while (size < need) size *= 2;
    destroy(b);
    b = makeBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    writeBufferDescriptors();
  }

  VkShaderModule module(const char* name, const uint32_t* code, size_t bytes) {
    auto it = modules.find(name);
    if (it != modules.end()) return it->second;
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = bytes;
    ci.pCode = code;
    VkShaderModule m;
    VKC(vkCreateShaderModule(dev, &ci, nullptr, &m));
    modules[name] = m;
    return m;
  }

  struct PipeDesc {
    VkShaderModule vs, fs = VK_NULL_HANDLE;
    VkRenderPass pass;
    VkPipelineLayout layout;
    VkPrimitiveTopology topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags cull = VK_CULL_MODE_NONE;
    bool blend = false, depthTest = false, depthWrite = false, vertexInput = true, positionOnly = false;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    bool hasColor = true;
  };

  VkPipeline makePipeline(const PipeDesc& d) {
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, d.vs, "main", nullptr};
    st[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, d.fs, "main", nullptr};
    VkVertexInputBindingDescription vb{0, 32, VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription va[3] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}, {2, 0, VK_FORMAT_R32G32_SFLOAT, 24}};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    if (d.vertexInput) {
      vi.vertexBindingDescriptionCount = 1;
      vi.pVertexBindingDescriptions = &vb;
      vi.vertexAttributeDescriptionCount = d.positionOnly ? 1 : 3;
      vi.pVertexAttributeDescriptions = va;
    }
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = d.topo;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = d.cull;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = d.samples;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = d.depthTest;
    ds.depthWriteEnable = d.depthWrite;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    VkPipelineColorBlendAttachmentState ba{};
    ba.colorWriteMask = 0xf;
    if (d.blend) {
      ba.blendEnable = VK_TRUE;
      ba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      ba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ba.colorBlendOp = VK_BLEND_OP_ADD;
      ba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ba.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = d.hasColor ? 1 : 0;
    cb.pAttachments = &ba;
    VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2;
    dy.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    ci.stageCount = d.fs ? 2 : 1;
    ci.pStages = st;
    ci.pVertexInputState = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState = &vp;
    ci.pRasterizationState = &rs;
    ci.pMultisampleState = &ms;
    ci.pDepthStencilState = &ds;
    ci.pColorBlendState = &cb;
    ci.pDynamicState = &dy;
    ci.layout = d.layout;
    ci.renderPass = d.pass;
    VkPipeline p;
    VKC(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &ci, nullptr, &p));
    return p;
  }

  void createPipelines() {
    VkShaderModule fsv = module("fullscreen.vert", kFullscreenVert, sizeof kFullscreenVert);
    PipeDesc d;
    d.layout = scenePL;
    d.pass = scenePass;
    d.samples = samples;
    d.vs = module("sky.vert", kSkyVert, sizeof kSkyVert);
    d.fs = module("sky.frag", kSkyFrag, sizeof kSkyFrag);
    d.vertexInput = false;
    skyPipe = makePipeline(d);

    PipeDesc s;
    s.layout = scenePL;
    s.pass = shadowPass;
    s.vs = module("shadow.vert", kShadowVert, sizeof kShadowVert);
    s.depthTest = s.depthWrite = true;
    s.positionOnly = true;
    s.hasColor = false;
    VkCullModeFlags culls[3] = {VK_CULL_MODE_NONE, VK_CULL_MODE_BACK_BIT, VK_CULL_MODE_FRONT_BIT};
    for (int i = 0; i < 3; i++) {
      s.cull = culls[i];
      shadowPipes[i] = makePipeline(s);
    }

    PipeDesc p;
    p.layout = postPL;
    p.pass = postPass;
    p.vs = fsv;
    p.vertexInput = false;
    p.fs = module("bright.frag", kBrightFrag, sizeof kBrightFrag);
    brightPipe = makePipeline(p);
    p.fs = module("blur.frag", kBlurFrag, sizeof kBlurFrag);
    blurPipe = makePipeline(p);
    p.pass = outPass;
    p.fs = module("final.frag", kFinalFrag, sizeof kFinalFrag);
    outPipe = makePipeline(p);
  }

  // Scene pipelines are made on first use: program, topology, culling, blending and depth state.
  VkPipeline scenePipe(Program prog, Topology topo, VkCullModeFlags cull, bool blend, bool depthTest, bool depthWrite) {
    uint32_t key = (uint32_t)prog | ((uint32_t)topo << 2) | ((uint32_t)cull << 4) | ((uint32_t)blend << 6) | ((uint32_t)depthTest << 7) |
                   ((uint32_t)depthWrite << 8);
    auto it = scenePipes.find(key);
    if (it != scenePipes.end()) return it->second;
    PipeDesc d;
    d.layout = scenePL;
    d.pass = scenePass;
    d.samples = samples;
    d.vs = prog == P_POINTS   ? module("points.vert", kPointsVert, sizeof kPointsVert)
           : prog == P_SPRITE ? module("sprite.vert", kSpriteVert, sizeof kSpriteVert)
                              : module("mesh.vert", kMeshVert, sizeof kMeshVert);
    d.fs = module("mesh.frag", kMeshFrag, sizeof kMeshFrag);
    d.topo = topo == Topology::Lines ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST
             : topo == Topology::Points ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST
                                        : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    d.cull = cull;
    d.blend = blend;
    d.depthTest = depthTest;
    d.depthWrite = depthWrite;
    VkPipeline p = makePipeline(d);
    scenePipes[key] = p;
    return p;
  }

  // ---------- one-off commands ----------
  void beginCmd() {
    VKC(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VKC(vkBeginCommandBuffer(cmd, &bi));
  }
  void submitAndWait() {
    VKC(vkEndCommandBuffer(cmd));
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    VKC(vkResetFences(dev, 1, &fence));
    VKC(vkQueueSubmit(queue, 1, &si, fence));
    VKC(vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX));
  }

  void barrier(VkImage img, VkImageAspectFlags aspect, VkImageLayout from, VkImageLayout to, uint32_t baseMip = 0, uint32_t mips = 1) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = {aspect, baseMip, mips, 0, 1};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
  }

  void initLayouts() {
    staging = makeBuffer(1 << 20, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    beginCmd();
    // Everything that is ever sampled starts in the read layout, so descriptors are always valid.
    for (Image* im : {&hdr, &bright}) barrier(im->img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    for (int i = 0; i < kBloomLevels; i++) {
      barrier(bloomH[i].img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      barrier(bloomV[i].img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    barrier(shadow.img, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    std::memset(staging.map, 255, 4);
    barrier(white.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy r{};
    r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    r.imageExtent = {1, 1, 1};
    vkCmdCopyBufferToImage(cmd, staging.buf, white.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
    barrier(white.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    submitAndWait();
  }

  // ---------- uploads ----------
  struct Pending {
    std::vector<std::pair<Geometry*, VkDeviceSize>> geos;  // geometry, staging offset
    std::vector<std::pair<Texture*, VkDeviceSize>> texs;
    VkDeviceSize bytes = 0;
  };

  static VkDeviceSize align(VkDeviceSize v, VkDeviceSize a) { return (v + a - 1) / a * a; }

  void stageGeometry(Geometry* g, Pending& p) {
    if (g->gpuSlot >= 0) return;
    size_t nv = g->vertexCount();
    VkDeviceSize vbytes = nv * 32, ibytes = g->indices.size() * 4;
    VkDeviceSize need = align(vbytes, 16) + align(ibytes, 16);
    int ci = -1;
    for (int i = 0; i < (int)chunks.size(); i++)
      if (chunks[i].buf.size - chunks[i].used >= need) { ci = i; break; }
    if (ci < 0) {
      Chunk c;
      c.buf = makeBuffer(std::max<VkDeviceSize>(need, 32 << 20),
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
      chunks.push_back(c);
      ci = (int)chunks.size() - 1;
    }
    GeoGPU gg;
    gg.chunk = ci;
    gg.vOff = chunks[ci].used;
    gg.iOff = gg.vOff + align(vbytes, 16);
    gg.indexed = !g->indices.empty();
    gg.elements = (uint32_t)g->elementCount();
    chunks[ci].used += need;
    g->gpuSlot = (int)geos.size();
    geos.push_back(gg);
    p.geos.push_back({g, p.bytes});
    p.bytes += need;
  }

  void stageTexture(Texture* t, Pending& p) {
    if (t->gpuSlot >= 0 && texs[t->gpuSlot].version == t->version) return;
    for (auto& q : p.texs)
      if (q.first == t) return;
    p.texs.push_back({t, p.bytes});
    p.bytes += align((VkDeviceSize)t->width * t->height * 4, 16);
  }

  void recordUploads(Pending& p) {
    if (p.geos.empty() && p.texs.empty()) return;
    if (staging.size < p.bytes) {
      destroy(staging);
      staging = makeBuffer(p.bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    }
    uint8_t* base = (uint8_t*)staging.map;
    for (auto& [g, off] : p.geos) {
      const GeoGPU& gg = geos[g->gpuSlot];
      size_t nv = g->vertexCount();
      float* v = (float*)(base + off);
      for (size_t i = 0; i < nv; i++) {
        float* o = v + i * 8;
        o[0] = g->positions[i * 3]; o[1] = g->positions[i * 3 + 1]; o[2] = g->positions[i * 3 + 2];
        bool hn = g->normals.size() == g->positions.size();
        o[3] = hn ? g->normals[i * 3] : 0; o[4] = hn ? g->normals[i * 3 + 1] : 1; o[5] = hn ? g->normals[i * 3 + 2] : 0;
        bool hu = g->uvs.size() == nv * 2;
        o[6] = hu ? g->uvs[i * 2] : 0; o[7] = hu ? g->uvs[i * 2 + 1] : 0;
      }
      VkDeviceSize vbytes = nv * 32;
      if (!g->indices.empty()) std::memcpy(base + off + (gg.iOff - gg.vOff), g->indices.data(), g->indices.size() * 4);
      VkBufferCopy c{off, gg.vOff, (gg.iOff - gg.vOff) + g->indices.size() * 4};
      if (g->indices.empty()) c.size = vbytes;
      vkCmdCopyBuffer(cmd, staging.buf, chunks[gg.chunk].buf.buf, 1, &c);
    }
    for (auto& [t, off] : p.texs) {
      // Rows go in bottom first, so v = 0 is the bottom of the image (uv origin at the bottom-left).
      size_t row = (size_t)t->width * 4;
      for (int y = 0; y < t->height; y++) std::memcpy(base + off + row * y, t->pixels.data() + row * (t->height - 1 - y), row);
      VkFormat fmt = t->srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
      uint32_t mips = t->mipmaps && mipBlit ? (uint32_t)std::floor(std::log2((float)std::max(t->width, t->height))) + 1 : 1;
      bool fresh = t->gpuSlot < 0;
      if (!fresh) {
        TexGPU& tg = texs[t->gpuSlot];
        if (tg.img.w != (uint32_t)t->width || tg.img.h != (uint32_t)t->height || tg.img.format != fmt) {
          throw std::runtime_error("textures cannot change size or format after first use");
        }
      } else {
        if ((int)texs.size() >= kMaxTextures) throw std::runtime_error("too many textures");
        TexGPU tg;
        tg.img = makeImage(t->width, t->height, fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                           VK_SAMPLE_COUNT_1_BIT, mips);
        t->gpuSlot = (int)texs.size();
        texs.push_back(tg);
        VkDescriptorImageInfo ii{texSampler, tg.img.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, sceneSet, 5, (uint32_t)t->gpuSlot, 1,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &ii, nullptr, nullptr};
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
      }
      TexGPU& tg = texs[t->gpuSlot];
      tg.version = t->version;
      Image& im = tg.img;
      barrier(im.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, im.mips);
      VkBufferImageCopy r{};
      r.bufferOffset = off;
      r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      r.imageExtent = {im.w, im.h, 1};
      vkCmdCopyBufferToImage(cmd, staging.buf, im.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
      int32_t mw = im.w, mh = im.h;
      for (uint32_t m = 1; m < im.mips; m++) {
        barrier(im.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m - 1, 1);
        VkImageBlit bl{};
        bl.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, m - 1, 0, 1};
        bl.srcOffsets[1] = {mw, mh, 1};
        mw = std::max(1, mw / 2);
        mh = std::max(1, mh / 2);
        bl.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, m, 0, 1};
        bl.dstOffsets[1] = {mw, mh, 1};
        vkCmdBlitImage(cmd, im.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, im.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bl, VK_FILTER_LINEAR);
        barrier(im.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m - 1, 1);
      }
      barrier(im.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, im.mips - 1, 1);
    }
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
  }

  // ---------- frame ----------
  struct Item {
    Mesh* m;
    uint32_t rec;
    float depth;
    int order;
    uint32_t pipeKey;
  };

  static void frustumPlanes(const mat4& vp, vec4 out[6]) {
    auto row = [&](int r) { return vec4{vp.m[r], vp.m[4 + r], vp.m[8 + r], vp.m[12 + r]}; };
    vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
    vec4 p[6] = {{r3.x + r0.x, r3.y + r0.y, r3.z + r0.z, r3.w + r0.w}, {r3.x - r0.x, r3.y - r0.y, r3.z - r0.z, r3.w - r0.w},
                 {r3.x + r1.x, r3.y + r1.y, r3.z + r1.z, r3.w + r1.w}, {r3.x - r1.x, r3.y - r1.y, r3.z - r1.z, r3.w - r1.w},
                 {r2.x, r2.y, r2.z, r2.w},                             {r3.x - r2.x, r3.y - r2.y, r3.z - r2.z, r3.w - r2.w}};
    for (int i = 0; i < 6; i++) {
      float l = std::sqrt(p[i].x * p[i].x + p[i].y * p[i].y + p[i].z * p[i].z);
      out[i] = {p[i].x / l, p[i].y / l, p[i].z / l, p[i].w / l};
    }
  }
  static bool inFrustum(const vec4 pl[6], const vec3& c, float r) {
    for (int i = 0; i < 6; i++)
      if (pl[i].x * c.x + pl[i].y * c.y + pl[i].z * c.z + pl[i].w < -r) return false;
    return true;
  }

  void render(Scene& scene, Camera& cam, const PostSettings& post) {
    scene.updateWorld();
    cam.update();
    mat4 viewProj = cam.proj * cam.view;

    // Shadow camera.
    const DirectionalLight& sun = scene.sun;
    mat4 sView = mat4::lookAtView(sun.position, sun.target, {0, 1, 0});
    mat4 sProj = mat4::ortho(sun.shadow.left, sun.shadow.right, sun.shadow.bottom, sun.shadow.top, sun.shadow.near, sun.shadow.far);
    mat4 sViewProj = sProj * sView;
    mat4 bias;
    bias.m[0] = 0.5f; bias.m[5] = -0.5f; bias.m[12] = 0.5f; bias.m[13] = 0.5f;
    vec4 camPl[6], sunPl[6];
    frustumPlanes(viewProj, camPl);
    frustumPlanes(sViewProj, sunPl);

    // Collect what to draw.
    std::vector<DrawRec> recs;
    std::vector<mat4> instMats;
    std::vector<vec4> instCols;
    std::vector<Item> opaque, transparent, casters;
    std::vector<PointLight*> plights;
    Pending pend;
    std::vector<Node*> stack{&scene};
    while (!stack.empty()) {
      Node* n = stack.back();
      stack.pop_back();
      if (!n->visible) continue;
      for (Node* c : n->children) stack.push_back(c);
      if (n->kind == NodeKind::PointLight) {
        auto* pl = static_cast<PointLight*>(n);
        if (pl->intensity > 0 && (int)plights.size() < kMaxPointLights) plights.push_back(pl);
        continue;
      }
      if (n->kind == NodeKind::Group) continue;
      auto* m = static_cast<Mesh*>(n);
      Geometry* g = m->geometry;
      Material* mat = m->material;
      if (!g || !mat || g->vertexCount() == 0) continue;
      uint32_t count = (uint32_t)std::min<size_t>(g->drawCount, g->elementCount() > g->drawStart ? g->elementCount() - g->drawStart : 0);
      if (count == 0) continue;
      bool inst = n->kind == NodeKind::Instanced;
      auto* im = inst ? static_cast<InstancedMesh*>(n) : nullptr;
      if (inst && im->count() == 0) continue;
      vec3 c = n->world.transformPoint(g->boundsCenter);
      float r = g->boundsRadius * n->world.maxScale();
      bool culled = n->frustumCulled && !inst;
      bool seen = !culled || inFrustum(camPl, c, r);
      bool casts = n->castShadow && sun.castShadow && (n->kind == NodeKind::Mesh || inst);
      if (casts && !inst && !inFrustum(sunPl, c, r)) casts = false;
      if (!seen && !casts) continue;
      if (mat->transparent && mat->opacity <= 0 && !mat->depthWrite && !casts) continue;

      stageGeometry(g, pend);
      if (mat->map) stageTexture(mat->map, pend);

      DrawRec d{};
      d.model = n->world;
      mat3 nm = n->world.normalMatrix();
      d.n0 = {nm.m[0], nm.m[1], nm.m[2], 0};
      d.n1 = {nm.m[3], nm.m[4], nm.m[5], 0};
      d.n2 = {nm.m[6], nm.m[7], nm.m[8], 0};
      d.color = {mat->color.r, mat->color.g, mat->color.b, mat->opacity};
      Color e = mat->emissive * mat->emissiveIntensity;
      d.emissive = {e.r, e.g, e.b, 0};
      d.params = {mat->roughness, mat->metalness, mat->pointSize, 0};
      uint32_t flags = 0;
      bool lit = mat->shading == Shading::Standard && n->kind != NodeKind::Lines && n->kind != NodeKind::Points && n->kind != NodeKind::Sprite;
      if (lit) flags |= F_LIT;
      if (mat->map) flags |= F_MAP;
      if (inst) {
        flags |= F_INSTANCED;
        d.info[2] = (uint32_t)instMats.size();
        instMats.insert(instMats.end(), im->matrices.begin(), im->matrices.end());
        if (!im->colors.empty()) {
          flags |= F_ICOLOR;
          for (const Color& col : im->colors) instCols.push_back({col.r, col.g, col.b, 1});
        }
        // Colours sit beside their matrices, so keep the two arrays the same length.
        instCols.resize(instMats.size(), vec4{1, 1, 1, 1});
      }
      if (n->receiveShadow && sun.castShadow) flags |= F_RECEIVE;
      if (mat->fog && scene.fog.enabled) flags |= F_FOG;
      if (mat->side == Side::Double) flags |= F_DOUBLE;
      d.info[0] = flags;  // info[1], the texture slot, is filled in once textures are uploaded
      uint32_t rec = (uint32_t)recs.size();
      recs.push_back(d);

      float depth = -(cam.view.transformPoint(c)).z;
      Program prog = n->kind == NodeKind::Points ? P_POINTS : n->kind == NodeKind::Sprite ? P_SPRITE : P_MESH;
      Item it{m, rec, depth, n->renderOrder, (uint32_t)prog};
      if (seen) (mat->transparent ? transparent : opaque).push_back(it);
      if (casts && g->topology == Topology::Triangles) casters.push_back(it);
    }

    beginCmd();
    recordUploads(pend);
    // Texture slots are known now.
    auto fixTex = [&](std::vector<Item>& v) {
      for (Item& it : v)
        if (it.m->material->map) recs[it.rec].info[1] = (uint32_t)it.m->material->map->gpuSlot;
    };
    fixTex(opaque);
    fixTex(transparent);

    // Per-frame data.
    FrameUBO u{};
    u.viewProj = viewProj;
    u.view = cam.view;
    u.invViewProj = viewProj.inverse();
    u.shadowViewProj = sViewProj;
    u.shadowMat = bias * sViewProj;
    u.camPos = vec4(cam.position, 1);
    vec3 sd = (sun.position - sun.target).normalized();
    u.sunDir = vec4(sd, sun.castShadow ? 1.f : 0.f);
    Color sc = sun.color * sun.intensity;
    u.sunColor = {sc.r, sc.g, sc.b, 0};
    u.fillDir = vec4((scene.fill.position - scene.fill.target).normalized(), 0);
    Color fc = scene.fill.color * scene.fill.intensity;
    u.fillColor = {fc.r, fc.g, fc.b, 0};
    Color hs = scene.hemi.sky * scene.hemi.intensity, hg = scene.hemi.ground * scene.hemi.intensity;
    u.hemiSky = {hs.r, hs.g, hs.b, 0};
    u.hemiGround = {hg.r, hg.g, hg.b, 0};
    u.fogColor = {scene.fog.color.r, scene.fog.color.g, scene.fog.color.b, scene.fog.enabled ? 1.f : 0.f};
    u.fogRange = {scene.fog.near, scene.fog.far, 0, 0};
    u.shadowParams = {sun.shadow.bias, sun.shadow.normalBias, 1.f / shadow.w, sun.shadow.radius};
    mat4 camWorld = cam.view.inverse();
    u.camRight = vec4(camWorld.column(0).normalized(), 0);
    u.camUp = vec4(camWorld.column(1).normalized(), 0);
    u.viewport = {(float)opt.width, (float)opt.height, opt.height / 720.f, (float)plights.size()};
    u.skyTop = {scene.sky.top.r, scene.sky.top.g, scene.sky.top.b, scene.sky.radius};
    u.skyHorizon = {scene.sky.horizon.r, scene.sky.horizon.g, scene.sky.horizon.b, 0};
    for (size_t i = 0; i < plights.size(); i++) {
      PointLight* pl = plights[i];
      u.pointPos[i] = vec4(pl->world.position(), 1);
      Color pc = pl->color * pl->intensity;
      u.pointColor[i] = {pc.r, pc.g, pc.b, 0};
      u.pointParams[i] = {pl->distance, pl->decay, 0, 0};
    }
    std::memcpy(ubo.map, &u, sizeof u);
    ensureCapacity(drawBuf, std::max<size_t>(1, recs.size()) * sizeof(DrawRec));
    ensureCapacity(instBuf, std::max<size_t>(1, instMats.size()) * sizeof(mat4));
    ensureCapacity(icolBuf, std::max<size_t>(1, instCols.size()) * sizeof(vec4));
    if (!recs.empty()) std::memcpy(drawBuf.map, recs.data(), recs.size() * sizeof(DrawRec));
    if (!instMats.empty()) std::memcpy(instBuf.map, instMats.data(), instMats.size() * sizeof(mat4));
    if (!instCols.empty()) std::memcpy(icolBuf.map, instCols.data(), instCols.size() * sizeof(vec4));

    // Sort: opaque by order, pipeline, then near to far; transparent by order, then far to near.
    auto opaqueKey = [&](const Item& it) {
      const Material* mt = it.m->material;
      return (it.pipeKey << 4) | ((uint32_t)mt->side << 1) | (uint32_t)mt->depthWrite;
    };
    std::stable_sort(opaque.begin(), opaque.end(), [&](const Item& a, const Item& b) {
      if (a.order != b.order) return a.order < b.order;
      uint32_t ka = opaqueKey(a), kb = opaqueKey(b);
      if (ka != kb) return ka < kb;
      return a.depth < b.depth;
    });
    std::stable_sort(transparent.begin(), transparent.end(), [](const Item& a, const Item& b) {
      if (a.order != b.order) return a.order < b.order;
      return a.depth > b.depth;
    });

    stats = {};
    VkBuffer boundVB = VK_NULL_HANDLE;
    VkDeviceSize boundVOff = ~0ull, boundIOff = ~0ull;
    auto bindGeo = [&](Geometry* g) {
      const GeoGPU& gg = geos[g->gpuSlot];
      VkBuffer b = chunks[gg.chunk].buf.buf;
      if (b != boundVB || gg.vOff != boundVOff) {
        vkCmdBindVertexBuffers(cmd, 0, 1, &b, &gg.vOff);
        boundVOff = gg.vOff;
      }
      if (gg.indexed && (b != boundVB || gg.iOff != boundIOff)) {
        vkCmdBindIndexBuffer(cmd, b, gg.iOff, VK_INDEX_TYPE_UINT32);
        boundIOff = gg.iOff;
      }
      boundVB = b;
    };
    auto drawItem = [&](const Item& it) {
      Geometry* g = it.m->geometry;
      const GeoGPU& gg = geos[g->gpuSlot];
      uint32_t count = std::min<uint32_t>(g->drawCount, gg.elements - std::min(gg.elements, g->drawStart));
      if (!count) return;
      uint32_t instances = it.m->kind == NodeKind::Instanced ? (uint32_t)static_cast<InstancedMesh*>(it.m)->count() : 1;
      bindGeo(g);
      uint32_t pcv[4] = {it.rec, 0, 0, 0};
      vkCmdPushConstants(cmd, scenePL, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16, pcv);
      if (gg.indexed) vkCmdDrawIndexed(cmd, count, instances, g->drawStart, 0, 0);
      else vkCmdDraw(cmd, count, instances, g->drawStart, 0);
      if (g->topology == Topology::Triangles) stats.triangles += (size_t)count / 3 * instances;
    };
    auto setViewport = [&](uint32_t w, uint32_t h, bool flip) {
      VkViewport vp{0, flip ? (float)h : 0.f, (float)w, flip ? -(float)h : (float)h, 0, 1};
      VkRect2D sc2{{0, 0}, {w, h}};
      vkCmdSetViewport(cmd, 0, 1, &vp);
      vkCmdSetScissor(cmd, 0, 1, &sc2);
    };

    // Shadow pass.
    if (sun.castShadow) {
      VkClearValue cv{};
      cv.depthStencil = {1, 0};
      VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      rb.renderPass = shadowPass;
      rb.framebuffer = shadowFb;
      rb.renderArea = {{0, 0}, {shadow.w, shadow.h}};
      rb.clearValueCount = 1;
      rb.pClearValues = &cv;
      vkCmdBeginRenderPass(cmd, &rb, VK_SUBPASS_CONTENTS_INLINE);
      setViewport(shadow.w, shadow.h, true);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePL, 0, 1, &sceneSet, 0, nullptr);
      int bound = -1;
      for (const Item& it : casters) {
        // Front-facing materials draw their back faces into the shadow map, which avoids acne on lit faces.
        Side s = it.m->material->side;
        int pi = s == Side::Double ? 0 : s == Side::Front ? 2 : 1;
        if (pi != bound) { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipes[pi]); bound = pi; }
        drawItem(it);
        stats.shadowDraws++;
      }
      vkCmdEndRenderPass(cmd);
    }

    // Scene pass.
    {
      VkClearValue cv[3]{};
      cv[0].color = {{0, 0, 0, 1}};
      cv[1].depthStencil = {1, 0};
      VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      rb.renderPass = scenePass;
      rb.framebuffer = sceneFb;
      rb.renderArea = {{0, 0}, {hdr.w, hdr.h}};
      rb.clearValueCount = samples == VK_SAMPLE_COUNT_1_BIT ? 2 : 3;
      rb.pClearValues = cv;
      vkCmdBeginRenderPass(cmd, &rb, VK_SUBPASS_CONTENTS_INLINE);
      setViewport(hdr.w, hdr.h, true);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePL, 0, 1, &sceneSet, 0, nullptr);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipe);
      vkCmdDraw(cmd, 3, 1, 0, 0);
      VkPipeline bound = VK_NULL_HANDLE;
      auto use = [&](VkPipeline p) {
        if (p != bound) { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p); bound = p; }
      };
      auto cullFor = [](Side s) -> VkCullModeFlags {
        return s == Side::Double ? VK_CULL_MODE_NONE : s == Side::Front ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT;
      };
      for (const Item& it : opaque) {
        const Material* mt = it.m->material;
        use(scenePipe((Program)it.pipeKey, it.m->geometry->topology, cullFor(mt->side), false, mt->depthTest, mt->depthWrite));
        drawItem(it);
        stats.draws++;
      }
      for (const Item& it : transparent) {
        const Material* mt = it.m->material;
        Topology topo = it.m->geometry->topology;
        if (mt->side == Side::Double && topo == Topology::Triangles) {
          // Two passes so the far side of a see-through shape is drawn first.
          use(scenePipe((Program)it.pipeKey, topo, VK_CULL_MODE_FRONT_BIT, true, mt->depthTest, mt->depthWrite));
          drawItem(it);
          use(scenePipe((Program)it.pipeKey, topo, VK_CULL_MODE_BACK_BIT, true, mt->depthTest, mt->depthWrite));
          drawItem(it);
          stats.draws += 2;
        } else {
          use(scenePipe((Program)it.pipeKey, topo, cullFor(mt->side), true, mt->depthTest, mt->depthWrite));
          drawItem(it);
          stats.draws++;
        }
      }
      vkCmdEndRenderPass(cmd);
    }

    // Bloom.
    auto postPassRun = [&](VkFramebuffer fb, const Image& target, VkPipeline pipe, VkDescriptorSet set, const void* pcData, uint32_t pcSize, VkRenderPass pass) {
      VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      rb.renderPass = pass;
      rb.framebuffer = fb;
      rb.renderArea = {{0, 0}, {target.w, target.h}};
      vkCmdBeginRenderPass(cmd, &rb, VK_SUBPASS_CONTENTS_INLINE);
      setViewport(target.w, target.h, false);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, postPL, 0, 1, &set, 0, nullptr);
      if (pcSize) vkCmdPushConstants(cmd, postPL, VK_SHADER_STAGE_FRAGMENT_BIT, 0, pcSize, pcData);
      vkCmdDraw(cmd, 3, 1, 0, 0);
      vkCmdEndRenderPass(cmd);
    };
    bool bloomOn = post.bloomStrength > 0;
    if (bloomOn) {
      float th[4] = {post.bloomThreshold, 0, 0, 0};
      postPassRun(brightFb, bright, brightPipe, brightSet, th, 16, postPass);
      // Wider kernels at the coarser levels; sigma is a third of the radius.
      const int radii[kBloomLevels] = {6, 10, 14, 18, 22};
      for (int i = 0; i < kBloomLevels; i++) {
        struct { float sx, sy; int radius; float sigma; } b{1.f / bloomH[i].w, 0, radii[i], radii[i] / 3.f};
        postPassRun(bloomHFb[i], bloomH[i], blurPipe, blurHSet[i], &b, 16, postPass);
        b = {0, 1.f / bloomV[i].h, radii[i], radii[i] / 3.f};
        postPassRun(bloomVFb[i], bloomV[i], blurPipe, blurVSet[i], &b, 16, postPass);
      }
    }

    // Tone map and grade into the output image, then copy it out.
    {
      const float factors[kBloomLevels] = {1.0f, 0.8f, 0.6f, 0.4f, 0.2f};
      float f[kBloomLevels];
      // Each level's weight leans toward the coarse levels as the radius grows; the overall gain is 3 × strength.
      for (int i = 0; i < kBloomLevels; i++) f[i] = bloomOn ? 3 * post.bloomStrength * lerp(factors[i], 1.2f - factors[i], post.bloomRadius) : 0.f;
      float pcData[16] = {f[0], f[1], f[2], f[3],
                          f[4], scene.exposure, (float)opt.width / opt.height, post.dither,
                          post.saturation, post.vignette, post.fade, post.flash,
                          post.focusCenter.x, post.focusCenter.y, post.focusRadius, post.focusAmount};
      postPassRun(outFb, out, outPipe, outSet, pcData, 64, outPass);
      VkBufferImageCopy r{};
      r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      r.imageExtent = {out.w, out.h, 1};
      vkCmdCopyImageToBuffer(cmd, out.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buf, 1, &r);
      VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
      mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      mb.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    }
    submitAndWait();
    std::memcpy(pixels.data(), readback.map, pixels.size());
  }

  ~Impl() {
    if (!dev) return;
    vkDeviceWaitIdle(dev);
    for (auto& [k, p] : scenePipes) vkDestroyPipeline(dev, p, nullptr);
    for (VkPipeline p : shadowPipes) if (p) vkDestroyPipeline(dev, p, nullptr);
    for (VkPipeline p : {skyPipe, brightPipe, blurPipe, outPipe}) if (p) vkDestroyPipeline(dev, p, nullptr);
    for (auto& [k, m] : modules) vkDestroyShaderModule(dev, m, nullptr);
    vkDestroyPipelineLayout(dev, scenePL, nullptr);
    vkDestroyPipelineLayout(dev, postPL, nullptr);
    vkDestroyDescriptorPool(dev, descPool, nullptr);
    vkDestroyDescriptorSetLayout(dev, sceneLayout, nullptr);
    vkDestroyDescriptorSetLayout(dev, postLayout, nullptr);
    for (VkFramebuffer f : {sceneFb, shadowFb, brightFb, outFb}) if (f) vkDestroyFramebuffer(dev, f, nullptr);
    for (int i = 0; i < kBloomLevels; i++) { vkDestroyFramebuffer(dev, bloomHFb[i], nullptr); vkDestroyFramebuffer(dev, bloomVFb[i], nullptr); }
    for (VkRenderPass p : {scenePass, shadowPass, postPass, outPass}) if (p) vkDestroyRenderPass(dev, p, nullptr);
    for (VkSampler s : {linearClamp, shadowSampler, texSampler}) if (s) vkDestroySampler(dev, s, nullptr);
    for (Image* im : {&msaaColor, &depth, &hdr, &shadow, &bright, &out, &white}) destroy(*im);
    for (int i = 0; i < kBloomLevels; i++) { destroy(bloomH[i]); destroy(bloomV[i]); }
    for (auto& t : texs) destroy(t.img);
    for (auto& c : chunks) destroy(c.buf);
    for (Buffer* b : {&ubo, &drawBuf, &instBuf, &icolBuf, &readback, &staging}) destroy(*b);
    vkDestroyFence(dev, fence, nullptr);
    vkDestroyCommandPool(dev, cmdPool, nullptr);
    vkDestroyDevice(dev, nullptr);
    if (messenger) {
      auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
      if (fn) fn(instance, messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
  }
};

Renderer::Renderer(const RendererOptions& o) : impl_(std::make_unique<Impl>()) {
  impl_->opt = o;
  impl_->init();
}
Renderer::~Renderer() = default;
void Renderer::render(Scene& s, Camera& c, const PostSettings& p) { impl_->render(s, c, p); }
const uint8_t* Renderer::pixels() const { return impl_->pixels.data(); }
int Renderer::width() const { return impl_->opt.width; }
int Renderer::height() const { return impl_->opt.height; }
std::string Renderer::deviceName() const { return impl_->props.deviceName; }
const Renderer::Stats& Renderer::stats() const { return impl_->stats; }

}  // namespace eng
