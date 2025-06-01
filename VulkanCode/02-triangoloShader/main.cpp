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

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>   // Necessary for uint32_t
#include <limits>    // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream>   // Necessary for std::ifstream

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static std::vector<char> readFile(const std::string &filename)
{
    // std::ios::ate ci permette di posizionare il puntatore alla fine del file, così possiamo ottenere la dimensione del file in un colpo solo così da allocare la memoria necessaria
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }
    // ora che abbiamo il puntatore alla fine del file, possiamo ottenere la dimensione del file e allocare la memoria necessaria per il buffer
    // il puntatore alla fine del file è un long long, quindi dobbiamo convertirlo in un size_t (che è un unsigned int)
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // dopo aver allocato la memoria necesssaria, possiamo finalmente andare all'inizio del file e leggerlo
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
class InformaticaGraficaApplication
{
public:
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

    VkRenderPass renderPass;         // passaggio di rendering Vulkan
    VkPipelineLayout pipelineLayout; // layout della pipeline Vulkan
    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers; // framebuffer della swap chain Vulkan
    VkCommandPool commandPool;                        // pool di comandi Vulkan
    VkCommandBuffer commandBuffer;                    // buffer di comandi Vulkan

    // sto usando dei vettori in caso ci siano dei frame aggiuntivi, ma essendo che non ci sono, si puà usare anche un solo elemento
    // i vettori adesso sono solo per scopo didattico, in un'applicazione reale si userebbero i frame in volo per gestire più frame contemporaneamente
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    void initWindow()
    {
        glfwInit();

        // essendo che GLFW ha un supporto di default per openGL,
        // dobbiamo specificare che vogliamo usare vulkan con la seguente riga
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // in più per evitare la gestione del disegno in caso di cambiamento di dimnsioni della finestra durante l'esecuzione
        // con la riga seguente disabilitiamo la proprietà di resize della finestra che genereremo
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // creiamo la finestra di dimensioni WIDTH e HEIGHT con il titolo "Vulkan" (gli ultimi 2 parametri li ignoriamo che non ci interessano)
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        // dobbiamo specificare che vogliamo usare dei tasti della tastiera per gestire gli input
        glfwSetKeyCallback(window, key_callback);
    }

    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        {
            std::cout << "Escape pressed" << std::endl;
            glfwSetWindowShouldClose(window, true);
        }
    }

    void initVulkan()
    {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }
    // questa funzione gestisce tutti gli eventi della finestra in esecuzione
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
    // questo fungerà da "distruttore" per la nostra applicazione, liberando le risorse allocate
    void cleanup()
    {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);
        for (auto framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        // essendo che non è solo una la image view, dobbiamo eliminarle tutte utilizzando un ciclo for each
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

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

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

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
    // questa funzione ci permette di scegliere il modo di presentazione della swap chain
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            // se non è un problema l'uso di energia, possiamo usare questa modalità
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        // questa modalità è sempre disponibile, quindi la usiamo come fallback
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // questa funzione ci permette di scegliere le dimensioni della swap chain
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

    // questa funzione ci permette di creare la swap chain
    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // ora che abbiamo scelto le dimensioni della swap chain, dobbiamo aggiungere altri parametri che ci servono per la creazione della swap chain
        // iniziando dal numero di immagini della swap chain, abbiamo un minimo ed un massimo (entrambi diversi da 0, perché lo 0 indica la mancanza di massimo)
        // in questo caso usiamo il numero minimo di immagini, ma è sempre meglio usare il numero di immagini richieste + 1, così da velocizzare i rendering delle immagini in successione
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
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

    // questa funzione ci permette di creare un shader module, che è un oggetto vulkan che rappresenta uno shader compilato
    VkShaderModule createShaderModule(const std::vector<char> &code)
    {
        // come al solito creiamo uno struct per specificare i parametri che ci servono, in questo caso per lo shader module
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        // la dimensione del codice è in byte, quindi dobbiamo convertirla in uint32_t perché il puntatore è un uint32_t,
        // per questo tipo di cast, dobbiamo assicurarci che la dimensione del codice sia un multiplo di 4, altrimenti il cast non funzionerà, per fortuna è un vector, quindi non abbiamo problemi
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        // ora che abbiamo settato tutti i parametri, possiamo finalmente creare lo shader module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    // questa funzione ci permette di creare la pipeline grafica, che è un oggetto vulkan che rappresenta la pipeline di rendering
    void createGraphicsPipeline()
    {
        // grazie alla funzione readFile che abbiamo creato, possiamo leggere i file binari che abbiamo creato utilizzando il file compile.bat, che sfrutta glslc per compilare i file shader in file binari
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        // le shader sono variabili locali e non della classe per far sì che vengano distrutte subito dopo la creazione della pipeline,
        // essendo che il linking e la compilazione delle shader non avviene fin quando non chiamiamo la funzione vkCreateGraphicsPipelines
        // dandoci la possibilità di distruggerle tranquillamente alla fine della funzione
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
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

        // questo struct specifica il formato in cui vengono inseriti i vertex, essendo che non abbiamo vertici da inserire qui, lo lasciamo vuoto
        // questo perché i vertici li abbiamo inseriti direttamente nella shader
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // opzionale
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // opzionale

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
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
        pipelineLayoutInfo.setLayoutCount = 0;            // opzionale
        pipelineLayoutInfo.pSetLayouts = nullptr;         // opzionale
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // opzionale
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // opzionale

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
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    // questa funzione ci permette di creare la render pass, che è un oggetto vulkan che rappresenta il passaggio di rendering
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

    // questa funzione ci permette di creare i framebuffer
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

    // vulkan non ha funzioni specifiche per le sue operazioni, tipo quelle di disegno o di memoria
    // esso utilizza dei buffer di comandi pieni di istruzioni che vengono eseguite dalla GPU
    // questo è vantaggioso perché permette di eseguire più operazioni in parallelo e di ottimizzare le prestazioni
    // quindi la funzione seguente ci permette di creare pull di comandi
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

    // ora che abbiamo creato la command pool, possiamo creare i buffer di comandi
    void createCommandBuffers()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // level indica se il buffer di comandi è primario o secondario, in questo caso primario
        //   i buffer di comandi primari possono essere inseriti in coda, ma non possono essere chiamati da altri buffer di comandi
        //   i buffer di comandi secondari possono essere chiamati da altri buffer di comandi, ma non possono essere inseriti direttamente in coda
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1; // il numero di buffer di comandi da allocare

        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    // per registrare i comandi abbiamo bisogno di un'altra funzione, il recordo command buffer
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
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // e anche la scissor, cioè la parte della finestra in cui vogliamo disegnare
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // ora finalmente possiamo disegnare il triangolo
        //  il primo parametro è il buffer di comandi, il secondo è il numero di vertici da disegnare, il terzo è il numero di istanze da disegnare (in questo caso 1), il quarto è l'indice del primo vertice da disegnare e l'ultimo è l'indice della prima istanza da disegnare
        //  in questo caso disegniamo 3 vertici (il triangolo) e 1 istanza (il triangolo stesso)
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        // ora che abbiamo finito di disegnare, possiamo finalmente terminare il render pass
        vkCmdEndRenderPass(commandBuffer);

        // se è un successo non avremo nessun errore
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame()
    {
        // questa funzione prende un array di fence e aspetta che una o tutte le fence siano pronte prima di ritornare un valore
        // il VK_TRUE indica che vogliamo aspettare tutte le fence (essendo solo una in questo caso, non cambia nulla)
        // mentre il UINT64_MAX indica il tempo massimo per un timeout di attesa (essendo massimo, esso viene disabilitato)
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);
        uint32_t imageIndex;
        // questa funzione ci permette di acquisire l'immagine successiva dalla swap chain, in questo caso l'immagine successiva è quella che abbiamo appena disegnato
        //  il primo parametro è il logical device, il secondo è la swap chain, il terzo è il timeout (in questo caso infinito),
        // il quarto è il semaforo che abbiamo creato prima (che ci permette di sincronizzare le operazioni tra la CPU e la GPU),
        // il quinto è la fence e l'ultimo parametro è un output e indica l'indice dell'immagine che vogliamo acquisire
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        // ora che abbiamo l'indice dell'immagine possiamo settare il command buffer per il disegno, lo resettiamo per assicurarci che sia pronto per essere registrato
        vkResetCommandBuffer(commandBuffer, 0);
        // ora registriamo il command buffer, che è il buffer di comandi che abbiamo creato prima
        recordCommandBuffer(commandBuffer, imageIndex);

        // per configurare la sincronizzazione usiamo il seguente struct
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // qui specifichiamo quanti e quali semafori aspettare e in che parte della pipeline rimanere in attesa
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // successivamente indichiamo quali command buffer vogliamo eseguire
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // infine specifichiamo i semafori che vogliamo segnalare al termine dell'esecuzione del command buffer
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
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
        vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    //  il semaforo serve per sincronizzare le operazioni tra la CPU e la GPU, in modo che la CPU non invii comandi alla GPU prima che sia pronta a riceverli
    //  la fence serve per sincronizzare le operazioni tra i vari comandi, in modo che la GPU non esegua un comando prima che il comando precedente sia stato completato
    //  questa funzione ci permette di creare i semafori e le fence
    void createSyncObjects()
    {
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
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores!");
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