#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

class Texture;

class Mesh
{
public:
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
         VkCommandPool commandPool, VkQueue graphicsQueue,
         const std::vector<Vertex> &vertices,
         const std::vector<uint32_t> &indices);
    ~Mesh();

    VkBuffer getVertexBuffer() const;
    VkBuffer getIndexBuffer() const;
    size_t getIndexCount() const;
    Texture *getTexture() const;
    void addTexture(Texture *texture);

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
};