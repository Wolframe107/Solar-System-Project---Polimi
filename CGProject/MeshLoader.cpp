// MeshLoader.cpp
#include <json.hpp>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
// This has been adapted from the Vulkan tutorial and modified for a solar system simulation

using json = nlohmann::json;
#include "Starter.hpp"

// The uniform buffer objects data structure
struct UniformBlock {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::vec3 lightPos;
};

// The vertex data structure
struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 normal;
};

class MeshLoader : public BaseProject {
protected:
    // Current aspect ratio
    float Ar;

    // Descriptor Layouts
    DescriptorSetLayout DSL;

    // Vertex formats
    VertexDescriptor VD;

    // Pipelines
    Pipeline P;

    // Solar system objects
    static const int NUM_PLANETS = 8;  // Mercury to Neptune
    Model<Vertex> sun;
    Model<Vertex> planets[NUM_PLANETS];
    Model<Vertex> moon;
    Model<Vertex> ship;
    DescriptorSet sunDS;
    DescriptorSet planetDS[NUM_PLANETS];
    DescriptorSet moonDS;
    DescriptorSet shipDS;
    Texture sunTexture;
    Texture planetTextures[NUM_PLANETS];
    Texture moonTexture;
    Texture shipTexture;

    // C++ storage for uniform variables
    UniformBlock sunUBO;
    UniformBlock planetUBO[NUM_PLANETS];
    UniformBlock moonUBO;
    UniformBlock shipUBO;

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
    float viewMode = 0; // 0 for first person (Look-in), 1 for third person (Look-at)
    float x_rot = 0.0f; //Rotations for the camera
    float y_rot = 0.0f;
    float z_rot = 0.0f;
    glm::mat4 rotationMatrix;

    // First person
    glm::vec3 shipPos = glm::vec3(0, 30, 100);
    glm::mat4 ViewMatrix = glm::translate(glm::mat4(1.0f), -shipPos);

    // Third person
    glm::vec3 thirdPersonCamPos = shipPos + glm::vec3(0, 5, -10); // Camera behind the ship
    glm::vec3 thirdPersonCamTarget = shipPos;
    glm::vec3 thirdPersonCamUp = glm::vec3(0, 1, 0);

    glm::mat4 thirdPersonViewMatrix = glm::lookAt(thirdPersonCamPos, thirdPersonCamTarget, thirdPersonCamUp);

    glm::mat4 View; // The actual view that gets used

    float time = 0.0f;

    // JSON data
    json solarSystemData;

    void setWindowParameters() {
        windowWidth = 1600;
        windowHeight = 900;
        windowTitle = "Solar System Simulation";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = { 0.0f, 0.0f, 0.02f, 1.0f };

        uniformBlocksInPool = NUM_PLANETS + 2;  // +2 for sun and moon
        uniformBlocksInPool = NUM_PLANETS + 2;
        texturesInPool = NUM_PLANETS + 2;
        setsInPool = NUM_PLANETS + 2;

        Ar = (float)windowWidth / (float)windowHeight;
    }

    void onWindowResize(int w, int h) {
        Ar = (float)w / (float)h;
    }

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

        // Pipelines
        P.init(this, &VD, "shaders/SolarSystemVert.spv", "shaders/SolarSystemFrag.spv", { &DSL });

        loadSolarSystemData();

        // Load sun model and texture
        sun.init(this, &VD, "Models/Sphere.gltf", GLTF);
        sunTexture.init(this, "textures/Sun.jpg");

        // Load planet models and textures
        std::string planetNames[] = { "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune" };
        for (int i = 0; i < NUM_PLANETS; i++) {
            planets[i].init(this, &VD, "Models/Sphere.gltf", GLTF);
            planetTextures[i].init(this, ("textures/" + planetNames[i] + ".jpg").c_str());
        }

        // Load moon model and texture
        moon.init(this, &VD, "Models/Sphere.gltf", GLTF);
        moonTexture.init(this, "textures/Moon.jpg");
        // Load ship model and texture
        ship.init(this, &VD, "Models/cube.obj", OBJ);
        shipTexture.init(this, "textures/checker.png");

        // Set planet properties
        float baseOrbitRadius = 5.0f;
        float baseOrbitSpeed = 1.0f;
        float baseRotationSpeed = 1.0f;
        float baseScale = 0.3f;

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

        // Create descriptor set for sun
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
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        sunDS.cleanup();
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetDS[i].cleanup();
        }
        moonDS.cleanup();
        shipDS.cleanup();
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
        shipTexture.cleanup();
        ship.cleanup();
        DSL.cleanup();
        P.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        P.bind(commandBuffer);

        // Draw sun
        sunDS.bind(commandBuffer, P, 0, currentImage);
        sun.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sun.indices.size()), 1, 0, 0, 0);

        // Draw planets
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetDS[i].bind(commandBuffer, P, 0, currentImage);
            planets[i].bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(planets[i].indices.size()), 1, 0, 0, 0);
        }

        // Draw moon
        moonDS.bind(commandBuffer, P, 0, currentImage);
        moon.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(moon.indices.size()), 1, 0, 0, 0);
        // Draw ship
        shipDS.bind(commandBuffer, P, 0, currentImage);
        ship.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ship.indices.size()), 1, 0, 0, 0);

    }

    void updateUniformBuffer(uint32_t currentImage) {
        static bool debounce = false;
        static int curDebounce = 0;

        // ***----------------------------***
        // INPUT + VIEWMATRIX STUFF

        // Integration with the timers and the controllers
        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);

        // getSixAxis() is defined in Starter.hpp in the base class.
        // It fills the float point variable passed in its first parameter with the time
        // since the last call to the procedure.
        // It fills vec3 in the second parameters, with three values in the -1,1 range corresponding
        // to motion (with left stick of the gamepad, or ASWD + RF keys on the keyboard)
        // It fills vec3 in the third parameters, with three values in the -1,1 range corresponding
        // to motion (with right stick of the gamepad, or Arrow keys + QE keys on the keyboard, or mouse)
        // If fills the last boolean variable with true if fire has been pressed:
        //          SPACE on the keyboard, A or B button on the Gamepad, Right mouse button

        // Update acceleration
        const float velMin = -5.0;
        const float velMax = 5.0;
        const float acc = 0.05f;

        if (m.z != 0 && vel >= velMin && vel <= velMax) {
            vel += m.z * acc;
            if (vel > velMax) vel = velMax;
            if (vel < velMin) vel = velMin;
        }


        //std::cout << "Velocity: " << vel << std::endl;

        // Set vel to 0 when around margin        
        if (vel > -0.25f && vel < 0.25f) {
            if (vel < 0.0) vel += 0.001f;
            if (vel > 0.0) vel -= 0.001f;
        }
        
        // Update rotation
        const float rotAmount = 0.005;

        // X-axis rotation
        if (r.x != 0 && x_rot < 1.0) {
            x_rot += rotAmount * r.x;
        }
        else if (x_rot > 0.0) {
            x_rot -= rotAmount;
        }
        else if (x_rot < 0.0) {
            x_rot += rotAmount;
        }

        // Y-axis rotation
        if (r.y != 0 && y_rot < 1.0) {
            y_rot += rotAmount * r.y;
        }
        else if (y_rot > 0.0) {
            y_rot -= rotAmount;
        }
        else if (y_rot < 0.0) {
            y_rot += rotAmount;
        }

        // Z-axis rotation
        if (r.z != 0 && z_rot < 1.0) {
            z_rot += rotAmount * r.z;
        }
        else if (z_rot > 0.0) {
            z_rot -= rotAmount;
        }
        else if (z_rot < 0.0) {
            z_rot += rotAmount;
        }

        // Updating the View Matrix
        const float rotation_speed = 1.0f;
        shipPos -= glm::vec3(0, 0, -vel * deltaT);

        //rotationMatrix.x = glm::rotate(glm::mat4(1), rotation_speed * x_rot * deltaT, glm::vec3(1, 0, 0));
        //rotationMatrix.y = glm::rotate(glm::mat4(1), rotation_speed * y_rot * deltaT, glm::vec3(0, 1, 0));
        //rotationMatrix.z = glm::rotate(glm::mat4(1), rotation_speed * z_rot * deltaT, glm::vec3(0, 0, 1));
        

        if (viewMode == 0) {
            // First-person (Look-in)
            ViewMatrix = glm::rotate(glm::mat4(1), rotation_speed * x_rot * deltaT, glm::vec3(1, 0, 0)) * ViewMatrix;
            ViewMatrix = glm::rotate(glm::mat4(1), rotation_speed * y_rot * deltaT, glm::vec3(0, 1, 0)) * ViewMatrix;
            ViewMatrix = glm::rotate(glm::mat4(1), -rotation_speed * z_rot * deltaT, glm::vec3(0, 0, 1)) * ViewMatrix;
            ViewMatrix = glm::translate(glm::mat4(1), -glm::vec3(0, 0, -vel * deltaT)) * ViewMatrix;
            
            View = ViewMatrix;
        }
        else {
            // Third-person (Look-at)
            thirdPersonCamPos = shipPos + glm::vec3(0, 5, -10);
            thirdPersonCamTarget = shipPos;
            //float Roll = 0.0f;
            glm::mat4 thirdPersonViewMatrix = glm::lookAt(thirdPersonCamPos, thirdPersonCamTarget, glm::vec3(0, 1, 0));
            View = thirdPersonViewMatrix;
        }

        // Perspective + View
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 500.0f;

        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;

        
        // Debugging key - P
        if (glfwGetKey(window, GLFW_KEY_P)) {
            if (!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_P;

                printMat4("ViewMatrix  ", View);

            }
        }
        else {
            if ((curDebounce == GLFW_KEY_P) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
        }

        // Third person view - V
        if (glfwGetKey(window, GLFW_KEY_V)) {
            if (!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_V;

                printMat4("ViewMatrix  ", View);

            }
        }
        else {
            if ((curDebounce == GLFW_KEY_V) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
        }

        // Standard procedure to quit when the ESC key is pressed
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // ***----------------------------***

        // Update time
        time += deltaT;

        // Light position (at the sun's position)
        glm::vec3 lightPos = glm::vec3(0, 0, 0);

        // Update sun uniform buffer
        glm::mat4 sunWorld = glm::scale(glm::mat4(1.0f), sunScale);
        sunUBO.mvpMat = Prj * View * sunWorld;
        sunUBO.lightPos = lightPos;
        sunDS.map(currentImage, &sunUBO, sizeof(sunUBO), 0);

        // Update planet uniform buffers
        for (int i = 0; i < NUM_PLANETS; i++) {
            // Calculate planet position
            float angle = time * planetProps[i].revolutionSpeed;
            glm::vec3 position(
                cos(angle) * planetProps[i].orbitRadius,
                sin(planetProps[i].eclipticInclination) * planetProps[i].orbitRadius * sin(angle),
                sin(angle) * planetProps[i].orbitRadius * cos(planetProps[i].eclipticInclination)
            );

            // Calculate planet rotation
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), planetProps[i].axialTilt, glm::vec3(0, 0, 1)) *
                glm::rotate(glm::mat4(1.0f), time * planetProps[i].rotationSpeed, glm::vec3(0, 1, 0));

            // Create world matrix
            glm::mat4 World = glm::translate(glm::mat4(1.0f), position) *
                rotation *
                glm::scale(glm::mat4(1.0f), planetProps[i].scale);

            // Update uniform buffer
            planetUBO[i].mvpMat = Prj * View * World;
            planetUBO[i].lightPos = lightPos;
            planetDS[i].map(currentImage, &planetUBO[i], sizeof(planetUBO[i]), 0);
        }

        // Update moon uniform buffer
        int earthIndex = 2; // Assuming Earth is the third planet (index 2) in our array
        float earthAngle = time * planetProps[earthIndex].revolutionSpeed;
        glm::vec3 earthPosition(
            cos(earthAngle)* planetProps[earthIndex].orbitRadius,
            sin(planetProps[earthIndex].eclipticInclination)* planetProps[earthIndex].orbitRadius* sin(earthAngle),
            sin(earthAngle)* planetProps[earthIndex].orbitRadius* cos(planetProps[earthIndex].eclipticInclination)
        );

        float moonAngle = time * moonProps.revolutionSpeed;
        glm::vec3 moonRelativePosition(
            cos(moonAngle)* moonProps.orbitRadius,
            sin(moonAngle)* moonProps.orbitRadius,
            0
        );

        glm::vec3 moonPosition = earthPosition + moonRelativePosition;

        glm::mat4 moonRotation = glm::rotate(glm::mat4(1.0f), time * moonProps.rotationSpeed, glm::vec3(0, 1, 0));

        glm::mat4 moonWorld = glm::translate(glm::mat4(1.0f), moonPosition) *
            moonRotation *
            glm::scale(glm::mat4(1.0f), moonProps.scale);

        moonUBO.mvpMat = Prj * View * moonWorld;
        moonUBO.lightPos = lightPos;
        moonDS.map(currentImage, &moonUBO, sizeof(moonUBO), 0);

        // Debugging key V
        if (glfwGetKey(window, GLFW_KEY_V)) {
            if (!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_V;
                printMat4("ViewMatrix  ", View);
            }
        }
        else {
            if ((curDebounce == GLFW_KEY_V) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
        }

        // Standard procedure to quit when the ESC key is pressed
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        // Update ship uniform buffers
        glm::mat4 shipWorld = glm::translate(glm::mat4(1.0f), shipPos);
        shipWorld = glm::scale(shipWorld, glm::vec3(1.0f));
        shipUBO.mvpMat = Prj * View * sunWorld;
        shipUBO.lightPos = lightPos;
        shipDS.map(currentImage, &shipUBO, sizeof(shipUBO), 0);
    }
};

// Main function
int main() {
    MeshLoader app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}