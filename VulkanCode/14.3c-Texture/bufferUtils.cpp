#include "bufferUtils.h"
#include <stdexcept> // For std::runtime_error
#include <iostream>

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    // ora che abbiamo creato il buffer, dobbiamo allocare la memoria per esso
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    // in teoria non è di norma chiamare direttamente vkAllocateMemory, perché se ci fossero molti più oggetti, occuperebbe molta memoria
    // ma per ora essendo solo un triangolo, non ci sono problemi, ma se ci fossero più oggetti, meglio creare un uallocatore personalizzato che gestisca la memoria in modo più efficiente
    //  dividendo la memoria in parti più piccole e allocando solo la memoria necessaria per ogni oggetto usando un offset
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    // se non ci sono errori, possiamo finalmente collegare il buffer alla memoria
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    // questo ciclo ci permette di trovare il tipo di memoria che ci serve
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        // se il tipo di memoria è supportato e ha le proprietà che ci servono, lo restituiamo
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

void createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                 uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D; // indica il tipo di immagine, può essere 1D, 2D o 3D: 1D di solito si usa per array di gradienti, 2D per texture e 3D per voxels
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    /* Il campo "tiling" può avere uno dei seguenti valori:

    VK_IMAGE_TILING_LINEAR: I texel sono disposti in ordine di riga, come il nostro array di pixel.
    VK_IMAGE_TILING_OPTIMAL: I texel sono disposti in un ordine definito dall'implementazione per un accesso ottimale.
     */
    imageInfo.tiling = tiling;
    /*
    Ci sono solo due possibili valori per il campo "initialLayout" di un'immagine:

    VK_IMAGE_LAYOUT_UNDEFINED: Non utilizzabile dalla GPU e la primissima transizione scarterà i texel.
    VK_IMAGE_LAYOUT_PREINITIALIZED: Non utilizzabile dalla GPU, ma la prima transizione preserverà i texel.
    Ci sono poche situazioni in cui è necessario che i texel vengano preservati durante la prima transizione.
    Un esempio, tuttavia, sarebbe se volessi utilizzare un'immagine come immagine di staging in combinazione con il layout VK_IMAGE_TILING_LINEAR.
    In tal caso, vorresti caricare i dati dei texel su di essa e poi transizionare l'immagine per essere una sorgente di trasferimento senza perdere i dati.
    Nel nostro caso, tuttavia, transizioneremo prima l'immagine per essere una destinazione di trasferimento e poi copieremo i dati dei texel su di essa da un oggetto buffer,
    quindi non abbiamo bisogno di questa proprietà e possiamo tranquillamente usare VK_IMAGE_LAYOUT_UNDEFINED.*/
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // questo è il modo in cui vogliamo usare l'immagine
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    // ora possiamo creare l'immagine
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    // allochiamo la memoria per l'immagine
    // l'allocamento della memoria per un'immagine è molto simile a quello per un buffer
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // l'immagine è un'immagine di colore
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    // vkCmdPipelineBarrier serve per sincronizzare l'accesso alle risorse tra diverse fasi della pipeline.
    // Parametri:
    // - commandBuffer: il command buffer su cui registrare la barriera.
    // - sourceStage: lo stadio della pipeline in cui le operazioni precedenti devono essere completate prima della barriera.
    // - destinationStage: lo stadio della pipeline che deve attendere la barriera prima di iniziare.
    // - 0: flags opzionali (di solito 0).
    // - 0, nullptr: numero e puntatore a memory barriers generiche (non usate qui).
    // - 0, nullptr: numero e puntatore a buffer memory barriers (non usate qui).
    // - 1, &barrier: numero e puntatore a image memory barriers (qui usiamo una sola barriera per l'immagine).
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

// funzione per copiare i dati da un buffer ad un image
void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                       VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}