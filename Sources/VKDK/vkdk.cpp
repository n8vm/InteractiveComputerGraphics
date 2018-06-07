#include "vkdk.hpp"
#include <iostream>
#include <fstream>
#include <streambuf>
#include <stdlib.h>
#include <set>
#include <algorithm>
#include <array>

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace VKDK {
	/* ------------------------------------------*/
	/* GLFW FIELDS                               */
	/* ------------------------------------------*/
	InitializationParameters currentSettings;
	GLFWwindow *DefaultWindow;
	GLFWmonitor *DefaultMonitor;
	uint32_t CurrentWindowSize[2] = { 512 , 512 };
	uint32_t CurrentWindowPos[2];
	uint32_t PreviousWindowSize[2] = { 512 , 512 };
	uint32_t PreviousWindowPos[2];
	
	/* ------------------------------------------*/
	/* VULKAN FIELDS                             */
	/* ------------------------------------------*/
	VkInstance instance;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation",
	};

#ifdef NDEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = true;
#endif
	
	VkDebugReportCallbackEXT callback;
	VkDevice device;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkQueue graphicsQueue;
	VkQueue presentQueue;	
	VkCommandPool commandPool;
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo;
	std::vector<VkCommandBuffer> drawCmdBuffers;	
	VkRenderPass renderPass;
	VkSurfaceKHR surface;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	uint32_t swapIndex = 0;
	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;	
	VkExtent2D swapChainExtent;	
	std::vector<VkImage> swapChainImages;	
	std::vector<VkImageView> swapChainImageViews;
	std::atomic_bool prepared = true;	
	SemaphoreStruct semaphores;

	/* Image Views */

	/* Depth buffer */
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	/* ------------------------------------------*/
	/* FUNCTIONS                                 */
	/* ------------------------------------------*/
	void print(std::string s, bool forced) {
		if (currentSettings.verbose || forced)
			printf("VKDK: %-70s\n", s.c_str());
	}

	/* Source file management */
	std::string loadFile(std::string name)
	{
		std::ifstream t(name, std::ios::binary);
		std::string str;

		t.seekg(0, std::ios::end);
		str.reserve(t.tellg());
		t.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());
		return str;
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	void ErrorCallback(int _error, const char* description)
	{
		VKDK::print("Error " + std::to_string(_error) + ": " + std::string(description));
	}

	/* Initializers */
	bool Initialize(InitializationParameters parameters) {
		/* Store the initialization parameters */
		currentSettings = parameters;
		CurrentWindowSize[0] = PreviousWindowSize[0] = parameters.initialWindowWidth;
		CurrentWindowSize[1] = PreviousWindowSize[1] = parameters.initialWindowHeight;

		/* Call Vulkan initialization/creation functions */
		try {
			InitGLFWWindow();
			CreateVulkanInstance();
			SetupDebugCallback();
			CreateSurface();

			PickPhysicalDevice();
			CreateLogicalDevice();
			CreateCommandPools();

			CreateSwapChain();
			CreateImageViews();
			CreateGlobalRenderPass();

			CreateDepthResources();
			CreateFrameBuffers();
			CreateCommandBuffers();
			CreateSemaphores();
		}
		catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
#ifndef NDEBUG
			throw std::runtime_error(e.what());
#endif
			return VK_ERROR_INITIALIZATION_FAILED;
		}
		return VK_SUCCESS;
	}

	void InitGLFWWindow() {
		print("Initializing Window");
		if (!glfwInit()) {
			throw std::runtime_error("GLFW failed to initialize!");
		}
		glfwSetErrorCallback(ErrorCallback);

		GLFWmonitor *monitor;
		monitor = NULL;

		DefaultMonitor = glfwGetPrimaryMonitor();
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		DefaultWindow = glfwCreateWindow(CurrentWindowSize[0], CurrentWindowSize[1], currentSettings.windowTitle.c_str(), monitor, NULL);
		glfwSetWindowSizeCallback(DefaultWindow, OnWindowResized);

		if (!DefaultWindow) {
			throw std::runtime_error("Failed to create default GLFW window");
		}

		SetFullScreen(currentSettings.fullScreenActive, CurrentWindowPos, CurrentWindowSize);
		SetWindowHidden(currentSettings.windowHidden);
	}

	void Terminate() {
		CleanupSwapChain();

		vkDestroySemaphore(device, semaphores.offscreenComplete, nullptr);
		vkDestroySemaphore(device, semaphores.renderComplete, nullptr);
		vkDestroySemaphore(device, semaphores.overlayComplete, nullptr);
		vkDestroySemaphore(device, semaphores.presentComplete, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyRenderPass(device, renderPass, nullptr);

		vkDestroyDevice(device, nullptr);
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(DefaultWindow);

		glfwTerminate();
	}

	void OnWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;
		CurrentWindowSize[0] = width;
		CurrentWindowSize[1] = height;
	}

	bool ShouldClose() {
		return glfwWindowShouldClose(DefaultWindow) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	}

	void SetFullScreen(bool fullscreen, uint32_t windowPos[2], uint32_t windowSize[2]) {
		if (!currentSettings.fullScreenActive && fullscreen == false) return;

		currentSettings.fullScreenActive = fullscreen;
		if (fullscreen)
		{
			int x = -1, y = -1, w = -1, h = -1;
			// backup window position and window size
			glfwGetWindowPos(DefaultWindow, &x, &y);
			glfwGetWindowSize(DefaultWindow, &w, &h);
			windowPos[0] = x; windowPos[1] = y; windowSize[0] = w; windowSize[1] = h;

			// get resolution of monitor
			const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			CurrentWindowSize[0] = mode->width;
			CurrentWindowSize[1] = mode->height;

			// switch to full screen
			glfwSetWindowMonitor(DefaultWindow, DefaultMonitor, 0, 0, mode->width, mode->height, 0);
		}
		else
		{
			CurrentWindowSize[0] = windowSize[0];
			CurrentWindowSize[1] = windowSize[1];

			// restore last window size and position
			glfwSetWindowMonitor(DefaultWindow, nullptr, windowPos[0], windowPos[1], windowSize[0], windowSize[1], 0);
		}
		/* TODO: How to replace this?*/
		//glViewport(0, 0, DefaultWindowSize[0], DefaultWindowSize[1]);

		/*
		_updateViewport = true;*/
	}

	void SetWindowHidden(bool hideWindow) {
		if (!currentSettings.windowHidden && hideWindow == false) return;
		currentSettings.windowHidden = hideWindow;
		if (hideWindow) {
			glfwHideWindow(DefaultWindow);
		}
		else {
			glfwShowWindow(DefaultWindow);
		}
	}

	void CreateVulkanInstance() {
		print("Creating Vulkan Instance");
    
		/* Optional initialization struct */
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = currentSettings.windowTitle.c_str();
		appInfo.applicationVersion = currentSettings.apiVersion;
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = currentSettings.apiVersion;
		appInfo.apiVersion = currentSettings.apiVersion;
	
		/* Required initialization struct */
		/* Specifies global extensions and validation layers we'd like to use */
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		/* Determine the required extensions */
		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		/* Check to see if we support the requested validation layers */
		if (enableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		/* Here, we specify the validation layers to use. */
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}
	
		/* Now we have everything we need to create a vulkan instance!*/
		// (struct pointer to create object, custom allocator callback, and a pointer to a vulkan instance)
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	bool CheckValidationLayerSupport() {
		/* Checks if all validation layers are supported */
	
		/* Determine how many validation layers are supported*/
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		/* Get the available layers */
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		/* Check to see if the validation layers we have selected are available */
		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	std::vector<const char*> GetRequiredExtensions() {
		/* Here, we specify that we'll need an extension to render to GLFW. */

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		/* Conditionally require the following extension, which allows us to get debug output from validation layers */
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		/* Now let's check out the supported vulkan extensions for this platform. */

		/* First, get the total number of supported extensions. */
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		/*Next, query the extension details */
		std::vector<VkExtensionProperties> extensionProperties(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

		/* Check to see if the extensions we have are what are required by GLFW */
		char** extensionstring = (char**)glfwExtensions;
		for (int i = 0; i < glfwExtensionCount; ++i) {
			std::string currentextension = std::string(*extensionstring);
			extensionstring += 1;
			bool extensionFound = false;
			for (int j = 0; j < extensions.size(); ++j)
				if (currentextension.compare(extensions[j]) == 0)
					extensionFound = true;
			if (!extensionFound) throw std::runtime_error("Missing extension " + currentextension);
		}

		return extensions;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData) {

		std::cerr << "validation layer: " << msg << std::endl;
		throw std::runtime_error("validation layer: " + std::string(msg));
		return VK_FALSE;
	}

	void SetupDebugCallback() {
		//the debug callback in Vulkan is managed with a handle that needs to be explicitly created and destroyed
		if (!enableValidationLayers) return;
		print("Adding Validation Callback");

		// Details about the callback
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = DebugCallback;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
		/* The function to assign a callback for validation layers isn't loaded by default. Here, we get the function. */
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	/* Physical Devices and Queue Families, and saves device properties */
	void PickPhysicalDevice() {
		print("Choosing Physical Device");

		/* We need to look for and select a graphics card in the system that supports the features we need */
		/* Could technically choose multiple graphics cards and use them simultaneously, but for now just choose the first one */


		/* First, get the count of devices */
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		/* Next, get a list of handles for those physical devices */
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		/* Check and see if physical devices are suitable, since not all cards are equal */
		for (const auto& device : devices) {
			if (IsDeviceSuitable(device)) {
				print("\tChoosing device " + std::string(deviceProperties.deviceName));

				physicalDevice = device;

				// Store properties (including limits), features and memory properties of the phyiscal device (so that examples can check against them)
				vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
				vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
				//vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	bool IsDeviceSuitable(VkPhysicalDevice device) {
		/* First, information like name, type, supported vulkan version can be queried */
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		/* Support for optional features like 64 bit floats, texture compression, multiple viewport rendering
		can be queried here */
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		/* as an example, we could filter based off these properties.
		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader;
			*/

		/* Look for a	queue family which supports what we need, */
		QueueFamilyIndices indices = FindQueueFamilies(device);

		/* Edit for Swap Chain */
		bool extensionsSupported = CheckDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		/* For this example, all we require is one supported image format and one supported presentation mode */
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		/* Retrieve a list of queue families */
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		/* VkQueueFamilyProperties contains information about a particular queue family. 
			We're looking for one that supports VK_QUEUE_GRAPHICS_BIT
		*/

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			/* Edit: It's possible the device we're using has a graphics queue, that that queue doesn't overlap
			with a presentation queue type for windowing purposes. Additionally, require support for that.
		
			Throughout the framework, present and graphics queues will be treated seperately, although they may be the same
			*/
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	/* Logical Device */
	void CreateLogicalDevice() {
		print("Creating Logical Device");

		/* Logical device creation is similar to instance creation, in that we specify features
			that we'd like to use. */

		/* We also need to specify queues we'll want for the created logical device */

		/* Multiple logical devices can be created from the same physical device to satisfy different requirements */

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		/* Need multiple queues, one for graphics, one for present */
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		/* Add these queue create infos to a vector to be used when creating the logical device */
		/* Vulkan allows you to specify a queue priority between 0 and one, which influences scheduling */
		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		/* Now we'll specify the device features we'll need */
		/* These are the features found in IsDeviceSuitable. */
		/* For now, we don't care to use any features. Init all fields to VK_FALSE*/
		//deviceFeatures = {};
		//deviceFeatures.samplerAnisotropy = VK_TRUE;
		//deviceFeatures.textureCompressionETC2 = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		/* Add pointers to the device features and queue creation structs */
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		/* We can specify device specific extensions, like "VK_KHR_swapchain", which may not be 
		available for particular compute only devices. */
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();


		/* Device specific validation layers have been depreciated, but for now just recycle global
		validation layers */
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		/* Now create the logical device! */
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		/* Although queues are implicitely created with the logical device, we need to get a handle for them. 
			Since we only have one queue for now, just chose the 0'th queue for the graphics queue
			Likewise for present queue, we only have one, so just choose the 0th queue
		*/
		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	}

	/* Window Surface */
	void CreateSurface() {
		print("Creating Surface");

		/*
			Although the VkSurfaceKHR object is included with vulkan, instantiating it is not.
			Creating a surface is dependent on operating system specific details. 

			GLFW will do all this for us, but for a better understanding, here's what a windows
			surface creation would look like.

				VkWin32SurfaceCreateInfoKHR createInfo;
				createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				createInfo.hwnd = glfwGetWin32Window(window); // <--- handle to the window, from GLFW window object
				createInfo.hinstance = GetModuleHandle(nullptr); // <--- handle to the process
	
			Now, the windows surface could be created using the vkCreateWin32SurfaceKHR, which is platform dependent and needs to be explicitly called. 
				auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

				if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
					throw std::runtime_error("failed to create window surface!");
				}
		*/
	
		/* GLFW niceness */
		if (glfwCreateWindowSurface(instance, DefaultWindow, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	/* Swap Chain */
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		/* Enumerate the extensions, and verify that all the required extensions are among them. */
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		/* The remove the intersection of availble and required extensions from thre required set.
			if the required set is empty, it's completely contained in the available set, and we're good to go.
		*/
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
		/* Just querying to see if a particular device supports swapchains isn't enough
			Swapchains can have a restriction of parameters to them, major ones are
				basic surface capabilities like window size, swap chain length 
				surface formats, like pixel format and color space
				available presentation modes
		*/

	
		SwapChainSupportDetails details;
		/* First, query the basic swapchain parameters*/
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		/* Next, Query surface formats. Since there can be multiple, follow 2 call style */
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		/* Finally, query supported presentation modes*/
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		/* Goals, try to use SRGB, which has a more accurate color space. */
	
		/* If there's only one presentation mode, just use that. */
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		/* If we're not free to use whatever we'd like, go through list and see if prefered is available. */
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		/* Finally if that fails, just chose the format vulkan suggests */
		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		/*There are four possible presentation modes
			VK_PRESENT_MODE_IMMEDIATE_KHR - submit images right away. Might tear. 
			VK_PRESENT_MODE_FIFO_KHR - display takes image on refresh. Most similar to v-sync
			VK_PRESENT_MODE_FIFO_RELAXED_KHR - similar to last. If queue is empty, submit image right away. Might tear.
			VK_PRESENT_MODE_MAILBOX_KHR - Instead of waiting for queue to empty, replace old images with new. Can be used for tripple buffering
		*/

		/* Prefer tripple buffer like mode */
	
		/* Only this one is guaranteed, so default return that.*/
		VkPresentModeKHR bestMode = (currentSettings.vsyncEnabled) ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;

		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		/* The extent is issentially the resolution of the image we're submitting to the swap chain
			Almost always the same resolution as our window

		*/

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetWindowSize(DefaultWindow, &width, &height);

			VkExtent2D actualExtent = { width, height };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void VKDK::CreateSwapChain() {
		print("Creating Swap Chain");

		/* Start out with the results of our helper functions, which chose nice swapchain parameters */
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

		/* Save these two results for later use */
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		/* Check to see how many images we can submit to the swapchain. 0 means within memory limits. */
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		/* Start creating swapchain creation struct */
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		/* Specify the details */
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // Always 1 unless developing stereoscopic 3D ^^
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // What we'll use the images for. Here, just directly rendering them

		/* Query for our present and graphics queues */
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

		/* Handle the special case that our graphics and present queues are seperate */
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		/* We can specify a particular transform to happen to images in the swapchain. */
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		/* We could also blend the contents of the window with other windows on the system. */
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		/* from chooseSwapPresentMode */
		createInfo.presentMode = presentMode;
	
		/* Clip pixels which are obscured by other windows. */
		createInfo.clipped = VK_TRUE;

		/* Assume for now that we'll only have one swapchain. */
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		/* Create the swap chain */
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		/* Retrieve handles to the swap chain images */
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	}

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	void CreateImageViews() {
		print("Creating Image Views");

		/* Resize list to be one to one with swap chain images list */
		swapChainImageViews.resize(swapChainImages.size());

		/* Itterate over all swapchain images */
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = CreateImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		/* Create a wrapper module for our code before sending it to the gpu */
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void CreateGlobalRenderPass() {
		print("Creating Render Pass");

		/* We need to tell vulkan about the frame buffer attachments that will be used for rendering */

		/* In our case, we just have a color image */
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clears image to black
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		/* Do I need this? */
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		/* Create the render pass */
		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void CreateFrameBuffers() {
		print("Creating Frame Buffers");

		/* Create a frame buffer for each swap chain image. */
		swapChainFramebuffers.resize(swapChainImageViews.size());
	
		/* Now create them */
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<VkImageView, 2> attachments = {
				swapChainImageViews[i],
				depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;
		
			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void CreateCommandPools() {
		print("Creating Default Command Pools");

		/* Command pools manage the memory that is used to store the buffers and command buffers are allocated from them */
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		/* Query available device memory properties */
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		/* Try to find some memory that matches the type we'd like. */
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		/* To create a VBO, we need to use this struct: */
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		/* Now create the buffer */
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		/* Identify the memory requirements for the vertex buffer */
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		/* Look for a suitable type that meets our property requirements */
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		/* Now, allocate the memory for that buffer */
		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		/* Associate the allocated memory with the VBO handle */
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}
	
	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer));

		// If requested, also start recording for the new command buffer
		if (begin)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
		}

		return cmdBuffer;
	}

	void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
	{
		if (commandBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = vks::initializers::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

		// Submit to the queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(device, fence, nullptr);

		if (free)
		{
			vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
		}
	}

	void VKDK::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		endSingleTimeCommands(commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

	//	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0; // TODO
		barrier.dstAccessMask = 0; // TODO

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void CreateDepthResources() {
		print("Creating Depth Resources");

		VkFormat depthFormat = findDepthFormat();
		CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		/* We need to transition this depth image to a usable layout for the GPU */
		transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void CreateTextureImage() {
		/* Load the texture */
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(ResourcePath "Chalet/chalet.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		/* Create staging buffer */
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);
		
		/* Clean up original image array */
		stbi_image_free(pixels);

		CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void CreateTextureImageView() {
		textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void CreateTextureSampler() {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	void CreateCommandBuffers() {
		print("Creating Default Command Buffers");

		/* Command buffer for each frame buffer */
		drawCmdBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)drawCmdBuffers.size();
		
		if (vkAllocateCommandBuffers(device, &allocInfo, drawCmdBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void VKDK::BeginRenderPass(VkCommandBuffer &commandBuffer, VkFramebuffer &framebuffer, VkRenderPass &renderPass, glm::vec4 clearColor) {
		/* Starting command buffer recording */
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		/* information about this particular render pass */
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = VKDK::swapChainExtent;
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		/* Start the render pass */
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VKDK::EndRenderPass(VkCommandBuffer &commandBuffer) {
		/* End this render pass */
		vkCmdEndRenderPass(VKDK::drawCmdBuffers[swapIndex]);

		if (vkEndCommandBuffer(drawCmdBuffers[swapIndex]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	/* Rendering and Presentation */
	void VKDK::CreateSemaphores() {
		print("Creating Default Semaphores");

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphores.offscreenComplete) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphores.renderComplete) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphores.presentComplete) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphores.overlayComplete) != VK_SUCCESS
			) {

			throw std::runtime_error("failed to create semaphores!");
		}
	}

	bool VKDK::OptimizeSwapchain(int swapchainResult) {
		if (swapchainResult == VK_ERROR_OUT_OF_DATE_KHR || swapchainResult == VK_SUBOPTIMAL_KHR) {
			RecreateSwapChain();
			return true;
		}
		else if (swapchainResult != VK_SUCCESS ) {
			throw std::runtime_error("failed to acquire/present swap chain image!");
		}
		return false;
	}

	/**
		STOLEN FROM SASCHA WILLIAMS

	* Acquires the next image in the swap chain
	*
	* @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
	* @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
	*
	* @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
	*
	* @return VkResult of the image acquisition
	*/
	VkResult VKDK::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex)
	{
		// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
		// With that we don't have to handle VK_NOT_READY
		return vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
	}

	/**
		STOLEN FROM SASCHA WILLIAMS

	* Queue an image for presentation
	*
	* @param queue Presentation queue for presenting the image
	* @param imageIndex Index of the swapchain image to queue for presentation
	* @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
	*
	* @return VkResult of the queue presentation
	*/
	VkResult VKDK::QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) {
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = NULL;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;
		// Check if a wait semaphore has been specified to wait for before presenting the image
		if (waitSemaphore != VK_NULL_HANDLE)
		{
			presentInfo.pWaitSemaphores = &waitSemaphore;
			presentInfo.waitSemaphoreCount = 1;
		}
		return vkQueuePresentKHR(queue, &presentInfo);
	}

	bool VKDK::PrepareFrame() {
		// Acquire the next image from the swap chain
		VkResult err = AcquireNextImage(semaphores.presentComplete, &swapIndex);

		// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
		if ((err == VK_ERROR_OUT_OF_DATE_KHR) || (err == VK_SUBOPTIMAL_KHR) 
			|| VKDK::PreviousWindowSize[0] != VKDK::CurrentWindowSize[0]
			|| VKDK::PreviousWindowSize[1] != VKDK::CurrentWindowSize[1]) {
			PreviousWindowSize[0] = CurrentWindowSize[0];
			PreviousWindowSize[1] = CurrentWindowSize[1];
			return true;
		}
		else {
			VK_CHECK_RESULT(err);
			return false;
		}
	}

	void VKDK::SubmitToGraphicsQueue(VKDK::SubmitToGraphicsQueueInfo &submitToGraphicsQueueInfo) {
		/* Submit command buffer to graphics queue for rendering */
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = submitToGraphicsQueueInfo.waitSemaphores.size();
		submitInfo.pWaitSemaphores = submitToGraphicsQueueInfo.waitSemaphores.data();
		submitInfo.pWaitDstStageMask = &submitToGraphicsQueueInfo.submitPipelineStages;
		submitInfo.commandBufferCount = submitToGraphicsQueueInfo.commandBuffers.size();
		submitInfo.pCommandBuffers = submitToGraphicsQueueInfo.commandBuffers.data();
		submitInfo.signalSemaphoreCount = submitToGraphicsQueueInfo.signalSemaphores.size();
		submitInfo.pSignalSemaphores = submitToGraphicsQueueInfo.signalSemaphores.data();

		if (vkQueueSubmit(submitToGraphicsQueueInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}

	bool VKDK::SubmitFrame() {
		/* TODO: add support for overlay */
		bool submitOverlay = false; //settings.overlay && UIOverlay->visible;

		//if (submitOverlay) {
		//	// Wait for color attachment output to finish before rendering the text overlay
		//	VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		//	submitInfo.pWaitDstStageMask = &stageFlags;

		//	// Set semaphores
		//	// Wait for render complete semaphore
		//	submitInfo.waitSemaphoreCount = 1;
		//	submitInfo.pWaitSemaphores = &semaphores.renderComplete;
		//	// Signal ready with UI overlay complete semaphpre
		//	submitInfo.signalSemaphoreCount = 1;
		//	submitInfo.pSignalSemaphores = &semaphores.overlayComplete;

		//	// Submit current UI overlay command buffer
		//	submitInfo.commandBufferCount = 1;
		//	submitInfo.pCommandBuffers = &UIOverlay->cmdBuffers[currentBuffer];
		//	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		//	// Reset stage mask
		//	submitInfo.pWaitDstStageMask = &submitPipelineStages;
		//	// Reset wait and signal semaphores for rendering next frame
		//	// Wait for swap chain presentation to finish
		//	submitInfo.waitSemaphoreCount = 1;
		//	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		//	// Signal ready with offscreen semaphore
		//	submitInfo.signalSemaphoreCount = 1;
		//	submitInfo.pSignalSemaphores = &semaphores.renderComplete;
		//}
		VkResult err = QueuePresent(presentQueue, swapIndex, submitOverlay ? semaphores.overlayComplete : semaphores.renderComplete);
		if (err != VK_SUCCESS 
			|| VKDK::PreviousWindowSize[0] != VKDK::CurrentWindowSize[0] 
			|| VKDK::PreviousWindowSize[1] != VKDK::CurrentWindowSize[1]) {
			PreviousWindowSize[0] = CurrentWindowSize[0];
			PreviousWindowSize[1] = CurrentWindowSize[1];
			return true;
		}
		else {
			VK_CHECK_RESULT(vkQueueWaitIdle(presentQueue));
			return false;
		}
	}
	
	void RecreateSwapChain() {
		prepared = false;

		/* Ensure all operations on the device have been finished before destroying resources */
		vkDeviceWaitIdle(device);

		/* Recreate the swap chain */
		CleanupSwapChain();
		CreateSwapChain();

		/* Recreate frame buffers */
		CreateImageViews();
		//CreateGlobalRenderPass();
		CreateDepthResources();
		CreateFrameBuffers();

		/* Recreate command buffers, since they may store references to the recreated frame buffer */
		CreateCommandBuffers();
		vkDeviceWaitIdle(device);
		prepared = true;
	}

	void CleanupSwapChain() {
		vkDestroyImageView(device, depthImageView, nullptr);
		vkDestroyImage(device, depthImage, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);

		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}

		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

}
