#define GLFW_INCLUDE_NONE
#define _USE_MATH_DEFINES

#include "renderers/RenderManager.h"

#include "Loader.h"
#include "Model.h"
#include "ShadowMap.h"
#include "entities/Entity.h"
#include "entities/Light.h"
#include "entities/Camera.h"
#include "entities/Player.h"
#include "entities/Terrain.h"
#include "InputState.h"
#include "GameTime.h"
#include "FrameBuffer.h"

#include "particles/ParticleManager.h"
#include "particles/ParticleSystem.h"

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cfloat>

// Windows specific headers.
#ifdef _WIN32
#include <io.h>
#include <GL/freeglut.h>
#endif



using namespace std;
using namespace glm;

int winX = 640;
int winY = 480;
const float SKYBOX_SIZE = 200.0f;

// Data structure storing mouse input info
InputState input;
glm::mat4 projection;

// Player must be declared here so that keyboard callbacks can be sent
Player* player;


// Error callback for GLFW. Simply prints error message to stderr.
void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Terminate program if escape is pressed
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwDestroyWindow(window);
        glfwTerminate();
        exit(0);
    }

    player->handleKeyboardEvents(window, key, scancode, action, mods);
}

void mouse_pos_callback(GLFWwindow* window, double x, double y) {
    // Use a global data structure to store mouse info
    // We can then use it however we want
    input.update((float)x, (float)y);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    input.updateScroll((float)xoffset, (float)yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        input.rMousePressed = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        input.rMousePressed = false;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        input.lMousePressed = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        input.lMousePressed = false;
    }
}

void setProjection(int winX, int winY) {
    // float aspect = (float) winX / winY;
    // FOV angle is in radians
    projection = glm::perspective(M_PI/4.0, double(winX) / double(winY), 1.0, 800.0);
}

// Called when the window is resized.
void reshape_callback(GLFWwindow *window, int x, int y ) {
    winX = x;
    winY = y;

    setProjection(x,y);
    glViewport( 0, 0, x, y );
}

GLFWwindow* initialise(){
    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) exit(1);

    // Specify that we want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window and OpenGL context
    window = glfwCreateWindow(winX, winY, "CG Assignment 4", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(1);
    }

    // Set callbacks key press and mouse press.
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, reshape_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    assert(glewInit() == GLEW_OK);
    glfwGetFramebufferSize(window, &winX, &winY);

    // Sets the (background) colour for each time the frame-buffer
    // (colour buffer) is cleared
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

/**
 * Program entry. Sets up OpenGL state, GLSL Shaders and GLFW window and function call backs
 * Takes no arguments
 */
int main(int argc, char **argv) {
    GLFWwindow* window = initialise();

    if(argc != 2){
        cerr << "USAGE: " << argv[0] << " basic|physics" << endl;
        exit(1);
    }

    // Check if desired controls are basic or physics
    bool basic_controls = strcmp(argv[1], "basic") == 0;

    if(basic_controls){
        cout << "Controls: \n\tw - forward\n\ts - backwards\n\ta/d - turn left/right" << endl;
    } else {
        cout << "Controls: \n\tw - throttle\n\ts - brake\n\ta/d - steer left/right\n\tspace - handbrake" << endl;
    }

    srand(time(NULL));

    // Define skybox textures
    // The current skybox is unfortunately quite low res and has compression issues.
    // However, I wanted a skybox that had no terrain AND a visible sun which was surprisingly hard to find.
    std::vector<std::string> skyboxTextures = {
        "res/sky/sky_right.tga",
        "res/sky/sky_left.tga",
        "res/sky/sky_top.tga",
        "res/sky/sky_bottom.tga",
        "res/sky/sky_back.tga",
        "res/sky/sky_front.tga"
    };

    // Load all of the requires models from disk.
    Model barrelModel = Loader::getLoader()->loadModel("res/Barrel/Barrel02.obj");
    Model playerModel = Loader::getLoader()->loadModel("res/car/car-n.obj");
    Model fenceModel = Loader::getLoader()->loadModel("res/fence/fence.obj");
    Model coneModel = Loader::getLoader()->loadModel("res/cone/cone2_obj.obj");
    Model treeModel = Loader::getLoader()->loadModel("res/tree/PineTree03.obj");
    Model stumpModel = Loader::getLoader()->loadModel("res/stump/TreeStump03.obj");

    // Vector to hold all of the world entities.
    std::vector<Entity*> entities;
    // Vector to hold lights
    std::vector<Light*> lights;

    // Create the skybox with the textures defined.
    SkyboxRenderer skybox(skyboxTextures, SKYBOX_SIZE);
    RenderManager manager;

    // Create Terrain using blend map, height map and all of the remaining texture components.
    std::vector<string> terrainImages = {"res/terrain/blendMap.png", "res/terrain/grass.jpg","res/terrain/road.jpg","res/terrain/dirt.png","res/terrain/mud.jpg"};
    Terrain* terrain = Terrain::loadTerrain(terrainImages, "res/terrain/heightmap.png");
    // Moves the terrain model to be centered about the origin.
    terrain->setPosition(vec3(-Terrain::TERRAIN_SIZE/2, 0.0f, -Terrain::TERRAIN_SIZE/2));

    // Load dust texture and create particle system.
    GLuint dust_texture = Loader::getLoader()->loadTexture( "./res/dust_single.png");\
    ParticleSystem particleSystem(30.0f, 3.0f, 0.5f, 0.5f, dust_texture);

    // Create the player object, scaling for the model, and setting its position in the world to somewhere interesting.
    player = new Player(&playerModel, terrain, basic_controls);
    player->setScale(vec3(0.4f, 0.4f, 0.4f));
    player->setPosition(terrain->getPositionFromPixel(555, 751));
    player->setRotationY((float)5*M_PI/8);
    entities.push_back(player);

    // Initialisation of camera, projection matrix
    setProjection(winX, winY);
    Camera* cam = new PlayerCamera(player);

    // Create light sources
    Light* sun =  new Light();
    sun->position = vec4(-1.25*SKYBOX_SIZE/10, 2.5*SKYBOX_SIZE/10, 3*SKYBOX_SIZE/10, 0.0f); // w = 0 - directional
    sun->specular = vec3(1.0f, 1.0f, 1.0f);
    sun->diffuse = vec3(0.7f, 0.7f, 0.7f);
    sun->ambient = vec3(0.1f, 0.1f, 0.1f);
    lights.push_back(sun);

    Light* headlight = new Light();
    headlight->position = vec4(2.0f, 8.0f,0.0f, 1.0f);
    headlight->specular =vec3(0.8f, 0.8f, 0.4f);
    headlight->diffuse = vec3(0.8f, 0.8f, 0.4f);
    headlight->coneDirection = vec3(0.0f, -1.0f, 0.0f);
    headlight->coneAngle = M_PI/4;
    headlight->radius = 10.0f;
    lights.push_back(headlight);

    // Adds entities to random positions on the map
    const size_t RAND_ENTITIES = 500;
    for(size_t i = 0; i < RAND_ENTITIES; i+= 2){
        int selection = rand() % 2;
        Entity* ent;
        switch(selection){
            case 0:
                ent = new Entity(&treeModel);
                break;
            case 1:
                ent = new Entity(&stumpModel);
                ent->setScale(glm::vec3(0.5, 0.5, 0.5));
                break;
        }

        ent->setPosition(terrain->getPositionFromPixel(rand() % 1024, rand() % 1024));
        entities.push_back(ent);
    }

    // Set of pre calculated cone positions on corners of the track
    vector<int> conePositions = {
        263, 262, 226, 250, 209, 273,
        213, 299, 342, 717, 329, 734,
        326, 751, 354, 755, 372, 754,
        750, 400, 765, 396, 748, 381,
        828, 480, 842, 476, 854, 478,
        852, 500, 852, 521, 842, 547,
        772, 402
    };

    // Creates cones from the positions and adds them.
    for(size_t i = 0; i < conePositions.size(); i+= 2){
        Entity* cone = new Entity(&coneModel);
        cone->setPosition(terrain->getPositionFromPixel(conePositions[i], conePositions[i+1]));
        cone->setScale(vec3(0.01f, 0.01f, 0.01f));  // The cone model was MASSIVE
        entities.push_back(cone);
    }

    // Add the bordering fences to the map.
    float fenceSize = fenceModel.getRangeInDim(0).second  - fenceModel.getRangeInDim(0).first;
    for(float x = -Terrain::TERRAIN_SIZE/2; x < Terrain::TERRAIN_SIZE/2; x += fenceSize){
        Entity* fence = new Entity(&fenceModel);
        fence->setPosition(vec3(x, 0.0f, Terrain::TERRAIN_SIZE/2 - 1.0f));
        entities.push_back(fence);

        fence = new Entity(&fenceModel);
        fence->setPosition(vec3(x, 0.0f, -Terrain::TERRAIN_SIZE/2 + 1.0f));
        entities.push_back(fence);
    }

    for(float z = -Terrain::TERRAIN_SIZE/2; z < Terrain::TERRAIN_SIZE/2; z += fenceSize){
        Entity* fence = new Entity(&fenceModel);
        fence->setPosition(vec3(Terrain::TERRAIN_SIZE/2 - 1.0f, 0.0f, z));
        fence->rotateY((float)-M_PI/2);
        entities.push_back(fence);

        fence = new Entity(&fenceModel);
        fence->setPosition(vec3(-Terrain::TERRAIN_SIZE/2 + 1.0f, 0.0f, z));
        fence->rotateY((float)-M_PI/2);
        entities.push_back(fence);
    }

    // Goes through each entity and aligns its bottom edge with the terrain at that position.
    for(size_t i = 0; i < entities.size(); i++){
        entities[i]->placeBottomEdge(terrain->getHeight(entities[i]->getPosition().x, entities[i]->getPosition().z));
    }

    // Create the large lake
    Entity* water = new Entity();
    water->setScale(vec3(100.0f, 1.0f, 50.0f));
    water->setPosition(terrain->getPositionFromPixel(650, 826));
    water->setPosition(glm::vec3(water->getPosition().x, 0.4f, water->getPosition().z));

    // Create the object for handling rendering to texture for shadows.
    ShadowMap shadowMap(player, lights[0], 4096);

    // Main logic/render loop.
    while (!glfwWindowShouldClose(window)) {
        GameTime::getGameTime()->update();
        cam->update(input);

        // Render entire scene
        manager.render(entities, lights, terrain, water, skybox, shadowMap, cam, projection, winX, winY);

        // Updates all particles and entities.
        ParticleManager::getParticleManager()->update();
        for(size_t i = 0; i < entities.size(); i++){
            entities[i]->update();
        }

        // Generate dust particles at the players positions if the car is going past enough or moving
        if(player->absVel > 5.0f || player->getThrottle() > 0.1f || (basic_controls && player->getBrake() > 0.1f)){
            particleSystem.generateParticles(player->getPosition() - player->getDirectionVector());
        }

        // Update the postion of the car headlights
        headlight->position = vec4(player->getPosition() + vec3(0.0f, 0.1f, 0.0f), 1.0f);
        headlight->coneDirection = player->getDirectionVector();

        glFlush();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup program, delete all the dynamic entities.
    delete player;
    delete water;
    for(size_t i = 0; i < entities.size(); i++){
        delete entities[i];
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
