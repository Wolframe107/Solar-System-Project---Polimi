<<<<<<< Updated upstream
// This has been adapted from the Vulkan tutorial

=======
// MeshLoader.cpp
>>>>>>> Stashed changes
#include "Starter.hpp"
#include <json.hpp>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>

using json = nlohmann::json;

// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)
// Example:
struct UniformBlock {
	alignas(16) glm::mat4 mvpMat;
};

// The vertices data structures
// Example
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
};





// MAIN ! 
class MeshLoader : public BaseProject {
	protected:

	// Current aspect ratio (used by the callback that resized the window
	float Ar;

	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSL;

	// Vertex formats
	VertexDescriptor VD;

<<<<<<< Updated upstream
	// Pipelines [Shader couples]
	Pipeline P;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	// Models
	Model<Vertex> M1, M2, M3, M4;
	// Descriptor sets
	DescriptorSet DS1, DS2, DS3, DS4;
	// Textures
	Texture T1, T2;
	
	// C++ storage for uniform variables
	UniformBlock ubo1, ubo2, ubo3, ubo4;

	// Other application parameters

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Mesh Loader";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 4;
		texturesInPool = 4;
		setsInPool = 4;
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}
	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSL.init(this, {
					// this array contains the bindings:
					// first  element : the binding number
					// second element : the type of element (buffer or texture)
					//                  using the corresponding Vulkan constant
					// third  element : the pipeline stage where it will be used
					//                  using the corresponding Vulkan constant
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				});

		// Vertex descriptors
		VD.init(this, {
				  // this array contains the bindings
				  // first  element : the binding number
				  // second element : the stride of this binging
				  // third  element : whether this parameter change per vertex or per instance
				  //                  using the corresponding Vulkan constant
				  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  // this array contains the location
				  // first  element : the binding number
				  // second element : the location number
				  // third  element : the offset of this element in the memory record
				  // fourth element : the data type of the element
				  //                  using the corresponding Vulkan constant
				  // fifth  elmenet : the size in byte of the element
				  // sixth  element : a constant defining the element usage
				  //                   POSITION - a vec3 with the position
				  //                   NORMAL   - a vec3 with the normal vector
				  //                   UV       - a vec2 with a UV coordinate
				  //                   COLOR    - a vec4 with a RGBA color
				  //                   TANGENT  - a vec4 with the tangent vector
				  //                   OTHER    - anything else
				  //
				  // ***************** DOUBLE CHECK ********************
				  //    That the Vertex data structure you use in the "offsetoff" and
				  //	in the "sizeof" in the previous array, refers to the correct one,
				  //	if you have more than one vertex format!
				  // ***************************************************
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
				         sizeof(glm::vec2), UV}
				});

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P.init(this, &VD, "shaders/ShaderVert.spv", "shaders/ShaderFrag.spv", {&DSL});
=======
    // Solar system objects
    static const int NUM_PLANETS = 8;  // Mercury to Neptune
    Model<Vertex> sun;
    Model<Vertex> planets[NUM_PLANETS];
    Model<Vertex> moon;
    Model<Vertex> saturnRing;
    DescriptorSet sunDS;
    DescriptorSet planetDS[NUM_PLANETS];
    DescriptorSet moonDS;
    DescriptorSet saturnRingDS;
    Texture sunTexture;
    Texture planetTextures[NUM_PLANETS];
    Texture moonTexture;
    Texture saturnRingTexture;

    // C++ storage for uniform variables
    UniformBlock sunUBO;
    UniformBlock planetUBO[NUM_PLANETS];
    UniformBlock moonUBO;
    UniformBlock saturnRingUBO;

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
    float time = 0.0f;

    // JSON data
    json solarSystemData;

    void setWindowParameters() {
        windowWidth = 1600;
        windowHeight = 900;
        windowTitle = "Solar System Simulation";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = { 0.0f, 0.0f, 0.02f, 1.0f };

        uniformBlocksInPool = NUM_PLANETS + 3;  // +3 for sun, moon, and Saturn's ring
        texturesInPool = NUM_PLANETS + 3;
        setsInPool = NUM_PLANETS + 3;
>>>>>>> Stashed changes

		// Models, textures and Descriptors (values assigned to the uniforms)

		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
		M1.init(this,   &VD, "Models/Cube.obj", OBJ);
		M2.init(this,   &VD, "Models/Sphere.gltf", GLTF);
		M3.init(this,   &VD, "Models/dish.005_Mesh.098.mgcg", MGCG);

<<<<<<< Updated upstream
		// Creates a mesh with direct enumeration of vertices and indices
		M4.vertices = {{{-6,-2,-6}, {0.0f,0.0f}}, {{-6,-2,6}, {0.0f,1.0f}},
					    {{6,-2,-6}, {1.0f,0.0f}}, {{ 6,-2,6}, {1.0f,1.0f}}};
		M4.indices = {0, 1, 2,    1, 3, 2};
		M4.initMesh(this, &VD);
		
		// Create the textures
		// The second parameter is the file name
		T1.init(this,   "textures/Checker.png");
		T2.init(this,   "textures/Textures_Food.png");
		
		// Init local variables
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		P.create();
=======
    void loadSolarSystemData() {
        std::ifstream file("solarSystemData.json");
        file >> solarSystemData;
    }

    void createRingModel() {
        std::vector<Vertex> ringVertices;
        std::vector<uint32_t> ringIndices;

        const int segments = 64;
        const float innerRadius = 1.2f;
        const float outerRadius = 2.0f;

        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float cosAngle = cos(angle);
            float sinAngle = sin(angle);

            // Inner vertex
            ringVertices.push_back({ {innerRadius * cosAngle, 0.0f, innerRadius * sinAngle},
                                    {(float)i / segments, 0.0f},
                                    {0.0f, 1.0f, 0.0f} });

            // Outer vertex
            ringVertices.push_back({ {outerRadius * cosAngle, 0.0f, outerRadius * sinAngle},
                                    {(float)i / segments, 1.0f},
                                    {0.0f, 1.0f, 0.0f} });

            if (i < segments) {
                int baseIndex = i * 2;
                ringIndices.push_back(baseIndex);
                ringIndices.push_back(baseIndex + 1);
                ringIndices.push_back(baseIndex + 2);
                ringIndices.push_back(baseIndex + 1);
                ringIndices.push_back(baseIndex + 3);
                ringIndices.push_back(baseIndex + 2);
            }
        }

        saturnRing.vertices = ringVertices;
        saturnRing.indices = ringIndices;
        saturnRing.initMesh(this, &VD);
    }

    void localInit() {
        // Descriptor Layouts
        DSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });
>>>>>>> Stashed changes

		// Here you define the data set
		DS1.init(this, &DSL, {
		// the second parameter, is a pointer to the Uniform Set Layout of this set
		// the last parameter is an array, with one element per binding of the set.
		// first  elmenet : the binding number
		// second element : UNIFORM or TEXTURE (an enum) depending on the type
		// third  element : only for UNIFORMs, the size of the corresponding C++ object. For texture, just put 0
		// fourth element : only for TEXTUREs, the pointer to the corresponding texture object. For uniforms, use nullptr
					{0, UNIFORM, sizeof(UniformBlock), nullptr},
					{1, TEXTURE, 0, &T1}
				});
		DS2.init(this, &DSL, {
					{0, UNIFORM, sizeof(UniformBlock), nullptr},
					{1, TEXTURE, 0, &T1}
				});
		DS3.init(this, &DSL, {
					{0, UNIFORM, sizeof(UniformBlock), nullptr},
					{1, TEXTURE, 0, &T2}
				});
		DS4.init(this, &DSL, {
					{0, UNIFORM, sizeof(UniformBlock), nullptr},
					{1, TEXTURE, 0, &T1}
				});
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		P.cleanup();

<<<<<<< Updated upstream
		// Cleanup datasets
		DS1.cleanup();
		DS2.cleanup();
		DS3.cleanup();
		DS4.cleanup();
	}
=======
        loadSolarSystemData();

        // Load sun model and texture
        sun.init(this, &VD, "Models/Sphere.gltf", GLTF);
        sunTexture.init(this, "textures/Sun.jpg");
>>>>>>> Stashed changes

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {
		// Cleanup textures
		T1.cleanup();
		T2.cleanup();
		
		// Cleanup models
		M1.cleanup();
		M2.cleanup();
		M3.cleanup();
		M4.cleanup();
		
		// Cleanup descriptor set layouts
		DSL.cleanup();
		
		// Destroies the pipelines
		P.destroy();		
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		// binds the pipeline
		P.bind(commandBuffer);
		// For a pipeline object, this command binds the corresponing pipeline to the command buffer passed in its parameter

<<<<<<< Updated upstream
		// binds the data set
		DS1.bind(commandBuffer, P, 0, currentImage);
		// For a Dataset object, this command binds the corresponing dataset
		// to the command buffer and pipeline passed in its first and second parameters.
		// The third parameter is the number of the set being bound
		// As described in the Vulkan tutorial, a different dataset is required for each image in the swap chain.
		// This is done automatically in file Starter.hpp, however the command here needs also the index
		// of the current image in the swap chain, passed in its last parameter
					
		// binds the model
		M1.bind(commandBuffer);
		// For a Model object, this command binds the corresponing index and vertex buffer
		// to the command buffer passed in its parameter
		
		// record the drawing command in the command buffer
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);
		// the second parameter is the number of indexes to be drawn. For a Model object,
		// this can be retrieved with the .indices.size() method.

		DS2.bind(commandBuffer, P, 0, currentImage);
		M2.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M2.indices.size()), 1, 0, 0, 0);
		DS3.bind(commandBuffer, P, 0, currentImage);
		M3.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M3.indices.size()), 1, 0, 0, 0);
		DS4.bind(commandBuffer, P, 0, currentImage);
		M4.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M4.indices.size()), 1, 0, 0, 0);
	}
=======
        // Load moon model and texture
        moon.init(this, &VD, "Models/Sphere.gltf", GLTF);
        moonTexture.init(this, "textures/Moon.jpg");

        // Create and load Saturn's ring
        createRingModel();
        saturnRingTexture.init(this, "textures/SaturnRing.png");

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
>>>>>>> Stashed changes

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		// Standard procedure to quit when the ESC key is pressed
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
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

		
		// Parameters
		// Camera FOV-y, Near Plane and Far Plane
		const float FOVy = glm::radians(90.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 100.0f;
		
		glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
		Prj[1][1] *= -1;
		glm::vec3 camTarget = glm::vec3(0,0,0);
		glm::vec3 camPos    = camTarget + glm::vec3(6,3,10) / 2.0f;
		glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));

<<<<<<< Updated upstream

		glm::mat4 World;

		World = glm::translate(glm::mat4(1), glm::vec3(-3, 0, 0));
		ubo1.mvpMat = Prj * View * World;
		DS1.map(currentImage, &ubo1, sizeof(ubo1), 0);
		// the .map() method of a DataSet object, requires the current image of the swap chain as first parameter
		// the second parameter is the pointer to the C++ data structure to transfer to the GPU
		// the third parameter is its size
		// the fourth parameter is the location inside the descriptor set of this uniform block
		World = glm::translate(glm::mat4(1), glm::vec3(3, 0, 0));
		ubo2.mvpMat = Prj * View * World;
		DS2.map(currentImage, &ubo2, sizeof(ubo2), 0);
		
		World = glm::scale(glm::mat4(1), glm::vec3(10.0f));
		ubo3.mvpMat = Prj * View * World;
		DS3.map(currentImage, &ubo3, sizeof(ubo3), 0);

		World = glm::translate(glm::mat4(1), glm::vec3(0, -5, 0)) *
				glm::scale(glm::mat4(1), glm::vec3(5.0f));
		ubo4.mvpMat = Prj * View * World;
		DS4.map(currentImage, &ubo4, sizeof(ubo4), 0);
	}	
=======
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

        // Create descriptor set for Saturn's ring
        saturnRingDS.init(this, &DSL, {
            {0, UNIFORM, sizeof(UniformBlock), nullptr},
            {1, TEXTURE, 0, &saturnRingTexture}
            });
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        sunDS.cleanup();
        for (int i = 0; i < NUM_PLANETS; i++) {
            planetDS[i].cleanup();
        }
        moonDS.cleanup();
        saturnRingDS.cleanup();
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

        // Draw Saturn's ring
        saturnRingDS.bind(commandBuffer, P, 0, currentImage);
        saturnRing.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(saturnRing.indices.size()), 1, 0, 0, 0);
    }

    void updateUniformBuffer(uint32_t currentImage) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);

        // Update time
        time += deltaT;

        // Camera parameters
        const float FOVy = glm::radians(45.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 500.0f;

        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;

        // Camera position and target
        glm::vec3 camTarget = glm::vec3(0, 0, 0);
        glm::vec3 camPos = camTarget + glm::vec3(0, 70, 140);
        glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

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

            // If this is Saturn, update its ring
            if (i == 5) { // Assuming Saturn is the 6th planet (index 5)
                glm::mat4 ringWorld = glm::translate(glm::mat4(1.0f), position) *
                    glm::rotate(glm::mat4(1.0f), planetProps[i].axialTilt, glm::vec3(0, 0, 1)) *
                    glm::scale(glm::mat4(1.0f), planetProps[i].scale * 3.0f); // Make ring larger than Saturn

                saturnRingUBO.mvpMat = Prj * View * ringWorld;
                saturnRingUBO.lightPos = lightPos;
                saturnRingDS.map(currentImage, &saturnRingUBO, sizeof(saturnRingUBO), 0);
            }
        }

        // Update moon uniform buffer
        int earthIndex = 2; // Assuming Earth is the third planet (index 2) in our array
        float earthAngle = time * planetProps[earthIndex].revolutionSpeed;
        glm::vec3 earthPosition(
            cos(earthAngle) * planetProps[earthIndex].orbitRadius,
            sin(planetProps[earthIndex].eclipticInclination) * planetProps[earthIndex].orbitRadius * sin(earthAngle),
            sin(earthAngle) * planetProps[earthIndex].orbitRadius * cos(planetProps[earthIndex].eclipticInclination)
        );

        float moonAngle = time * moonProps.revolutionSpeed;
        glm::vec3 moonRelativePosition(
            cos(moonAngle) * moonProps.orbitRadius,
            sin(moonAngle) * moonProps.orbitRadius,
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
    }
>>>>>>> Stashed changes
};


// This is the main: probably you do not need to touch this!
int main() {
    MeshLoader app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}