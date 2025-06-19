#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <functional>
#include "assimp/scene.h"       // Assimp output data structure
#include "assimp/postprocess.h" // Assimp post processing flags
class Texture;

/**
 * @brief Struttura per rappresentare un vertice del modello 3D.
 */
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
    /**
     * @brief Costruttore della classe Mesh.
     *
     * Inizializza la mesh con i vertici e gli indici forniti, creando i buffer Vulkan necessari.
     *
     * @param device Il dispositivo Vulkan su cui operare.
     * @param physicalDevice Il dispositivo fisico Vulkan.
     * @param commandPool Il command pool per le operazioni di copia.
     * @param graphicsQueue La coda grafica per l'esecuzione dei comandi.
     * @param vertices I vertici del modello 3D.
     * @param indices Gli indici del modello 3D.
     */
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
         VkCommandPool commandPool, VkQueue graphicsQueue,
         const std::vector<Vertex> &vertices = {},
         const std::vector<uint32_t> &indices = {});

    /**
     * @brief Distruttore della classe Mesh.
     * Rilascia le risorse Vulkan allocate per la mesh, inclusi i buffer e la memoria.
     */
    ~Mesh();

    /**
     * @brief Restituisce il buffer dei vertici.
     * @return Il buffer dei vertici.
     */
    VkBuffer getVertexBuffer() const;

    /**
     * @brief Restituisce il numero di vertici nella mesh.
     * @return Il numero di vertici.
     */
    size_t getVertexCount() const;

    /**
     * @brief Restituisce il buffer degli indici.
     * @return Il buffer degli indici.
     */
    VkBuffer getIndexBuffer() const;

    /**
     * @brief Restituisce il numero di indici nella mesh.
     * @return Il numero di indici.
     */
    size_t getIndexCount() const;

    /**
     * @brief Restituisce la mappa delle texture.
     * @return Una mappa che associa i nomi delle texture ai loro indici.
     */
    std::map<std::string, int> getTextures() const;

    /**
     * @brief Aggiunge una texture alla mappa delle texture.
     *
     * @param name Il nome della texture.
     * @param index L'indice della texture.
     */
    void addTexture(std::string name, int index);

    /**
     * @brief Imposta l'indice di una texture nella mappa delle texture.
     *
     * @param name Il nome della texture.
     * @param index L'indice della texture.
     */
    void setTextureIndex(std::string name, int index);

    /**
     * @brief Imposta il set di descrittori per un frame specifico.
     * @param frameIndex L'indice del frame.
     * @param set Il set di descrittori da impostare.
     */
    void setDescriptorSet(uint32_t frameIndex, VkDescriptorSet set);

    /**
     * @brief Restituisce il set di descrittori per un frame specifico.
     * @param frameIndex L'indice del frame.
     * @return Il set di descrittori per il frame specificato.
     */
    VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const;

    /**
     * @brief Restituisce i sub-mesh della mesh.
     * @return Un vettore di sub-mesh.
     */
    const std::vector<SubMesh> &getSubMeshes() const;

    /**
     * @brief Aggiunge un sub-mesh alla mesh.
     *
     * @param offset L'offset degli indici nel buffer degli indici.
     * @param count Il numero di indici nel sub-mesh.
     * @param texIdx L'indice della texture associata al sub-mesh.
     */
    void addSubMesh(uint32_t offset, uint32_t count, int texIdx);

    /**
     * @brief Carica un modello 3D da un file utilizzando Assimp.
     *
     * @param filename Il percorso del file del modello.
     * @param flags I flag di post-elaborazione da utilizzare con Assimp.
     * @throws std::runtime_error Se si verifica un errore durante il caricamento del modello.
     */
    void loadFromFile(const std::string &filename, unsigned int flags = aiProcess_FlipUVs);

    /**
     * @brief Disegna la mesh utilizzando un comando di disegno Vulkan.
     *
     * @param cmd Il comando di disegno Vulkan.
     * @param frameIndex L'indice del frame corrente.
     * @param pipelineLayout Il layout della pipeline Vulkan.
     * @param updateUniformCallback Una funzione di callback per aggiornare i dati uniformi.
     */
    void draw(VkCommandBuffer cmd, uint32_t frameIndex,
              VkPipelineLayout pipelineLayout,
              std::function<void(uint32_t submeshIndex)> updateUniformCallback);

private:
    /**
     * @brief Crea il buffer dei vertici e la memoria associata.
     */
    void createVertexBuffer();

    /**
     * @brief Crea il buffer degli indici e la memoria associata.
     */
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