#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Shader.h>
#include <tools/evolution.h>
#include <tools/EvolutionData.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

const float SCREEN_SIZE = 800.00f;
const float PADDING = 50.0f;

MathArray<Square,GENE_SIZE> shared_squares;
std::mutex data_mutex; 
std::atomic<bool> is_running = true; 
std::atomic<bool> is_rendering_enabled = true;
std::atomic<size_t> generation_number = 0;

EvolutionData evolutionData;


void evolution_worker() {
    initializeGenes();
    while (is_running) {
        std::pair<Gene,double> data = evolve_once();
        Gene best_squares = data.first;
        double average_score = data.second;
        evolutionData.pushGeneration(data,generation_number);

        std::lock_guard<std::mutex> guard(data_mutex);
        shared_squares = best_squares.data;
        generation_number++;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // We only want to toggle on the initial press of the key
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        // Toggle the rendering state
        is_rendering_enabled = !is_rendering_enabled;
        if (is_rendering_enabled) {
            std::cout << "Rendering ON" << std::endl;
        } else {
            std::cout << "Rendering OFF" << std::endl;
        }
    }
}

void drawPolygon(Shader& shader, unsigned int VAO, unsigned int VBO, const std::vector<Point>& vertices, glm::mat4& model) {
    if (vertices.size() < 2) {
        return;
    }

    shader.setMat4("model", model);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Point), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINE_LOOP, 0, vertices.size());

    glBindVertexArray(0);
}

int main() {
    cout << filesystem::current_path();


    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version to 3.3 and use the Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(SCREEN_SIZE, SCREEN_SIZE, "epico", NULL, NULL);
    // Inside main(), after glfwCreateWindow(...)
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback); // Register our new callback
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set the viewport
    glViewport(0, 0, SCREEN_SIZE, SCREEN_SIZE);

    // initialize shader
    Shader shader("./src/shaders/vertex.glsl","./src/shaders/fragment.glsl");

    float vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f,  0.5f, 0.0f,
    -0.5f, -0.5f, -0.5f };
    unsigned int VBO;
    unsigned int VAO;
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Point),(void*) 0);
    glEnableVertexAttribArray(0);

    // Main render loop
    initializeGenes();
    /*MathArray<Gene,POPULATION_SIZE> population_info = evolve_once();
    MathArray<Square,GENE_SIZE> squares = population_info[0].data;
    float L = population_info[0].L;
    const float MAGNIFICATION = SCREEN_SIZE / L;
    const float FPS = 60;
    const float S = 0.5;
    const size_t WAIT_FRAME = (size_t) FPS/S;*/
    size_t frame_count = 0;
    size_t i =0;
    std::thread worker(evolution_worker);
// Inside main()
    while (!glfwWindowShouldClose(window)) {
        // Poll for events first to ensure keyboard input is responsive
        glfwPollEvents();

        // Only execute rendering code if the flag is true
        if (is_rendering_enabled) {
            // --- All your existing rendering code goes here ---
            float time = glfwGetTime();
            int time_location = glGetUniformLocation(shader.ID,"time");

            glClearColor(0.2f,0.3f,0.3f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            shader.use();

            glm::mat4 projection = glm::ortho(0.0f, SCREEN_SIZE, 0.0f, SCREEN_SIZE, -1.0f, 1.0f);
            shader.setMat4("projection", projection);
            glm::mat4 mat(1.0f);
            shader.setMat4("view", mat);

            MathArray<Square,GENE_SIZE> squares_to_draw;
            {
                std::lock_guard<std::mutex> guard(data_mutex);
                squares_to_draw = shared_squares;
            } 

            if (!squares_to_draw.empty()) { 
                float L = BOX_SIDE_LENGTH; 
                const float MAGNIFICATION = (SCREEN_SIZE - PADDING * 2) / L; // Adjusted for padding
                for(const Square& sq : squares_to_draw){
                    glm::mat4 model = glm::mat4(1.0f);
                    std::vector<Point> vertices = sq.getVertices();
                    
                    // Adjust model to account for padding
                    model = glm::translate(model, glm::vec3(PADDING, PADDING, 0.0f)); 
                    model = glm::scale(model,glm::vec3(MAGNIFICATION, MAGNIFICATION, 1.0f));
                    drawPolygon(shader, VAO, VBO, vertices, model);
                }
            }
            
            glUniform1f(time_location,time);
            glBindVertexArray(VAO);
            
            // Swap buffers
            glfwSwapBuffers(window);
        }
    }

    // Clean up
    is_running = false;
    worker.join();
    glfwTerminate();
    return 0;
}