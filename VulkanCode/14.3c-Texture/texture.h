#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "bufferUtils.h"
class Texture {
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice,
            VkCommandPool commandPool, VkQueue graphicsQueue,
            const std::string& filename);
    ~Texture();

    VkDescriptorImageInfo getDescriptorInfo() const;

private:
    // Crea l'immagine della texture e ne carica i dati dalla sorgente.
    void createTextureImage(const char* filename);
    
    // Crea la view per la texture image.
    void createTextureImageView();
    
    // Crea il sampler per la texture.
    void createSampler();

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
};