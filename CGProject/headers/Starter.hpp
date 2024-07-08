#ifndef STARTER_HPP
#define STARTER_HPP

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <chrono>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#include <tiny_gltf.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <plusaes.hpp>

#define SINFL_IMPLEMENTATION
#include <sinfl.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

extern std::vector<const char*> deviceExtensions;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete();
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

struct errorcode {
    VkResult resultCode;
    std::string meaning;
};

extern errorcode ErrorCodes[];

void PrintVkError(VkResult result);

std::vector<char> readFile(const std::string& filename);

class BaseProject;

struct VertexBindingDescriptorElement {
    uint32_t binding;
    uint32_t stride;
    VkVertexInputRate inputRate;
};

enum VertexDescriptorElementUsage { POSITION, NORMAL, UV, COLOR, TANGENT, OTHER };

struct VertexDescriptorElement {
    uint32_t binding;
    uint32_t location;
    VkFormat format;
    uint32_t offset;
    uint32_t size;
    VertexDescriptorElementUsage usage;
};

struct VertexComponent {
    bool hasIt;
    uint32_t offset;
};

struct VertexDescriptor {
    BaseProject* BP;

    VertexComponent Position;
    VertexComponent Normal;
    VertexComponent UV;
    VertexComponent Color;
    VertexComponent Tangent;

    std::vector<VertexBindingDescriptorElement> Bindings;
    std::vector<VertexDescriptorElement> Layout;

    void init(BaseProject* bp, std::vector<VertexBindingDescriptorElement> B, std::vector<VertexDescriptorElement> E);
    void cleanup();

    std::vector<VkVertexInputBindingDescription> getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

enum ModelType { OBJ, GLTF, MGCG };

template <class Vert>
class Model {
    BaseProject* BP;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VertexDescriptor* VD;

public:
    std::vector<Vert> vertices{};
    std::vector<uint32_t> indices{};
    void loadModelOBJ(std::string file);
    void loadModelGLTF(std::string file, bool encoded);
    void createIndexBuffer();
    void createVertexBuffer();

    void init(BaseProject* bp, VertexDescriptor* VD, std::string file, ModelType MT);
    void initMesh(BaseProject* bp, VertexDescriptor* VD);
    void cleanup();
    void bind(VkCommandBuffer commandBuffer);
};

struct Texture {
    BaseProject* BP;
    uint32_t mipLevels;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    int imgs;
    static const int maxImgs = 6;

    void createTextureImage(const char* const files[], VkFormat Fmt);
    void createTextureImageView(VkFormat Fmt);
    void createTextureSampler(VkFilter magFilter,
        VkFilter minFilter,
        VkSamplerAddressMode addressModeU,
        VkSamplerAddressMode addressModeV,
        VkSamplerMipmapMode mipmapMode,
        VkBool32 anisotropyEnable,
        float maxAnisotropy,
        float maxLod
    );

    void init(BaseProject* bp, const char* file, VkFormat Fmt, bool initSampler);
    void initCubic(BaseProject* bp, const char* files[6]);
    void cleanup();
};

struct DescriptorSetLayoutBinding {
    uint32_t binding;
    VkDescriptorType type;
    VkShaderStageFlags flags;
};

struct DescriptorSetLayout {
    BaseProject* BP;
    VkDescriptorSetLayout descriptorSetLayout;

    void init(BaseProject* bp, std::vector<DescriptorSetLayoutBinding> B);
    void cleanup();
};

struct Pipeline {
    BaseProject* BP;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    std::vector<DescriptorSetLayout*> D;

    VkCompareOp compareOp;
    VkPolygonMode polyModel;
    VkCullModeFlagBits CM;
    bool transp;

    VertexDescriptor* VD;

    void init(BaseProject* bp, VertexDescriptor* vd,
        const std::string& VertShader, const std::string& FragShader,
        std::vector<DescriptorSetLayout*> D);
    void setAdvancedFeatures(VkCompareOp _compareOp, VkPolygonMode _polyModel,
        VkCullModeFlagBits _CM, bool _transp);
    void create();
    void destroy();
    void bind(VkCommandBuffer commandBuffer);

    VkShaderModule createShaderModule(const std::vector<char>& code);
    void cleanup();
};

enum DescriptorSetElementType { UNIFORM, TEXTURE };

struct DescriptorSetElement {
    int binding;
    DescriptorSetElementType type;
    int size;
    Texture* tex;
};

struct DescriptorSet {
    BaseProject* BP;

    std::vector<std::vector<VkBuffer>> uniformBuffers;
    std::vector<std::vector<VkDeviceMemory>> uniformBuffersMemory;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<bool> toFree;

    void init(BaseProject* bp, DescriptorSetLayout* L,
        std::vector<DescriptorSetElement> E);
    void cleanup();
    void bind(VkCommandBuffer commandBuffer, Pipeline& P, int setId, int currentImage);
    void map(int currentImage, void* src, int size, int slot);
};

// Add this to Starter.hpp, after all the other declarations and before the #endif

class BaseProject {
public:
    virtual void setWindowParameters() = 0;
    void run() {
        windowResizable = GLFW_FALSE;
        setWindowParameters();
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

protected:
    uint32_t windowWidth;
    uint32_t windowHeight;
    bool windowResizable;
    std::string windowTitle;
    VkClearColorValue initialBackgroundColor;
    int uniformBlocksInPool;
    int texturesInPool;
    int setsInPool;

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;

    VkDescriptorPool descriptorPool;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    size_t currentFrame = 0;
    bool framebufferResized = false;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    void initWindow();
    virtual void onWindowResize(int w, int h) = 0;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    virtual void localInit() = 0;
    virtual void pipelinesAndDescriptorSetsInit() = 0;
    void initVulkan();
    void mainLoop();
    virtual void updateUniformBuffer(uint32_t currentImage) = 0;
    virtual void pipelinesAndDescriptorSetsCleanup() = 0;
    virtual void localCleanup() = 0;
    void cleanup();
    void recreateSwapChain();
    void cleanupSwapChain();

    // ... [Other methods like createInstance, pickPhysicalDevice, etc.]

    // Helper methods
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
        int imgCount, VkSampleCountFlagBits numSamples, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VkImageCreateFlags cflags, VkMemoryPropertyFlags properties,
        VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format,
        VkImageAspectFlags aspectFlags, uint32_t mipLevels,
        VkImageViewType type, int layerCount);

    // ... [Other helper methods]

    // Public utility methods
    void RebuildPipeline();
    void handleGamePad(int id, glm::vec3& m, glm::vec3& r, bool& fire);
    void getSixAxis(float& deltaT, glm::vec3& m, glm::vec3& r, bool& fire);

    // Debug methods
    void printFloat(const char* Name, float v);
    void printVec2(const char* Name, glm::vec2 v);
    void printVec3(const char* Name, glm::vec3 v);
    void printVec4(const char* Name, glm::vec4 v);
    void printMat3(const char* Name, glm::mat3 v);
    void printMat4(const char* Name, glm::mat4 v);
};

#endif // STARTER_HPP