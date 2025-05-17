#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

class Texture;

class Mesh
{
public:
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
         VkCommandPool commandPool, VkQueue graphicsQueue,
         const std::vector<Vertex> &vertices = {},
         const std::vector<uint32_t> &indices = {});
    ~Mesh();

    VkBuffer getVertexBuffer() const;
    size_t getVertexCount() const;
    VkBuffer getIndexBuffer() const;
    size_t getIndexCount() const;
    std::vector<Texture *> getTextures() const;
    void addTexture(Texture *texture);
    void setDescriptorSet(uint32_t frameIndex, VkDescriptorSet set);
    VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const;

private:
    void createVertexBuffer();
    void createIndexBuffer();

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<Texture *> textures; // vettore di puntatori a texture

    std::vector<VkDescriptorSet> descriptorSets;
};