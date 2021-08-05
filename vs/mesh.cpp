//  Copyright © 2020 Subph. All rights reserved.
//

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unordered_map>

#include "mesh.h"

Mesh::~Mesh() {}
Mesh::Mesh( VkDevice device, VkPhysicalDevice physicalDevice ) :
    m_device( device ),
    m_physicalDevice( physicalDevice ) {}

void Mesh::cleanup() {
    m_indexBuffer->cleanup();
    m_vertexBuffer->cleanup();
}

void Mesh::createPlane() {
    m_positions = {{-2., -1., 2.}, {2., -1., 2.}, {2., -1., -2.}, {-2., -1., -2.}};
    m_normals   = {{ 0., 1., 0.}, { 0., 1., 0.}, { 0., 1., 0.}, { 0., 1.,  0.}};
    m_colors    = {{ 1., 1., 1.}, { 1., 1., 1.}, { 1., 1., 1.}, { 1., 1., 1.}};
    m_texCoords = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    m_indices   = { 0, 1, 2, 2, 3, 0 };
}

void Mesh::createCube() {
    glm::vec3 cubeVertices[8] = {
        {-.5,  .5,  .5}, {-.5, -.5,  .5}, { .5,  .5,  .5}, { .5, -.5,  .5},
        { .5,  .5, -.5}, { .5, -.5, -.5}, {-.5,  .5, -.5}, {-.5, -.5, -.5}
    };
    glm::vec3 cubeColors[8] = {
        {1., 1., 1.}, {0., 0., 1.}, {1., 0., 0.}, {0., 1., 0.},
        {1., 1., 1.}, {0., 0., 1.}, {1., 0., 0.}, {0., 1., 0.}
    };
    unsigned int cubeIndices[] = {
        6, 7, 0, 0, 7, 1,   2, 3, 4, 4, 3, 5,
        1, 7, 3, 3, 7, 5,   6, 0, 4, 4, 0, 2,
        4, 5, 6, 6, 5, 7,   0, 1, 2, 2, 1, 3
    };

    for (int i = 0; i < 36; i++) {
        glm::vec3 vertex = cubeVertices[cubeIndices[i]];
        glm::vec3 color  = cubeColors[cubeIndices[i]];

        glm::vec3 normal = { .0, .0, .0 };
        int side = i / 6;
        int axis = side / 2;
        normal[axis] = side % 2 * 2.f - 1.f;

        glm::vec2 texture = { .0, .0 };
        if (axis == 0) texture.x = vertex.y > 0;
        else           texture.x = vertex.x > 0;
        if (axis == 2) texture.y = vertex.y > 0;
        else           texture.y = vertex.z < 0;

        m_positions.emplace_back(vertex);
        m_normals  .emplace_back(normal);
        m_colors   .emplace_back(color);
        m_texCoords.emplace_back(texture);
    }
    
    m_indices = {
        0 ,1 ,2 ,3 ,4 ,5 ,   6 ,7 ,8 ,9 ,10,11,
        12,13,14,15,16,17,   18,19,20,21,22,23,
        24,25,26,27,28,29,   30,31,32,33,34,35
    };
}

void Mesh::cmdCreateVertexBuffer() {
    VkDeviceSize bufferSize = sizeofPositions() + sizeofNormals() + sizeofColors();
    
    Buffer* vertexBuffer = new Buffer( m_device, m_physicalDevice );
    vertexBuffer->setup(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
    vertexBuffer->create();
    
    int32_t shift = 0;
    for (int i = 0; i < m_positions.size(); i++) {
        vertexBuffer->fillBuffer(&m_positions[i], sizeofPosition, shift);
        shift += sizeofPosition;
        vertexBuffer->fillBuffer(&m_normals  [i], sizeofNormal  , shift);
        shift += sizeofNormal;
        vertexBuffer->fillBuffer(&m_colors   [i], sizeofColor   , shift);
        shift += sizeofColor;
    }
    
    m_vertexBuffer = vertexBuffer;
}

void Mesh::cmdCreateIndexBuffer() {
    VkDeviceSize bufferSize = sizeofIndices();
    
    Buffer* indexBuffer = new Buffer( m_device, m_physicalDevice );
    indexBuffer->setup(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
    indexBuffer->create();
    indexBuffer->fillBufferFull(m_indices.data());
    
    m_indexBuffer = indexBuffer;
}

VkPipelineVertexInputStateCreateInfo* Mesh::createVertexInputInfo() {
    int32_t stride = sizeofPosition + sizeofNormal + sizeofColor;
    
    bindingDescription.binding = 0;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    attributeDescriptions.resize(3);
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = sizeofPosition;
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = sizeofPosition + sizeofNormal;
    
    stateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    stateCreateInfo.vertexBindingDescriptionCount = 1;
    stateCreateInfo.vertexAttributeDescriptionCount = UINT32(attributeDescriptions.size());
    stateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    stateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    return &stateCreateInfo;
}

void Mesh::scale(glm::vec3 size)               { m_model = glm::scale(m_model, size); }
void Mesh::rotate(float angle, glm::vec3 axis) { m_model = glm::rotate(m_model, glm::radians(angle), axis); }
void Mesh::translate(glm::vec3 translation)    { m_model = glm::translate(m_model, translation); }
glm::mat4 Mesh::getMatrix() { return m_model; }

int32_t Mesh::sizeofPositions() { return sizeofPosition * (int32_t) m_positions.size(); }
int32_t Mesh::sizeofNormals  () { return sizeofNormal   * (int32_t) m_normals.size(); }
int32_t Mesh::sizeofColors   () { return sizeofColor    * (int32_t) m_colors.size(); }
int32_t Mesh::sizeofTexCoords() { return sizeofTexCoord * (int32_t) m_texCoords.size(); }
int32_t Mesh::sizeofIndices  () { return sizeofIndex    * (int32_t) m_indices.size(); }
