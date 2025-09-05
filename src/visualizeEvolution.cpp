#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Shader.h>
#include <tools/evolution.h>
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
void evolution_worker() {
    initializePopulation();
    while (is_running) {
        MathArray<Gene,POPULATION_SIZE> population_info = evolve_once();
        MathArray<Square,GENE_SIZE> best_squares = population_info[0].data;
        std::lock_guard<std::mutex> guard(data_mutex);
        shared_squares = best_squares;
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
    initializePopulation();
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
    while (!glfwWindowShouldClose(window)) {

        float time = glfwGetTime();
        int time_location = glGetUniformLocation(shader.ID,"time");
        // Rendering
        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();

        glm::mat4 projection = glm::ortho(0.0f, SCREEN_SIZE, 0.0f, SCREEN_SIZE, -1.0f, 1.0f);
        glm::mat4 view = glm::mat4(1.0f); 

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        // Move it to position (200, 300)
        //model1 = glm::translate(model1, glm::vec3(200.0f, 300.0f, 0.0f));
        MathArray<Square,GENE_SIZE> squares_to_draw;
        {
            std::lock_guard<std::mutex> guard(data_mutex);
            squares_to_draw = shared_squares;
        } 

        if (!squares_to_draw.empty()) { 
            float L = 4.75f; 
            const float MAGNIFICATION = SCREEN_SIZE / L;
            for(const Square& sq : squares_to_draw){
                glm::mat4 model = glm::mat4(1.0f);
                std::vector<Point> vertices = sq.getVertices();
                //model = glm::translate(model, glm::vec3(MAGNIFICATION,MAGNIFICATION, 0.0f));
                model = glm::scale(model,glm::vec3(MAGNIFICATION, MAGNIFICATION, 1.0f));
                drawPolygon(shader, VAO, VBO, vertices, model);
            }
        }
        /*for(const Square& sq : population_info[i].data){
            model = glm::mat4(1.0f);
            std::vector<Point> vertices = sq.getVertices();
            model = glm::scale(model,glm::vec3(MAGNIFICATION,MAGNIFICATION,5.0f));
            drawPolygon(shader, VAO, VBO, vertices, model);
        }*/
        
        glUniform1f(time_location,time);
        glBindVertexArray(VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        
        //glClearColor(0.2f, 0.5f, 0.3f, 1.0f); // Green background
        //glClear(GL_COLOR_BUFFER_BIT);

        // Swap buffers and poll for events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    is_running = false;
    worker.join();
    glfwTerminate();
    return 0;
}