#include "SolarSystem.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

void SolarSystem::setWindowParameters() {
    windowWidth = 1280;
    windowHeight = 720;
    windowTitle = "Solar System Simulation";
    initialBackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    Ar = static_cast<float>(windowWidth) / windowHeight;
}

void SolarSystem::setupWindow() {
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseCallbackWrapper);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void SolarSystem::localInit() {
    loadPlanetData();
    setupWindow();

    // Initialize camera
    cameraPos = glm::vec3(0.0f, 50.0f, 100.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraSpeed = 50.0f;
    mouseSensitivity = 0.1f;
    pitch = 0.0f;
    yaw = -90.0f;
    firstMouse = true;
    lastX = windowWidth / 2.0f;
    lastY = windowHeight / 2.0f;
    lastFrameTime = 0.0f;
    deltaT = 0.0f;

    // Create planet models and textures
    for (const auto& planet : planets) {
        Model<VertexMesh> planetModel;
        std::vector<VertexMesh> vertices;
        std::vector<uint32_t> indices;
        createPlanetMesh(planet.radius, vertices, indices);

        planetModel.initMesh(this, &VMesh);
        planetModel.vertices = vertices;
        planetModel.indices = indices;
        planetModel.createVertexBuffer();
        planetModel.createIndexBuffer();

        planetModels.push_back(planetModel);

        Texture planetTexture;
        planetTexture.init(this, ("textures/" + planet.name + ".png").c_str());
        planetTextures.push_back(planetTexture);
    }

    // Create sun model and texture
    std::vector<VertexMesh> sunVertices;
    std::vector<uint32_t> sunIndices;
    createPlanetMesh(planets[0].radius, sunVertices, sunIndices);
    sunModel.initMesh(this, &VMesh);
    sunModel.vertices = sunVertices;
    sunModel.indices = sunIndices;
    sunModel.createVertexBuffer();
    sunModel.createIndexBuffer();
    sunTexture.init(this, "textures/Sun.png");

    // Create skybox
    createSkyboxMesh();
    skyboxModel.createVertexBuffer();
    skyboxModel.createIndexBuffer();
    skyboxTexture.init(this, "textures/Skybox.png");
}

void SolarSystem::updateDeltaT() {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    deltaT = time - lastFrameTime;
    lastFrameTime = time;
}

void SolarSystem::loadPlanetData() {
    std::ifstream file("solarSystemData.json");
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open solarSystemData.json");
    }

    nlohmann::json data;
    file >> data;

    for (const auto& [name, planetData] : data.items()) {
        planets.push_back({
            name,
            planetData["distance_from_sun"],
            planetData["revolution_period"],
            planetData["ecliptic_inclination"],
            planetData["rotation_period"],
            planetData["axial_tilt"],
            planetData["radius"]
            });
    }
}

void SolarSystem::pipelinesAndDescriptorSetsInit() {
    // Planet pipeline
    DSLPlanet.init(this, {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

    PPlanet.init(this, &VMesh, "shaders/Planet.vert", "shaders/Planet.frag", { &DSLPlanet });
    PPlanet.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);

    // Sun pipeline
    DSLSun.init(this, {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

    PSun.init(this, &VMesh, "shaders/Sun.vert", "shaders/Sun.frag", { &DSLSun });
    PSun.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);

    // Skybox pipeline
    DSLSkybox.init(this, {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

    PSkybox.init(this, &VMesh, "shaders/Skybox.vert", "shaders/Skybox.frag", { &DSLSkybox });
    PSkybox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, false);

    // Create descriptor sets
    for (size_t i = 0; i < planets.size(); i++) {
        DescriptorSet ds;
        ds.init(this, &DSLPlanet, {
            {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
            {1, TEXTURE, 0, &planetTextures[i]}
            });
        planetDS.push_back(ds);
    }

    sunDS.init(this, &DSLSun, {
        {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
        {1, TEXTURE, 0, &sunTexture}
        });

    skyboxDS.init(this, &DSLSkybox, {
        {0, UNIFORM, sizeof(MeshUniformBlock), nullptr},
        {1, TEXTURE, 0, &skyboxTexture}
        });
}

void SolarSystem::populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
    // Bind skybox pipeline and draw
    PSkybox.bind(commandBuffer);
    skyboxDS.bind(commandBuffer, PSkybox, 0, currentImage);
    skyboxModel.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(skyboxModel.indices.size()), 1, 0, 0, 0);

    // Bind sun pipeline and draw
    PSun.bind(commandBuffer);
    sunDS.bind(commandBuffer, PSun, 0, currentImage);
    sunModel.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sunModel.indices.size()), 1, 0, 0, 0);

    // Bind planet pipeline and draw planets
    PPlanet.bind(commandBuffer);
    for (size_t i = 0; i < planets.size(); i++) {
        planetDS[i].bind(commandBuffer, PPlanet, 0, currentImage);
        planetModels[i].bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(planetModels[i].indices.size()), 1, 0, 0, 0);
    }
}

void SolarSystem::updateUniformBuffer(uint32_t currentImage) {
    updateDeltaT();
    simulationTime += deltaT * timeScale;

    // Camera/View transformation
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // Projection matrix
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 5000.0f);
    proj[1][1] *= -1; // Flip Y-coordinate for Vulkan

    // Update skybox
    MeshUniformBlock skyboxUBO{};
    skyboxUBO.mMat = glm::mat4(1.0f);
    skyboxUBO.mvpMat = proj * glm::mat4(glm::mat3(view)); // Remove translation from view matrix
    skyboxDS.map(currentImage, &skyboxUBO, sizeof(skyboxUBO), 0);

    // Update sun
    MeshUniformBlock sunUBO{};
    sunUBO.mMat = glm::mat4(1.0f);
    sunUBO.mvpMat = proj * view * sunUBO.mMat;
    sunDS.map(currentImage, &sunUBO, sizeof(sunUBO), 0);

    // Update planets
    for (size_t i = 0; i < planets.size(); i++) {
        const auto& planet = planets[i];
        float angle = glm::radians(360.0f * (simulationTime / planet.revolutionPeriod));
        float distance = planet.distanceFromSun;

        glm::mat4 model = glm::mat4(1.0f);

        // Ecliptic inclination
        model = glm::rotate(model, glm::radians(planet.eclipticInclination), glm::vec3(0.0f, 0.0f, 1.0f));

        // Orbital revolution
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        // Distance from sun
        model = glm::translate(model, glm::vec3(distance, 0.0f, 0.0f));

        // Axial tilt
        model = glm::rotate(model, glm::radians(planet.axialTilt), glm::vec3(0.0f, 0.0f, 1.0f));

        // Self-rotation
        model = glm::rotate(model, glm::radians(360.0f * (simulationTime / planet.rotationPeriod)), glm::vec3(0.0f, 1.0f, 0.0f));

        MeshUniformBlock ubo{};
        ubo.mMat = model;
        ubo.mvpMat = proj * view * model;

        // Additional uniform data if needed
        ubo.nMat = glm::transpose(glm::inverse(model)); // Normal matrix for lighting calculations
        ubo.color = glm::vec3(1.0f); // Default color, modify if needed

        planetDS[i].map(currentImage, &ubo, sizeof(ubo), 0);
    }
}

void SolarSystem::createPlanetMesh(float radius, std::vector<VertexMesh>& vertices, std::vector<uint32_t>& indices) {
    const int segments = 64;
    const int rings = 32;

    for (int y = 0; y <= rings; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)rings;
            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            VertexMesh vertex;
            vertex.pos = { xPos * radius, yPos * radius, zPos * radius };
            vertex.normal = { xPos, yPos, zPos };
            vertex.texCoord = { xSegment, ySegment };
            vertices.push_back(vertex);
        }
    }

    for (int y = 0; y < rings; y++) {
        for (int x = 0; x < segments; x++) {
            indices.push_back((y + 1) * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x + 1);

            indices.push_back((y + 1) * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x + 1);
            indices.push_back((y + 1) * (segments + 1) + x + 1);
        }
    }
}

void SolarSystem::createSkyboxMesh() {
    std::vector<VertexMesh> vertices;
    std::vector<uint32_t> indices;

    float size = 1000.0f;  // Large size to encompass the entire scene
    vertices = {
        {{-size, -size, -size}, {0, 0, 0}, {0, 0}},
        {{size, -size, -size}, {0, 0, 0}, {1, 0}},
        {{size, size, -size}, {0, 0, 0}, {1, 1}},
        {{-size, size, -size}, {0, 0, 0}, {0, 1}},
        {{-size, -size, size}, {0, 0, 0}, {0, 0}},
        {{size, -size, size}, {0, 0, 0}, {1, 0}},
        {{size, size, size}, {0, 0, 0}, {1, 1}},
        {{-size, size, size}, {0, 0, 0}, {0, 1}}
    };

    indices = {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        5, 4, 7, 7, 6, 5,
        4, 0, 3, 3, 7, 4,
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4
    };

    skyboxModel.initMesh(this, &VMesh);
    skyboxModel.vertices = vertices;
    skyboxModel.indices = indices;
}

void SolarSystem::pipelinesAndDescriptorSetsCleanup() {
    PPlanet.cleanup();
    PSun.cleanup();
    PSkybox.cleanup();

    DSLPlanet.cleanup();
    DSLSun.cleanup();
    DSLSkybox.cleanup();
}

void SolarSystem::localCleanup() {
    for (auto& model : planetModels) {
        model.cleanup();
    }
    for (auto& texture : planetTextures) {
        texture.cleanup();
    }
    sunModel.cleanup();
    sunTexture.cleanup();
    skyboxModel.cleanup();
    skyboxTexture.cleanup();
}

void SolarSystem::onWindowResize(int w, int h) {
    Ar = static_cast<float>(w) / h;
}

void SolarSystem::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = this->cameraSpeed * deltaT;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        timeScale *= 1.1f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        timeScale /= 1.1f;
}



void SolarSystem::mouseCallbackWrapper(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<SolarSystem*>(glfwGetWindowUserPointer(window));
    app->mouseCallback(xpos, ypos);
}

void SolarSystem::mouseCallback(double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
