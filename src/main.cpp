#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <memory>
#include <ranges>
/**
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
**/

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

const std::vector<char const*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {

        extensions.push_back(vk::EXTDebugUtilsExtensionName );
    }

    return extensions;
}

// names of extensions and features the GPU will need to have as a requirment for my engine
std::vector<const char*> deviceExtensions = {
	vk::KHRSwapchainExtensionName,
	vk::KHRSpirv14ExtensionName,
	vk::KHRSynchronization2ExtensionName,
	vk::KHRCreateRenderpass2ExtensionName
};

// GPU pointer
vk::raii::PhysicalDevice physicalDevice = nullptr;

class HelloTriangleApplication
{
  public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

  private:
	GLFWwindow *window = nullptr;
	vk::raii::Context  context;
	vk::raii::Instance instance = nullptr;

	void initWindow()
	{
		// Set platform to Wayland if available
		if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND))
			glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
		else if (glfwPlatformSupported(GLFW_PLATFORM_X11))
			glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
			
		if (!glfwInit())
			throw std::runtime_error("Failed to initialize GLFW");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		if (!window)
			throw std::runtime_error("Failed to create GLFW window");
			
		glfwShowWindow(window);
	}

	void initVulkan()
	{
		createInstance();
		//setupDebugMessenger(); TODO?
		pickPhysicalDevice();	// find and select a GPU to use
	}
	void pickPhysicalDevice()
	{
		std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices(); // make a list of all GPUs
		// the bellow long and complicated code is just filtering the list for a GPU that satisfies our version and features needed
		const auto devIter = std::ranges::find_if(devices,
		[&](auto const & device) {
				auto queueFamilies = device.getQueueFamilyProperties();
				bool isSuitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
				const auto qfpIter = std::ranges::find_if(queueFamilies,
				[]( vk::QueueFamilyProperties const & qfp )
						{
							return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
						} );
				isSuitable = isSuitable && ( qfpIter != queueFamilies.end() );
				auto extensions = device.enumerateDeviceExtensionProperties( );
				bool found = true;
				for (auto const & extension : deviceExtensions) {
					auto extensionIter = std::ranges::find_if(extensions, [extension](auto const & ext) {return strcmp(ext.extensionName, extension) == 0;});
					found = found &&  extensionIter != extensions.end();
				}
				isSuitable = isSuitable && found;
				if (isSuitable) {
					printf("found suitable GPU!\n");
					physicalDevice = device;
				}
				return isSuitable;
		});
		// we could not find a suitable GPU
		if (devIter == devices.end()) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{ .pApplicationName   = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
			.pEngineName        = "No Engine",
			.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
			.apiVersion         = vk::ApiVersion14 };

		// Get the required layers
		std::vector<char const*> requiredLayers;
		if (enableValidationLayers) {
			printf("DEBUG ON!\n");
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}
		// Check if the required layers are supported by the Vulkan implementation.
		auto layerProperties = context.enumerateInstanceLayerProperties();
		if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
			return std::ranges::none_of(layerProperties,
									   [requiredLayer](auto const& layerProperty)
									   { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
		}))
		{
			throw std::runtime_error("One or more required layers are not supported!");
		}


		// Get the required instance extensions from GLFW.
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			bool found = false;
			for (const auto& extensionProperty : extensionProperties)
			{
				if (strcmp(extensionProperty.extensionName, glfwExtensions[i]) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
			}
		}
		/*
		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions};
		*/
		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo        = &appInfo,
			.enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames     = requiredLayers.data(),
			.enabledExtensionCount   = 0,
			.ppEnabledExtensionNames = nullptr };
		
		instance = vk::raii::Instance(context, createInfo);
		printf("extensions: %i\n", glfwExtensionCount);
	}
	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow(window);

		glfwTerminate();
	}
};

int main()
{
	try
	{
		HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
