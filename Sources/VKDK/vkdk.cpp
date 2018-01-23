#include "vkdk.hpp"
#include <iostream>
#include <fstream>
#include <streambuf>
#include <stdlib.h>
#include <set>
#include <algorithm>

/* ------------------------------------------*/
/* FIELDS                                    */
/* ------------------------------------------*/
namespace VKDK {

	bool Verbose = false;

	bool error = false;

	GLFWwindow *DefaultWindow;
	GLFWmonitor *DefaultMonitor;
	bool FullScreenActive = false;
	bool WindowHidden = false;
	int DefaultWindowSize[2] = { 512 , 512 };
	int DefaultWindowPos[2];
	int PreviousWindowSize[2] = { 512 , 512 };
	int PreviousWindowPos[2];
	std::string DefaultWindowTitle = "";
	//std::vector<std::pair<std::string, std::string>> DefaultSources;
	//std::string BuildOptions = "";
	//std::unordered_map<std::string, GLuint> Shaders;

	/* Vulkan Instance */
	VkInstance instance;

	/* Validation Layers */
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation",
	};

	VkDebugReportCallbackEXT callback;

	#ifdef NDEBUG
	const bool enableValidationLayers = false;
	#else
	const bool enableValidationLayers = true;
	#endif

	/* Physical Devices and Queue Families */
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	/* Logical Device */
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	/* Window Surface */
	VkSurfaceKHR surface;

	/* Swap Chain */
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	/* Image Views */
	std::vector<VkImageView> swapChainImageViews;

	/* Graphics pipeline */
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	/* Render pass */
	VkRenderPass renderPass;

	/* Frame Buffers */
	std::vector<VkFramebuffer> swapChainFramebuffers;

	/* Command buffers */
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	/* Rendering and presentation */
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	/* ------------------------------------------*/
	/* FUNCTIONS                                 */
	/* ------------------------------------------*/
	void print(std::string s, bool forced) {
		if (Verbose || error || forced)
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
		error = _error;
		VKDK::print("Error: " + std::string(description));
	}

	/* Initializers */
	bool Initialize(InitializationParameters parameters) {
		/* Store the initialization parameters */
		DefaultWindowSize[0] = parameters.windowWidth;
		DefaultWindowSize[1] = parameters.windowHeight;
		DefaultWindowTitle = parameters.windowTitle;
		FullScreenActive = parameters.fullScreenActive;
		WindowHidden = parameters.windowHidden;
		Verbose = parameters.verbose;

		/* Call initialization/creation functions */
		try {
			InitWindow();
			CreateInstance();
			SetupDebugCallback();
			CreateSurface();
			PickPhysicalDevice();
			CreateLogicalDevice();
			CreateSwapChain();
			CreateImageViews();
			CreateRenderPass();
			CreateGraphicsPipeline();
			CreateFrameBuffers();
			CreateCommandPools();
			CreateCommandBuffers();
			CreateSemaphores();
		}
		catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			return VK_ERROR_INITIALIZATION_FAILED;
		}
		return VK_SUCCESS;
	}

	void InitWindow() {
		if (!glfwInit()) {
			print("GLFW failed to initialize!", true);
		}
		glfwSetErrorCallback(ErrorCallback);


		GLFWmonitor *monitor;
		monitor = NULL;

		DefaultMonitor = glfwGetPrimaryMonitor();
		CreateNewWindow(DefaultWindowSize[0], DefaultWindowSize[1], DefaultWindowTitle, NULL, NULL);
		MakeContextCurrent(DefaultWindow);

		SetFullScreen(FullScreenActive, DefaultWindowPos, DefaultWindowSize);
		SetWindowHidden(WindowHidden);
	}

	std::string getFileName(const std::string& s) {
		using namespace std;
		char sep = '/';

		/* cmake
		#ifdef _WIN32
		sep = '\\';
		#endif*/

		size_t i = s.rfind(sep, s.length());
		if (i != std::string::npos) {
			string fullname = s.substr(i + 1, s.length() - i);
			size_t lastindex = fullname.find_last_of(".");
			string rawname = fullname.substr(0, lastindex);

			return(rawname);
		}

		return("");
	}

	void Terminate() {
		CleanupSwapChain();

		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(DefaultWindow);

		glfwTerminate();
	}

	int CreateNewWindow(int width, int height, std::string title, GLFWmonitor *monitor, GLFWwindow *share) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		DefaultWindow = glfwCreateWindow(width, height, title.c_str(), monitor, share);
		glfwSetWindowSizeCallback(DefaultWindow, OnWindowResized);

		if (!DefaultWindow) {
			return -1;
		}
		return 0;
	}

	void OnWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;

		RecreateSwapChain();
	}

	void MakeContextCurrent(GLFWwindow *window) {
		glfwMakeContextCurrent(window);
	}

	bool ShouldClose() {
		return glfwWindowShouldClose(DefaultWindow);
	}

	void SetFullScreen(bool fullscreen, int windowPos[2], int windowSize[2]) {
		if (!FullScreenActive && fullscreen == false) return;

		FullScreenActive = fullscreen;
		if (fullscreen)
		{
			// backup windwo position and window size
			glfwGetWindowPos(DefaultWindow, &windowPos[0], &windowPos[1]);
			glfwGetWindowSize(DefaultWindow, &windowSize[0], &windowSize[1]);

			// get reolution of monitor
			const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

			DefaultWindowSize[0] = mode->width;
			DefaultWindowSize[1] = mode->height;

			// switch to full screen
			glfwSetWindowMonitor(DefaultWindow, DefaultMonitor, 0, 0, mode->width, mode->height, 0);
		}
		else
		{
			DefaultWindowSize[0] = windowSize[0];
			DefaultWindowSize[1] = windowSize[1];

			// restore last window size and position
			glfwSetWindowMonitor(DefaultWindow, nullptr, windowPos[0], windowPos[1], windowSize[0], windowSize[1], 0);
		}
		/* TODO: How to replace this?*/
		//glViewport(0, 0, DefaultWindowSize[0], DefaultWindowSize[1]);

		/*
		_updateViewport = true;*/
	}

	void SetWindowHidden(bool hideWindow) {
		if (!WindowHidden && hideWindow == false) return;
		WindowHidden = hideWindow;
		if (hideWindow) {
			glfwHideWindow(DefaultWindow);
		}
		else {
			glfwShowWindow(DefaultWindow);
		}
	}

	void CreateInstance() {
		/* Optional initialization struct */
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = DefaultWindowTitle.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
	
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

		if (Verbose) {
			std::cout << "available extensions:" << std::endl;

			for (const auto& currentextension : extensionProperties) {
				std::cout << "\t" << currentextension.extensionName << std::endl;
			}
		}

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

		return VK_FALSE;
	}

	void SetupDebugCallback() {
		//the debug callback in Vulkan is managed with a handle that needs to be explicitly created and destroyed
		if (!enableValidationLayers) return;

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

	/* Physical Devices and Queue Families */
	void PickPhysicalDevice() {
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
				physicalDevice = device;
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

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
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
		VkPhysicalDeviceFeatures deviceFeatures = {};


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
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

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

	void CreateSwapChain() {
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

	void CreateImageViews() {
		/* Resize list to be one to one with swap chain images list */
		swapChainImageViews.resize(swapChainImages.size());

		/* Itterate over all swapchain images */
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];

			/* Viewtype allows us to treat this image as a 2D, 3D, or cube map texture*/
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;

			/* Texture swizzling can possibly happen here */
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			/* Subresource range is used to specify how this image should be used.
				in our case, the image is a color target with no mip mapping or multiple layers.
				Multiple layers could be used for stereo */
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			/* Create the image view! */
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}

		}
	}

	void CreateGraphicsPipeline() {
		auto vertShaderCode = readFile(ResourcePath "TutorialShaders/vert.spv");
		auto fragShaderCode = readFile(ResourcePath "TutorialShaders/frag.spv");

		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		/*These modules are just dummy wrappers around our code. We still need to link them together */

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	
		/* Fixed function stuff */

		/* Vertex Input */
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

		/* Input assembly */
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		/* Viewports and scissor rectangles */
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		/* Now we can create a viewport state from this viewport and scissor */
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		/* Rasterizer */
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // Helpful for shadow maps.
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geometry never goes through rasterization, or to frame buffer
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // could be line or point too. those require GPU features to be enabled.
		rasterizer.lineWidth = 1.0f; // anything thicker than 1.0 requires wideLines GPU feature
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		/* Multisampling */
		/* Not using this right now, but essentially this is a faster AA method */
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		/* Depth and Stencil testing */
		/* Not using right now... */
		//VkPipelineDepthStencilStateCreateInfo

		/* Color blending */
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		/* Dynamic state */
		/* The graphics pipeline can actually be tweaked a little bit during draw. This is an optional pipeline stage */
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		/* This creates a uniform layout, and although we're not using it, we're required to make it. */
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	

		/* Now we can create a graphics pipeline! */
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional

		pipelineInfo.layout = pipelineLayout;

		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
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

	void CreateRenderPass() {
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

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		/* Create the render pass */
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void CreateFrameBuffers() {
		/* Create a frame buffer for each swap chain image. */
		swapChainFramebuffers.resize(swapChainImageViews.size());
	
		/* Now create them */
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void CreateCommandPools() {
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

	void CreateCommandBuffers() {
		/* Command buffer for each frame buffer */
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
		
		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		/* Starting command buffer recording */
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
	
			/* Starting a render pass */
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }; // CLEAR COLOR HERE!
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			/* Basic drawing commands */
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}

	}

	void VKDK::SetClearColor(float r, float g, float b, float a) {
		/* To change the background color, we need to rerecord the command buffers. */

		/* Starting command buffer recording */
		/* Technically, if this is being called each frame, I could just update the command buffer
			for the image view currently being rendered to. This is slower, but I won't be doing it often. */
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			/* Setup command buffer information */
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			/* Begin recording the command buffer */
			vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

			/* Create render pass information  */
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;
			VkClearValue clearColor = { r, g, b, a}; // CLEAR COLOR HERE!
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			/* Begin render pass (just clear) */
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	/* Rendering and Presentation */
	void CreateSemaphores() {
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

			throw std::runtime_error("failed to create semaphores!");
		}
	}

	bool VKDK::OptimizeSwapchain(int swapchainResult) {
		if (swapchainResult == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return true;
		}
		else if (swapchainResult != VK_SUCCESS && swapchainResult != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire/present swap chain image!");
		}
		return false;
	}

	void VKDK::TestDraw1() {
		/* Aqure an image from the swap chain */
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		
		/* Check if the swap chain was optimized, don't render this frame if it did. */
		if (OptimizeSwapchain(result)) return;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		//VkRenderPassCreateInfo renderPassInfo = {};
		//renderPassInfo.dependencyCount = 1;
		//renderPassInfo.pDependencies = &dependency;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		OptimizeSwapchain(result);

		vkQueueWaitIdle(presentQueue);
	}

	void DrawFrame() {
		uint32_t imageIndex;
	
		VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}


		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		vkQueueWaitIdle(presentQueue);
	}

	void RecreateSwapChain() {
		vkDeviceWaitIdle(device);

		CleanupSwapChain();

		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandBuffers();
	}

	void CleanupSwapChain() {
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}

		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

}
