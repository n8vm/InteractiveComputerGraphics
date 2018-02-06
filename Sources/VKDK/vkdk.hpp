#pragma once 

/*
	A Significant portion of this code was taken and modified from https://vulkan-tutorial.com/
*/

#define OPENGL_SOURCES_PATH "./Sources/OpenGL/opengl_sources.txt"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <stdarg.h>
#include <vector>
#include <unordered_map>
#include <iostream>
//#define GL_LOG_FILE "gl.log"
#include "objload.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#define NUM_DESCRIPTOR_SETS 10

namespace VKDK {
	/* ------------------------------------------*/
	/* STRUCTS                                   */
	/* ------------------------------------------*/

	/* i_Width, i_Height, -s_title, b_fullscreen, b_WindowHidden*/
	struct InitializationParameters {
		int windowWidth;
		int windowHeight;
		std::string windowTitle;
		bool fullScreenActive;
		bool windowHidden;
		bool verbose;
	};
	
	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;

		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/* ------------------------------------------*/
	/* FIELDS                                    */
	/* ------------------------------------------*/
	extern bool Verbose;
	extern GLFWwindow *DefaultWindow;
	extern GLFWmonitor *DefaultMonitor;
	extern bool FullScreenActive;
	extern bool WindowHidden;
	extern int DefaultWindowSize[2];
	extern int DefaultWindowPos[2];
	extern int PreviousWindowSize[2];
	extern int PreviousWindowPos[2];
	extern std::string DefaultWindowTitle;
	extern VkInstance instance;
	extern const std::vector<const char*> validationLayers;
	extern const bool enableValidationLayers;
	extern VkPhysicalDevice physicalDevice;
	extern VkDevice device;
	extern VkQueue graphicsQueue;
	extern VkQueue presentQueue;
	extern VkSurfaceKHR surface;
	extern const std::vector<const char*> deviceExtensions;
	extern VkSwapchainKHR swapChain;
	extern std::vector<VkImage> swapChainImages;
	extern VkFormat swapChainImageFormat;
	extern VkExtent2D swapChainExtent;
	extern std::vector<VkImageView> swapChainImageViews;
	//extern VkPipelineLayout pipelineLayout;
	//extern VkPipeline graphicsPipeline;
	extern VkRenderPass renderPass;
	extern std::vector<VkFramebuffer> swapChainFramebuffers;
	extern VkCommandPool commandPool;
	extern std::vector<VkCommandBuffer> commandBuffers;
	extern VkSemaphore imageAvailableSemaphore;
	extern VkSemaphore renderFinishedSemaphore;
	extern VkDebugReportCallbackEXT callback;


	/* ------------------------------------------*/
	/* FUNCTIONS                                 */
	/* ------------------------------------------*/
	extern bool Initialize(InitializationParameters parameters);
	
	/* GLFW Initialization */
	extern void InitWindow();
	extern int CreateNewWindow(int width, int height, std::string title, GLFWmonitor *monitor, GLFWwindow *share);
	extern void MakeContextCurrent(GLFWwindow *window);
	extern void SetFullScreen(bool fullscreen, int windowPos[2] = PreviousWindowPos, int windowSize[2] = PreviousWindowSize);
	extern void SetWindowHidden(bool hidewindow);

	/* Vulkan Instance Creation */
	extern void CreateInstance();
	extern bool CheckValidationLayerSupport();
	extern std::vector<const char*> GetRequiredExtensions();

	/* Vulkan Validation Layer */
	extern void SetupDebugCallback();
	extern VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	extern VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
	extern void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	/* Create Vulkan Surface */
	extern void CreateSurface();

	/* Choose Physical Device */
	extern void PickPhysicalDevice();
	extern bool IsDeviceSuitable(VkPhysicalDevice device);
	extern bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	/* Create Logical Device */
	extern void CreateLogicalDevice();
	extern QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	/* Create Swap Chain */
	extern void CreateSwapChain();
	extern SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	extern VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	extern VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	extern VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	/* Create Image Views */
	extern void CreateImageViews();

	/* Create Render Pass */
	extern void CreateRenderPass();

	/* Create Descriptor Set */
	//extern void CreateDescriptorSetLayout();

	/* Create Graphics Pipeline */
	extern void CreateGraphicsPipeline();
	extern VkShaderModule createShaderModule(const std::vector<char>& code);
	
	/* Create Frame Buffers */
	extern void CreateFrameBuffers();
	
	/* Create Command Pools */
	extern void CreateCommandPools();
	
	/* Create Depth Resources */
	extern void CreateDepthResources();

	/* Create Texture Image */
	extern void CreateTextureImage();

	/* Create Texture Image View */
	extern void CreateTextureImageView();

	/* Create Texture Sampler */
	extern void CreateTextureSampler();

	extern void LoadModel();

	/* Create Vertex Buffer */
	extern void CreateVertexBuffer();
	extern void CreateIndexBuffer();
	extern void CreateUniformBuffer();
	extern void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	extern void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	/* Create Descriptor Pool */
	//extern void CreateDescriptorPool();

	/* Create Descriptor Set */
	//extern void CreateDescriptorSet();

	/* Create Command Buffers */
	extern void CreateCommandBuffers();
	extern void StartCommandBufferRecording(int imageIndex);
	extern void StopCommandBufferRecording(int imageIndex);

	/* Create Semaphores */
	extern void CreateSemaphores();
	
	/* Callbacks */
	extern void OnWindowResized(GLFWwindow* window, int width, int height);
	extern void ErrorCallback(int _error, const char* description);
	

	/* Misc Functions */
	extern void print(std::string s, bool forced = false);
	extern std::string loadFile(std::string name);
	extern bool ShouldClose();
	
	extern void DrawFrame(uint32_t imageIndex);
	extern void TestDraw1();
	extern void UpdateUniformBuffer();
	extern bool OptimizeSwapchain(int swapchainResult);
	uint32_t GetNextImageIndex();
	extern void RecreateSwapChain();
	extern bool SwapChainOutOfDate;
	extern void SetClearColor(float r, float g, float b, float a);

	/* Cleanup */
	extern void CleanupSwapChain();
	extern void Terminate();
};
