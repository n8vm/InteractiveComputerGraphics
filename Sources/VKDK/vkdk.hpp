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
#include <atomic>
#define NUM_DESCRIPTOR_SETS 10

#define NOMINMAX
#include "VulkanTools.hpp"


namespace VKDK {
	/* ------------------------------------------*/
	/* STRUCTS                                   */
	/* ------------------------------------------*/

	/* i_Width, i_Height, -s_title, b_fullscreen, b_WindowHidden, b_verbose, b_vsync_enabled */
	struct InitializationParameters {
		int initialWindowWidth = 512;
		int initialWindowHeight = 512;
		std::string windowTitle = "Default Window Title";
		bool fullScreenActive = false;
		bool windowHidden = false;
		bool verbose = false;
		bool vsyncEnabled = false;
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
	/* GLFW FIELDS                               */
	/* ------------------------------------------*/

	/* Keeps track of settings like vsync, default window size, window title, etc */
	extern InitializationParameters currentSettings;

	/* Handle to the glfw window, which can be used for event queries */
	extern GLFWwindow *DefaultWindow;

	/* Opaque monitor object */
	extern GLFWmonitor *DefaultMonitor;

	extern uint32_t CurrentWindowSize[2];
	extern uint32_t CurrentWindowPos[2];
	extern uint32_t PreviousWindowSize[2];
	extern uint32_t PreviousWindowPos[2];

	/* ------------------------------------------*/
	/* VULKAN FIELDS                             */
	/* ------------------------------------------*/

	/* Handle to the current vulkan context */
	extern VkInstance instance;

	/* A vector to the validation layers to enable */
	extern const std::vector<const char*> validationLayers;

	/* Disable/Enable validation layers. False by default on release, true by default on debug.  */
	extern const bool enableValidationLayers;

	/* Function called when validation layers "validate" */
	extern VkDebugReportCallbackEXT callback;

	/* Logical device, application's view of the physical device (GPU) */
	extern VkDevice device;

	/* Physical device, handle used to create logical device */
	extern VkPhysicalDevice physicalDevice;

	/* Properties of the physical device including limits that the application can check against */
	extern VkPhysicalDeviceProperties deviceProperties;

	/* Features of the physical device that an application can use to check if a feature is supported */
	extern VkPhysicalDeviceFeatures deviceFeatures;

	/* A vector to the requested extensions for the logical device */
	extern const std::vector<const char*> deviceExtensions;
	
	/* Handle to the device graphics queue that command buffers are submitted to */
	extern VkQueue graphicsQueue;

	/* Handle to the device present queue that command buffers are submitted to */
	extern VkQueue presentQueue;

	/* Command buffer pool */
	extern VkCommandPool commandPool;

	/* Pipeline stages used to wait at for graphics queue submissions */
	extern VkPipelineStageFlags submitPipelineStages;

	/* Contains command buffers and semaphores to be presented to the queue */
	extern VkSubmitInfo submitInfo;

	/* Command buffers used for final rendering pass */
	extern std::vector<VkCommandBuffer> drawCmdBuffers;

	/* Global render pass for frame buffer writes */
	extern VkRenderPass renderPass;

	/* List of available frame buffers (same as number of swap chain images) */
	extern std::vector<VkFramebuffer> swapChainFramebuffers;

	/* Active frame buffer index */
	extern uint32_t swapIndex;

	/* Pipeline cache object (TODO: figure out how to implement this )*/
	// VkPipelineCache pipelineCache

	/* Wraps the swap chain to present images (framebuffers) to the windowing system */
	extern VkSwapchainKHR swapChain;
	
	/* The image format for the given swapchain images */
	extern VkFormat swapChainImageFormat;
	
	/* The 2D extent of the swapchain images. Usually this matches the window dimensions. */
	extern VkExtent2D swapChainExtent;

	/* The final images to be rendered to before presenting */
	extern std::vector<VkImage> swapChainImages;

	/* Opaque handles to the swaptchain images */
	extern std::vector<VkImageView> swapChainImageViews;

	extern std::atomic_bool prepared;

	/* Abstracts a native platform surface or window object for use with Vulkan */
	extern VkSurfaceKHR surface;

	struct SemaphoreStruct {
		/* Offscreen submission and execution */
		VkSemaphore offscreenComplete;
		/* Swap chain image presentation */
		VkSemaphore presentComplete;
		/* Command buffer submission and execution */
		VkSemaphore renderComplete;
		/* UI overlay submission and execution */
		VkSemaphore overlayComplete;
	};

	/* Synchronization semaphores */
	extern SemaphoreStruct semaphores;

	/* ------------------------------------------*/
	/* FUNCTIONS                                 */
	/* ------------------------------------------*/

	/* Initializes all required Vulkan objects. Should be called on application startup. */
	extern bool Initialize(InitializationParameters parameters);
	
	/* GLFW Initialization */
	extern void InitWindow();
	extern int CreateNewWindow(int width, int height, std::string title, GLFWmonitor *monitor, GLFWwindow *share);
	extern void MakeContextCurrent(GLFWwindow *window);
	extern void SetFullScreen(bool fullscreen, uint32_t windowPos[2] = PreviousWindowPos, uint32_t windowSize[2] = PreviousWindowSize);
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

	/* Create Global Render Pass */
	extern void CreateGlobalRenderPass();
	
	/* Create Frame Buffers */
	extern void CreateFrameBuffers();
	
	/* Create Command Pools */
	extern void CreateCommandPools();
	
	/* Create Depth Resources */
	extern void CreateDepthResources();

	/* Create Texture Image (TODO: consider removing this) */
	//extern void CreateTextureImage();

	/* Create Texture Image View */
	extern void CreateTextureImageView();

	/* Create Texture Sampler (TODO: consider removing this) */
	//extern void CreateTextureSampler();
	
	/* Returns the index to some device memory properties */
	extern uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	/* Creates a device side buffer attached to device memory of a given size */
	extern void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	
	/* Copies a given number of bytes from a source buffer to a destination buffer */
	extern void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	/* Create Render Command Buffers */
	extern void CreateCommandBuffers();
	extern void BeginRenderPass(VkCommandBuffer &commandBuffer, VkFramebuffer &framebuffer, VkRenderPass &renderPass, glm::vec4 clearColor = glm::vec4(0.0,0.0,0.0,0.0));
	extern void EndRenderPass(VkCommandBuffer &commandBuffer);
	//extern void DrawFrame();
	extern VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex);
	extern VkResult QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
	extern bool PrepareFrame();
	extern bool SubmitFrame();

	/* Allocates a one time command buffer, and begins recording commands on it. */
	VkCommandBuffer beginSingleTimeCommands();

	/* Ends a one time command buffer, and destroys it. */
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	/**
	* Allocate a command buffer from the command pool
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	*
	* @return A handle to the allocated command buffer
	*/
	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = false);
	
	/**
	* Finish command buffer recording and submit it to a queue
	*
	* @param commandBuffer Command buffer to flush
	* @param queue Queue to submit the command buffer to
	* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
	*
	* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
	* @note Uses a fence to ensure command buffer has finished executing
	*/
	void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

	/* Create Semaphores */
	extern void CreateSemaphores();
	
	/* Callbacks */
	extern void OnWindowResized(GLFWwindow* window, int width, int height);
	extern void ErrorCallback(int _error, const char* description);
	
	/* Misc Functions */
	extern void print(std::string s, bool forced = false);
	//extern std::string loadFile(std::string name);
	
	/* Returns whether or not the current GLFW window should close */
	extern bool ShouldClose();
	

	extern bool OptimizeSwapchain(int swapchainResult);
	uint32_t AquireNewFrameBuffer();
	extern void RecreateSwapChain();
	extern bool SwapChainOutOfDate;
	
	/* Cleanup */
	extern void CleanupSwapChain();
	extern void Terminate();
};
