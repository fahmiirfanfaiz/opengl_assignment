#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

// Struktur untuk vertex
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

// Struktur untuk objek 3D
struct Object {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    glm::vec3 position;
    glm::vec3 color;
};

// Fungsi untuk memuat objek OBJ
bool loadOBJ(
    const std::string& path,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices
) {
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<std::vector<int>> faces;

    std::ifstream file(path);
    if (!file) {
        std::cerr << "Tidak dapat membuka file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {  // Vertex position
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (prefix == "vn") {  // Vertex normal
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {  // Face
            std::vector<int> face_indices;
            std::string vertex;
            
            while (iss >> vertex) {
                std::istringstream v_iss(vertex);
                std::string token;
                
                // Format: vertex_idx/texture_idx/normal_idx
                std::getline(v_iss, token, '/');
                int pos_idx = std::stoi(token) - 1;
                
                std::getline(v_iss, token, '/');  // Skip texture coord
                
                std::getline(v_iss, token, '/');
                int normal_idx = std::stoi(token) - 1;
                
                // Cari apakah kombinasi vertex dan normal sudah ada
                bool found = false;
                for (size_t i = 0; i < vertices.size(); i++) {
                    if (vertices[i].Position == temp_positions[pos_idx] && 
                        vertices[i].Normal == temp_normals[normal_idx]) {
                        indices.push_back(i);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    Vertex new_vertex;
                    new_vertex.Position = temp_positions[pos_idx];
                    new_vertex.Normal = temp_normals[normal_idx];
                    vertices.push_back(new_vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }
            
            // Triangulasi wajah jika diperlukan (jika face memiliki lebih dari 3 vertex)
            for (size_t i = 2; i < face_indices.size(); i++) {
                face_indices.push_back(face_indices[0]);
                face_indices.push_back(face_indices[i-1]);
                face_indices.push_back(face_indices[i]);
            }
        }
    }

    return true;
}

// Fungsi untuk membuat shader
unsigned int createShader(const char* vertexPath, const char* fragmentPath) {
    // Baca file shader
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vertexShaderFile;
    std::ifstream fragmentShaderFile;

    vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vertexShaderFile.open(vertexPath);
        fragmentShaderFile.open(fragmentPath);
        std::stringstream vertexStream, fragmentStream;

        vertexStream << vertexShaderFile.rdbuf();
        fragmentStream << fragmentShaderFile.rdbuf();

        vertexShaderFile.close();
        fragmentShaderFile.close();

        vertexCode = vertexStream.str();
        fragmentCode = fragmentStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return 0;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // Kompilasi shader
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    // Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    // Hapus shader karena sudah terhubung ke program dan tidak diperlukan lagi
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shaderProgram;
}

// Callback function untuk resize window
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Callback function untuk keyboard input
void processInput(GLFWwindow* window, bool& perspectiveCamera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Toggle kamera antara perspektif dan orthogonal dengan tombol 'C'
    static bool cPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cPressed) {
            perspectiveCamera = !perspectiveCamera;
            cPressed = true;
            std::cout << "Camera mode switched to " << (perspectiveCamera ? "Perspective" : "Orthographic") << std::endl;
        }
    } else {
        cPressed = false;
    }
}

// Setup objek
void setupObject(Object& obj) {
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);
    
    glBindVertexArray(obj.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER, obj.vertices.size() * sizeof(Vertex), &obj.vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(unsigned int), &obj.indices[0], GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

int main() {
    // Inisialisasi GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Buat window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL 3D Objects with Phong Shading", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Set initial camera mode (perspective)
    bool perspectiveCamera = true;

    // Load shader
    unsigned int shaderProgram = createShader("vertex.vs", "fragment.fs");
    if (shaderProgram == 0) {
        std::cerr << "Failed to create shader program" << std::endl;
        return -1;
    }

    // Load objek 3D
    std::vector<Object> objects;
    
    // Objek 1 - Cube
    Object cube;
    if (!loadOBJ("model1.obj", cube.vertices, cube.indices)) {
        std::cerr << "Failed to load model1.obj" << std::endl;
        return -1;
    }
    cube.position = glm::vec3(-2.0f, 0.0f, 0.0f);
    cube.color = glm::vec3(1.0f, 0.0f, 0.0f); // Merah
    setupObject(cube);
    objects.push_back(cube);
    
    // Objek 2 - Sphere
    Object sphere;
    if (!loadOBJ("model2.obj", sphere.vertices, sphere.indices)) {
        std::cerr << "Failed to load model2.obj" << std::endl;
        return -1;
    }
    sphere.position = glm::vec3(0.0f, 0.0f, 0.0f);
    sphere.color = glm::vec3(0.0f, 1.0f, 0.0f); // Hijau
    setupObject(sphere);
    objects.push_back(sphere);
    
    // Objek 3 - Teapot
    Object teapot;
    if (!loadOBJ("model3.obj", teapot.vertices, teapot.indices)) {
        std::cerr << "Failed to load model3.obj" << std::endl;
        return -1;
    }
    teapot.position = glm::vec3(2.0f, 0.0f, 0.0f);
    teapot.color = glm::vec3(0.0f, 0.0f, 1.0f); // Biru
    setupObject(teapot);
    objects.push_back(teapot);

    // Set up lighting properties
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    
    // Set up material properties
    float ambientStrength = 0.2f;
    float diffuseStrength = 0.7f;
    float specularStrength = 0.5f;
    float shininess = 32.0f;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Input
        processInput(window, perspectiveCamera);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        glUseProgram(shaderProgram);

        // Set light properties
        int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

        // Set material properties
        int ambientStrengthLoc = glGetUniformLocation(shaderProgram, "ambientStrength");
        int diffuseStrengthLoc = glGetUniformLocation(shaderProgram, "diffuseStrength");
        int specularStrengthLoc = glGetUniformLocation(shaderProgram, "specularStrength");
        int shininessLoc = glGetUniformLocation(shaderProgram, "shininess");
        glUniform1f(ambientStrengthLoc, ambientStrength);
        glUniform1f(diffuseStrengthLoc, diffuseStrength);
        glUniform1f(specularStrengthLoc, specularStrength);
        glUniform1f(shininessLoc, shininess);

        // Dapatkan waktu untuk animasi rotasi
        float timeValue = glfwGetTime();
        float rotAngle = timeValue * 50.0f;  // Derajat rotasi per detik

        // Setup view & projection transformations
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -10.0f));
        
        glm::mat4 projection = glm::mat4(1.0f);
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float aspect = (float)width / (float)height;
        
        if (perspectiveCamera) {
            // Perspective projection
            projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        } else {
            // Orthographic projection
            float scale = 5.0f;
            projection = glm::ortho(-scale * aspect, scale * aspect, -scale, scale, 0.1f, 100.0f);
        }
        
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Set camera position for specular calculation
        glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 10.0f);  // Camera position
        int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

        // Render semua objek
        for (size_t i = 0; i < objects.size(); i++) {
            // Model transformation
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, objects[i].position);
            model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            // Set object color
            int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(objects[i].color));

            // Draw object
            glBindVertexArray(objects[i].VAO);
            glDrawElements(GL_TRIANGLES, objects[i].indices.size(), GL_UNSIGNED_INT, 0);
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    for (auto& obj : objects) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
        glDeleteBuffers(1, &obj.EBO);
    }
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}