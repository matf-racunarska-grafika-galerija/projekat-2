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

#include <rg/setup.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);
bool norm(glm::vec3 v1, glm::vec3 v2);
bool uslovi();
// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
bool hdr = true;
bool bloom = true;
bool hdrKeyPressed = false;
bool bloomKeyPressed = false;
float exposure = 1.0f;
glm::vec3 zombiePos = glm::vec3(100.0f, -3.0f, 100.0f);

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float pulseCycleTime = 0.4f;
float pulseAccTime = 0.0f;
std::vector<float> flickerMode(4);
float flickerAccTime = 0.0f;
float flickerCycleTime = 1.0f;
int mode = 0;
float lastChange = 0.0f;

void updateFlickering();
bool oneSecondPassed(float lastChange);

struct ProgramState {
    bool ImGuiEnabled = false;
    bool exposureWindowEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool spotlight = true;
    float whiteAmbientLightStrength = 0.0f;
    bool grayscaleEnabled = false;
    bool AAEnabled = true;
    bool introComplete = false;
    bool enabledKeyboardInput = false;
    bool enabledMouseInput = false;
    bool showZombie = true;
    bool zombieRendered = false;
    bool creativeMode = false;
    bool renderuj = false;
    bool odobreno = false;

    ProgramState()
            : camera(glm::vec3((-0.8f, 1.0f, 150.0f))) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
};
void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << spotlight << '\n'
        << whiteAmbientLightStrength << '\n';
}
void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> spotlight
           >> whiteAmbientLightStrength;
    }
}
ProgramState *programState;

static void HelpMarker(const char* desc, bool extraText = false);
void DrawImGui(ProgramState *programState);

glm::mat4 CalcFlashlightPosition();
glm::vec3 CalcZombiePosition();

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
    glfwSwapInterval( 0 );
    glfwSetWindowAspectRatio(window, 4, 3); // dozvoljava da prozor menja velicinu, ali cuva 4:3 odnos

    // glfw callbacks setup
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

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
    Shader screenShader("resources/shaders/postProcessing.vs", "resources/shaders/postProcessing.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shaderGeometryPass("resources/shaders/gBuffer.vs", "resources/shaders/gBuffer.fs");
    Shader shaderLightingPass("resources/shaders/deferredShadingLightingPassShader.vs", "resources/shaders/deferredShadingLightingPassShader.fs");
    Shader shaderLightBox("resources/shaders/deferredLightShow.vs", "resources/shaders/deferredLightShow.fs");
    Shader instancedGrass("resources/shaders/instancedGrass.vs", "resources/shaders/instancedGrass.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader tvScreenShader("resources/shaders/tvScreen.vs", "resources/shaders/tvScreen.fs");

    // load models
    Model ourModel("resources/objects/backpack/backpack.obj");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model ulicnaSvetiljkaModel("resources/objects/street_lamp/StreetLamp.obj");
    ulicnaSvetiljkaModel.SetShaderTextureNamePrefix("material.");

    Model roadModel("resources/objects/road/road.obj");
    roadModel.SetShaderTextureNamePrefix("material.");

    Model roadStopModel("resources/objects/ograda/rust_fence.obj");
    roadStopModel.SetShaderTextureNamePrefix("material.");

    // iz nekog razloga mora da se obrne tekstura
    stbi_set_flip_vertically_on_load(false);

    Model flashlightModel("resources/objects/flashlight/flashlight.obj");
    flashlightModel.SetShaderTextureNamePrefix("material.");

    Model cottageHouseModel("resources/objects/cottage_house/cottage_blender.obj");
    cottageHouseModel.SetShaderTextureNamePrefix("material.");

    Model cottageHouseModel2("resources/objects/cottage_house2/cottage2.obj");
    cottageHouseModel2.SetShaderTextureNamePrefix("material.");

    Model carModel("resources/objects/car/LowPolyCars.obj");
    carModel.SetShaderTextureNamePrefix("material.");

    Model treeModel("resources/objects/tree/tree.obj");
    treeModel.SetShaderTextureNamePrefix("material.");

    Model tvModel("resources/objects/tv/tv.obj");
    tvModel.SetShaderTextureNamePrefix("material.");

    Model stoolModel("resources/objects/tv/wooden stool.obj");
    stoolModel.SetShaderTextureNamePrefix(".material");

    Model zombieModel("resources/objects/zombie/zombie.obj");
    zombieModel.SetShaderTextureNamePrefix("material.");

    Model signModel("resources/objects/sign/sign.obj");
    signModel.SetShaderTextureNamePrefix("material.");

    Model dumpModel("resources/objects/dump/dump.obj");
    dumpModel.SetShaderTextureNamePrefix("material.");

    Model trailerModel("resources/objects/trailer/trailer.obj");
    trailerModel.SetShaderTextureNamePrefix("material.");

    stbi_set_flip_vertically_on_load(true);

    unsigned int podlogaVAO = setupFloorPlane();

    unsigned int cubeMapTexture;
    unsigned int skyboxVAO = setupSkybox(cubeMapTexture);

    unsigned int amount = 9001; // IT'S OVER 9000 !!!
    unsigned int tallgrassVAO = setupTallGrass(amount);

    // Anti-aliasing i HDR
    unsigned int framebuffer, textureColorBufferMultiSampled;
    unsigned int colorBuffers[2];
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    unsigned int screenVAO = setupPostProcessing(framebuffer, textureColorBufferMultiSampled, colorBuffers,
                                                 pingpongFBO, pingpongColorbuffers, SCR_WIDTH, SCR_HEIGHT);


    unsigned int gPosition, gNormal, gAlbedoSpec;
    unsigned int gBuffer = setupGBuffer(gPosition, gNormal, gAlbedoSpec, SCR_WIDTH, SCR_HEIGHT);

    // load textures
    unsigned int podlogaDiffuseMap = TextureFromFile("grass_diffuse.png", "resources/textures");
    unsigned int podlogaSpecularMap = TextureFromFile("grass_specular.png", "resources/textures");
    unsigned int tvScreenTexture = TextureFromFile("tv_screen.png", "resources/textures");
    unsigned int tallgrassTexture = TextureFromFile("grass.png", "resources/textures");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // konfiguracija shadera
    screenShader.use();
    screenShader.setInt("hdrBuffer", 0);
    screenShader.setInt("bloomBlur", 1);
    screenShader.setInt("screenTexture", 2);

    objShader.use();
    objShader.setInt("material.texture_diffuse1", 0);
    objShader.setInt("material.texture_specular1", 1);

    blurShader.use();
    blurShader.setInt("image", 0);
    blurShader.setInt("SCR_WIDTH", SCR_WIDTH);
    blurShader.setInt("SCR_HEIGHT", SCR_HEIGHT);

    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);

    tvScreenShader.use();
    tvScreenShader.setInt("slika", 0);

    // ostale konfiguracije i inicijalizacije
    glm::vec3 lightPos(-5.0f, 4.0f, -5.0f); // pozicija point lighta
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 projection;
    glm::mat4 view;

    // svetla na banderama
    const unsigned int NR_LIGHTS = 13;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        lightPositions.push_back(glm::vec3(-0.35f, 6.55f, i * 12.0f));
        float rColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 0.6
        float gColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 0.6
        float bColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 0.6
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

    // pozicije drveca
    srand(9); // lupao sam random seedove dok nisam naisao na neki koji mi se svidja (ne menjaj)
    const unsigned int NR_TREES = 35;
    std::vector<glm::vec3> treePos;
    for(int i = 0; i < NR_TREES; i++)
        treePos.push_back(glm::vec3(rand() % 200 - 100, 0.0f, rand() % 200 - 100));


    if(programState->introComplete == false) {
        programState->enabledKeyboardInput = false;
        programState->enabledMouseInput = false;
        programState->camera.Position = glm::vec3(-0.1f, 1.0f, 150.0f);
        programState->camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
        programState->camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = (float) glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        updateFlickering();  // racuna "treptanje" svetla prve bandere

        // input
        processInput(window);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        if (programState->introComplete == false) {
            // intro speed
            programState->camera.Position.z -= 10.0f * deltaTime;
        }

        if (programState->introComplete == false && programState->camera.Position.z < 0) {
            programState->enabledKeyboardInput = true;
            programState->enabledMouseInput = true;
            programState->camera.Position.x = -1.5f;
            programState->camera.Position.y = 1.8f;
            programState->introComplete = true;
        }

        // ovo je intro render dok se "vozimo kolima"
        if (!programState->introComplete) {
            // 1. geometry pass: render scene's geometry/color data into gbuffer
            // -----------------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                          (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 200.0f);
            view = programState->camera.GetViewMatrix();
            model = glm::mat4(1.0f);
            shaderGeometryPass.use();
            shaderGeometryPass.setMat4("projection", projection);
            shaderGeometryPass.setMat4("view", view);
            for (unsigned int i = 0; i < NR_LIGHTS; i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 12.0f));
                model = glm::scale(model, glm::vec3(0.5f));
                shaderGeometryPass.setMat4("model", model);
                ulicnaSvetiljkaModel.Draw(shaderGeometryPass);
            }
            glDisable(GL_CULL_FACE);

            for (int i = 0; i < NR_TREES; i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, treePos[i]);
                model = glm::scale(model, glm::vec3(2.0f));
                shaderGeometryPass.setMat4("model", model);
                treeModel.Draw(shaderGeometryPass);
            }

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
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 137.72f));
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
                                            glm::cos(glm::radians(
                                                    15.0f + (sin((float) glfwGetTime()) / 2.0f + 0.5) * 3)));
                shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].outerCutOff",
                                            glm::cos(glm::radians(
                                                    25.0f + (cos((float) glfwGetTime()) / 2.0f + 0.5) * 5)));
            }
            shaderLightingPass.setVec3("viewPos", programState->camera.Position);
            // finally render quad
            renderQuad();

            // copy content of geometry's depth buffer to default framebuffer's depth buffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT,
                              GL_NEAREST);
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
                shaderLightBox.setVec3("lightColor", sin((float) glfwGetTime() * lightColors[i]) / 2.0f + 0.5f);
                renderCube();
            }
        }

        if (programState->introComplete) {
            // ANTI-ALIASING: preusmeravamo renderovanje na nas framebuffer da bismo imali MSAA
            // *************************************************************************************************************
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            // *************************************************************************************************************
        }

        //object shader
        objShader.use();
        objShader.setVec3("viewPosition", programState->camera.Position);
        objShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
        view = programState->camera.GetViewMatrix();
        objShader.setMat4("projection", projection);
        objShader.setMat4("view", view);

        // directional light
        objShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        objShader.setVec3("dirLight.ambient", glm::vec3(programState->whiteAmbientLightStrength));
        objShader.setVec3("dirLight.diffuse", 0.05f, 0.05f, 0.05);   //privremeno samo za hdr
        objShader.setVec3("dirLight.specular", 0.2f, 0.2f, 0.2f);

//        objShader.setVec3("pointLight.position", lightPos);
//        objShader.setVec3("pointLight.ambient", glm::vec3(0.0f));
//        objShader.setVec3("pointLight.diffuse", 13.0f, 0.0f, 0.0f);
//        objShader.setVec3("pointLight.specular", 0.2f, 0.2f, 0.2f);
//        objShader.setFloat("pointLight.constant", 1.0f);
//        objShader.setFloat("pointLight.linear", 0.09f);
//        objShader.setFloat("pointLight.quadratic", 0.032f);

        // spotlight - baterijska lampa
        objShader.setVec3("lampa.position", programState->camera.Position + 0.35f * programState->camera.Front +
                                            0.07f * programState->camera.Right - 0.08f * programState->camera.Up);
        objShader.setVec3("lampa.direction", programState->camera.Front);
        objShader.setVec3("lampa.ambient", 0.0f, 0.0f, 0.0f);
        if (programState->spotlight) {
            objShader.setVec3("lampa.diffuse", 3.0f, 3.0f, 3.0f);
            objShader.setVec3("lampa.specular", glm::vec3(0.2f));
        } else {
            objShader.setVec3("lampa.diffuse", 0.0f, 0.0f, 0.0f);
            objShader.setVec3("lampa.specular", 0.0f, 0.0f, 0.0f);
        }
        objShader.setFloat("lampa.constant", 1.0f);
        objShader.setFloat("lampa.linear", 0.09f);
        objShader.setFloat("lampa.quadratic", 0.032f);
        objShader.setFloat("lampa.cutOff", glm::cos(glm::radians(10.0f)));
        objShader.setFloat("lampa.outerCutOff", glm::cos(glm::radians(15.0f)));

        // spotlight - flickering light
        objShader.setVec3("flickeringLight.position", lightPositions[0]);
        objShader.setVec3("flickeringLight.direction", glm::vec3(0.0f, -1.0f, 0.0f));
        objShader.setVec3("flickeringLight.ambient", 0.0f, 0.0f, 0.0f);
        objShader.setVec3("flickeringLight.diffuse", flickerMode[mode] * glm::vec3(1.0f, 1.0f, 0.5f));
        objShader.setVec3("flickeringLight.specular", 1.0f, 1.0f, 1.0f);
        objShader.setFloat("flickeringLight.constant", 1.0f);
        objShader.setFloat("flickeringLight.linear", 0.09f);
        objShader.setFloat("flickeringLight.quadratic", 0.032f);
        objShader.setFloat("flickeringLight.cutOff", glm::cos(glm::radians(15.0f)));
        objShader.setFloat("flickeringLight.outerCutOff", glm::cos(glm::radians(30.0f)));

        // spotlight - svetlo tv-a
        objShader.setVec3("tvLight.position", glm::vec3(2.0f, 0.635f, -39.8f));
        objShader.setVec3("tvLight.direction", glm::vec3(-1.0f, 0.0f, 1.0f));
        objShader.setVec3("tvLight.ambient", 0.02f, 0.02f, 0.02f);
        objShader.setVec3("tvLight.diffuse", glm::vec3(10.0f, 10.0f, 10.0f));
        objShader.setVec3("tvLight.specular", 1.0f, 1.0f, 1.0f);
        objShader.setFloat("tvLight.constant", 1.0f);
        objShader.setFloat("tvLight.linear", 0.9f);
        objShader.setFloat("tvLight.quadratic", 0.032f);
        objShader.setFloat("tvLight.cutOff", glm::cos(glm::radians(45.0f)));
        objShader.setFloat("tvLight.outerCutOff", glm::cos(glm::radians(60.0f)));


        if (programState->introComplete) {
            // renderovanje baterijske lampe:
            model = CalcFlashlightPosition();
            objShader.setMat4("model", model);
            flashlightModel.Draw(objShader);

            // renderovanje automobila:
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.4f, 0.2f, 1.0f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.9f));
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

        // renderovanje TV-a:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.625f, -40.0f));
        model = glm::rotate(model, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        objShader.setMat4("model", model);
        tvModel.Draw(objShader);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, -40.0f));
        model = glm::rotate(model, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.25f, 0.15f, 0.45f));
        objShader.setMat4("model", model);
        stoolModel.Draw(objShader);

        // renderovanje znaka:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.875f, 0.0f, -41.07f));
        model = glm::rotate(model, glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(30.0f));
        objShader.setMat4("model", model);
        signModel.Draw(objShader);


        // renderovanje kuca:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(14.0f, 0.0f, 10.0f));
        model = glm::scale(model, glm::vec3(0.33f));
        objShader.setMat4("model", model);
        cottageHouseModel.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, 0.01f, -20.0f));
        model = glm::scale(model, glm::vec3(0.05f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        objShader.setMat4("model", model);
        cottageHouseModel2.Draw(objShader);

        // renderovanje deponije:
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, 0.0f, -5.0f));
        model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.12f));
        objShader.setMat4("model", model);
        dumpModel.Draw(objShader);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(13.5f, 0.06f, -9.0f));
        model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.18f));
        objShader.setMat4("model", model);
        trailerModel.Draw(objShader);

        // renderovanje zombija:


        if (uslovi()) { //bas nisam kreativan
            programState->renderuj  = true;
            model = glm::mat4(1.0f);
            model = glm::translate(model, CalcZombiePosition());
            model = glm::rotate(model, glm::radians(130.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.04f));
            objShader.setMat4("model", model);
            zombieModel.Draw(objShader);
    }

        glDisable(GL_CULL_FACE);

        if(programState->introComplete) {
            // renderovanje drveca
            for(int i = 0; i < NR_TREES; i++)
            {
                model = glm::mat4 (1.0f);
                model = glm::translate(model, treePos[i]);
                model = glm::scale(model, glm::vec3(2.0f));
                objShader.setMat4("model", model);
                treeModel.Draw(objShader);
            }

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
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-0.2f, -1.0f, 137.72f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shaderGeometryPass.setMat4("model", model);
            roadModel.Draw(shaderGeometryPass);

            for (unsigned int i = 0; i < NR_LIGHTS; i++) {
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

            glBindVertexArray(podlogaVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        instancedGrass.use();
        instancedGrass.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        instancedGrass.setVec3("dirLight.ambient", glm::vec3(programState->whiteAmbientLightStrength));
        instancedGrass.setVec3("dirLight.diffuse", 0.05f, 0.05f, 0.05);
        instancedGrass.setVec3("dirLight.specular", 0.2f, 0.2f, 0.2f);
        instancedGrass.setInt("texture_diffuse1", 0);
        instancedGrass.setMat4("projection", projection);
        instancedGrass.setMat4("view", view);
        glBindVertexArray(tallgrassVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tallgrassTexture);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, amount);
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);

        // renderovanje svetlecih kutija:
        tvScreenShader.use();
            // svetlo tv-a
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.02f, 0.82f, -40.1f) + glm::vec3(-0.077f, 0.0f, 0.282f));
        model = glm::rotate(model, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 0.216f, 0.34f));
        tvScreenShader.setMat4("model", model);
        tvScreenShader.setMat4("projection", projection);
        tvScreenShader.setMat4("view", view);
        tvScreenShader.setVec3("lightColor", glm::vec3(12.0f, 12.0f, 10.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tvScreenTexture);
        renderCube();

        shaderLightBox.use();
            // bandera
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPositions[0]);
        model = glm::scale(model, glm::vec3(0.38f, 0.1f, 0.28f));
        shaderLightBox.setMat4("model", model);
        shaderLightBox.setMat4("projection", projection);
        shaderLightBox.setMat4("view", view);
        shaderLightBox.setVec3("lightColor", flickerMode[mode] * glm::vec3(11.0f, 11.0f, 5.0f));
        renderCube();

            // point light kocka
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, lightPos);
//        model = glm::scale(model, glm::vec3(0.5f));
//        shaderLightBox.setMat4("model", model);
//        shaderLightBox.setVec3("lightColor", glm::vec3(13.0f, 0.0f, 0.0f));
//        renderCube();

        //object rendering end, start of skybox rendering
        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);

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
            // ANTI-ALIASING: ukljucivanje
            // *************************************************************************************************************
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindVertexArray(screenVAO);
            bool horizontal = true, first_iteration = true;
            unsigned int NR_BLUR_ITERATIONS = 10;
            blurShader.use();
            for (unsigned int i = 0; i < NR_BLUR_ITERATIONS; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
                blurShader.setInt("horizontal", horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
                glDrawArrays(GL_TRIANGLES, 0, 6);
                horizontal = !horizontal;
                if (first_iteration)
                    first_iteration = false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            screenShader.use();

            screenShader.setFloat("SCR_WIDTH", SCR_WIDTH);
            screenShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);
            screenShader.setBool("grayscaleEnabled", programState->grayscaleEnabled);

            screenShader.setBool("hdr", hdr);
            screenShader.setBool("bloom", bloom);
            screenShader.setFloat("exposure", exposure);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorBuffers[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, pingpongColorbuffers[!horizontal]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        }

        if (programState->ImGuiEnabled || programState->exposureWindowEnabled)
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
  
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hdrKeyPressed)
    {
          hdr = !hdr;
          hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
          hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.25f)
             exposure -= 0.005f;
        else
            exposure = 0.25f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        if (exposure < 4.0f)
            exposure += 0.005f;
        else
            exposure = 4.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }


    if(programState->enabledKeyboardInput) {

        if(!programState->creativeMode)         // zakljucava kretanje po y-osi ako nismo u creative modu
            programState->camera.Position.y = 1.75f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(RIGHT, deltaTime);
        if(programState->creativeMode) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                programState->camera.ProcessKeyboard(DOWN, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                programState->camera.ProcessKeyboard(UP, deltaTime);
        }
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        programState->spotlight = !programState->spotlight;
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

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
        programState->creativeMode = !programState->creativeMode;
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if(programState->ImGuiEnabled) {
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(600, 130), ImGuiCond_Once);
            ImGui::Begin("Camera settings", NULL, ImGuiWindowFlags_NoCollapse);
            const Camera &c = programState->camera;
            ImGui::Text("Camera Info:");
            ImGui::Indent();
            ImGui::Bullet();
            ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
            ImGui::Bullet();
            ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
            ImGui::Bullet();
            ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
            ImGui::Unindent();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Text("Toggle camera movement on/off");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3, 0.3, 1.0, 1.0), "(toggle cursor)");
            ImGui::SameLine();
            ImGui::Text(": C");
            ImGui::End();
        }

        {
            ImGui::SetNextWindowPos(ImVec2(0, 170));
            ImGui::SetNextWindowSize(ImVec2(600, 135));
            ImGui::Begin("General settings:", NULL, ImGuiWindowFlags_NoCollapse);
            ImGui::Bullet();
            ImGui::Checkbox("Spectator mode (shortcut: N)", &programState->creativeMode);
            ImGui::SameLine();
            HelpMarker("Allows you to move freely across the scene\nUse SPACE to move upwards and SHIFT to move downwards");
            ImGui::TextColored(ImVec4(1.0, 0.5, 0.5, 1.0), "Note: spectator mode changes will not take effect until intro is complete");
            ImGui::Bullet();
            ImGui::DragFloat("Movement Speed", (float *) &programState->camera.MovementSpeed, 0.5f, 2.5f, 100.0f);
            ImGui::Bullet();
            ImGui::DragFloat("Ambient Light Strength", (float *) &programState->whiteAmbientLightStrength, 0.005f,
                             0.05f, 1.0f);
            ImGui::Bullet();
            ImGui::Text("Toggle flashlight on/off: RMB (Right Click)");
            ImGui::End();
        }

        {
            ImGui::SetNextWindowPos(ImVec2(0, 350));
            ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Once);
            ImGui::Begin("Post-Processing settings", NULL, ImGuiWindowFlags_NoCollapse);
            ImGui::Bullet();
            ImGui::Checkbox("Anti-Aliasing (shortcut: F2)", &programState->AAEnabled);
            ImGui::Bullet();
            ImGui::Checkbox("Grayscale (shortcut: F3)", &programState->grayscaleEnabled);
            ImGui::Bullet();
            ImGui::Checkbox("HDR (shortcut: H)", &hdr);
            ImGui::Bullet();
            ImGui::Checkbox("Bloom (shortcut: B)", &bloom);
            ImGui::Spacing();
            ImGui::Bullet();
            ImGui::DragFloat("Exposure", &exposure, 0.05f, 0.25f, 4.0f);
            ImGui::SameLine();
            HelpMarker("Use Q and E to decrease/increase exposure level", true);
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0, 0.5, 0.5, 1.0), "Note: changes will not take effect until intro is complete");
            ImGui::End();
        }

        {
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - 60, 0), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(60, 50));
            ImGui::Begin("FPS:");
            ImGui::Text(("%.2f"), floor(1 / deltaTime));
            ImGui::End();
        }
    }

    if(programState->exposureWindowEnabled)
    {
        {
            ImGui::SetNextWindowPos(ImVec2(0, SCR_HEIGHT - 25));
            ImGui::SetNextWindowSize(ImVec2(70, 50), ImGuiCond_Always);
            ImGui::Begin("Menu: F1", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse);
            ImGui::End();
        }
        {
            ImGui::SetNextWindowPos(ImVec2(550, 0));
            ImGui::SetNextWindowSize(ImVec2(70, 50), ImGuiCond_Always);
            if(ImGui::IsKeyPressed(GLFW_KEY_Q) || ImGui::IsKeyPressed(GLFW_KEY_E))
                lastChange = glfwGetTime();

            if(ImGui::IsKeyDown(GLFW_KEY_Q) || ImGui::IsKeyDown(GLFW_KEY_E) || !oneSecondPassed(lastChange))
            {
                ImGui::Begin("EXPOSURE", NULL, ImGuiWindowFlags_NoCollapse);
                    ImGui::Text("%.3f", exposure);
                ImGui::End();
            }
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

void updateFlickering()
{
    pulseAccTime += deltaTime;
    if(pulseAccTime > pulseCycleTime)
        pulseAccTime = 0.0f;

    flickerAccTime += deltaTime;
    if(flickerAccTime > flickerCycleTime) {
        flickerCycleTime = (rand() % 100) / 50.0f;
        mode = rand() % 5;
        flickerAccTime = 0.0f;
    }

    flickerMode[0] = (float)((sin(glfwGetTime()) / 2 + 0.5) * cos(rand()));
    flickerMode[1] = 0.2f;
    flickerMode[2] = (float)(0.65 - cos(M_PI * (pulseAccTime / pulseCycleTime)) * 0.5);
    flickerMode[3] = 0.0f;
    flickerMode[4] = 1.0f;
}

bool oneSecondPassed(float lastChange)
{
    if(glfwGetTime() - lastChange > 1.0f)
        return true;
    return false;
}


glm::mat4 CalcFlashlightPosition() {
    Camera c = programState->camera;

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, c.Position + 0.35f * c.Front + 0.07f * c.Right - 0.08f * c.Up);
    model = glm::rotate(model, -glm::radians(c.Yaw + 180), c.Up);
    model = glm::rotate(model, glm::radians(c.Pitch), c.Right);
    model = glm::scale(model, glm::vec3(0.025f));

    return model;
}

glm::vec3 CalcZombiePosition()
{
    if(programState->zombieRendered)
        return zombiePos;
    else if(programState->showZombie)
    {
        zombiePos = programState->camera.Position + glm::vec3(-1.2f, -1.8f, 1.2f);
        programState->zombieRendered = true;
    }

    return zombiePos;
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
static void HelpMarker(const char* desc, bool extraText)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        if(extraText){
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 0.7f), "`What did that sign say again?`");
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool norm(glm::vec3 v1, glm::vec3 v2)
{
    return sqrt(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2) + pow(v1.z - v2.z, 2)) < 6.0f;
}

bool uslovi()
{
    if ((programState->renderuj || exposure <= 0.3f || !bloom) &&
        norm(glm::vec3(2.0f, 0.625f, -40.0f), programState->camera.Position)) {
        programState->odobreno = true;
        return programState->odobreno;
    }
    else if((programState->renderuj || exposure == 0.25f || !bloom) && !norm(glm::vec3(2.0f, 0.625f, -40.0f), programState->camera.Position))
        return programState->odobreno;
    else
        return false;
}