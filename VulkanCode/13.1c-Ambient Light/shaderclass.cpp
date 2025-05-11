#include "shaderclass.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
namespace fs = std::filesystem;

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

ShaderClass::ShaderClass(const std::string &shaderDir, const VkDevice &device) : sdkPath("C:/StageGLToVulkan/VulkanCode/base/VulkanSDK/Bin/glslc.exe"), shaderDir(shaderDir), device(device)
{
    // Inizializza i moduli shader
    fragShaderModule = VK_NULL_HANDLE;
    vertShaderModule = VK_NULL_HANDLE;
}

ShaderClass::~ShaderClass()
{
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

bool ShaderClass::init()
{
    if (!compileAllIfNeeded())
    {
        std::cerr << "Failed to compile shaders!" << std::endl;
        return false;
    }
    return true;
}

void ShaderClass::enable()
{
    // Abilita gli shader compilati
    if (vertShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    if (fragShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

    vertShaderModule = VK_NULL_HANDLE;
    fragShaderModule = VK_NULL_HANDLE;

    for (const auto &shader : shaders)
    {
        if (shader.type == "vertex")
            vertShaderModule = createShaderModule(readFile(shader.compiledPath));
        else if (shader.type == "fragment")
            fragShaderModule = createShaderModule(readFile(shader.compiledPath));
    }
}

void ShaderClass::disable()
{
    // Disabilita gli shader compilati
    if (vertShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    if (fragShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

    vertShaderModule = VK_NULL_HANDLE;
    fragShaderModule = VK_NULL_HANDLE;
}

VkShaderModule ShaderClass::getVertShaderModule() const
{
    return vertShaderModule;
}

VkShaderModule ShaderClass::getFragShaderModule() const
{
    return fragShaderModule;
}

bool ShaderClass::needsRecompile(const std::string &srcPath, const std::string &spvPath)
{
    if (!std::filesystem::exists(spvPath))
        return true;

    auto srcTime = std::filesystem::last_write_time(srcPath);
    auto spvTime = std::filesystem::last_write_time(spvPath);
    return srcTime > spvTime;
}

bool ShaderClass::compileAllIfNeeded()
{
    bool allCompiled = true;
    for (const auto &entry : std::filesystem::directory_iterator(shaderDir))
    {
        if (!entry.is_regular_file())
            continue;

        std::string path = entry.path().string();
        std::string ext = entry.path().extension().string();

        if (ext != ".vert" && ext != ".frag")
            continue;

        std::string baseName = entry.path().filename().string(); // es. triangle.vert
        std::string outputPath = "compiled/" + baseName + ".spv";

        if (needsRecompile(path, outputPath))
        {
            std::cout << "Compilazione necessaria per: " << baseName << "\n";
            allCompiled = false;
        }
    }
    if (!allCompiled)
    {
        std::system("start cmd /K compile.bat");
        //essendo che l'applicazione è da riavviare per forza, non ha senso continuare
        std::cerr << "[INFO] Shader compilati. Riavvia manualmente l'applicazione." << std::endl;
        throw std::runtime_error("riavvio necessario per la compilazione delle shader");
    }

    shaders.clear();
    for (const auto &entry : std::filesystem::directory_iterator("compiled"))
    {
        if (!entry.is_regular_file())
            continue;

        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();

        if (filename.find(".vert.spv") != std::string::npos)
            shaders.push_back({"vertex", path});
        else if (filename.find(".frag.spv") != std::string::npos)
            shaders.push_back({"fragment", path});
    }

    return true;
}

VkShaderModule ShaderClass::createShaderModule(const std::vector<char> &code)
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
