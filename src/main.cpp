#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
bool bloom = true;
bool hdr = true;
bool hdrKeyPressed = false;
bool bloomKeyPressed = false;
float exposure = 1.0f;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool spotlight = true;
    float whiteAmbientLightStrength = 0.0f;
    bool grayscaleEnabled = false;
    bool AAEnabled = true;
    bool introComplete = false;
    bool enabledKeyboardInput = false;
    bool enabledMouseInput = false;

    ProgramState()
            : camera(glm::vec3((-0.8f, 1.0f, 120.0f))) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
    glm::vec3 tempPosition=glm::vec3(0.0f, 2.0f, -7.0f);
    float tempScale=1.0f;
    float tempRotation=0.0f;
};
void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << spotlight << '\n'
        << whiteAmbientLightStrength << '\n';
        //<< introComplete;
}
void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> spotlight
           >> whiteAmbientLightStrength;
          // >> introComplete;
    }
}
ProgramState *programState;

void DrawImGui(ProgramState *programState);

glm::mat4 CalcFlashlightPosition();

unsigned int loadCubeMap(vector<std::string> faces);
void renderQuad();
void renderCube();

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Projekat", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowAspectRatio(window, 4, 3); // dozvoljava da prozor menja velicinu, ali cuva 4:3 odnos

    // glfw callbacks setup
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); // odsecamo zadje strane objekata

    // tell stb_image.h to flip loaded textures on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // build and compile shaders
    Shader objShader("resources/shaders/objectShader.vs", "resources/shaders/objectShader.fs");
    Shader screenShader("resources/shaders/anti-aliasing.vs", "resources/shaders/anti-aliasing.fs");    // za ANTI-ALIASING
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader hdrShader("resources/shaders/hdr.vs", "resources/shaders/hdr.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    //podseti me samo da izmenimo shader kojim crtas svetlo za svetiljku zbog bluma
    Shader jednaSvetiljka("resources8/shaders/objectShader.vs", "resources/shaders/jedna.fs");


    Shader shaderGeometryPass("resources/shaders/gBuffer.vs", "resources/shaders/gBuffer.fs");
    Shader shaderLightingPass("resources/shaders/deferredShadingLightingPassShader.vs", "resources/shaders/deferredShadingLigthingPassShader.fs");
    Shader shaderLightBox("resources/shaders/deferredLightShow.vs", "resources/shaders/deferredLightShow.fs");
    

    // load models
    Model ourModel("resources/objects/backpack/backpack.obj");
    ourModel.SetShaderTextureNamePrefix("material.");
    Model ulicnaSvetiljkaModel("resources/objects/Street Lamp2/StreetLamp.obj");
    //ulicnaSvetiljkaModel.SetShaderTextureNamePrefix("material.");

    // iz nekog razloga mora da se obrne tekstura
    stbi_set_flip_vertically_on_load(false);
    Model flashlightModel("resources/objects/flashlight/flashlight.obj");
    flashlightModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    Model houseModel("resources/objects/House/House.obj");
    houseModel.SetShaderTextureNamePrefix("material.");

    stbi_set_flip_vertically_on_load(false);
    Model cottageHouseModel("resources/objects/cottage_house/cottage_blender.obj");
    cottageHouseModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    Model roadModel("resources/objects/road/road.obj");
    roadModel.SetShaderTextureNamePrefix("material.");

    Model roadStopModel("resources/objects/ograda/rust_fence.obj");
    roadStopModel.SetShaderTextureNamePrefix("material.");

    stbi_set_flip_vertically_on_load(false);
    Model carModel("resources/objects/car/LowPolyCars.obj");
    carModel.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);

    //podloga
    float podlogaVertices[] = {
            // positions                           // normals                     // texture coords
            200.0f,  0.0f, -200.0f, 0.0f, 1.0f, 0.0f,   200.0f, 200.0f, // top right
            200.0f, 0.0f, 200.0f, 0.0f, 1.0f, 0.0f,  200.0f, 0.0f, // bottom right
            -200.0f, 0.0f, 200.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
            -200.0f,  0.0f, -200.0f, 0.0f, 1.0f, 0.0f,  0.0f, 200.0f  // top left
    };
    unsigned int podlogaIndices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    unsigned int podlogaVBO, podlogaVAO, podlogaEBO;
    glGenVertexArrays(1, &podlogaVAO);
    glGenBuffers(1, &podlogaVBO);
    glGenBuffers(1, &podlogaEBO);

    glBindVertexArray(podlogaVAO);

    glBindBuffer(GL_ARRAY_BUFFER, podlogaVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(podlogaVertices), podlogaVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, podlogaEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(podlogaIndices), podlogaIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normale coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/front.png"),
                    FileSystem::getPath("resources/textures/skybox/back.png"),
                    FileSystem::getPath("resources/textures/skybox/top.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox/left.png"),
                    FileSystem::getPath("resources/textures/skybox/right.png")
            };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);

    unsigned int cubeMapTexture;
    cubeMapTexture = loadCubeMap(faces);

    // uspravna trava
    float tallgrassVertices[] = {
            // positions                    // normals                        // texture Coords
            0.0f, -0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  0.0f,  0.0f,
            0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,  0.0f,  1.0f,
            1.0f,  0.5f,0.0f, 0.0f, 0.0f, 1.0f, 1.0f,  1.0f,

            0.0f, -0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  0.0f,  0.0f,
            1.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  1.0f,
            1.0f, -0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f
    };

    // tallgrass VAO for grass
    unsigned int tallgrassVAO, tallgrassVBO;
    glGenVertexArrays(1, &tallgrassVAO);
    glGenBuffers(1, &tallgrassVBO);
    glBindVertexArray(tallgrassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tallgrassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tallgrassVertices), tallgrassVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
    vector<glm::vec3> vegetation
        {
            glm::vec3(-1.5f, 0.5f, -0.48f),
            glm::vec3( 1.5f, 0.5f, 0.51f),
            glm::vec3( 0.0f, 0.5f, 0.7f),
            glm::vec3(-0.7f, 0.5f, -2.3f),
            glm::vec3 (1.0f, 0.5f, -1.2f),
            glm::vec3 (-0.1f, 0.5f, -0.63f),
            glm::vec3 (-1.75f, 0.5f, 1.0f),
            glm::vec3 (-0.6f, 0.5f, -2.0f)
        };

    // load textures
    unsigned int podlogaDiffuseMap = TextureFromFile("grass_diffuse.png", "resources/textures", true);  //gamma korekcija
    unsigned int podlogaSpecularMap = TextureFromFile("grass_specular.png", "resources/textures", true);
    unsigned int tallgrassTexture = TextureFromFile("grass.png", "resources/textures/", true);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    //trying this shii again
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    // ANTI-ALIASING
    float quadVertices[] = {
            // positions            // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "setupAntiAliasing::ERROR::FRAMEBUFFER Framebuffer is not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  
    // ------------------------------------------------------------------------------------------------------------------------
    //HDR i BLOOM
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int pingpongFBO[2];
    unsigned int pingpongColorBuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorBuffers);
    for (unsigned int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorBuffers[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //da probamo ovo umesto antialiasing framebuffera
//    unsigned int screenFBO;
//    unsigned int screenColorBuffer;
//    glGenFramebuffers(1, &screenFBO);
//    glGenTextures(1, &screenColorBuffer);
//    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
//    glBindTexture(GL_TEXTURE_2D, screenColorBuffer);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 720, 0, GL_RGBA, GL_FLOAT, nullptr);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenColorBuffer, 0);
//    //---------------------------------------------

    // configure g-buffer framebuffer
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    unsigned int gPosition, gNormal, gAlbedoSpec;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth2;
    glGenRenderbuffers(1, &rboDepth2);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth2);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth2);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    

    // konfiguracija shadera
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    objShader.use();
    objShader.setInt("material.texture_diffuse1", 0);
    objShader.setInt("material.texture_specular1", 1);

    hdrShader.use();
    hdrShader.setInt("scene", 0);
    hdrShader.setInt("bloomBlur", 1);

    blurShader.use();
    blurShader.setInt("image", 0);

    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);

    // ostale konfiguracije i inicijalizacije
    glm::vec3 lightPos(-5.0f, 4.0f, -5.0f); // pozicija point lighta
    glm::mat4 model = glm::mat4(1.0f);

    // svetla na banderama
    const unsigned int NR_LIGHTS = 10;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        lightPositions.push_back(glm::vec3(-0.35f, 6.55f, i * 12.0f));
        float rColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 1.0
        float gColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 1.0
        float bColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 1.0
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

    if(programState->introComplete == false) {
        programState->enabledKeyboardInput = false;
        programState->enabledMouseInput = false;
        programState->camera.Position = glm::vec3(-0.8f, 1.0f, 120.0f);
        programState->camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
        programState->camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(programState->introComplete == false)
        {
            // intro speed
            programState->camera.Position.z -= 15.0f * deltaTime;
        }

        if(programState->introComplete == false && programState->camera.Position.z < 0) {
            programState->enabledKeyboardInput = true;
            programState->enabledMouseInput = true;
            programState->camera.Position.x = -1.5f;
            programState->introComplete = true;
        }

        // ovo je intro render dok se "vozimo kolima"
        if(!programState->introComplete) {
            // 1. geometry pass: render scene's geometry/color data into gbuffer
            // -----------------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                    (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = programState->camera.GetViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);
            shaderGeometryPass.use();
            shaderGeometryPass.setMat4("projection", projection);
            shaderGeometryPass.setMat4("view", view);
            for (unsigned int i = 0; i < 10; i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 12.0f));
                model = glm::scale(model, glm::vec3(0.5f));
                shaderGeometryPass.setMat4("model", model);
                ulicnaSvetiljkaModel.Draw(shaderGeometryPass);
            }
            glDisable(GL_CULL_FACE);
                // crtanje podloge
                model = glm::mat4(1.0f);
                shaderGeometryPass.setMat4("model", model);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, podlogaDiffuseMap);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, podlogaSpecularMap);
                glActiveTexture(GL_TEXTURE2);
                glBindVertexArray(podlogaVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);                
            glEnable(GL_CULL_FACE);

            // renderovanje ulice
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 11.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shaderGeometryPass.setMat4("model", model);
            roadModel.Draw(shaderGeometryPass);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 42.68f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shaderGeometryPass.setMat4("model", model);
            roadModel.Draw(shaderGeometryPass);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 74.36f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shaderGeometryPass.setMat4("model", model);
            roadModel.Draw(shaderGeometryPass);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 106.04f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shaderGeometryPass.setMat4("model", model);
            roadModel.Draw(shaderGeometryPass);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
            // -----------------------------------------------------------------------------------------------------------------------
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shaderLightingPass.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
            // send light relevant uniforms
            for (unsigned int i = 0; i < lightPositions.size(); i++) {
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].position", lightPositions[i]);
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].direction", glm::vec3(0.0f, -1.0f, 0.0f));

                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].color",
                                           sin((float) glfwGetTime() * lightColors[i]) / 2.0f + 0.5f);
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].ambient", glm::vec3(0.01f));
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].diffuse", 1.0f, 1.0f, 1.0f);
                shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);

                shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].constant", 1.0f);
                shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].linear", 0.06f);
                shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].quadratic", 0.032f);

                shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].cutOff",
                                            glm::cos(glm::radians(15.0f + (sin((float)glfwGetTime()) / 2.0f + 0.5) * 3)));
                shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].outerCutOff",
                                            glm::cos(glm::radians(25.0f + (cos((float)glfwGetTime()) / 2.0f + 0.5) * 5)));
            }
            shaderLightingPass.setVec3("viewPos", programState->camera.Position);
            // finally render quad
            renderQuad();

            // copy content of geometry's depth buffer to default framebuffer's depth buffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 3. render lights on top of scene
            // --------------------------------
            shaderLightBox.use();
            shaderLightBox.setMat4("projection", projection);
            shaderLightBox.setMat4("view", view);
            for (unsigned int i = 0; i < lightPositions.size(); i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, lightPositions[i]);
                model = glm::scale(model, glm::vec3(0.35f, 0.1f, 0.30f));
                shaderLightBox.setMat4("model", model);
                shaderLightBox.setVec3("lightColor", lightColors[i]);
                renderCube();
            }
        }  

        if(programState->introComplete) {
          // ANTI-ALIASING postaje HDR
          // *************************************************************************************************************
          //glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
          glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
          //glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          //glEnable(GL_DEPTH_TEST);    ovo sto sam zakomentarisao moze da se proveri
          // *************************************************************************************************************
//             // ANTI-ALIASING: preusmeravamo renderovanje na nas framebuffer da bismo imali MSAA
//             // *************************************************************************************************************
//             glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
//             glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
//             glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//             glEnable(GL_DEPTH_TEST);
//             // *************************************************************************************************************
        }

        //object shader
        objShader.use();
        objShader.setVec3("viewPosition", programState->camera.Position);
        objShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        objShader.setMat4("projection", projection);
        objShader.setMat4("view", view);


        // directional light
        objShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        objShader.setVec3("dirLight.ambient", glm::vec3(programState->whiteAmbientLightStrength));
        objShader.setVec3("dirLight.diffuse", 0.05f, 0.05f, 0.05);   //privremeno samo za hdr
        objShader.setVec3("dirLight.specular", 0.2f, 0.2f, 0.2f);

        objShader.setVec3("pointLight.position", lightPos);
        objShader.setVec3("pointLight.ambient", glm::vec3(1.0f));
        objShader.setVec3("pointLight.diffuse", 0.05f, 0.05f, 0.05);
        objShader.setVec3("pointLight.specular", 0.2f, 0.2f, 0.2f);
        objShader.setFloat("pointLight.constant", 1.0f);
        objShader.setFloat("pointLight.linear", 0.09f);
        objShader.setFloat("pointLight.quadratic", 0.032f);

        // spotlight - baterijska lampa
        objShader.setVec3("lampa.position", programState->camera.Position + 0.35f * programState->camera.Front + 0.07f * programState->camera.Right - 0.08f * programState->camera.Up);
        objShader.setVec3("lampa.direction", programState->camera.Front);
        objShader.setVec3("lampa.ambient", 0.0f, 0.0f, 0.0f);
        if(programState->spotlight) {
            objShader.setVec3("lampa.diffuse", 5.0f, 5.0f, 5.0f);
            objShader.setVec3("lampa.specular", 1.0f, 1.0f, 1.0f);
        }
        else {
            objShader.setVec3("lampa.diffuse", 0.0f, 0.0f, 0.0f);
            objShader.setVec3("lampa.specular", 0.0f, 0.0f, 0.0f);
        }
        objShader.setFloat("lampa.constant", 1.0f);
        objShader.setFloat("lampa.linear", 0.09f);
        objShader.setFloat("lampa.quadratic", 0.032f);
        objShader.setFloat("lampa.cutOff", glm::cos(glm::radians(10.0f)));
        objShader.setFloat("lampa.outerCutOff", glm::cos(glm::radians(15.0f)));

        // renderovanje ranca:
        model = glm::mat4(1.0f);
        model = glm::translate(model,programState->tempPosition);
        model = glm::scale(model, glm::vec3(programState->tempScale));
        objShader.setMat4("model", model);
        ourModel.Draw(objShader);

        if(programState->introComplete) {
            // renderovanje baterijske lampe:
            model = CalcFlashlightPosition();
            objShader.setMat4("model", model);
            flashlightModel.Draw(objShader);

            // renderovanje automobila:
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.4f, 0.2f, 1.0f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.85f));
            objShader.setMat4("model", model);
            carModel.Draw(objShader);
        }

        // renderovanje stop znaka:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.69f, 0.15f, -4.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.25f));
        objShader.setMat4("model", model);
        roadStopModel.Draw(objShader);

        glDisable(GL_CULL_FACE);
        // renderovanje kuce:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20, 0.01, -20));
        model = glm::scale(model, glm::vec3(0.7f));
        objShader.setMat4("model", model);
        houseModel.Draw(objShader);

        // renderovanje supe:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(25, 0, 25));
        model = glm::scale(model, glm::vec3(0.3));
        objShader.setMat4("model", model);
        cottageHouseModel.Draw(objShader);

        glBindVertexArray(tallgrassVAO);
        glBindTexture(GL_TEXTURE_2D, tallgrassTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            //model = glm::rotate(model, (float)i*60.0f, glm::vec3(0.0, 0.1, 0.0));
            objShader.setMat4("model", model);
            objShader.setMat4("projection", projection);
            objShader.setMat4("view", view);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        if(programState->introComplete) {
            // renderovanje ulice
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 11.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            objShader.setMat4("model", model);
            roadModel.Draw(objShader);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 42.68f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            objShader.setMat4("model", model);
            roadModel.Draw(objShader);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 74.36f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            objShader.setMat4("model", model);
            roadModel.Draw(objShader);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 106.04f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            objShader.setMat4("model", model);
            roadModel.Draw(objShader);

            for (unsigned int i = 0; i < 10; i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 12.0f));
                model = glm::scale(model, glm::vec3(0.5f));
                objShader.setMat4("model", model);
                ulicnaSvetiljkaModel.Draw(objShader);
            }

            //podloga
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, podlogaDiffuseMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, podlogaSpecularMap);

            model = glm::mat4(1.0f);
            objShader.setMat4("model", model);

            //za blinfonga treba shinnes 4* veci al trava ne sme da se presijava bas tako da tu treba obratiti paznju
            //objShader.setFloat("material.shininess", 32.0f);
            glBindVertexArray(podlogaVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        glEnable(GL_CULL_FACE);


        //object rendering end, start of skybox rendering

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content

        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // set depth function back to default

        
        if(programState->introComplete) {
//             // ANTI-ALIASING: ukljucivanje
//             // *************************************************************************************************************
//             glBindFramebuffer(GL_FRAMEBUFFER, 0);
//             glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
//             glClear(GL_COLOR_BUFFER_BIT);
//             glDisable(GL_DEPTH_TEST);


            // BLOOM
            // ****************************************************************************************************
            //zavrseno renderovanje u bloomframebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            bool horizontal = true;
            bool first_iteration = true;
            blurShader.use();
            unsigned int amount = 10;
            for (unsigned int i = 0; i < amount; ++i) {
                glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
                blurShader.setBool("horizontal", horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[i] : pingpongColorBuffers[!horizontal]);

                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);

                horizontal = !horizontal;
                if (first_iteration) {
                    first_iteration = false;
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            hdrShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horizontal]);
            hdrShader.setBool("hdr", hdr);
            hdrShader.setBool("bloom", bloom);
            hdrShader.setFloat("exposure", exposure);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            screenShader.use();
            //dal ovde negde treba depthtestdisable?
            screenShader.setFloat("SCR_WIDTH", SCR_WIDTH);
            screenShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);
            screenShader.setBool("grayscaleEnabled", programState->grayscaleEnabled);

            glBindVertexArray(quadVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            // *************************************************************************************************************
        }

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glDeleteVertexArrays(1, &tallgrassVAO);
    glDeleteBuffers(1, &tallgrassVAO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(programState->enabledKeyboardInput) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(DOWN, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(UP, deltaTime);

    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if(programState->enabledMouseInput) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        if (programState->CameraMouseMovementUpdateEnabled)
            programState->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            programState->CameraMouseMovementUpdateEnabled = false;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            programState->CameraMouseMovementUpdateEnabled = true;
        }

    }
    //hdr bloom
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        hdr = !hdr;
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        bloom = !bloom;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.01f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.01f;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS && programState->ImGuiEnabled) {
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
        if(programState->CameraMouseMovementUpdateEnabled == true)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // ANTI-ALIASING key callbacks:
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        programState->AAEnabled = !programState->AAEnabled;
        if(programState->AAEnabled)
            glEnable(GL_MULTISAMPLE);
        if(!programState->AAEnabled)
            glDisable(GL_MULTISAMPLE);
    }

    if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
        programState->grayscaleEnabled = !programState->grayscaleEnabled;


    // reset the camera to default position
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        programState->camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
        programState->camera.Yaw = -90.0f;
        programState->camera.Pitch = 0.0f;
        programState->camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        programState->spotlight = !programState->spotlight;
    }
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(500, 150));
        ImGui::Begin("Settings:");
        ImGui::DragFloat("Ambient Light Strength", (float *) &programState->whiteAmbientLightStrength, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat3("Backpack position", (float*)&programState->tempPosition);
        ImGui::DragFloat("Backpack scale", &programState->tempScale, 0.05, 0.1, 4.0);
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0,200));
        ImGui::SetNextWindowSize(ImVec2(500, 150), ImGuiCond_Once);
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Spacing();
        ImGui::Bullet();
        ImGui::Text("Toggle camera movement on/off: C");
        ImGui::Bullet();
        ImGui::Text("Reset camera position: P");
        ImGui::Bullet();
        ImGui::Text("Toggle camera spotlight on/off: K");
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0,400));
        ImGui::SetNextWindowSize(ImVec2(500, 150), ImGuiCond_Once);
        ImGui::Begin("Anti-aliasing settings");
        ImGui::Checkbox("Anti-Aliasing (shortcut: F2)", &programState->AAEnabled);
        ImGui::Checkbox("Grayscale (shortcut: F3)", &programState->grayscaleEnabled);
        ImGui::End();
    }

    {
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - 60,0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(60, 50));
        ImGui::Begin("FPS:");
        ImGui::Text(("%.2f"), floor(1/deltaTime));
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

glm::mat4 CalcFlashlightPosition() {
    Camera c = programState->camera;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, c.Position + 0.35f * c.Front + 0.07f * c.Right - 0.12f * c.Up);

    model = glm::rotate(model, -glm::radians(c.Yaw + 180), c.Up);
    model = glm::rotate(model, glm::radians(c.Pitch), c.Right);

    model = glm::scale(model, glm::vec3(0.025f));

    return model;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int loadCubeMap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "CubeMap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
