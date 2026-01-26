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
#include <optional>
#include <memory>
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
// HELPER FUNCTIONS
uint32_t findQueueFamilies(vk::raii::PhysicalDevice physicalDevice) {
	// find the index of the first queue family that supports graphics
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

	// get the first index into queueFamilyProperties which supports graphics
	auto graphicsQueueFamilyProperty =
	  std::find_if( queueFamilyProperties.begin(),
					queueFamilyProperties.end(),
					[]( vk::QueueFamilyProperties const & qfp ) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; } );

	return static_cast<uint32_t>( std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty ) );
}

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
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device device = nullptr;
	vk::raii::Queue graphicsQueue = nullptr;
	vk::raii::SurfaceKHR surface = nullptr;
	vk::raii::Queue presentQueue = nullptr;

	std::vector<const char*> deviceExtensions = {
		vk::KHRSwapchainExtensionName};

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
		createSurface();
		pickPhysicalDevice();	// find and select a GPU to use
		createLogicalDevice();	// create logical device out of physical device selected
	}
	void createSurface()
	{
		VkSurfaceKHR       _surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
			throw std::runtime_error("failed to create window surface!");
		}
		surface = vk::raii::SurfaceKHR(instance, _surface);
	}
	void createLogicalDevice()
	{
		// gather queue families and their properties
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	    // get the first index into queueFamilyProperties which supports graphics
	    auto graphicsQueueFamilyProperty = std::ranges::find_if( queueFamilyProperties, []( auto const & qfp )
	                    { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); } );

	    auto graphicsIndex = static_cast<uint32_t>( std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty ) );

	    // determine a queueFamilyIndex that supports present
	    // first check if the graphicsIndex is good enough
	    auto presentIndex = physicalDevice.getSurfaceSupportKHR( graphicsIndex, *surface )
	                                       ? graphicsIndex
	                                       : static_cast<uint32_t>( queueFamilyProperties.size() );
	    if ( presentIndex == queueFamilyProperties.size() )
	    {
	        // the graphicsIndex doesn't support present -> look for another family index that supports both
	        // graphics and present
	        for ( size_t i = 0; i < queueFamilyProperties.size(); i++ )
	        {
	            if ( ( queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics ) &&
	                 physicalDevice.getSurfaceSupportKHR( static_cast<uint32_t>( i ), *surface ) )
	            {
	                graphicsIndex = static_cast<uint32_t>( i );
	                presentIndex  = graphicsIndex;
	                break;
	            }
	        }
	        if ( presentIndex == queueFamilyProperties.size() )
	        {
	            // there's nothing like a single family index that supports both graphics and present -> look for another
	            // family index that supports present
	            for ( size_t i = 0; i < queueFamilyProperties.size(); i++ )
	            {
	                if ( physicalDevice.getSurfaceSupportKHR( static_cast<uint32_t>( i ), *surface ) )
	                {
	                    presentIndex = static_cast<uint32_t>( i );
	                    break;
	                }
	            }
	        }
	    }
	    if ( ( graphicsIndex == queueFamilyProperties.size() ) || ( presentIndex == queueFamilyProperties.size() ) )
	    {
	        throw std::runtime_error( "Could not find a queue for graphics or present -> terminating" );
	    }

	    // query for Vulkan 1.3 features
	    auto features = physicalDevice.getFeatures2();
	    vk::PhysicalDeviceVulkan13Features vulkan13Features;
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures;
	    vulkan13Features.dynamicRendering = vk::True;
	    extendedDynamicStateFeatures.extendedDynamicState = vk::True;
	    vulkan13Features.pNext = &extendedDynamicStateFeatures;
	    features.pNext = &vulkan13Features;
	    // create a Device
	    float                     queuePriority = 0.5f;
	    vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
	    vk::DeviceCreateInfo      deviceCreateInfo{ .pNext =  &features, .queueCreateInfoCount = 1, .pQueueCreateInfos = &deviceQueueCreateInfo };
	    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
	    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	    device = vk::raii::Device( physicalDevice, deviceCreateInfo );
	    graphicsQueue = vk::raii::Queue( device, graphicsIndex, 0 );
	    presentQueue = vk::raii::Queue( device, presentIndex, 0 );
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

		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions};
		/*
		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo        = &appInfo,
			.enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames     = requiredLayers.data(),
			.enabledExtensionCount   = 0,
			.ppEnabledExtensionNames = nullptr };
		*/
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
