#include "texture.h"
#include "bufferUtils.h"
#include "mesh.h"

Mesh::Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
           VkCommandPool commandPool, VkQueue graphicsQueue,
           const std::vector<Vertex> &vertices,
           const std::vector<uint32_t> &indices) : device(device),
                                                        physicalDevice(physicalDevice),
                                                        commandPool(commandPool),
                                                        graphicsQueue(graphicsQueue),
                                                        vertices(vertices),
                                                        indices(indices)
{
    if (vertices.size() > 0)
    {
        createVertexBuffer();
    }
    if (indices.size() > 0)
    {
        createIndexBuffer(); // se ci sono indici, creiamo il buffer degli indici
    }
}

Mesh::~Mesh()
{
    vkDestroyBuffer(device, vertexBuffer, nullptr);    // distruggiamo il buffer dei vertici
    vkFreeMemory(device, vertexBufferMemory, nullptr); // distruggiamo la memoria del buffer dei vertici

    // vkDestroyBuffer(device, indexBuffer, nullptr); // distruggiamo il buffer degli indici
    // vkFreeMemory(device, indexBufferMemory, nullptr); // distruggiamo la memoria del buffer degli indici

    for (auto texture : textures)
    {
        delete texture; // distruggiamo le texture
    }
}

VkBuffer Mesh::getVertexBuffer() const
{
    return vertexBuffer; // ritorniamo il buffer dei vertici
}

size_t Mesh::getVertexCount() const
{
    return vertices.size(); // ritorniamo il numero di vertici
}

VkBuffer Mesh::getIndexBuffer() const
{
    return indexBuffer; // ritorniamo il buffer degli indici
}
size_t Mesh::getIndexCount() const
{
    return indices.size(); // ritorniamo il numero di indici
}
std::vector<Texture *> Mesh::getTextures() const
{
    return textures; // ritorniamo le texture
}

void Mesh::addTexture(Texture *texture)
{
    textures.push_back(texture); // aggiungiamo la texture al vettore di texture
}

// questa funzione ci permette di creare il vertex Buffer
void Mesh::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size(); // la dimensione del buffer è la somma della dimensione di tutti i vertici

    // per ottimizzare le prestazioni, usiamo un buffer di staging, che è un buffer temporaneo che viene usato per copiare i dati nella GPU, è come un ponte
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // ora piazziamo i dati nel buffer di staging, poi successivamente verranno inseriti nel buffer finale
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data); // mappiamo la memoria per poterci scrivere
    // ora possiamo copiare i dati nel buffer, essendo che il buffer è un array di vertici, possiamo usare la funzione memcpy per copiare i dati
    // però c'è un problema, i dati possono non essere inseriti immediatamente dentro il buffer, per esempio per colpa del caching e quindi essi potrebbero non essere visibili alla GPU
    // per questo motivo ci sono 2 possibili fix, abbiamo scelto il primo, che è più veloce e semplice
    //  1. usare un heap che sia coerente all'host, infatti abbiamo usato VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //  2. chiamare 2 funzioni: una vkFlushMappedMemoryRanges dopo la memcpy e una vkInvalidateMappedMemoryRanges prima di leggere i dati dalla memoria mappata
    memcpy(data, vertices.data(), (size_t)bufferSize); // copiamo i dati nel buffer
    vkUnmapMemory(device, stagingBufferMemory);        // unmapiamo la memoria per poterla usare

    // creiamo il buffer finale, che è quello che verrà usato dalla GPU
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    // ora possiamo copiare i dati dal buffer di staging al buffer finale, che è quello che verrà usato dalla GPU
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize); // copiamo i dati dal buffer di staging al buffer finale

    // e ora puliamo il buffer di staging, che non ci serve più
    vkDestroyBuffer(device, stagingBuffer, nullptr);    // distruggiamo il buffer di staging
    vkFreeMemory(device, stagingBufferMemory, nullptr); // distruggiamo la memoria del buffer di staging
}

// in caso di immagini più complesse, come un semplice rettangolo, formato da 2 triangoli, ci servirà un buffer di indici
// così da evitare ridondanza ripetendo gli stessi vertici più volte
// la creazione del buffer di indici è simile a quella del buffer di vertici, ma con alcune differenze
void Mesh::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // la differenza principale è che il buffer di indici deve essere creato con VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::setDescriptorSet(uint32_t frameIndex, VkDescriptorSet set)
{
    if (descriptorSets.size() <= frameIndex)
    {
        descriptorSets.resize(frameIndex + 1); // se il frame index è maggiore della dimensione del vettore, lo ridimensioniamo
    }
    descriptorSets[frameIndex] = set; // settiamo il descriptor set per il frame corrente
}

VkDescriptorSet Mesh::getDescriptorSet(uint32_t frameIndex) const
{
    return descriptorSets[frameIndex]; // ritorniamo il descriptor set per il frame corrente
}