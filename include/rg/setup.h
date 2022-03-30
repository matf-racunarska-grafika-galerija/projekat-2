//
// Created by vladimir on 28.3.22..
//

#ifndef PROJECT_BASE_SETUP_H
#define PROJECT_BASE_SETUP_H

unsigned int loadCubeMap(vector<std::string> faces);

unsigned int setupSkybox(unsigned int &cubeMapTexture)
{
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

    cubeMapTexture = loadCubeMap(faces);

    return skyboxVAO;
}

unsigned int setupFloorPlane()
{
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
    
    return podlogaVAO;
}

unsigned int setupTallGrass()
{
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
    
    return tallgrassVAO;
}

unsigned int setupAntiAliasing(unsigned int &framebuffer, unsigned int &textureColorBufferMultiSampled, const unsigned int SCR_WIDTH, const unsigned int SCR_HEIGHT)
{
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

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

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

    return quadVAO;
}

unsigned int setupDepthMap(unsigned int &depthCubeMap, const unsigned int SHADOW_WIDTH, const unsigned int SHADOW_HEIGHT)
{
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    for(int i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    // u ovaj framebuffer necemo da renderujemo boju, jer ce da cuva samo dubinu fragmenata
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "setupDepthMap::ERROR::FRAMEBUFFER Framebuffer is not complete!\n";
//    switch (status) {
//        case GL_FRAMEBUFFER_UNDEFINED: std::cerr << "ERROR::FRAMEBUFFER not defined\n"; break;
//        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << "ERROR::FRAMEBUFFER incomplete attachment\n"; break;
//        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << "ERROR::FRAMEBUFFER missing attachment\n"; break;
//        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << "ERROR::FRAMEBUFFER incomplete draw buffer\n"; break;
//        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << "ERROR::FRAMEBUFFER incomplete read buffer\n"; break;
//        case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << "ERROR::FRAMEBUFFER unsupported\n"; break;
//        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: std::cerr << "ERROR::FRAMEBUFFER incomplete multisample\n"; break;
//        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: std::cerr << "ERROR::FRAMEBUFFER incomplete layer targets\n"; break;
//    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return depthMapFBO;
}

unsigned int setupDepthMap2(unsigned int &depth, unsigned int &depthCubeMap, const unsigned int SHADOW_WIDTH, const unsigned int SHADOW_HEIGHT)
{
    // Create the FBO
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    // Create the depth buffer
    glGenTextures(1, &depth);
    glBindTexture(GL_TEXTURE_2D, depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create the cube map
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for (uint i = 0 ; i < 6 ; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

    // Disable writes to the color buffer
    glDrawBuffer(GL_NONE);

    // Disable reads from the color buffer
    glReadBuffer(GL_NONE);

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (Status != GL_FRAMEBUFFER_COMPLETE) {
        printf("FB error, status: 0x%x\n", Status);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return depthMapFBO;
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
#endif //PROJECT_BASE_SETUP_H
