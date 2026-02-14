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
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <memory>
#include <fstream>
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;
/*
vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}
*/



static std::vector<char> readFile(const std::string& filename) {
	std::cout << filename << std::endl;
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

	std::vector<char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
	file.close();
	return buffer;
}

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
/*
std::vector<const char*> deviceExtensions = {
	vk::KHRSwapchainExtensionName,
	vk::KHRSpirv14ExtensionName,
	vk::KHRSynchronization2ExtensionName,
	vk::KHRCreateRenderpass2ExtensionName
};
*/


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
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	vk::SurfaceFormatKHR swapChainSurfaceFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::Image> swapChainImages;
	vk::raii::SwapchainKHR swapChain = nullptr;
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vk::raii::Pipeline       graphicsPipeline = nullptr;
	vk::raii::PipelineLayout pipelineLayout   = nullptr;
	std::vector<const char*> deviceExtensions = {
		vk::KHRSwapchainExtensionName};
	vk::raii::CommandPool commandPool = nullptr;

	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const {
		printf("reading shader file\n");
		vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
		printf("creating shader module\n");
		vk::raii::ShaderModule shaderModule{ device, createInfo };
		return shaderModule;
	}
	// internal helper functions
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
				return availablePresentMode;
			}
		}
		return vk::PresentModeKHR::eFifo;
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

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
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();	// find and select a GPU to use
		createLogicalDevice();	// create logical device out of physical device selected
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
	    createCommandPool();
	}
	void createCommandPool(){
		
	}
	void setupDebugMessenger(){
		if (!enableValidationLayers){
			printf("debugger is not on btw\n");
			return;
		}
		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT  debugUtilsMessengerCreateInfoEXT{
		     .messageSeverity = severityFlags,
		     .messageType     = messageTypeFlags,
		     .pfnUserCallback = &debugCallback};
		debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
		printf("debuggest set up\n");
	}
	void createGraphicsPipeline() 
	{
		printf("creating graphics pipeline\n");
    	vk::raii::ShaderModule shaderModule = createShaderModule(readFile("slang.spv"));
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
		std::vector dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{  .topology = vk::PrimitiveTopology::eTriangleList };
		vk::Viewport{ 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f };
		vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

		vk::PipelineRasterizationStateCreateInfo rasterizer{  .depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack,
			.frontFace = vk::FrontFace::eClockwise, .depthBiasEnable = vk::False,
			.depthBiasSlopeFactor = 1.0f, .lineWidth = 1.0f };
		vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
		//VkPipelineDepthStencilStateCreateInfo = nullptr;
			
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable    = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
		
		vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{  .setLayoutCount = 0, .pushConstantRangeCount = 0 };
		vk::raii::PipelineLayout pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		    {.stageCount          = 2,
		     .pStages             = shaderStages,
		     .pVertexInputState   = &vertexInputInfo,
		     .pInputAssemblyState = &inputAssembly,
		     .pViewportState      = &viewportState,
		     .pRasterizationState = &rasterizer,
		     .pMultisampleState   = &multisampling,
		     .pColorBlendState    = &colorBlending,
		     .pDynamicState       = &dynamicState,
		     .layout              = pipelineLayout,
		     .renderPass          = nullptr},
		    {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};
		graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}
	void createImageViews()
	{
		assert(swapChainImageViews.empty());
		vk::ImageViewCreateInfo imageViewCreateInfo{.viewType = vk::ImageViewType::e2D, .format = swapChainSurfaceFormat.format, .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
		// potentially repalce above line with:
		/*
		vk::ImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
			imageViewCreateInfo.format = swapChainSurfaceFormat.format;
			imageViewCreateInfo.components = vk::ComponentMapping{};
			imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
		*/
		for (auto& image : swapChainImages)
		{
			imageViewCreateInfo.image = image;
			swapChainImageViews.emplace_back(device, imageViewCreateInfo);
		}
		printf("image viewer creation complete\n");

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
		vk::PhysicalDeviceVulkan11Features vulkan11Features{};
	    vk::PhysicalDeviceVulkan13Features vulkan13Features;
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures;
		// Enable required features
		vulkan11Features.shaderDrawParameters = vk::True;
		vulkan13Features.dynamicRendering = vk::True;
		extendedDynamicStateFeatures.extendedDynamicState = vk::True;
		// Chain them properly
		extendedDynamicStateFeatures.pNext = nullptr;
		vulkan13Features.pNext = &extendedDynamicStateFeatures;
		vulkan11Features.pNext = &vulkan13Features;
		features.pNext = &vulkan11Features;

	    // create a Device
	    float                     queuePriority = 0.5f;
	    vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
	    vk::DeviceCreateInfo      deviceCreateInfo{ .pNext =  &features, .queueCreateInfoCount = 1, .pQueueCreateInfos = &deviceQueueCreateInfo };
	    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
	    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	    device = vk::raii::Device( physicalDevice, deviceCreateInfo );
	    graphicsQueue = vk::raii::Queue( device, graphicsIndex, 0 );
	    presentQueue = vk::raii::Queue( device, presentIndex, 0 );

		// get surface capabilities for swapchain
		auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( surface );
		// get supported surface formats
		std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR( surface );
		// get available presentation forms
		std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR( surface );
	}
	void createSwapChain() {
		auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
		swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR( *surface ));
		swapChainExtent = chooseSwapExtent(surfaceCapabilities);
		auto minImageCount = std::max( 3u, surfaceCapabilities.minImageCount );
		minImageCount = ( surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount ) ? surfaceCapabilities.maxImageCount : minImageCount;

		
		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

		if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
			imageCount = surfaceCapabilities.maxImageCount;
		}
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
			.flags = vk::SwapchainCreateFlagsKHR(),
			.surface = *surface,
			.minImageCount = minImageCount,
			.imageFormat = swapChainSurfaceFormat.format,
			.imageColorSpace = swapChainSurfaceFormat.colorSpace,
			.imageExtent = swapChainExtent,
			.imageArrayLayers =1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.preTransform = surfaceCapabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR( *surface )),
			.clipped = vk::True,
			.oldSwapchain = nullptr
		};

		swapChain = vk::raii::SwapchainKHR( device, swapChainCreateInfo );
		swapChainImages = swapChain.getImages();

		vk::Format swapChainImageFormat = vk::Format::eUndefined;
		vk::Extent2D swapChainExtent;

		//swapChainImageFormat = surfaceFormat.format;
		//swapChainExtent = extent;

		//figure out what all this is

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
			auto requiredExtensions = getRequiredExtensions();
				vk::InstanceCreateInfo createInfo;
				createInfo.pApplicationInfo        = &appInfo;
				createInfo.enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size());
				createInfo.ppEnabledLayerNames     = requiredLayers.data();
				createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size());
				createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		/*
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
		
		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo        = &appInfo,
			.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
			.ppEnabledLayerNames     = validationLayers.data(),
			.enabledExtensionCount   = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions,
		};
		
		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo        = &appInfo,
			.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
			.ppEnabledLayerNames     = validationLayers.data(),
			.enabledExtensionCount   = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions,
		};
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
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
	{
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
		}

		return vk::False;
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
