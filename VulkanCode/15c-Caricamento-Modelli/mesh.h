#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <functional>
class Texture;

struct SubMesh
{
    uint32_t indexOffset;
    uint32_t indexCount;
    int textureIndex; // indice nel global texture array

    SubMesh(uint32_t offset, uint32_t count, int texIdx)
        : indexOffset(offset), indexCount(count), textureIndex(texIdx) {}
};

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
    std::map<std::string, int> getTextures() const;
    void addTexture(std::string name, int index);
    void setTextureIndex(std::string name, int index);
    void setDescriptorSet(uint32_t frameIndex, VkDescriptorSet set);
    VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const;
    
    const std::vector<SubMesh> &getSubMeshes() const;
    void addSubMesh(uint32_t offset, uint32_t count, int texIdx);
    void draw(VkCommandBuffer cmd, uint32_t frameIndex,
          VkPipelineLayout pipelineLayout,
          std::function<void(uint32_t submeshIndex)> updateUniformCallback);
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

    std::map<std::string, int> textures; // mappa di puntatori a texture index - texture
    std::vector<SubMesh> subMeshes;

    std::vector<VkDescriptorSet> descriptorSets;
};