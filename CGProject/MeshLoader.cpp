// This has been adapted from the Vulkan tutorial and modified for a solar system simulation

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
    DescriptorSet sunDS;
    DescriptorSet planetDS[NUM_PLANETS];
    Texture sunTexture;
    Texture planetTextures[NUM_PLANETS];

    // C++ storage for uniform variables
    UniformBlock sunUBO;
    UniformBlock planetUBO[NUM_PLANETS];

    // Planet properties
    struct PlanetProperties {
        float orbitRadius;
        float rotationSpeed;
        float orbitSpeed;
        glm::vec3 scale;
    };
    PlanetProperties planetProps[NUM_PLANETS];

    // Other application parameters
	glm::vec3 CamPos = glm::vec3(0, 10, 100); // Camera start position
    glm::vec3 CamTarget = glm::vec3(0, 0, 0); // Camera target (sun)
    glm::vec3 CamUp = glm::vec3(0, 1, 0);     // Camera up direction

    glm::mat4 ViewMatrix = glm::lookAt(CamPos, CamTarget, CamUp);

    float acc = 0.0f;
    float vel = 0.0f;
    float x_rot = 0.0f;
    float y_rot = 0.0f;
    float z_rot = 0.0f;
    const float rotation_speed = 1.0f;
	
    float time = 0.0f;

    void setWindowParameters() {
        windowWidth = 1600;
        windowHeight = 900;
        windowTitle = "Solar System Simulation";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = { 0.0f, 0.0f, 0.02f, 1.0f };

        uniformBlocksInPool = NUM_PLANETS + 1;
        texturesInPool = NUM_PLANETS + 1;
        setsInPool = NUM_PLANETS + 1;

        Ar = (float)windowWidth / (float)windowHeight;
    }

    void onWindowResize(int w, int h) {
        Ar = (float)w / (float)h;
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

        // Load sun model and texture
        sun.init(this, &VD, "Models/Sphere.gltf", GLTF);
        sunTexture.init(this, "textures/Sun.jpg");

        // Load planet models and textures
        std::string planetNames[] = { "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune" };
        for (int i = 0; i < NUM_PLANETS; i++) {
            planets[i].init(this, &VD, "Models/Sphere.gltf", GLTF);
            planetTextures[i].init(this, ("textures/" + planetNames[i] + ".jpg").c_str());
        }

        // Set planet properties
        float baseOrbitRadius = 5.0f;
        float baseOrbitSpeed = 1.0f;
        float baseRotationSpeed = 1.0f;
        float baseScale = 0.3f;

        for (int i = 0; i < NUM_PLANETS; i++) {
            planetProps[i].orbitRadius = baseOrbitRadius * (i + 1);
            planetProps[i].rotationSpeed = baseRotationSpeed / sqrt(i + 1);
            planetProps[i].orbitSpeed = baseOrbitSpeed / pow(i + 1, 1.5);
            planetProps[i].scale = glm::vec3(baseScale * (0.5f + i * 0.2f));
        }
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
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        sunDS.cleanup();
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetDS[i].cleanup();
        }
    }

    void localCleanup() {
        sunTexture.cleanup();
        sun.cleanup();
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetTextures[i].cleanup();
            planets[i].cleanup();
        }
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
        const float accAmount = 0.0005f;
        const float accMin = -0.01f;
        const float accMax = 0.01f;
        const float decAmount = 0.0002f; // Deceleration amount

        if (m.z != 0) {
            acc += accAmount * m.z;
            if (acc > accMax) acc = accMax;
            if (acc < accMin) acc = accMin;
        }
        else {
            // Decelerate to zero if no key is pressed
            if (acc > 0) {
                acc -= decAmount;
                if (acc < 0) acc = 0;
            }
            else if (acc < 0) {
                acc += decAmount;
                if (acc > 0) acc = 0;
            }
        }
        //std::cout << "acc: " << acc << std::endl;

        vel += acc;
        //std::cout << "Velocity: " << vel << std::endl;
        // 
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
        ViewMatrix = glm::rotate(glm::mat4(1), rotation_speed * x_rot * deltaT, glm::vec3(1, 0, 0)) * ViewMatrix;
        ViewMatrix = glm::rotate(glm::mat4(1), rotation_speed * y_rot * deltaT, glm::vec3(0, 1, 0)) * ViewMatrix;
        ViewMatrix = glm::rotate(glm::mat4(1), -rotation_speed * z_rot * deltaT, glm::vec3(0, 0, 1)) * ViewMatrix;
        ViewMatrix = glm::translate(glm::mat4(1), -glm::vec3(0, 0, -vel * deltaT)) * ViewMatrix;

        // Perspective + View
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 500.0f;

        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;

        glm::mat4 View = ViewMatrix;

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
        
        // ***----------------------------***
        
        // Update time
        time += deltaT;

        // Light position (at the sun's position)
        glm::vec3 lightPos = glm::vec3(0, 0, 0);

        // Update sun uniform buffer
        glm::mat4 sunWorld = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
        sunUBO.mvpMat = Prj * View * sunWorld;
        sunUBO.lightPos = lightPos;
        sunDS.map(currentImage, &sunUBO, sizeof(sunUBO), 0);

        // Update planet uniform buffers
        for (int i = 0; i < NUM_PLANETS; i++) {
            // Calculate planet position
            float angle = time * planetProps[i].orbitSpeed;
            glm::vec3 position(
                cos(angle) * planetProps[i].orbitRadius,
                0.0f,
                sin(angle) * planetProps[i].orbitRadius
            );

            // Calculate planet rotation
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), time * planetProps[i].rotationSpeed, glm::vec3(0, 1, 0));

            // Create world matrix
            glm::mat4 World = glm::translate(glm::mat4(1.0f), position) *
                rotation *
                glm::scale(glm::mat4(1.0f), planetProps[i].scale);

            // Update uniform buffer
            planetUBO[i].mvpMat = Prj * View * World;
            planetUBO[i].lightPos = lightPos;
            planetDS[i].map(currentImage, &planetUBO[i], sizeof(planetUBO[i]), 0);
        }
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