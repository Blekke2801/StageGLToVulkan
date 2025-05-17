#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};
// funzione per creare un buffer
// e allocare memoria per il buffer
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory);

// funzione per trovare il tipo di memoria
// che ci serve per il buffer
uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                        uint32_t typeFilter, VkMemoryPropertyFlags properties);

// funzione per copiare i dati da un buffer ad un altro
void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

// funzione per creare un image
// e allocare memoria per l'image
void createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                 uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory);

void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout);

// funzione per copiare i dati da un buffer ad un image
void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                       VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);