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

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

void setShader(Shader ourShader, DirLight dirLight, PointLight pointLight, SpotLight spotLight);


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool spotLightOn = false;
bool pointLightOn = true;
bool plKeyPressed = false;
bool slKeyPressed = false;

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

    DirLight dirLight;
    PointLight pointLight;
    SpotLight spotLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}


    //Skybox
    vector<std::string> faces;
    unsigned int cubemapTexture;

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
   // stbi_set_flip_vertically_on_load(true);


   // dodatak
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   // dodatak

    // dodatak
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    // dodatak

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

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    //light
    // ---------------------------------------------------
    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(20.0f, -10.0f, 50.0f);
    dirLight.ambient = glm::vec3(0.02);
    dirLight.diffuse = glm::vec3(0.3, 0.1, 0.0);
    dirLight.specular = glm::vec3(0.4, 0.3, 0.2);

    PointLight& pointLight = programState->pointLight;
    pointLight.ambient = glm::vec3(1.0, 1.0, 1.0);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.23f;
    pointLight.linear = 0.31f;
    pointLight.quadratic = 0.36f;


    SpotLight& spotLight = programState->spotLight;
    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3 (0.6f);
    spotLight.diffuse = glm::vec3 (0.9f);

    spotLight.specular = glm::vec3 (0.9f, 0.9f, 0.9f);
    spotLight.constant = 0.001f;
    spotLight.linear = 0.006f;
    spotLight.quadratic = 0.004f;

    spotLight.cutOff = glm::cos(glm::radians(2.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(5.0f));

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    // load models
    // -----------

    Model street("resources/objects/road/10563_RoadSectionStraight_v1-L3.obj");
    street.SetShaderTextureNamePrefix("material.");

    Model stopSign("resources/objects/stop-sign/StopSign.obj");
    stopSign.SetShaderTextureNamePrefix("material.");

    Model speedSign("resources/objects/speed-limit-sign/10566_Speed Limit Sign (70 MPH)_v2-L3.obj");
    speedSign.SetShaderTextureNamePrefix("material.");

    Model car("resources/objects/car/source/AbandonedSnowCar/AbandonedSnowCar.fbx");
    car.SetShaderTextureNamePrefix("material.");

    vector<glm::vec3> streetPositions {
            glm::vec3(16.0f,5.0f,10.0f),
            glm::vec3(16.0f,6.75f,8.25f),
            glm::vec3(16.0f,8.5f,6.5f),
            glm::vec3(16.0f,10.25f,4.75f),
            glm::vec3(16.0f,12.0f,3.0f),
            glm::vec3(16.0f,13.75f,1.25f),
            glm::vec3(16.0f,15.5f,-0.5f),
            glm::vec3(16.0f,17.25f,-2.25f),
            glm::vec3(16.0f,19.0f,-4.0f)

    };

    float vertices[] = {
            // positions          // colors           // texture coords
            0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, // top right
            0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // bottom left
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f  // top left
    };


    unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    // Skybox vertices
    //____________________________________________________________________________________________
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

    // parking VAO
    // ----------------------------------------------------------
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox VAO
    // --------------------------------------------------------
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    unsigned int parkingTexture = loadTexture(FileSystem::getPath("resources/textures/parking.png").c_str());

    // skybox textures
    // ---------------------------------------------------------
    programState->faces =
            {
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg"),
            };

    programState->cubemapTexture = loadCubemap(programState->faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        setShader(ourShader, dirLight, pointLight, spotLight);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        ourShader.use();
        glBindTexture(GL_TEXTURE_2D, parkingTexture);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(18.0f, 18.0f, -2.5f));
        model = glm::scale(model, glm::vec3(0.6f));
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        glad_glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glDisable(GL_CULL_FACE);

        ourShader.use();
        glm::mat4 modelStreet = glm::mat4(1.0f);
        for(int i=0; i<streetPositions.size(); i++) {
            ourShader.use();
            modelStreet = glm::mat4(1.0f);
            modelStreet = glm::translate(modelStreet, streetPositions[i]);
            modelStreet = glm::scale(modelStreet, glm::vec3(0.008f));
            modelStreet = glm::rotate(modelStreet, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            modelStreet = glm::rotate(modelStreet, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            ourShader.setMat4("model", modelStreet);
            street.Draw(ourShader);
        }

        ourShader.use();
        glm::mat4 modelStopSign = glm::mat4(1.0f);
        modelStopSign = glm::translate(modelStopSign, glm::vec3(18.75f,7.6f,8.0f));
        modelStopSign = glm::scale(modelStopSign, glm::vec3(0.3f));
        modelStopSign = glm::rotate(modelStopSign, glm::radians(105.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        modelStopSign = glm::rotate(modelStopSign, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        ourShader.setMat4("model", modelStopSign);
        stopSign.Draw(ourShader);

        ourShader.use();
        glm::mat4 modelSpeedSign = glm::mat4(1.0f);
        modelSpeedSign = glm::translate(modelSpeedSign, glm::vec3(13.2f,5.8f,9.0f));
        modelSpeedSign = glm::scale(modelSpeedSign, glm::vec3(0.008f));
        modelSpeedSign = glm::rotate(modelSpeedSign, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        modelSpeedSign = glm::rotate(modelSpeedSign, glm::radians(75.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelSpeedSign = glm::rotate(modelSpeedSign, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        ourShader.setMat4("model", modelSpeedSign);
        speedSign.Draw(ourShader);

        ourShader.use();
        glm::mat4 modelCar = glm::mat4(1.0f);
        modelCar = glm::translate(modelCar, glm::vec3(16.3f,15.0f,1.75f));
        modelCar = glm::rotate(modelCar, glm::radians(20.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        modelCar = glm::rotate(modelCar, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelCar = glm::rotate(modelCar, glm::radians(20.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        modelCar = glm::scale(modelCar, glm::vec3(0.6f));
        ourShader.setMat4("model", modelCar);
        car.Draw(ourShader);

        // draw skybox
        // -------------------------------------------------------------------
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !plKeyPressed){
        pointLightOn = !pointLightOn;
        plKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
    {
        plKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !slKeyPressed){
        spotLightOn = !spotLightOn;
        slKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
    {
        slKeyPressed = false;
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

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }



    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
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

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void setShader(Shader ourShader, DirLight dirLight, PointLight pointLight, SpotLight spotLight) {

    ourShader.use();

    ourShader.setVec3("dirLight.direction", dirLight.direction);
    ourShader.setVec3("dirLight.ambient", dirLight.ambient);
    ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
    ourShader.setVec3("dirLight.specular", dirLight.specular);

    ourShader.setInt("pointLightOn", pointLightOn);

    pointLight.position = glm::vec3(17.0f, 17.0f, -1.0f);

    ourShader.setVec3("pointLight1.position", pointLight.position);
    ourShader.setVec3("pointLight1.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight1.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight1.specular", pointLight.specular);
    ourShader.setFloat("pointLight1.constant", pointLight.constant);
    ourShader.setFloat("pointLight1.linear", pointLight.linear);
    ourShader.setFloat("pointLight1.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);

    pointLight.position = glm::vec3(18.0f, 7.0f, 10.0f);

    ourShader.setVec3("pointLight2.position", pointLight.position);
    ourShader.setVec3("pointLight2.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight2.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight2.specular", pointLight.specular);
    ourShader.setFloat("pointLight2.constant", pointLight.constant);
    ourShader.setFloat("pointLight2.linear", pointLight.linear);
    ourShader.setFloat("pointLight2.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);

    pointLight.position = glm::vec3(13.0f, 3.5f, 11.0f);

    ourShader.setVec3("pointLight3.position", pointLight.position);
    ourShader.setVec3("pointLight3.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight3.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight3.specular", pointLight.specular);
    ourShader.setFloat("pointLight3.constant", pointLight.constant);
    ourShader.setFloat("pointLight3.linear", pointLight.linear);
    ourShader.setFloat("pointLight3.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);

    ourShader.setInt("spotLightOn", spotLightOn);
    ourShader.setVec3("spotLight.position", programState->camera.Position);
    ourShader.setVec3("spotLight.direction", programState->camera.Front);
    ourShader.setVec3("spotLight.ambient", spotLight.ambient);
    ourShader.setVec3("spotLight.diffuse", spotLight.diffuse);
    ourShader.setVec3("spotLight.specular", spotLight.specular);
    ourShader.setFloat("spotLight.constant", spotLight.constant);
    ourShader.setFloat("spotLight.linear", spotLight.linear);
    ourShader.setFloat("spotLight.quadratic", spotLight.quadratic);
    ourShader.setFloat("spotLight.cutOff", spotLight.cutOff);
    ourShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

}
