#include "texture.h"
#include "bufferUtils.h"
#include "shaderclass.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice,
                 VkCommandPool commandPool, VkQueue graphicsQueue,
                 const std::string &filename)
    : device(device), physicalDevice(physicalDevice),
      commandPool(commandPool), graphicsQueue(graphicsQueue)
{
    createTextureImage(filename.c_str());
    createTextureImageView();
    createSampler();
}

Texture::~Texture()
{
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
}

VkDescriptorImageInfo Texture::getDescriptorInfo() const
{
    VkDescriptorImageInfo descriptorInfo{};
    descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorInfo.imageView = textureImageView;
    descriptorInfo.sampler = textureSampler;
    return descriptorInfo;
}

void Texture::createTextureImage(const char *filename)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(device, physicalDevice,
                 imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    // puliamo i pixel
    stbi_image_free(pixels);

    /*
    A differenza del layout di un'immagine, la modalità di tiling non può essere modificata successivamente.
    Se vuoi poter accedere direttamente ai texel nella memoria dell'immagine, devi usare VK_IMAGE_TILING_LINEAR.
    Tuttavia, utilizzeremo un buffer di staging invece di un'immagine di staging, quindi questo non sarà necessario.
    Useremo VK_IMAGE_TILING_OPTIMAL per un accesso efficiente dallo shader.
    */
    createImage(device, physicalDevice,
                texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage, textureImageMemory);

    transitionImageLayout(device, commandPool, graphicsQueue,
                          textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); // teniamo il layout undefined perché non ci interessa prima del trasferimento

    // copiamo i dati dal buffer di staging all'immagine
    copyBufferToImage(device, commandPool, graphicsQueue,
                      stagingBuffer, textureImage,
                      static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    transitionImageLayout(device, commandPool, graphicsQueue,
                          textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // distruggiamo il buffer di staging
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Texture::createTextureImageView()
{
    textureImageView = createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    /*
    VK_SAMPLER_ADDRESS_MODE_REPEAT: Ripete la texture quando si superano le dimensioni dell'immagine.
    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Come repeat, ma inverte le coordinate per specchiare l'immagine quando si superano le dimensioni.
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Prende il colore del bordo più vicino alla coordinata oltre i limiti dell'immagine.
    VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Come clamp to edge, ma usa il bordo opposto a quello più vicino.
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Restituisce un colore solido quando si campiona oltre le dimensioni dell'immagine.
    */
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties); // nelle proprietà del dispositivo fisico c'è uno spazio che indica i limiti del dispositivo,
    // ci servono per sapere il massimo valore di anisotropia supportato

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // più basso è il valore, più bassa è la qualità ma più alta la performance
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;         // colore del bordo, non ci interessa in questo caso
    samplerInfo.unnormalizedCoordinates = VK_FALSE;                     // se è true, le coordinate non sono normalizzate, quindi vanno da 0 a 1
    samplerInfo.compareEnable = VK_FALSE;                               // non ci interessa in questo caso ma se fosse true, i texel verrebbero confrontati con un valore
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                       // non ci interessa in questo caso

    // per il mipmapping
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // ora che abbiamo settato tutto, possiamo creare il sampler
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void Texture::setIndex(int index)
{
    this->index = index; // settiamo l'indice della texture
}

int Texture::getIndex() const
{
    return index; // ritorniamo l'indice della texture
}