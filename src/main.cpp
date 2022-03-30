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
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;

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

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
    glm::vec3 tempPosition=glm::vec3(0.0f, 2.0f, 0.0f);
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
    }
}
ProgramState *programState;

void DrawImGui(ProgramState *programState);

glm::mat4 CalcFlashlightPosition();

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
    Shader objShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader screenShader("resources/shaders/anti-aliasing.vs", "resources/shaders/anti-aliasing.fs");    // za ANTI-ALIASING
    Shader simpleDepthShader("resources/shaders/depthShader.vs", "resources/shaders/depthShader.fs", "resources/shaders/depthShader.gs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    Shader shaderGeometryPass("resources/shaders/8.1.g_buffer.vs", "resources/shaders/8.1.g_buffer.fs");
    Shader shaderLightingPass("resources/shaders/8.1.deferred_shading.vs", "resources/shaders/8.1.deferred_shading.fs");
    Shader shaderLightBox("resources/shaders/8.1.deferred_light_box.vs", "resources/shaders/8.1.deferred_light_box.fs");
    

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

    //podloga
    unsigned int podlogaVAO = setupFloorPlane();

    // skybox
    unsigned int cubeMapTexture;
    unsigned int skyboxVAO = setupSkybox(cubeMapTexture);

    // uspravna trava
    unsigned int tallgrassVAO = setupTallGrass();
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
    unsigned int podlogaDiffuseMap = TextureFromFile("grass_diffuse.png", "resources/textures");
    unsigned int podlogaSpecularMap = TextureFromFile("grass_specular.png", "resources/textures");
    unsigned int tallgrassTexture = TextureFromFile("grass.png", "resources/textures/");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // depthMap
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthCubeMap;
    //unsigned int depthMapFBO = setupDepthMap(depthCubeMap, SHADOW_WIDTH, SHADOW_HEIGHT);

    unsigned int depth;
    unsigned int depthMapFBO = setupDepthMap2(depth, depthCubeMap, SHADOW_WIDTH, SHADOW_HEIGHT);

    // ANTI-ALIASING
    unsigned int framebuffer, textureColorBufferMultiSampled;
    unsigned int quadVAO = setupAntiAliasing(framebuffer, textureColorBufferMultiSampled, SCR_WIDTH, SCR_HEIGHT);

    // konfiguracija shadera
    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    objShader.use();
    objShader.setInt("material.texture_diffuse1", 0);
    objShader.setInt("material.texture_specular1", 1);
    objShader.setInt("depthMap", 2);


    // ostale konfiguracije i inicijalizacije
    glm::vec3 lightPos(-5.0f, 4.0f, -5.0f);
    glm::mat4 model = glm::mat4(1.0f);

    // configure g-buffer framebuffer
    // ------------------------------
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
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);

    const unsigned int NR_LIGHTS = 10;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        lightPositions.push_back(glm::vec3(-0.4f, 6.55f, i * 5.0f));
        float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }


    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        shaderGeometryPass.use();
        shaderGeometryPass.setMat4("projection", projection);
        shaderGeometryPass.setMat4("view", view);
        for (unsigned int i = 0; i < 10; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 5.0f));
            model = glm::scale(model, glm::vec3(0.5f));
            shaderGeometryPass.setMat4("model", model);
            ulicnaSvetiljkaModel.Draw(shaderGeometryPass);
        }
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
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
            // update attenuation parameters and calculate radius
            const float linear = 0.7;
            const float quadratic = 1.8;
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
        }
        shaderLightingPass.setVec3("viewPos", programState->camera.Position);
        // finally render quad
        renderQuad();

        // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // ----------------------------------------------------------------------------------
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
        // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
        // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. render lights on top of scene
        // --------------------------------
        shaderLightBox.use();
        shaderLightBox.setMat4("projection", projection);
        shaderLightBox.setMat4("view", view);
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPositions[i]);
            model = glm::scale(model, glm::vec3(0.125f));
            shaderLightBox.setMat4("model", model);
            shaderLightBox.setVec3("lightColor", lightColors[i]);
            renderCube();
        }



//        // renderovanje scene iz pozicije svetla
//        // -------------------------------------------
//        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
//        glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
//
//        float near_plane = 1.0f;
//        float far_plane  = 25.0f;
//        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
//        std::vector<glm::mat4> shadowTransforms;
//        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
//        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
//        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
//        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
//        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
//        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
//
//        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
//        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
//            glClear(GL_DEPTH_BUFFER_BIT);
//
//            simpleDepthShader.use();
//            for(unsigned int i = 0; i < 6; i++)
//                simpleDepthShader.setMat4("shadowMatrices[" + std::to_string((i)) +"]", shadowTransforms[i]);
//            simpleDepthShader.setFloat("far_plane", far_plane);
//            simpleDepthShader.setVec3("lightPos", lightPos);
//
//            // renderovanje ranca:
//            model = glm::mat4(1.0f);
//            model = glm::translate(model,programState->tempPosition);
//            model = glm::scale(model, glm::vec3(programState->tempScale));
//            simpleDepthShader.setMat4("model", model);
//            ourModel.Draw(simpleDepthShader);
//
//
//            //renderovanje lampe
//            for(int i = 0; i < 5; i++) {
//                model = glm::mat4(1.0f);
//                model = glm::translate(model, glm::vec3(-4.0f, 0.0f, i * 4.0f));
//                model = glm::scale(model, glm::vec3(0.4f));
//                model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0, 1, 0));
//                simpleDepthShader.setMat4("model", model);
//                ulicnaSvetiljkaModel.Draw(simpleDepthShader);
//            }
//
//            //podloga
//
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, podlogaDiffuseMap);
//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, podlogaSpecularMap);
//
//            model = glm::mat4(1.0f);
//            simpleDepthShader.setMat4("model", model);
//
//            //za blinfonga treba shinnes 4* veci al trava ne sme da se presijava bas tako da tu treba obratiti paznju
//            //simpleDepthShader.setFloat("material.shininess", 32.0f);
//            glBindVertexArray(podlogaVAO);
//            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//
//        glBindFramebuffer(GL_FRAMEBUFFER, 0);
//        // -------------------------------------------
//
//        // render
//
//        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//        // ANTI-ALIASING: preusmeravamo renderovanje na nas framebuffer da bismo imali MSAA
//        // *************************************************************************************************************
//        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
//        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        glEnable(GL_DEPTH_TEST);
//        // *************************************************************************************************************
//
//
//        //object shader
//        objShader.use();
//        objShader.setVec3("viewPosition", programState->camera.Position);
//        objShader.setFloat("material.shininess", 32.0f);
//
//        // view/projection transformations
//        projection = glm::perspective(glm::radians(programState->camera.Zoom),
//                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
//        view = programState->camera.GetViewMatrix();
//        objShader.setMat4("projection", projection);
//        objShader.setMat4("view", view);
//
//
//        // directional light
//        objShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
//        objShader.setVec3("dirLight.ambient", glm::vec3(programState->whiteAmbientLightStrength));
//        objShader.setVec3("dirLight.diffuse", 0.05f, 0.05f, 0.05);
//        objShader.setVec3("dirLight.specular", 0.2f, 0.2f, 0.2f);
//
//        //ovde treba pointlajtovi
//        //nasao sam kul nacin da obradjujemo vise pointlatova ako nam treba
//
//        // jedan pointlajt za testiranje senki
////        lightPos.x = sin(glfwGetTime()) * 3.0f;
////        lightPos.z = cos(glfwGetTime()) * 2.0f;
////        lightPos.y = 5.0 + cos(glfwGetTime()) * 1.0f;
//        objShader.setVec3("pointLight.position", lightPos);
//        objShader.setVec3("pointLight.ambient", glm::vec3(1.0f));
//        objShader.setVec3("pointLight.diffuse", 0.05f, 0.05f, 0.05);
//        objShader.setVec3("pointLight.specular", 0.2f, 0.2f, 0.2f);
//        objShader.setFloat("pointLight.constant", 1.0f);
//        objShader.setFloat("pointLight.linear", 0.09f);
//        objShader.setFloat("pointLight.quadratic", 0.032f);
//
//        // spotlight - baterijska lampa
//        objShader.setVec3("lampa.position", programState->camera.Position + 0.35f * programState->camera.Front + 0.07f * programState->camera.Right - 0.08f * programState->camera.Up);
//        objShader.setVec3("lampa.direction", programState->camera.Front);
//        objShader.setVec3("lampa.ambient", 0.0f, 0.0f, 0.0f);
//        if(programState->spotlight) {
//            objShader.setVec3("lampa.diffuse", 1.0f, 1.0f, 1.0f);
//            objShader.setVec3("lampa.specular", 1.0f, 1.0f, 1.0f);
//        }
//        else {
//            objShader.setVec3("lampa.diffuse", 0.0f, 0.0f, 0.0f);
//            objShader.setVec3("lampa.specular", 0.0f, 0.0f, 0.0f);
//        }
//        objShader.setFloat("lampa.constant", 1.0f);
//        objShader.setFloat("lampa.linear", 0.09f);
//        objShader.setFloat("lampa.quadratic", 0.032f);
//        objShader.setFloat("lampa.cutOff", glm::cos(glm::radians(10.0f)));
//        objShader.setFloat("lampa.outerCutOff", glm::cos(glm::radians(15.0f)));
//
//        objShader.setFloat("far_plane", far_plane);
//
//        // renderovanje ranca:
//        model = glm::mat4(1.0f);
//        model = glm::translate(model,programState->tempPosition);
//        model = glm::scale(model, glm::vec3(programState->tempScale));
//        objShader.setMat4("model", model);
//        ourModel.Draw(objShader);
//
//        // renderovanje baterijske lampe:
//        model = CalcFlashlightPosition();
//        objShader.setMat4("model", model);
//        flashlightModel.Draw(objShader);
//
//
//
//        glDisable(GL_CULL_FACE);
//        // renderovanje kuce:
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(-20, 0.01, -20));
//        model = glm::scale(model, glm::vec3(0.7f));
//        objShader.setMat4("model", model);
//        houseModel.Draw(objShader);
//
//        // renderovanje supe:
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(25, 0, 25));
//        model = glm::scale(model, glm::vec3(0.3));
//        objShader.setMat4("model", model);
//        cottageHouseModel.Draw(objShader);
//
//        glBindVertexArray(tallgrassVAO);
//        glBindTexture(GL_TEXTURE_2D, tallgrassTexture);
//        for (unsigned int i = 0; i < vegetation.size(); i++)
//        {
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, vegetation[i]);
//            //model = glm::rotate(model, (float)i*60.0f, glm::vec3(0.0, 0.1, 0.0));
//            objShader.setMat4("model", model);
//            objShader.setMat4("projection", projection);
//            objShader.setMat4("view", view);
//            glDrawArrays(GL_TRIANGLES, 0, 6);
//        }
//
//        //podloga
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, podlogaDiffuseMap);
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, podlogaSpecularMap);
//        glActiveTexture(GL_TEXTURE2);
//        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
//
//        model = glm::mat4(1.0f);
//        objShader.setMat4("model", model);
//
//        //za blinfonga treba shinnes 4* veci al trava ne sme da se presijava bas tako da tu treba obratiti paznju
//        //objShader.setFloat("material.shininess", 32.0f);
//        glBindVertexArray(podlogaVAO);
//        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//        glEnable(GL_CULL_FACE);
//
//
//        //object rendering end, start of skybox rendering
//        skyboxShader.use();
//        skyboxShader.setInt("skybox", 0);
//
//        glDepthMask(GL_FALSE);
//        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
//
//        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
//        skyboxShader.setMat4("view", view);
//        skyboxShader.setMat4("projection", projection);
//
//        glBindVertexArray(skyboxVAO);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
//        glDrawArrays(GL_TRIANGLES, 0, 36);
//        glBindVertexArray(0);
//        glDepthMask(GL_TRUE);
//        glDepthFunc(GL_LESS); // set depth function back to default
//
//
//        // ANTI-ALIASING: ukljucivanje
//        // *************************************************************************************************************
//        glBindFramebuffer(GL_FRAMEBUFFER, 0);
//        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//        glDisable(GL_DEPTH_TEST);
//
//        screenShader.use();
//
//        screenShader.setFloat("SCR_WIDTH", SCR_WIDTH);
//        screenShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);
//        screenShader.setBool("grayscaleEnabled", programState->grayscaleEnabled);
//
//        glBindVertexArray(quadVAO);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
//        glDrawArrays(GL_TRIANGLES, 0, 6);
//        // *************************************************************************************************************
//
//        if (programState->ImGuiEnabled)
//            DrawImGui(programState);

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
    model = glm::translate(model, c.Position + 0.35f * c.Front + 0.07f * c.Right - 0.08f * c.Up);

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
