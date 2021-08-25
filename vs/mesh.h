#pragma once

#include "common.h"
#include "buffer.h"

class Mesh {
    
public:
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Mesh();
    
    void cleanup();
    void createPlane();
    void createQuad();
    void createCube();
    void cmdCreateVertexBuffer();
    void cmdCreateIndexBuffer ();
    
    void scale(glm::vec3 size);
    void rotate(float angle, glm::vec3 axis);
    void translate(glm::vec3 translation);
    
    glm::mat4 getMatrix();
    
    int32_t sizeofPositions();
    int32_t sizeofNormals();
    int32_t sizeofColors();
    int32_t sizeofTexCoords();
    int32_t sizeofIndices();
    
    VkPipelineVertexInputStateCreateInfo* createVertexInputInfo();

    Buffer* m_vertexBuffer = nullptr;
    Buffer* m_indexBuffer = nullptr;

    std::vector<int32_t>   m_indices;
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec3> m_colors;
    std::vector<glm::vec2> m_texCoords;

private:
    
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    

    glm::mat4 m_model = glm::mat4(1.0f);
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    VkPipelineVertexInputStateCreateInfo stateCreateInfo{};
    VkVertexInputBindingDescription bindingDescription{};
    
    const int32_t sizeofPosition = sizeof(glm::vec3);
    const int32_t sizeofNormal   = sizeof(glm::vec3);
    const int32_t sizeofColor    = sizeof(glm::vec3);
    const int32_t sizeofTexCoord = sizeof(glm::vec2);
    const int32_t sizeofIndex    = sizeof(int);
    
};
