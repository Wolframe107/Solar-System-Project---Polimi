// SolarSimulator.cpp
// This has been adapted from Starter.hpp, the Vulkan tutorial and the lecures to be modified for a solar system simulation

#include <json.hpp>
#include <fstream>
#include <math.h>
#include "Starter.hpp"
#define _USE_MATH_DEFINES

using json = nlohmann::json;

// The uniform buffer objects data structure
struct UniformBlock {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 lightPos;
};

// Uniform buffer for Skybox
struct skyBoxUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
};

// The vertex data structure for planets and other objects
struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 normal;
};

// The vertex data structure for skybox
struct SkyboxVertex {
    glm::vec3 pos;
};

class SolarSimulator : public BaseProject {
protected:
    float speedMultiplier = 0.75f;
    const float speedStep = 0.05f;
    const float minSpeed = 0.1f;
    const float maxSpeed = 3.0f;
    float accumulatedTime = 0.0f;

    // Current aspect ratio
    float Ar;

    // Descriptor Layouts
    DescriptorSetLayout DSL;
    DescriptorSetLayout DSLskyBox;

    // Vertex formats
    VertexDescriptor VD;
    VertexDescriptor skyboxVD;

    // Pipelines
    Pipeline P, sunP, skyboxP;

    // Solar system objects
    static const int NUM_PLANETS = 8;  // Mercury to Neptune
    Model<Vertex> sun;
    Model<Vertex> planets[NUM_PLANETS];
    Model<Vertex> moon;
    Model<Vertex> saturnRing;
    Model<SkyboxVertex> skybox;
    DescriptorSet sunDS;
    DescriptorSet planetDS[NUM_PLANETS];
    DescriptorSet moonDS;
    DescriptorSet saturnRingDS;
    DescriptorSet skyboxDS;
    Texture sunTexture;
    Texture planetTextures[NUM_PLANETS];
    Texture moonTexture;
    Texture saturnRingTexture;
    Texture skyboxTexture;

    // C++ storage for uniform variables
    UniformBlock sunUBO;
    UniformBlock planetUBO[NUM_PLANETS];
    UniformBlock moonUBO;
    UniformBlock saturnRingUBO;
    skyBoxUniformBufferObject skyboxUBO;

    // Planet properties
    struct PlanetProperties {
        float orbitRadius;
        float revolutionSpeed;
        float rotationSpeed;
        float eclipticInclination;
        float axialTilt;
        glm::vec3 scale;
    };
    PlanetProperties planetProps[NUM_PLANETS];

    // Moon properties
    struct MoonProperties {
        float orbitRadius;
        float revolutionSpeed;
        float rotationSpeed;
        glm::vec3 scale;
    };
    MoonProperties moonProps;

    // Sun scale
    glm::vec3 sunScale;

    // Other application parameters
    float vel = 0.0f; // Ship velocity
    float x_rot = 0.0f; //Rotations for the camera
    float y_rot = 0.0f;
    float z_rot = 0.0f;

    // First person view matrices
    glm::vec3 initPos = glm::vec3(0.0f, 10.0f, 100.0f);
    glm::mat4 ViewMatrix = glm::translate(glm::mat4(1.0f), -initPos);
    glm::mat4 View;

    // JSON data
    json solarSystemData;

    void setWindowParameters() {
        windowWidth = 1600;
        windowHeight = 900;
        windowTitle = "Solar System Simulation";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = { 0.0f, 0.0f, 0.02f, 1.0f };

        uniformBlocksInPool = NUM_PLANETS + 4;  // +4 for sun, moon, ring, and skybox
        texturesInPool = NUM_PLANETS + 4;
        setsInPool = NUM_PLANETS + 4;

        Ar = (float)windowWidth / (float)windowHeight;
    }

    // Changes aspect ratio when window size changes
    void onWindowResize(int w, int h) {
        Ar = (float)w / (float)h;
    }

    // Loads planetery data
    void loadSolarSystemData() {
        std::ifstream file("solarSystemData.json");
        file >> solarSystemData;
    }

    void localInit() {
        // Descriptor Layouts
        DSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });

        DSLskyBox.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
            });

        // Vertex descriptors
        VD.init(this, {
            {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                    sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                    sizeof(glm::vec2), UV},
                {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                    sizeof(glm::vec3), NORMAL}
            });

        // Skybox vertex descriptor
        skyboxVD.init(this, {
            {0, sizeof(SkyboxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkyboxVertex, pos),
                    sizeof(glm::vec3), POSITION}
            });

        // Pipelines
        P.init(this, &VD, "shaders/SolarSystemVert.spv", "shaders/SolarSystemFrag.spv", { &DSL });
        sunP.init(this, &VD, "shaders/SunVert.spv", "shaders/SunFrag.spv", { &DSL });
        skyboxP.init(this, &skyboxVD, "shaders/SkyboxVert.spv", "shaders/SkyboxFrag.spv", { &DSLskyBox });
        skyboxP.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_BACK_BIT, false);

        loadSolarSystemData();

        // Load sun model and texture
        sun.init(this, &VD, "Models/Sphere.gltf", GLTF);
        if (sun.vertices.empty() || sun.indices.empty()) {
            throw std::runtime_error("Failed to load sun model");
        }
        sunTexture.init(this, "textures/Sun.jpg");

        // Load planet models and textures
        std::string planetNames[] = { "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune" };
        for (int i = 0; i < NUM_PLANETS; i++) {
            planets[i].init(this, &VD, "Models/Sphere.gltf", GLTF);
            if (planets[i].vertices.empty() || planets[i].indices.empty()) {
                throw std::runtime_error("Failed to load planet model: " + planetNames[i]);
            }
            planetTextures[i].init(this, ("textures/" + planetNames[i] + ".jpg").c_str());
        }

        // Load moon model and texture
        moon.init(this, &VD, "Models/Sphere.gltf", GLTF);
        if (moon.vertices.empty() || moon.indices.empty()) {
            throw std::runtime_error("Failed to load moon model");
        }
        moonTexture.init(this, "textures/Moon.jpg");

        // Load saturn ring model and texture
        saturnRing.init(this, &VD, "Models/saturnRing.obj", OBJ);
        if (saturnRing.vertices.empty() || saturnRing.indices.empty()) {
            throw std::runtime_error("Failed to load ring model");
        }
        saturnRingTexture.init(this, "textures/ringAlpha.png");

        // Load skybox model and texture
        skybox.init(this, &skyboxVD, "Models/SkyBoxCube.obj", OBJ);
        if (skybox.vertices.empty() || skybox.indices.empty()) {
            throw std::runtime_error("Failed to load skybox model");
        }
        skyboxTexture.init(this, "Textures/Skybox.jpg");

        // Set planet properties based on JSON data
        for (int i = 0; i < NUM_PLANETS; i++) {
            const auto& planetData = solarSystemData[planetNames[i]];
            planetProps[i].orbitRadius = planetData["distance_from_sun"];
            planetProps[i].revolutionSpeed = 1.0f / planetData["revolution_period"].get<float>();
            planetProps[i].rotationSpeed = 1.0f / planetData["rotation_period"].get<float>();
            planetProps[i].eclipticInclination = glm::radians(planetData["ecliptic_inclination"].get<float>());
            planetProps[i].axialTilt = glm::radians(planetData["axial_tilt"].get<float>());
            planetProps[i].scale = glm::vec3(planetData["radius"].get<float>());
        }

        // Set moon properties
        const auto& moonData = solarSystemData["Moon"];
        moonProps.orbitRadius = moonData["distance_from_planet"];
        moonProps.revolutionSpeed = 1.0f / moonData["revolution_period"].get<float>();
        moonProps.rotationSpeed = 1.0f / moonData["rotation_period"].get<float>();
        moonProps.scale = glm::vec3(moonData["radius"].get<float>());

        // Set sun scale
        sunScale = glm::vec3(solarSystemData["Sun"]["radius"].get<float>());
    }

    void pipelinesAndDescriptorSetsInit() {
        P.create();
        sunP.create();
        skyboxP.create();

        // Create descriptor sets for sun, planets, moon, ship, and skybox
        sunDS.init(this, &DSL, {
            {0, UNIFORM, sizeof(UniformBlock), nullptr},
            {1, TEXTURE, 0, &sunTexture}
            });

        // Create descriptor sets for planets
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetDS[i].init(this, &DSL, {
                {0, UNIFORM, sizeof(UniformBlock), nullptr},
                {1, TEXTURE, 0, &planetTextures[i]}
                });
        }

        // Create descriptor set for moon
        moonDS.init(this, &DSL, {
            {0, UNIFORM, sizeof(UniformBlock), nullptr},
            {1, TEXTURE, 0, &moonTexture}
            });

        // Create descriptor set for Saturns ring
        saturnRingDS.init(this, &DSL, {
            {0, UNIFORM, sizeof(UniformBlock), nullptr},
            {1, TEXTURE, 0, &saturnRingTexture}
            });

        // Create descriptor set for skyBox
        skyboxDS.init(this, &DSLskyBox, {
            {0, UNIFORM, sizeof(skyBoxUniformBufferObject), nullptr},
            {1, TEXTURE, 0, &skyboxTexture}
            });
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        sunP.cleanup();
        skyboxP.cleanup();
        sunDS.cleanup();
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetDS[i].cleanup();
        }
        moonDS.cleanup();
        saturnRingDS.cleanup();
        skyboxDS.cleanup();
    }

    void localCleanup() {
        sunTexture.cleanup();
        sun.cleanup();
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetTextures[i].cleanup();
            planets[i].cleanup();
        }
        moonTexture.cleanup();
        moon.cleanup();
        saturnRingTexture.cleanup();
        saturnRing.cleanup();
        skyboxTexture.cleanup();
        skybox.cleanup();
        DSL.cleanup();
        DSLskyBox.cleanup();
        P.destroy();
        sunP.destroy();
        skyboxP.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        // Draw skybox first
        skyboxP.bind(commandBuffer);
        skybox.bind(commandBuffer);
        skyboxDS.bind(commandBuffer, skyboxP, 0, currentImage);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(skybox.indices.size()), 1, 0, 0, 0);

        // Draw sun
        if (!sun.vertices.empty() && !sun.indices.empty()) {
            sunP.bind(commandBuffer);
            sunDS.bind(commandBuffer, sunP, 0, currentImage);
            sun.bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sun.indices.size()), 1, 0, 0, 0);
        }

        // Draw planets, moon
        P.bind(commandBuffer);

        // Draw planets
        for (int i = 0; i < NUM_PLANETS; i++) {
            if (!planets[i].vertices.empty() && !planets[i].indices.empty()) {
                planetDS[i].bind(commandBuffer, P, 0, currentImage);
                planets[i].bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(planets[i].indices.size()), 1, 0, 0, 0);
            }
        }

        // Draw moon
        if (!moon.vertices.empty() && !moon.indices.empty()) {
            moonDS.bind(commandBuffer, P, 0, currentImage);
            moon.bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(moon.indices.size()), 1, 0, 0, 0);
        }

        // Draw saturn ring
        if (!saturnRing.vertices.empty() && !saturnRing.indices.empty()) {
            saturnRingDS.bind(commandBuffer, P, 0, currentImage);
            saturnRing.bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(saturnRing.indices.size()), 1, 0, 0, 0);
        }

    }

    void updateUniformBuffer(uint32_t currentImage) {
        static bool debounce = false;
        static int curDebounce = 0;

        // Integration with the timers and the controllers
        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);

        // Some movement parameters
        const float velMin = -5.0;
        const float velMax = 5.0;
        const float acc = 0.05f;

        // Update velocity
        if (m.z != 0 && vel >= velMin && vel <= velMax) {
            vel += m.z * acc;
            if (vel > velMax) vel = velMax;
            if (vel < velMin) vel = velMin;
        }

        // Set vel to 0 when around margin        
        if (vel > -0.25f && vel < 0.25f) {
            if (vel < 0.0) vel += 0.001f;
            if (vel > 0.0) vel -= 0.001f;
        }

        // Update rotation
        const float rotAmount = 0.01;

        // X-axis rotation
        if (r.x != 0 && x_rot > -1.0 && x_rot < 1.0) {
            x_rot += rotAmount * r.x;
        }
        else if (x_rot > 0.0) {
            x_rot -= rotAmount;
        }
        else if (x_rot < 0.0) {
            x_rot += rotAmount;
        }

        if (x_rot > -0.001f && x_rot < 0.001f) {
            x_rot = 0.0f;
        }

        // Y-axis rotation
        if (r.y != 0 && y_rot > -1.0 && y_rot < 1.0) {
            y_rot += rotAmount * r.y;
        }
        else if (y_rot > 0.0) {
            y_rot -= rotAmount;
        }
        else if (y_rot < 0.0) {
            y_rot += rotAmount;
        }

        if (y_rot > -0.001f && y_rot < 0.001f) {
            y_rot = 0.0f;
        }

        // Z-axis rotation
        if (r.z != 0 && z_rot > -1.0 && z_rot < 1.0) {
            z_rot += rotAmount * r.z;
        }
        else if (z_rot > 0.0) {
            z_rot -= rotAmount;
        }
        else if (z_rot < 0.0) {
            z_rot += rotAmount;
        }

        if (z_rot > -0.001f && z_rot < 0.001f) {
            z_rot = 0.0f;
        }

        // Create rotation matrix for all axes
        float rotation_speed = 1.0f;
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotation_speed * x_rot * deltaT, glm::vec3(1, 0, 0))
            * glm::rotate(glm::mat4(1.0f), rotation_speed * y_rot * deltaT, glm::vec3(0, 1, 0))
            * glm::rotate(glm::mat4(1.0f), -rotation_speed * z_rot * deltaT, glm::vec3(0, 0, 1));

        // Update perspective projection
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 500.0f;
        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;

        // Update ViewmMtrix based on movement
        ViewMatrix = rotationMatrix * ViewMatrix;
        ViewMatrix = glm::translate(glm::mat4(1), glm::vec3(0, 0, vel * deltaT)) * ViewMatrix;
        View = ViewMatrix;

        // Debugging Print key - P
        if (glfwGetKey(window, GLFW_KEY_P)) {
            if (!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_P;

                std::cout << "xrot: " << x_rot << std::endl;
                std::cout << "yrot: " << y_rot << std::endl;
                std::cout << "zrot: " << z_rot << std::endl;

                std::cout << "SpeedMultiplier: " << speedMultiplier << std::endl;

                printMat4("View  ", View);
                printMat4("rotation  ", rotationMatrix);
            }
        }
        else {
            if ((curDebounce == GLFW_KEY_P) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
        }

        // Reset Position - I
        if (glfwGetKey(window, GLFW_KEY_I)) {
            if (!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_I;

                vel = 0.0f;
                ViewMatrix = glm::translate(glm::mat4(1.0f), -initPos);
            }
        }
        else {
            if ((curDebounce == GLFW_KEY_I) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
        }

        // Handle speed changes
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {  // 'M' key (More speed)
            speedMultiplier = glm::min(speedMultiplier + speedStep, maxSpeed);
        }
        else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {  // 'N' key (Nless speed)
            speedMultiplier = glm::max(speedMultiplier - speedStep, minSpeed);
        }

        // Close game with ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Update accumulated time
        accumulatedTime += deltaT * speedMultiplier;

        // Light position (at the sun's position)
        glm::vec3 lightPos = glm::vec3(0, 0, 0);

        // Update sun uniform buffer
        glm::mat4 sunWorld = glm::scale(glm::mat4(1.0f), sunScale);
        sunUBO.model = sunWorld;
        sunUBO.view = View;
        sunUBO.proj = Prj;
        sunUBO.lightPos = lightPos;
        sunDS.map(currentImage, &sunUBO, sizeof(sunUBO), 0);

        // Update planet uniform buffers
        for (int i = 0; i < NUM_PLANETS; i++) {
            // Calculate planet position
            float angle = accumulatedTime * planetProps[i].revolutionSpeed;
            glm::vec3 position(
                cos(angle) * planetProps[i].orbitRadius,
                sin(planetProps[i].eclipticInclination) * planetProps[i].orbitRadius * sin(angle),
                sin(angle) * planetProps[i].orbitRadius * cos(planetProps[i].eclipticInclination)
            );

            // Calculate planet rotation
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), planetProps[i].axialTilt, glm::vec3(0, 0, 1)) *
                glm::rotate(glm::mat4(1.0f), accumulatedTime * planetProps[i].rotationSpeed, glm::vec3(0, 1, 0));

            // Create world matrix
            glm::mat4 World = glm::translate(glm::mat4(1.0f), position) *
                rotation *
                glm::scale(glm::mat4(1.0f), planetProps[i].scale);

            // Update uniform buffer
            planetUBO[i].model = World;
            planetUBO[i].view = View;
            planetUBO[i].proj = Prj;
            planetUBO[i].lightPos = lightPos;
            planetDS[i].map(currentImage, &planetUBO[i], sizeof(planetUBO[i]), 0);
        }

        // Update moon uniform buffer
        int earthIndex = 2; // Assuming Earth is the third planet (index 2) in our array
        float earthAngle = accumulatedTime * planetProps[earthIndex].revolutionSpeed;
        glm::vec3 earthPosition(
            cos(earthAngle) * planetProps[earthIndex].orbitRadius,
            sin(planetProps[earthIndex].eclipticInclination) * planetProps[earthIndex].orbitRadius * sin(earthAngle),
            sin(earthAngle) * planetProps[earthIndex].orbitRadius * cos(planetProps[earthIndex].eclipticInclination)
        );

        float moonAngle = accumulatedTime * moonProps.revolutionSpeed;
        glm::vec3 moonRelativePosition(
            cos(moonAngle) * moonProps.orbitRadius,
            sin(moonAngle) * moonProps.orbitRadius,
            0
        );

        glm::vec3 moonPosition = earthPosition + moonRelativePosition;

        glm::mat4 moonRotation = glm::rotate(glm::mat4(1.0f), accumulatedTime * moonProps.rotationSpeed, glm::vec3(0, 1, 0));

        glm::mat4 moonWorld = glm::translate(glm::mat4(1.0f), moonPosition) *
            moonRotation *
            glm::scale(glm::mat4(1.0f), moonProps.scale);

        moonUBO.model = moonWorld;
        moonUBO.view = View;
        moonUBO.proj = Prj;
        moonUBO.lightPos = lightPos;
        moonDS.map(currentImage, &moonUBO, sizeof(moonUBO), 0);

        // Update Saturn Ring uniform buffer
        int saturnIndex = 5; // Assuming Saturn is the sixth planet (index 5) in our array
        float saturnAngle = accumulatedTime * planetProps[saturnIndex].revolutionSpeed;
        glm::vec3 saturnPosition(
            cos(saturnAngle) * planetProps[saturnIndex].orbitRadius,
            sin(planetProps[saturnIndex].eclipticInclination) * planetProps[saturnIndex].orbitRadius * sin(saturnAngle),
            sin(saturnAngle) * planetProps[saturnIndex].orbitRadius * cos(planetProps[saturnIndex].eclipticInclination)
        );

        glm::mat4 ringRotation = glm::rotate(glm::mat4(1.0f), 59.6f, glm::vec3(0, 1, 0));

        glm::mat4 saturnRingWorld = glm::translate(glm::mat4(1.0f), saturnPosition) *
            ringRotation *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.18f));

        saturnRingUBO.model = saturnRingWorld;
        saturnRingUBO.view = View;
        saturnRingUBO.proj = Prj;
        saturnRingUBO.lightPos = lightPos;
        saturnRingDS.map(currentImage, &saturnRingUBO, sizeof(saturnRingUBO), 0);

        // Update skybox uniform buffer
        glm::mat4 skyboxModel = glm::scale(glm::mat4(1.0f), glm::vec3(farPlane / 2.0f));
        skyboxUBO.mvpMat = Prj * glm::mat4(glm::mat3(View)) * skyboxModel; // Remove translation and scale
        skyboxDS.map(currentImage, &skyboxUBO, sizeof(skyboxUBO), 0);

        // Display speed indicator (you can replace this with on-screen rendering later)
        static float lastPrintTime = 0.0f;
        if (accumulatedTime - lastPrintTime > 0.1f) {  // Update every tenth second
            int speedPercentage = static_cast<int>((speedMultiplier / maxSpeed) * 100);
            std::cout << "\rSpeed: " << speedPercentage << "% " << std::string(speedPercentage / 2, '|') << std::flush;
            lastPrintTime = accumulatedTime;
        }
    }
};

// Main function
int main() {
    SolarSimulator app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}