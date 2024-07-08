#pragma once
#include "Starter.hpp"
#include <json.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define the VertexMesh structure
struct VertexMesh {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct MeshUniformBlock {
    glm::mat4 mMat;
    glm::mat4 mvpMat;
    glm::mat4 nMat;  // Normal matrix
    glm::vec3 color;
};

struct PlanetData {
    std::string name;
    float distanceFromSun;
    float revolutionPeriod;
    float eclipticInclination;
    float rotationPeriod;
    float axialTilt;
    float radius;
};

class SolarSystem : public BaseProject {
protected:
    // Current aspect ratio
    float Ar;

    DescriptorSetLayout DSLPlanet, DSLSun, DSLSkybox;

    VertexDescriptor VMesh;

    Pipeline PPlanet, PSun, PSkybox;

    // Models, textures and Descriptors
    std::vector<Model<VertexMesh>> planetModels;
    Model<VertexMesh> sunModel, skyboxModel;
    std::vector<Texture> planetTextures;
    Texture sunTexture, skyboxTexture;
    std::vector<DescriptorSet> planetDS;
    DescriptorSet sunDS, skyboxDS;

    // Add these to the protected section
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float cameraSpeed;
    float mouseSensitivity;
    float pitch;
    float yaw;
    bool firstMouse;
    float lastX, lastY;
    float lastFrameTime;
    float deltaT;
    // Add these method declarations

    std::vector<PlanetData> planets;
    float simulationTime = 0.0f;
    float timeScale = 1.0f;

    void updateDeltaT();
    void setWindowParameters() override;
    void localInit() override;
    void pipelinesAndDescriptorSetsInit() override;
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) override;
    void updateUniformBuffer(uint32_t currentImage) override;
    void pipelinesAndDescriptorSetsCleanup() override;
    void localCleanup() override;
    void onWindowResize(int w, int h) override;
    void processInput();
    void mouseCallback(double xpos, double ypos);
    void setupWindow();
    static void mouseCallbackWrapper(GLFWwindow* window, double xpos, double ypos);

    void loadPlanetData();
    void createPlanetMesh(float radius, std::vector<VertexMesh>& vertices, std::vector<uint32_t>& indices);
    void createSkyboxMesh();
};