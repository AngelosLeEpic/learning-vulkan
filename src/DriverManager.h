//
// Created by hydrogen on 1/23/26.
//

#ifndef LEARNING_VULKAN_DRIVERMANAGER_H
#define LEARNING_VULKAN_DRIVERMANAGER_H


class DriverManager
{


    public:
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
};


#endif //LEARNING_VULKAN_DRIVERMANAGER_H