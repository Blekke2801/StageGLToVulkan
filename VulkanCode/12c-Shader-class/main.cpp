#define GLFW_DLL
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shaderclass.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <chrono>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2; // numero di frame in volo
auto previousTime = std::chrono::high_resolution_clock::now();

/**
 * @brief Struttura per l'oggetto del buffer uniforme.
 *
 * Questo struct contiene tutti i dati che devono essere passati alla shader.
 * Ogni elemento della struttura deve essere allineato a 16 byte per garantire che Vulkan possa accedervi correttamente.
 */
struct UniformBufferObject
{
    glm::mat4 transform;
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * @brief Struttura dedicata alla camera all'interno dello spazio 3D.
 *
 * Questa struttura contiene le informazioni necessarie per calcolare la matrice di vista della camera.
 */
struct MyCamera
{
    glm::vec3 pos;
    glm::vec3 target;
    glm::vec3 up;
    float yaw;
    float pitch;
};

/**
 * @brief Struttura per i vertici del modello.
 *
 * Questa struttura contiene tutte le informazioni necessarie da elaborare all'interno della vertex shader.
 * Essa include anche metodi statici dedicati all'ottenere descrizioni di collegamento e descrizione sugli attributi dello stesso.
 */
struct Vertex
{
    glm::vec3 pos;

    glm::vec3 color;
    // vulkan ha bisogno di sapere come interpretare i dati che gli passiamo, quindi dobbiamo specificare il formato dei dati
    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // questo invece è per dire come estrarre i dati dai vertici, quindi dobbiamo specificare il formato dei dati e l'offset (cioè la posizione del dato all'interno della struttura)
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        // che formato ha il dato?
        // float: VK_FORMAT_R32_SFLOAT
        // vec2: VK_FORMAT_R32G32_SFLOAT
        // vec3: VK_FORMAT_R32G32B32_SFLOAT
        // vec4: VK_FORMAT_R32G32B32A32_SFLOAT
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        // come sopra, ma per il colore
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};
// il nostro triangolo è composto da 3 vertici, ognuno con una posizione
// essendo che in vulkan l'origine è in alto a sinistra e non in basso a sinistra come in opengl, dobbiamo invertire la y
// le opzioni sono 3, nella shader, qui, oppure possiamo ribaltare la viewport, ribalteremo la viewport in modo da non dover modificare lo shader o i singoli vertici
const std::vector<Vertex> vertices = {
    {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

    {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},

    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},

    {{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},

    {{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},

    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
};

// gli indici servono a dire quali vertici usare per formare il triangolo
// per ottenere i 4 triangoli con colori diversi, il primo vertice del triangolo deve essere quello dominante
// rispetto a openGL, in vulkan l'ordine degli indici è invertito, quindi dobbiamo invertire gli indici
const std::vector<uint32_t> indices = {};

glm::mat4 cubeTransform = glm::mat4(1.0f); // matrice di trasformazione del cubo

/**
 * @brief Struttura per gli indici delle famiglie di code.
 *
 * Questa struttura contiene gli indici delle famiglie di code necessarie per il rendering e la presentazione.
 */
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

/**
 * @brief Struttura per i dettagli del supporto della swap chain.
 *
 * Questa struttura contiene le informazioni necessarie per creare una swap chain.
 */
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// questo struct servirà a passare i dati alla shader, in questo caso la matrice di proiezione e la matrice di vista
struct MyCamera camera{
    glm::vec3(0.0f, 0.0f, 4.0f), // posizione della camera
    glm::vec3(0.0f, 0.0f, 0.0f), // target della camera
    glm::vec3(0.0f, 1.0f, 0.0f), // vettore up della camera
    -90.0f, 0.0f};               // angoli di yaw e pitch della camera

class InformaticaGraficaApplication
{
public:
    /**
     * @brief metodo principale per eseguire l'applicazione Vulkan.
     * @return non ritorna nulla
     */
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;   // puntatore della finestra GLFW
    VkInstance instance;  // istanza Vulkan
    VkSurfaceKHR surface; // esssendo che vulkan non gestisce le finestre, dobbiamo creare una superficie per la finestra che abbiamo creato con GLFW, così può disegnare

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // dispositivo fisico Vulkan
    VkDevice device;                                  // dispositivo logico Vulkan

    VkQueue graphicsQueue; // coda di rendering Vulkan
    VkQueue presentQueue;  // coda di presentazione Vulkan

    VkSwapchainKHR swapChain;             // swap chain Vulkan
    std::vector<VkImage> swapChainImages; // immagini della swap chain Vulkan
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews; // image view della swap chain Vulkan

    VkRenderPass renderPass;                     // passaggio di rendering Vulkan
    VkDescriptorSetLayout descriptorSetLayout;   // layout del set di descrittori Vulkan
    VkDescriptorPool descriptorPool;             // pool di descrittori Vulkan
    std::vector<VkDescriptorSet> descriptorSets; // set di descrittori Vulkan
    VkPipelineLayout pipelineLayout;             // layout della pipeline Vulkan
    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers; // framebuffer della swap chain Vulkan
    VkCommandPool commandPool;                        // pool di comandi Vulkan
    std::vector<VkCommandBuffer> commandBuffers;      // buffer di comandi Vulkan
    VkBuffer vertexBuffer;                            // buffer dei vertici Vulkan
    VkDeviceMemory vertexBufferMemory;                // memoria del buffer dei vertici Vulkan
    std::vector<VkBuffer> uniformBuffers;             // buffer uniformi Vulkan
    std::vector<VkDeviceMemory> uniformBuffersMemory; // memoria dei buffer uniformi Vulkan
    std::vector<void *> uniformBuffersMapped;         // puntatori ai buffer uniformi Vulkan
    VkBuffer indexBuffer;                             // buffer degli indici Vulkan
    VkDeviceMemory indexBufferMemory;                 // memoria del buffer degli indici Vulkan

    // sto usando dei vettori in caso ci siano dei frame aggiuntivi, ma essendo che non ci sono, si puà usare anche un solo elemento
    // i vettori adesso sono solo per scopo didattico, in un'applicazione reale si userebbero i frame in volo per gestire più frame contemporaneamente
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0; // frame corrente

    /**
     * @brief metodo di inizializzazione della finestra GLFW.
     *
     * metodo che inizializza GLFW, crea una finestra e imposta i callback per gli input della tastiera e del mouse se necessario.
     *
     * @return non ritorna nulla
     */
    void initWindow()
    {
        glfwInit();

        // essendo che GLFW ha un supporto di default per openGL,
        // dobbiamo specificare che vogliamo usare vulkan con la seguente riga
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // creiamo la finestra di dimensioni WIDTH e HEIGHT con il titolo "Vulkan"
        auto monitor = glfwGetPrimaryMonitor();
        // per avere la finestra in finstra senza bordi, dobbiamo specificare che vogliamo una finestra massimizzata e senza decorazioni
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        auto mode = glfwGetVideoMode(monitor);
        window = glfwCreateWindow(mode->width, mode->height, "Vulkan", monitor, nullptr);

        // dobbiamo specificare che vogliamo usare dei tasti della tastiera per gestire gli input
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glfwSetCursorPos(window, width / 2.0, height / 2.0);
        glfwSetCursorPosCallback(window, mouse_callback);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    /**
     * @brief metodo di callback per i comandi da tastiera
     * @param window la finestra GLFW
     * @param key il tasto premuto
     * @param scancode il codice del tasto premuto
     * @param action l'azione eseguita (premuto, rilasciato, ripetuto)
     * @param mods i modificatori della tastiera (shift, ctrl, alt, etc.)
     * @return non ritorna nulla
     */
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                std::cout << "Escape pressed" << std::endl;
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_UP:
            case GLFW_KEY_DOWN:
            case GLFW_KEY_LEFT:
            case GLFW_KEY_RIGHT:
            case GLFW_KEY_SPACE:
                cameraControls(key);
                break;
            case GLFW_KEY_W:
            case GLFW_KEY_A:
            case GLFW_KEY_S:
            case GLFW_KEY_D:
                cubeControls(key);
                break;
            default:
                break;
            }
        }
    }

    /**
     * @brief metodo di callback per il mouse
     * @param window la finestra GLFW
     * @param xpos la posizione X del mouse
     * @param ypos la posizione Y del mouse
     * @return non ritorna nulla
     */
    static void mouse_callback(GLFWwindow *window, double xpos, double ypos)
    {
        static float lastX = 0.0f;
        static float lastY = 0.0f;

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // invertito per l'asse Y
        if (std::abs(xoffset) > 100 || std::abs(yoffset) > 100)
        {
            lastX = xpos;
            lastY = ypos;
            return;
        }

        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        camera.yaw = fmod(camera.yaw + xoffset + 360.0f, 360.0f); // questo in modo da tenere lo yaw nel range [0, 360]
        camera.pitch += yoffset;

        // Limita il pitch per evitare che la camera si ribalti
        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
            camera.pitch = -89.0f;

        // Calcola la nuova direzione
        glm::vec3 direction;
        direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        direction.y = sin(glm::radians(camera.pitch));
        direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.target = camera.pos + glm::normalize(direction);
    }

    /**
     * @brief metodo per gestire i controlli della camera
     * @param key il tasto premuto
     * @return non ritorna nulla
     */
    static void cameraControls(int key)
    {
        // sta volta, le trasformazioni verranno applicate alla camera, quindi dovremo modificare la "view"

        float _speed = 0.05f;
        // calcola il vettore perpendicolare alla direzione della camera
        glm::vec3 direction = glm::normalize(camera.target - camera.pos);
        glm::vec3 right = glm::normalize(glm::cross(direction, camera.up));
        switch (key)
        {
        case GLFW_KEY_UP:
            // spostiamo la camera in avanti nella direzione in cui sta guardando
            camera.pos += direction * _speed;
            camera.target += direction * _speed;
            break;
        case GLFW_KEY_DOWN:
            // spostiamo la camera indietro nella direzione opposta a quella in cui sta guardando
            camera.pos -= direction * _speed;
            camera.target -= direction * _speed;
            break;
        case GLFW_KEY_LEFT:
            // spostiamo la camera a sinistra nella direzione perpendicolare alla direzione in cui sta guardando
            camera.pos -= right * _speed;
            camera.target -= right * _speed;
            break;
        case GLFW_KEY_RIGHT:
            // spostiamo la camera a destra nella direzione opposta a quella in cui sta guardando
            camera.pos += right * _speed;
            camera.target += right * _speed;
            break;
        case GLFW_KEY_SPACE:
            // reset della camera
            camera.pos = glm::vec3(0.0f, 0.0f, 4.0f);
            camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
            camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.yaw = -90.0f;
            camera.pitch = 0.0f;
            cubeTransform = glm::mat4(1.0f); // reset della matrice di trasformazione del cubo
            break;
        }
    }

    /**
     * @brief metodo per gestire i controlli di rotazione del modello
     * @param key il tasto premuto
     * @return non ritorna nulla
     */
    static void cubeControls(int key)
    {
        float _speed = 10.0f;

        switch (key)
        {
        case GLFW_KEY_W:
            // ruotiamo il cubo in avanti
            cubeTransform = glm::rotate(glm::mat4(), -glm::radians(_speed), glm::vec3(1.0f, 0.0f, 0.0f)) * cubeTransform;
            break;
        case GLFW_KEY_A:
            // ruotiamo il cubo a sinistra
            cubeTransform = glm::rotate(glm::mat4(), -glm::radians(_speed), glm::vec3(0.0f, 1.0f, 0.0f)) * cubeTransform;
            break;
        case GLFW_KEY_S:
            // ruotiamo il cubo indietro
            cubeTransform = glm::rotate(glm::mat4(), glm::radians(_speed), glm::vec3(1.0f, 0.0f, 0.0f)) * cubeTransform;
            break;
        case GLFW_KEY_D:
            // ruotiamo il cubo a destra
            cubeTransform = glm::rotate(glm::mat4(), glm::radians(_speed), glm::vec3(0.0f, 1.0f, 0.0f)) * cubeTransform;
            break;
        }
    }
    /**
     * @brief metodo per inizializzare Vulkan
     * @return non ritorna nulla
     */
    void initVulkan()
    {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        // createIndexBuffer();
        createCommandBuffers();
        createSyncObjects();
    }

    /**
     * @brief metodo per eseguire il ciclo principale dell'applicazione
     *
     * Questo metodo esegue il ciclo principale dell'applicazione, gestendo gli eventi della finestra e disegnando i frame.
     *
     * @return non ritorna nulla
     */
    void mainLoop()
    {

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
        // aspettiamo che il dispositivo sia idle prima di chiudere l'applicazione
        vkDeviceWaitIdle(device);
    }

    /**
     * @brief metodo per pulire le risorse allocate da Vulkan
     *
     * Questo metodo distrugge tutte le risorse allocate da Vulkan, come la swap chain, i buffer, le immagini, i semafori, etc.
     *
     * @return non ritorna nulla
     */
    void cleanup()
    {
        cleanupSwapChain();

        // vkDestroyBuffer(device, indexBuffer, nullptr);
        // vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    /**
     * @brief Crea un buffer e alloca memoria per esso.
     *
     * Questo metodo crea un buffer a seconda delle specifiche fornite e alloca la memoria necessaria.
     *
     * @param size La dimensione del buffer da creare.
     * @param usage Le flag di utilizzo del buffer
     * @param properties Le proprietà della memoria del buffer.
     * @param buffer Il buffer creato.
     * @param bufferMemory La memoria allocata per il buffer.
     */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
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
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

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

    /**
     * @brief metodo per creare l'istanza Vulkan
     *
     * Questo metodo crea l'istanza Vulkan, che è il primo oggetto da creare in un'applicazione Vulkan.
     *
     * @return non ritorna nulla
     */
    void createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Informatica Grafica";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
    }

    /**
     * @brief metodo per creare la superficie di rendering Vulkan
     *
     * Questo metodo crea la superficie di rendering Vulkan, che è necessaria per visualizzare il contenuto della finestra.
     *
     * @return non ritorna nulla
     */
    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    /**
     * @brief metodo per scegliere il dispositivo fisico Vulkan
     *
     * Questo metodo sceglie il dispositivo fisico Vulkan, che è il dispositivo che eseguirà le operazioni di rendering.
     *
     * @return non ritorna nulla
     */
    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto &device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    /**
     * @brief metodo per verificare se il dispositivo fisico Vulkan è adatto
     *
     * Questo metodo verifica se il dispositivo fisico Vulkan è adatto per l'applicazione, controllando le queue family, le estensioni e le swap chain.
     *
     * @param device il dispositivo fisico Vulkan da verificare
     * @return true se il dispositivo è adatto, false altrimenti
     */
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);

        // controlliamo se il dispositivo supporta le estensioni che vogliamo usare
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // controlliamo se il dispositivo supporta le swap chain
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    /**
     * @brief metodo per verificare se il dispositivo fisico supporta le estensioni necessarie
     *
     * Questo metodo verifica se il dispositivo fisico supporta le estensioni necessarie per l'applicazione.
     *
     * @param device il dispositivo fisico Vulkan da verificare
     * @return true se il dispositivo supporta le estensioni, false altrimenti
     */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    /**
     * @brief metodo per trovare le queue family del dispositivo fisico
     *
     * Questo metodo trova le queue family del dispositivo fisico, che sono necessarie per creare il dispositivo logico.
     *
     * @param device il dispositivo fisico Vulkan da verificare
     * @return QueueFamilyIndices struct contenente gli indici delle queue family
     */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            // ora che useremo anche il transferimento di memoria, ci serve anche il VKQUEUE_TRANSFER_BIT
            // per fortuna, il VKQUEUE_GRAPHICS_BIT include anche il VKQUEUE_TRANSFER_BIT, quindi non ci serve fare altro
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = i;
            }
            i++;
        }

        return indices;
    }

    /**
     * @brief metodo per creare il dispositivo logico Vulkan
     *
     * Questo metodo crea il dispositivo logico Vulkan utilizzando le queue family trovate in precedenza.
     * Il dispositivo logico è l'oggetto che ci permette di interagire con il dispositivo fisico e di eseguire le operazioni di rendering.
     *
     * @return non ritorna nulla
     */
    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        // Vulkan usa come valore di priorità un numero tra 0 e 1, possiamo usare un float per rappresentare il valore di priorità.
        // Più il valore è alto, più la priorità è alta.
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Creiamo uno struct per specificare le feature del dispositivo fisico che vogliamo usare.
        // Per ora non ci serve nulla di particolare, quindi lo lasciamo vuoto.
        VkPhysicalDeviceFeatures deviceFeatures{};

        // Ora con questi struct possiamo finalmente creare il dispositivo logico.
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        // Indichiamo specificatamente che vogliamo lo struct che abbiamo appena creato (queueCreateInfos) per la creazione del dispositivo logico.
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        // Specifichiamo le features del dispositivo fisico che vogliamo usare utilizzando lo struct creato prima.
        createInfo.pEnabledFeatures = &deviceFeatures;

        // Anche se inutile perché ora non è più necessario farlo, essendo che dalle recenti implementazioni Vulkan esse vengono ignorate,
        // possiamo specificare le estensioni che vogliamo usare.
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Ora che è stato settato tutto, possiamo ufficialmente creare il dispositivo logico.
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        // Ora che abbiamo creato il dispositivo logico, possiamo ottenere la coda di rendering (graphicsQueue) che ci serve per disegnare.
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        // E la coda di presentazione (presentQueue) che ci serve per presentare il disegno.
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    /**
     * @brief metodo per controllare il supporto dei validation layers
     *
     * Questo metodo controlla se i validation layers richiesti sono supportati dal sistema.
     * I validation layers sono strumenti utili durante lo sviluppo per rilevare errori e problemi di utilizzo dell'API Vulkan.
     *
     * @return true se i validation layers sono supportati, false altrimenti
     */
    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief metodo per verificare capacità della superfice di presentazione, formati e modalità di presentazione
     *
     * Questo metodo verifica le capacità della superficie di presentazione, i formati supportati e le modalità di presentazione disponibili per il dispositivo fisico.
     *
     * @param device il dispositivo fisico Vulkan da verificare
     * @return SwapChainSupportDetails struct contenente le capacità della superficie, i formati e le modalità di presentazione
     */
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        // la linea seguente ci dice quanti formati di superficie sono supportati dal dispositivo fisico
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        // in caso formatCount fosse diverso da 0 dopo la chiamata, significa che il dispositivo supporta almeno un formato di superficie
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // simile alla chiamata precedente, ma per i present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    /**
     * @brief metodo per scegliere il formato della superficie di presentazione più adatto
     * @param availableFormats i formati di superficie disponibili per il dispositivo fisico
     * @return il formato di superficie scelto
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    /**
     * @brief metodo per scegliere la modalità di presentazione più adatta
     * @param availablePresentModes le modalità di presentazione disponibili per il dispositivo fisico
     * @return la modalità di presentazione scelta
     */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            // se non è un problema l'uso di energia, possiamo usare questa modalità
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                std::cout << "Using VK_PRESENT_MODE_MAILBOX_KHR" << std::endl;
                return availablePresentMode;
            }
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            {
                std::cout << "Using VK_PRESENT_MODE_FIFO_RELAXED_KHR" << std::endl;
                return availablePresentMode;
            }
        }
        // questa modalità è sempre disponibile, quindi la usiamo come fallback
        std::cout << "Using VK_PRESENT_MODE_FIFO_KHR" << std::endl;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /**
     * @brief metodo per scegliere le dimensioni della swap chain
     *
     * Questo metodo sceglie le dimensioni della swap chain in base alle capacità della superficie.
     * Le dimensioni della swap chain sono importanti perché determinano la risoluzione delle immagini che verranno presentate sullo schermo.
     *
     * @param capabilities le capacità della superficie
     * @return le dimensioni della swap chain scelte
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        // se le dimensioni della superficie sono già state definite, usiamo quelle
        // altrimenti usiamo le dimensioni della finestra
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            // dobbiamo usare la funzione glfwGetFramebufferSize per ottenere le dimensioni della finestra, in quanto vulkan lavora con i pixel
            // e non con le dimensioni della finestra in coordinate logiche, quindi dobbiamo convertire le dimensioni della finestra in pixel
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            // ora dobbiamo assicurarci che le dimensioni siano comprese tra le dimensioni minime e massime supportate dal dispositivo fisico (perchè vulkan non lo fa automaticamente, e alcuni schermi sono diversi da altri)
            // quindi dobbiamo assicurarci che le dimensioni siano comprese tra le dimensioni minime e massime supportate dal dispositivo fisico
            // in caso non lo siano, le impostiamo a quelle minime e massime supportate dal dispositivo fisico
            // in questo caso, usiamo std::clamp per assicurarci che le dimensioni siano comprese tra le dimensioni minime e massime supportate dal dispositivo fisico
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    /**
     * @brief metodo per creare la swap chain
     *
     * Questo metodo crea la swap chain, che è una serie di immagini che vengono presentate sullo schermo.
     * La swap chain è necessaria per visualizzare il contenuto della finestra.
     *
     * @return non ritorna nulla
     */
    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // ora che abbiamo scelto le dimensioni della swap chain, dobbiamo aggiungere altri parametri che ci servono per la creazione della swap chain
        // iniziando dal numero di immagini della swap chain, abbiamo un minimo ed un massimo (entrambi diversi da 0, perché lo 0 indica la mancanza di massimo)
        // in questo caso usiamo il numero minimo di immagini, ma è sempre meglio usare il numero di immagini richieste + 1, così da velocizzare i rendering delle immagini in successione
        uint32_t imageCount;
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            imageCount = swapChainSupport.capabilities.minImageCount + 1;
        }
        else
        {
            imageCount = swapChainSupport.capabilities.minImageCount + 2;
        }
        // ora dobbiamo assicurarci che il numero di immagini non superi il numero massimo di immagini supportate dal dispositivo fisico
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // come è solito in vulkan, dobbiamo usare uno struct per specificare i parametri della swap chain
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        // questo è il numero di layer che vogliamo usare per la swap chain, in questo caso 1
        // i layer sono come le immagini, ma sono usati per il rendering 3D, quindi non ci interessano in questo caso
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // qui dobbiamo specificare le code che vogliamo usare per la swap chain
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;     // opzionale
            createInfo.pQueueFamilyIndices = nullptr; // opzionale
        }

        // possiamo specificare se vogliamo che le immagini possano subire delle trasformazioni (rotazioni, riflessioni, ecc.) prima di essere presentate, se supportato dal dispositivo fisico
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // questo è abbastanza auto esplicativo, il secondo parametro, con VK_TRUE, indica che non ci interessa se i pixel vengono oscurati o meno
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        // questo serve in caso serva ricreare la swap chain, per esempio se la finestra viene ridimensionata
        //  in questo caso, non ci interessa, quindi lo lasciamo a VK_NULL_HANDLE
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        // ora che abbiamo settato tutti i parametri, possiamo finalmente creare la swap chain
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        // Ottiene il numero di immagini nella swap chain (prima chiamata per il conteggio).
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);

        // Ridimensiona il vettore per contenere tutte le immagini.
        swapChainImages.resize(imageCount);

        // Recupera le immagini della swap chain e le memorizza nel vettore.
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    /**
     * @brief metodo per pulire la swap chain
     *
     * Questo metodo pulisce le risorse allocate per la swap chain, come le immagini, i framebuffer e le depth resources.
     *
     * @return non ritorna nulla
     */
    void cleanupSwapChain()
    {
        for (auto framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    /**
     * @brief metodo per ricreare la swap chain
     *
     * Questo metodo ricrea la swap chain in caso la finestra venga ridimensionata o minimizzata.
     * Dobbiamo aspettare che le dimensioni della finestra siano valide prima di ricreare la swap chain.
     * Quindi in attesa di dimensioni valide, blocchiamo il programma in un ciclo di attesa di eventi della finestra.
     *
     * @return non ritorna nulla
     */
    void recreateSwapChain()
    {
        // in caso la finestra venga minimizzata, il framebuffer size è 0, quindi dobbiamo aspettare che venga ridimensionata, quindi lo mettiamo in "pausa"
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    /**
     * @brief metodo per creare le image views della swap chain
     *
     * Questo metodo crea le image views per le immagini della swap chain.
     * Le image views sono necessarie per accedere alle immagini della swap chain e per utilizzarle come attachment nei framebuffer.
     *
     * @return non ritorna nulla
     */
    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];

            // il formato dell'immagine della swap chain, cioè come viene interpretata l'immagine
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            // questo è per modificare la colorazione dell'immagine, non ci serve, quindi per ogni colore usiamo l'identità
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // questo è per specificare come vogliamo visualizzare l'immagine, in questo caso non ci interessa, quindi lo lasciamo a VK_IMAGE_ASPECT_COLOR_BIT
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            // ora che abbiamo settato tutti i parametri, possiamo finalmente creare l'immagine
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    /**
     * @brief metodo di creazione della pipeline grafica
     *
     * Questo metodo crea la pipeline grafica, che è responsabile del rendering delle immagini.
     * La pipeline grafica è un insieme di stati che definiscono come i vertici vengono trasformati in pixel e come i pixel vengono colorati.
     * La pipeline grafica è composta da diversi stadi, come il vertex shader, il fragment shader, la rasterizzazione, etc.
     *
     * @return non ritorna nulla
     */
    void createGraphicsPipeline()
    {
        ShaderClass shaderClass("shaders", device);
        if (!shaderClass.init())
        {
            throw std::runtime_error("failed to create shader module!");
        }
        shaderClass.enable();

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = shaderClass.getVertShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = shaderClass.getFragShaderModule();
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // essendo che una parte della pipeline può cambiare anche durante l'esecuzione, dobbiamo specificare che alcune proprietà verranno ignorate
        //  e che verranno impostate in un secondo momento, per questo usiamo lo struct VkPipelineDynamicStateCreateInfo
        // std::vector<VkDynamicState> dynamicStates = {
        //     VK_DYNAMIC_STATE_VIEWPORT,
        //     VK_DYNAMIC_STATE_SCISSOR};

        // VkPipelineDynamicStateCreateInfo dynamicState{};
        // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        // dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        // dynamicState.pDynamicStates = dynamicStates.data();

        // questo struct specifica il formato in cui vengono inseriti i vertex, a differenza dello scorso progetto, usiamo un buffer
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;            // questo è il binding description che abbiamo creato prima
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // questo è l'attribute description che abbiamo creato prima

        // questo struct specifica 2 cose: che tipo di geometria verrà disegnata e se vogliamo usare il primitive restart (che non ci interessa in questo caso)
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        /*  specifica il tipo di geometria che vogliamo disegnare: può avere i seguenti valori:
                VK_PRIMITIVE_TOPOLOGY_POINT_LIST: punti singoli
                VK_PRIMITIVE_TOPOLOGY_LINE_LIST: linee da 2 vertici separati senza riutilizzarli
                VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: il vertice finale di una linea è il primo vertice della linea successiva
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangoli da 3 vertici separati senza riutilizzarli
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: gli ultimi 2 vertici di un triangolo sono i primi 2 vertici del triangolo successivo
            visto che dobbiamo disegnare un singolo triangolo useremo VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        */
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // questo struct specifica lo stato della viewport e della scissor, cioè la parte della finestra in cui vogliamo disegnare
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // questo struct descrive la rasterizzazione, cioè il processo di conversione dei vertici in pixel
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        // specifica il modo in cui vogliamo disegnare i poligoni (riempito, contorno o entrambi)
        //  in questo caso vogliamo disegnare i poligoni riempiti, quindi usiamo VK_POLYGON_MODE_FILL
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // essendo che abbiamo la viewport ribaltata, dobbiamo usare il counter clockwise, altrimenti non vedremmo nulla
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // opzionale
        rasterizer.depthBiasClamp = 0.0f;          // opzionale
        rasterizer.depthBiasSlopeFactor = 0.0f;    // opzionale

        // questo struct configura il multisampling (come suggerisce il nome), cioè il processo di campionamento dei pixel per migliorare la qualità dell'immagine
        //  in questo caso non ci interessa, quindi lo lasciamo a VK_SAMPLE_COUNT_1_BIT (cioè 1 campione per pixel)
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;          // opzionale
        multisampling.pSampleMask = nullptr;            // opzionale
        multisampling.alphaToCoverageEnable = VK_FALSE; // opzionale
        multisampling.alphaToOneEnable = VK_FALSE;      // opzionale

        // questo struct specifica lo stato del color blending, cioè il processo di combinazione dei pixel per creare effetti di trasparenza e ombre
        // essendo che noi usiamo solo un colore, non ci interessa, quindi lo lasciamo a VK_FALSE, ma per completezza mostrerò tutti i parametri
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // opzionale
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // opzionale
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // opzionale
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // opzionale
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // opzionale
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // opzionale

        // questo struct è per settare altri parametri del color blending
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // opzionale
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // opzionale
        colorBlending.blendConstants[1] = 0.0f; // opzionale
        colorBlending.blendConstants[2] = 0.0f; // opzionale
        colorBlending.blendConstants[3] = 0.0f; // opzionale

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // questo struct specifica lo stato della pipeline, cioè il processo di creazione della pipeline
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;                 // abbiamo solo 1 descriptor set layout, quindi 1
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // il layout dei descriptor set, che abbiamo creato prima
        pipelineLayoutInfo.pushConstantRangeCount = 0;         // opzionale
        pipelineLayoutInfo.pPushConstantRanges = nullptr;      // opzionale
        // ora che abbiamo settato tutti i parametri, possiamo finalmente creare la pipeline layout
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // finalmente creiamo la pipeline grafica, che è un oggetto vulkan che rappresenta la pipeline di rendering
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // infine, come spiegato prima, distruggiamo gli shader module
        shaderClass.disable();
    }

    /**
     * @brief metodo per creare il descriptor set layout
     *
     * Questo metodo crea il descriptor set layout, che è un oggetto vulkan che rappresenta il layout dei descriptor set.
     * I descriptor set sono usati per passare i dati alla pipeline grafica, come le texture e i buffer.
     *
     * @return non ritorna nulla
     */
    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Opzionale, è per lo più usato per le texture

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    /**
     * @brief metodo per creare il render pass
     *
     * Questo metodo crea il render pass, che è un oggetto vulkan che rappresenta il passaggio di rendering.
     * Il render pass definisce come i dati vengono elaborati durante il rendering, inclusi gli attachment e i subpass.
     * Gli attachment sono le immagini che vengono utilizzate durante il rendering, come le immagini della swap chain e le depth resources.
     * I subpass sono i passaggi di rendering che vengono eseguiti all'interno del render pass.
     *
     * @return non ritorna nulla
     */
    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat; // deve essere lo stesso della swap chain
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // questi 2 parametri indicano cosa bisogna fare con i dati prima di scriverli e dopo averli scritti
        /*
            per il loadOp abbiamo 3 opzioni:
                VK_ATTACHMENT_LOAD_OP_LOAD: mantiene i dati esistenti
                VK_ATTACHMENT_LOAD_OP_CLEAR: pulisce i valori esistenti e li sostituisce con delle costanti
                VK_ATTACHMENT_LOAD_OP_DONT_CARE: i dati sono indefiniti, quindi non ci interessa cosa succede
            invece per lo storeOp abbiamo 2 opzioni:
                VK_ATTACHMENT_STORE_OP_STORE: i dati vengono scritti nella memoria
                VK_ATTACHMENT_STORE_OP_DONT_CARE: i dati non vengono scritti nella memoria
        */
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // questi è per usare texture come colori (non lo usiamo per ora), verrà descritto meglio nel progetto delle texture
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // questo struct indica l'attachment dei sottopassi di rendering
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // abbiamo solo 1 attachment, quindi l'indice è 0
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // questo struct specifica le dipendenze tra i sottopassi di rendering
        VkSubpassDependency dependency{};
        // questi primi 2 parametri specificano gli indici dei sottopassi di rendering che dipendono l'uno dall'altro
        //  VK_SUBPASS_EXTERNAL si riferisce al sottopasso prima o dopo il passo di rendering a seconda di dove viene piazzato
        //  invece lo 0 si riferisce al sottopasso che abbiamo creato prima, esso deve essere sempre maggiore del srcSubpass per evitare cicli a meno che esso sia VK_SUBPASS_EXTERNAL
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        // questi 2 parametri specificano dove aspettare che i dati siano disponibili e in che stage dobbiamo eseguire le operazioni
        // essendo che prima di poter colorare l'immagine, dobbiamo avere il disegno pronto, dobbiamo aspettare in quello stage
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        // qui indichiamo le operazioni che devono aspettare per essere eseguite
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    /**
     * @brief metodo per creare i framebuffer
     *
     * Questo metodo crea i framebuffer, che sono oggetti vulkan che rappresentano le immagini della swap chain e le depth resources.
     * I framebuffer sono usati per il rendering delle immagini e devono essere creati dopo la creazione della swap chain e delle depth resources.
     * I framebuffer devono essere creati per ogni immagine della swap chain, questo perché ogni immagine della swap chain può essere presentata in un momento diverso.
     *
     * @return non ritorna nulla
     */
    void createFramebuffers()
    {
        // come si vede nelle righe seguenti, creare i frambuffer è molto semplice
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                swapChainImageViews[i]};

            // questo struct specifica i parametri del framebuffer, in questo caso abbiamo solo 1 attachment
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    /**
     * @brief metodo per creare la command pool
     *
     * vulkan non ha funzioni specifiche per le sue operazioni, tipo quelle di disegno o di memoria
     * esso utilizza dei buffer di comandi pieni di istruzioni che vengono eseguite dalla GPU
     * questo è vantaggioso perché permette di eseguire più operazioni in parallelo e di ottimizzare le prestazioni
     * quindi la funzione seguente ci permette di creare pool di comandi
     *
     * @return non ritorna nulla
     */
    void createCommandPool()
    {
        // per creare la command pool ci servono 2 parametri: la famiglia di code e le proprietà della command pool
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        /* ci sono 2 tipi di flag:
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: permette di resettare i buffer di comandi per ogni comando
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: permette di usare i buffer di comandi per operazioni temporanee (non ci interessa in questo caso)
        */
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    /**
     * @brief metodo per creare i buffer di comandi
     *
     * Questo metodo crea i buffer di comandi, che conterranno le istruzioni da eseguire dalla GPU.
     *
     * @return non ritorna nulla
     */
    void createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // level indica se il buffer di comandi è primario o secondario, in questo caso primario
        //   i buffer di comandi primari possono essere inseriti in coda, ma non possono essere chiamati da altri buffer di comandi
        //   i buffer di comandi secondari possono essere chiamati da altri buffer di comandi, ma non possono essere inseriti direttamente in coda
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size(); // il numero di buffer di comandi da allocare

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    /**
     * @brief metodo per registrare i comandi nei buffer dedicato
     *
     * Questo metodo registra i comandi nei buffer di comandi, che verranno eseguiti dalla GPU.
     * I comandi vengono registrati in un buffer di comandi primario, che può essere inserito in coda per l'esecuzione.
     * I comandi vengono registrati all'interno di un render pass, che definisce come i dati vengono elaborati durante il rendering.
     *
     * @param commandBuffer il buffer di comandi in cui registrare i comandi
     * @param imageIndex l'indice dell'immagine della swap chain da utilizzare
     *
     * @note Questa metodologia di condivisione dei comandi è efficiente perché permette di registrare i comandi una sola volta e di eseguirli più volte, riducendo il carico sulla CPU.
     * @return non ritorna nulla
     */
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        // come ogni cosa in vulkan, usiamo uno struct per specificare i parametri
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // le flag non ci interessano per ora, ma abbiamo altri valori utilizzabili:
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: il buffer di comandi verrà usato una sola volta
        // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: il buffer di comandi verrà usato in un render pass secondario
        // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: il buffer di comandi può essere usato in più queue contemporaneamente
        beginInfo.flags = 0; // Optional
        // pInheritanceInfo non ci interessa per ora, ma è usato per i buffer di comandi secondari
        beginInfo.pInheritanceInfo = nullptr; // Optional

        // se è già stato registrato, lo resettiamo
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // questi primi parametri sono per i binding, cioè per specificare quali buffer di comandi vogliamo usare
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

        // questi altri 2 sono per la dimensione della zona di rendering
        //  in questo caso usiamo le dimensioni della swap chain, per performance migliori
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        // colore di sfondo (nero con opacità 1.0f)
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // ora che abbiamo settato tutti i parametri, possiamo finalmente iniziare il render pass
        // il primo parametro della funzione deve essere sempre il buffer di comandi
        // il secondo è lo struct che abbiamo creato prima, che contiene i parametri del render pass
        // il terzo può essere uno dei seguenti valori:
        // VK_SUBPASS_CONTENTS_INLINE: il render pass viene eseguito inline, cioè il buffer di comandi viene eseguito direttamente e non ci sono buffer di comandi secondari
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: il render pass viene eseguito in un buffer di comandi secondario
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // ora possiamo collegare la pipeline grafica al buffer di comandi
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // ora dobbiamo specificare la viewport, cioè la parte della finestra in cui vogliamo disegnare
        // ricordiamo che dobbiamo ribaltare le coordinate Y, quindi l'altezza sarà negativa
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapChainExtent.height); // Sposta l'origine Y in basso per compensare l'inversione
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = -static_cast<float>(swapChainExtent.height); // Altezza negativa per ribaltare l'asse Y
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // e anche la scissor, cioè la parte della finestra in cui vogliamo disegnare
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

        // ora dobbiamo specificare il buffer di indici
        // vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0); // disegniamo i vertici

        // ora che abbiamo finito di disegnare, possiamo finalmente terminare il render pass
        vkCmdEndRenderPass(commandBuffer);

        // se è un successo non avremo nessun errore
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    /**
     * @brief Crea il buffer dei vertici e la memoria associata.
     */
    void createVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size(); // la dimensione del buffer è la somma della dimensione di tutti i vertici

        // per ottimizzare le prestazioni, usiamo un buffer di staging, che è un buffer temporaneo che viene usato per copiare i dati nella GPU, è come un ponte
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

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
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        // ora possiamo copiare i dati dal buffer di staging al buffer finale, che è quello che verrà usato dalla GPU
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize); // copiamo i dati dal buffer di staging al buffer finale

        // e ora puliamo il buffer di staging, che non ci serve più
        vkDestroyBuffer(device, stagingBuffer, nullptr);    // distruggiamo il buffer di staging
        vkFreeMemory(device, stagingBufferMemory, nullptr); // distruggiamo la memoria del buffer di staging
    }

    // in caso di immagini più complesse, come un semplice rettangolo, formato da 2 triangoli, ci servirà un buffer di indici
    // così da evitare ridondanza ripetendo gli stessi vertici più volte
    // la creazione del buffer di indici è simile a quella del buffer di vertici, ma con alcune differenze

    /**
     * @brief Crea il buffer degli indici e la memoria associata.
     */
    void createIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void *data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        // la differenza principale è che il buffer di indici deve essere creato con VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    /**
     * @brief metodo per creare i buffer uniformi
     *
     * Questo metodo crea i buffer uniformi, che verranno utilizzati per passare i dati alle shader.
     *
     * @return non ritorna nulla
     */
    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    /**
     * @brief metodo per aggiornare i buffer uniformi
     *
     * Questo metodo aggiorna i buffer uniformi con i dati correnti.
     *
     * @param frame il numero del frame corrente
     * @return non ritorna nulla
     */
    void updateUniformBuffer(const uint32_t frame)
    {

        // matrice di rotazione
        struct UniformBufferObject ubo{};
        ubo.transform = glm::scale(glm::mat4(), glm::vec3(0.5f, 0.5f, 0.5f)) * cubeTransform;                                 // identità
        ubo.view = glm::lookAt(camera.pos, camera.target, camera.up);                                                         // matrice di vista della camera
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f); // proiezione prospettica
        memcpy(uniformBuffersMapped[frame], &ubo, sizeof(ubo));
    }

    /**
     * @brief metodo per creare il descriptor pool
     *
     * Questo metodo crea il descriptor pool, oggetto che prende in gestione i descriptor set.
     *
     * @return non ritorna nulla
     */
    void createDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    /**
     * @brief metodo per creare i descriptor set
     *
     * Questo metodo crea i descriptor set, oggetti che rappresentano le risorse da utilizzare nella pipeline grafica.
     * Un descriptor set contiene informazioni inerenti a buffer e texture che verranno utilizzate durante il rendering.
     *
     * @return non ritorna nulla
     */
    void createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT /*possiamo usare anche VK_WHOLE_SIZE*/; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;

            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;

            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr;       // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    /**
     * @brief Copia i dati da un buffer ad un altro.
     *
     * Questo metodo copia i dati da un buffer di origine a un buffer di destinazione.
     *
     * @param srcBuffer Il buffer di origine da cui copiare i dati.
     * @param dstBuffer Il buffer di destinazione in cui copiare i dati.
     * @param size La dimensione dei dati da copiare.
     */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        // le operazioni di copia vengono eseguite in un command buffer, quindi dobbiamo crearne uno temporaneo
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        // essendo che lo usiamo solo una volta, dobbiamo specificarlo
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // questo dice che verrà usato solo una volta

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // ora specifichiamo la regione di copia con uno struct che specifica inizio e dimensione
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;                                             // offset di partenza del buffer sorgente
        copyRegion.dstOffset = 0;                                             // offset di partenza del buffer di destinazione
        copyRegion.size = size;                                               // dimensione della copia
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion); // copiamo i dati

        // ora che abbiamo finito di copiare i dati, dobbiamo terminare il command buffer
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE); // submitto il command buffer alla queue
        vkQueueWaitIdle(graphicsQueue);                               // aspetto che la queue sia vuota
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer); // libero il command buffer temporaneo
    }

    /**
     * @brief Trova il tipo di memoria adatto per un buffer.
     *
     * Questo metodo cerca il tipo di memoria più adatto per un buffer in base alle specifiche fornite.
     * @param physicalDevice Il dispositivo fisico Vulkan.
     * @param typeFilter I filtri di tipo di memoria.
     * @param properties Le proprietà della memoria desiderate.
     * @return Il tipo di memoria trovato.
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

    /**
     * @brief metodo per disegnare un frame
     *
     * Questo metodo gestisce il processo di disegno di un frame, inclusa l'acquisizione dell'immagine dalla swap chain e la registrazione dei comandi di disegno.
     * È il metodo principale che viene chiamato per ogni frame per renderizzare la scena.
     *
     * @return non ritorna nulla
     */
    void drawFrame()
    {

        // questa funzione prende un array di fence e aspetta che una o tutte le fence siano pronte prima di ritornare un valore
        // il VK_TRUE indica che vogliamo aspettare tutte le fence (essendo solo una in questo caso, non cambia nulla)
        // mentre il UINT64_MAX indica il tempo massimo per un timeout di attesa (essendo massimo, esso viene disabilitato)
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        uint32_t imageIndex;

        // ci serve sapere se dobbiamo ricreare la swap chain, quindi usiamo la funzione vkAcquireNextImageKHR
        // essa restituisce un valore che indica se ci sono stati errori o meno, in questo caso ci interessa solo il VK_ERROR_OUT_OF_DATE_KHR e il VK_SUBOPTIMAL_KHR
        //   VK_ERROR_OUT_OF_DATE_KHR: la swap chain è obsoleta e dobbiamo ricrearla
        //   VK_SUBOPTIMAL_KHR: la swap chain è ottimale, ma non perfetta (in questo caso non ci interessa)

        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // dopo aver aspettato le fence, dobbiamo resettarla per il prossimo frame
        // meglio ritardare il reset in caso di VK_ERROR_OUT_OF_DATE_KHR perché altrimenti la prossima chiamata di drawFrame verrà bloccata non superando il vkWaitForFences
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        // questa funzione ci permette di acquisire l'immagine successiva dalla swap chain, in questo caso l'immagine successiva è quella che abbiamo appena disegnato
        //  il primo parametro è il logical device, il secondo è la swap chain, il terzo è il timeout (in questo caso infinito),
        //  il quarto è il semaforo che abbiamo creato prima (che ci permette di sincronizzare le operazioni tra la CPU e la GPU),
        //  il quinto è la fence e l'ultimo parametro è un output e indica l'indice dell'immagine che vogliamo acquisire
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        // ora che abbiamo l'indice dell'immagine possiamo settare il command buffer per il disegno, lo resettiamo per assicurarci che sia pronto per essere registrato
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        // ora registriamo il command buffer, che è il buffer di comandi che abbiamo creato prima
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        // aggiorniamo il buffer di uniformi con la matrice di rotazione del triangolo
        updateUniformBuffer(currentFrame); // aggiorna la matrice di rotazione del triangolo

        // per configurare la sincronizzazione usiamo il seguente struct
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // qui specifichiamo quanti e quali semafori aspettare e in che parte della pipeline rimanere in attesa
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // successivamente indichiamo quali command buffer vogliamo eseguire
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        // infine specifichiamo i semafori che vogliamo segnalare al termine dell'esecuzione del command buffer
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // ora onfiguriamo la presentazione dell'immagine, cioè la parte finale del rendering
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // questo è abbastanza auto esplicativo, il secondo parametro è il semaforo che vogliamo aspettare prima di presentare l'immagine
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        // qui specifichiamo la swap chain e l'indice dell'immagine che vogliamo presentare
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        // esiste un ultimo parametro opzionale che ci permette di controllare il risultato della presentazione, ma non ci interessa in questo caso
        presentInfo.pResults = nullptr; // Optional

        // ora che abbiamo settato tutti i parametri, possiamo finalmente presentare l'immagine
        // vkqueuePresentKHR restituisce gli stessi valori di vkAcquireNextImageKHR, quindi possiamo controllare se ci sono errori
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // in caso di errore, controlliamo se la swap chain è obsoleta e dobbiamo ricrearla
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // ora che abbiamo presentato l'immagine, possiamo passare al frame successivo
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    /**
     * @brief metodo per creare gli oggetti utili alla sincronizzazione delle operazioni all'interno del sistema
     *
     * Questo metodo crea i semafori e le fence necessari per la sincronizzazione tra la CPU e la GPU.
     * Il semaforo serve per sincronizzare le operazioni tra la CPU e la GPU, in modo che la CPU non invii comandi alla GPU prima che sia pronta a riceverli
     * La fence serve per sincronizzare le operazioni tra i vari comandi, in modo che la GPU non esegua un comando prima che il comando precedente sia stato completato
     *
     * @note Essendo che Vulkan non ha un sistema di sincronizzazione automatico come OpenGL, è necessario gestire manualmente la sincronizzazione tra le operazioni della CPU e della GPU.
     * @return non ritorna nulla
     */
    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // essendo che in "drawFrame" aspettiamo immediatamente che la fence precedente venga segnalata, abbiamo il problema che per il primo frame da disegnare aspetterà in eterno
        // essendo che non esiste un frame precedente al primo, quindi per evitare questo problema usiamo il flag VK_FENCE_CREATE_SIGNALED_BIT
        //   che indica che la fence è già segnalata e quindi non aspetterà in eterno
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        // creiamo i semafori e le fence, se non ci sono errori, altrimenti lanciamo un'eccezione
        // essendo che i semafori e le fence sono oggetti vulkan, dobbiamo usare la funzione vkCreateSemaphore e vkCreate
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // creiamo i semafori e le fence, se non ci sono errori, altrimenti lanciamo un'eccezione
            // essendo che i semafori e le fence sono oggetti vulkan, dobbiamo usare la funzione vkCreateSemaphore e vkCreate
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }
};

int main()
{
    InformaticaGraficaApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}