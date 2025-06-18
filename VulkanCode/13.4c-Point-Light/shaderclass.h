#ifndef SHADERCLASS_HPP
#define SHADERCLASS_HPP
#define GLFW_DLL
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

namespace fs = std::filesystem;
struct Shader
{
    std::string type; // "vertex" or "fragment"
    std::string compiledPath;
};

static std::vector<char> readFile(const std::string &filename);

class ShaderClass
{
public:
    // Costruttore: Inizializza la classe ShaderClass con la directory degli shader e il dispositivo Vulkan.
    ShaderClass(const std::string &shaderDir, const VkDevice &device);

    // Distruttore: Libera le risorse utilizzate dalla classe ShaderClass.
    ~ShaderClass();

    // Inizializza i moduli shader e compila gli shader se necessario.
    bool init();

    // Abilita i moduli shader e crea i moduli shader Vulkan.
    void enable();

    // Disabilita i moduli shader e rilascia le risorse Vulkan.
    void disable();

    // Restituisce il modulo shader Vulkan per lo shader vertex.
    VkShaderModule getVertShaderModule() const;

    // Restituisce il modulo shader Vulkan per lo shader fragment.
    VkShaderModule getFragShaderModule() const;

    // aggiorna e restituisce lo struct da mappare successivamente nel buffer uniforme
    // void updateUniformBuffer(const std::vector<void *> uniformBufferMapped, const uint32_t frame);

protected:
    // Verifica se il file sorgente dello shader necessita di ricompilazione in base ai timestamp.
    bool needsRecompile(const std::string &srcPath, const std::string &spvPath);

    // Compila tutti gli shader nella directory se è necessaria la ricompilazione.
    bool compileAllIfNeeded();

    // Crea un modulo shader Vulkan dal bytecode SPIR-V fornito.
    VkShaderModule createShaderModule(const std::vector<char> &code);

private:
    VkDevice device;                              // Handle del dispositivo Vulkan
    std::string shaderDir;                        // Directory contenente i file sorgente degli shader
    std::vector<Shader> shaders;                  // Lista degli shader con i loro tipi e percorsi compilati
    VkShaderModule fragShaderModule;              // Modulo shader fragment
    VkShaderModule vertShaderModule;              // Modulo shader vertex
    VkPipelineShaderStageCreateInfo stageInfos{}; // Informazioni sugli stage shader per la creazione della pipeline
    // UniformBufferObject ubo;                      // struttura del buffer uniforme
};

#endif