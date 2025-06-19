#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "bufferUtils.h"
class Texture
{
public:
    /**
     * @brief Costruttore della classe Texture.
     *
     * Inizializza la texture caricando l'immagine da un file e creando le risorse Vulkan necessarie.
     *
     * @param device Il dispositivo Vulkan su cui operare.
     * @param physicalDevice Il dispositivo fisico Vulkan.
     * @param commandPool Il command pool per le operazioni di copia.
     * @param graphicsQueue La coda grafica per l'esecuzione dei comandi
     * @param filename Il percorso del file immagine da caricare come texture.
     * @note Questo costruttore carica l'immagine della texture, crea la view dell'immagine e il sampler.
     *       Assicurarsi che il file immagine sia accessibile e valido.
     * @throws std::runtime_error Se si verifica un errore durante il caricamento dell'immagine o la creazione delle risorse Vulkan.
     * @see createTextureImage, createTextureImageView, createSampler
     */
    Texture(VkDevice device, VkPhysicalDevice physicalDevice,
            VkCommandPool commandPool, VkQueue graphicsQueue,
            const std::string &filename);

    /**
     * @brief Distruttore della classe Texture.
     *
     * Questo distruttore rilascia le risorse Vulkan allocate per la texture.
     */
    ~Texture();

    /**
     * @brief Ottiene l'immagine della texture.
     *
     * @return VkImage L'immagine della texture.
     */
    VkDescriptorImageInfo getDescriptorInfo() const;

    /**
     * @brief Imposta l'indice della texture.
     *
     * @param index L'indice della texture nell'array di texture.
     */
    void setIndex(int index);

    /**
     * @brief Ottiene l'indice della texture.
     *
     * @return int L'indice della texture nell'array di texture.
     */
    int getIndex() const;

private:
    /**
     * @brief Crea l'immagine della texture a partire da un file.
     *
     * Carica l'immagine da un file e crea un'immagine Vulkan per la texture.
     * @param filename Il percorso del file immagine da caricare.
     * @throws std::runtime_error Se si verifica un errore durante il caricamento dell'immagine o la creazione dell'immagine Vulkan.
     * @note Assicurarsi che il file immagine sia accessibile e valido.
     */
    void createTextureImage(const char *filename);

    /**
     * @brief Crea la view dell'immagine della texture.
     *
     * Crea un VkImageView per l'immagine della texture, che consente di accedere ai dati dell'immagine.
     * @throws std::runtime_error Se si verifica un errore durante la creazione della view dell'immagine.
     */
    void createTextureImageView();

    /**
     * @brief Crea il sampler per la texture.
     *
     * Crea un VkSampler per la texture, che controlla come la texture viene campionata.
     */
    void createSampler();

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    int index; // indice della texture nell'array di texture (serve alla mesh per far sì che ogni texture sappia dove si trova nell'array)
};