#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

/**
 * @brief Struttura per i vertici del modello.
 *
 * Questa struttura contiene tutte le informazioni necessarie da elaborare all'interno della vertex shader.
 * Essa include anche metodi statici dedicati all'ottenere descrizioni di collegamento e descrizione sugli attributi dello stesso.
 */

/**
 * @brief Struttura per i vertici del modello.
 *
 * Questa struttura contiene tutte le informazioni necessarie da elaborare all'interno della vertex shader.
 * Essa include anche metodi statici dedicati all'ottenere descrizioni di collegamento e descrizione sugli attributi dello stesso.
 */
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

/**
 * @brief Crea un buffer e alloca memoria per esso.
 *
 * Questo metodo crea un buffer a seconda delle specifiche fornite e alloca la memoria necessaria.
 * @param device Il dispositivo Vulkan su cui creare il buffer.
 * @param physicalDevice Il dispositivo fisico Vulkan.
 * @param size La dimensione del buffer da creare.
 * @param usage Le flag di utilizzo del buffer
 * @param properties Le proprietà della memoria del buffer.
 * @param buffer Il buffer creato.
 * @param bufferMemory La memoria allocata per il buffer.
 */
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory);

/**
 * @brief Trova il tipo di memoria adatto per un buffer.
 *
 * Questo metodo cerca il tipo di memoria più adatto per un buffer in base alle specifiche fornite.
 * @param physicalDevice Il dispositivo fisico Vulkan.
 * @param typeFilter I filtri di tipo di memoria.
 * @param properties Le proprietà della memoria desiderate.
 * @return Il tipo di memoria trovato.
 */
uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                        uint32_t typeFilter, VkMemoryPropertyFlags properties);

/**
 * @brief Copia i dati da un buffer ad un altro.
 *
 * Questo metodo copia i dati da un buffer di origine a un buffer di destinazione.
 *
 * @param device Il dispositivo Vulkan su cui eseguire la copia.
 * @param commandPool Il pool di comandi da utilizzare per la copia.
 * @param graphicsQueue La coda grafica su cui inviare i comandi.
 * @param srcBuffer Il buffer di origine da cui copiare i dati.
 * @param dstBuffer Il buffer di destinazione in cui copiare i dati.
 * @param size La dimensione dei dati da copiare.
 */
void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

/**
 * @brief Crea un'immagine e alloca memoria per essa.
 *
 * Questo metodo crea un'immagine a seconda delle specifiche fornite e alloca la memoria necessaria.
 *
 * @param device Il dispositivo Vulkan su cui creare l'immagine.
 * @param physicalDevice Il dispositivo fisico Vulkan.
 * @param width La larghezza dell'immagine.
 * @param height L'altezza dell'immagine.
 * @param format Il formato dell'immagine.
 * @param tiling Il tipo di tiling dell'immagine
 * @param usage Le flag di utilizzo dell'immagine
 * @param properties Le proprietà della memoria dell'immagine.
 * @param image L'immagine creata.
 * @param imageMemory La memoria allocata per l'immagine.
 */
void createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                 uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory);

/**
 * @brief Transizione del layout di un'immagine.
 *
 * Questo metodo esegue la transizione del layout di un'immagine.
 *
 * @param device Il dispositivo Vulkan su cui eseguire la transizione.
 * @param commandPool Il pool di comandi da utilizzare per la transizione.
 * @param graphicsQueue La coda grafica su cui inviare i comandi.
 * @param image L'immagine di cui cambiare il layout.
 * @param format Il formato dell'immagine.
 * @param oldLayout Il layout precedente dell'immagine.
 * @param newLayout Il nuovo layout dell'immagine.
 */
void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout);

/**
 * @brief Copia i dati da un buffer a un'immagine.
 *
 * Questo metodo copia i dati da un buffer a un'immagine.
 *
 * @param device Il dispositivo Vulkan su cui eseguire la copia.
 * @param commandPool Il pool di comandi da utilizzare per la copia.
 * @param graphicsQueue La coda grafica su cui inviare i comandi.
 * @param buffer Il buffer da cui copiare i dati.
 * @param image L'immagine in cui copiare i dati.
 * @param width La larghezza dell'immagine.
 * @param height L'altezza dell'immagine.
 */
void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                       VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

/**
 * @brief Inizia un comando di singola transazione.
 *
 * Questo metodo inizia un comando di singola transazione per eseguire operazioni di copia o transizione.
 *
 * @param device Il dispositivo Vulkan su cui eseguire il comando.
 * @param commandPool Il pool di comandi da utilizzare per il comando.
 * @return Il command buffer creato.
 */
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

/**
 * @brief Termina un comando di singola transazione.
 *
 * Questo metodo termina un comando di singola transazione e invia i comandi al queue grafico.
 *
 * @param device Il dispositivo Vulkan su cui eseguire il comando.
 * @param commandPool Il pool di comandi da utilizzare per il comando.
 * @param graphicsQueue La coda grafica su cui inviare i comandi.
 * @param commandBuffer Il command buffer da terminare.
 */
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);

/**
 * @brief Crea una vista immagine per un'immagine.
 *
 * Questo metodo crea una vista immagine per un'immagine specificata.
 *
 * @param device Il dispositivo Vulkan su cui creare la vista immagine.
 * @param image L'immagine per cui creare la vista.
 * @param format Il formato dell'immagine.
 * @return La vista immagine creata.
 */
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);