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

/**
 * @brief Struttura per rappresentare uno shader.
 */
struct Shader
{
    std::string type; // "vertex" or "fragment"
    std::string compiledPath;
};

/**
 * @brief Legge il contenuto di un file in un vettore di caratteri.
 *
 * @param filename Il percorso del file da leggere.
 * @return Un vettore di caratteri contenente il contenuto del file.
 * @throws std::runtime_error Se si verifica un errore durante la lettura del file
 */
static std::vector<char> readFile(const std::string &filename);

class ShaderClass
{
public:
    /**
     * @brief Costruttore della classe ShaderClass.
     *
     * Inizializza la classe con la directory degli shader e il dispositivo Vulkan.
     * @param shaderDir La directory contenente i file sorgente degli shader.
     * @param device Il dispositivo Vulkan su cui operare.
     * @note Questo costruttore non compila gli shader, ma prepara la classe per l'inizializzazione.
     * @throws std::runtime_error Se si verifica un errore durante la creazione del
     */
    ShaderClass(const std::string &shaderDir, const VkDevice &device);

    /**
     * @brief Distruttore della classe ShaderClass.
     *
     * Rilascia le risorse Vulkan allocate per gli shader.
     */
    ~ShaderClass();

    /**
     * @brief Inizializza i moduli shader e compila gli shader se necessario.
     * @return bool il risultato dell'inizializzazione.
     * @returns true se l'inizializzazione è riuscita, false altrimenti.
     */
    bool init();

    /**
     * @brief Abilita i moduli shader e crea i moduli shader Vulkan.
     */
    void enable();

    /**
     * @brief Disabilita i moduli shader e rilascia le risorse Vulkan.
     */
    void disable();

    /**
     * @brief Restituisce il modulo shader Vulkan per lo shader vertex.
     * @return Il modulo shader Vulkan per lo shader vertex.
     */
    VkShaderModule getVertShaderModule() const;

    /**
     * @brief Restituisce il modulo shader Vulkan per lo shader fragment.
     * @return Il modulo shader Vulkan per lo shader fragment.
     */
    VkShaderModule getFragShaderModule() const;

    // aggiorna e restituisce lo struct da mappare successivamente nel buffer uniforme
    // void updateUniformBuffer(const std::vector<void *> uniformBufferMapped, const uint32_t frame);

protected:
    /**
     * @brief Verifica se il file sorgente dello shader necessita di ricompilazione in base ai timestamp.
     * @param srcPath Il percorso del file sorgente dello shader.
     * @param spvPath Il percorso del file SPIR-V compilato.
     * @return true se è necessaria la ricompilazione, false altrimenti.
     * @throws std::runtime_error Se si verifica un errore durante la verifica dei file
     */
    bool needsRecompile(const std::string &srcPath, const std::string &spvPath);

    /**
     * @brief Compila tutti gli shader nella directory se è necessaria la ricompilazione.
     * @return true se la compilazione è riuscita, false altrimenti.
     * @throws std::runtime_error Se si verifica un errore durante la compilazione degli
     */
    bool compileAllIfNeeded();

    /**
     * @brief Crea un modulo shader Vulkan dal bytecode SPIR-V fornito.
     * @param code Il bytecode SPIR-V da utilizzare per creare il modulo shader.
     * @return Il modulo shader Vulkan creato.
     * @throws std::runtime_error Se si verifica un errore durante la creazione del modulo shader.
     */
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