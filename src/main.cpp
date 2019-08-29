#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "fluid.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::chrono::time_point<std::chrono::steady_clock> GetMicroseconds()
{
    return std::chrono::high_resolution_clock::now();
}

struct vertex {
    tinyobj::real_t x, y, z;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        return;
}

static bool MouseDown = false;
static const float DefaultThetaX = 0;
static const float DefaultThetaY = 0;
static float ThetaX = DefaultThetaX;
static float ThetaY = DefaultThetaY;

static void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    static int StartX, StartY;
    static const float Speed = 0.005f;
    if (action == GLFW_PRESS) {
        StartX = x;
        StartY = y;
        MouseDown = true;
    }
    else if (MouseDown) {
        ThetaX = DefaultThetaX + Speed * (x - StartX);
        ThetaY = DefaultThetaY + Speed * (y - StartY);
    }
    else if (action == GLFW_RELEASE) {
        MouseDown = false;
    }
}

GLuint loadTexture(std::string path)
{
    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(image);
    return texture;
}

GLuint emptyTexture()
{
    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLubyte data[] = { 255, 255, 255, 255 };

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return texture;
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    if (!glfwInit())
    {
        return -1;
    }

    std::string inputfile = "D:\\Git\\opengl-tutorial\\models\\revil\\resident-evil-racoon-city-party-girl.obj";
    std::string base_dir = "D:\\Git\\opengl-tutorial\\models\\revil\\";

    /*std::string inputfile = "D:\\Git\\opengl-tutorial\\models\\standard-male\\standard-male-figure.obj";
    std::string base_dir = "D:\\Git\\opengl-tutorial\\models\\standard-male\\";*/

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str(), base_dir.c_str());

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        exit(1);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(PezGetConfig().Width, PezGetConfig().Height, PezGetConfig().Title, NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    //PezInitialize();

    auto previousTime = GetMicroseconds();

    auto program = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/model.vert"),
        Shader(GL_FRAGMENT_SHADER, "shaders/model.frag")
    });

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textures;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < attrib.vertices.size(); i += 3) {
        glm::vec3 vertex;
        vertex.x = attrib.vertices[i];
        vertex.y = attrib.vertices[i + 1];
        vertex.z = attrib.vertices[i + 2];
        vertices.push_back(vertex);
    }
    for (unsigned int i = 0; i < attrib.normals.size(); i += 3) {
        glm::vec3 normal;
        normal.x = attrib.normals[i];
        normal.y = attrib.normals[i + 1];
        normal.z = attrib.normals[i + 2];
        normals.push_back(normal);
    }
    for (unsigned int i = 0; i < attrib.texcoords.size(); i += 2) {
        glm::vec2 texture;
        texture.x = attrib.texcoords[i];
        texture.y = attrib.texcoords[i + 1];
        textures.push_back(texture);
    }
    for (const auto shape : shapes)
    {
        for (unsigned int i = 0; i < shape.mesh.indices.size(); i++) {
            indices.push_back(shape.mesh.indices[i].vertex_index);

            /*int ti = shape.mesh.indices[i].texcoord_index;
            glm::vec2 texcoord;

            if (ti != -1)
            {
                texcoord.x = attrib.texcoords[ti];
                texcoord.y = attrib.texcoords[ti + 1];
                textures.push_back(texcoord);
            }
            else
            {
                texcoord.x = 0;
                texcoord.y = 0;
            }*/

        }
    }

    GLuint VBO = 0;
    GLuint VBO_tex = 0;
    GLuint VBO_normal = 0;
    GLuint VAO = 0;
    GLuint EBO = 0;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &VBO_tex);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_tex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * textures.size(), &textures[0], GL_STATIC_DRAW);

    glGenBuffers(1, &VBO_normal);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * normals.size(), &normals[0], GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_tex);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint tex_empty = emptyTexture();
    GLuint tex_torso = loadTexture("D:\\Git\\opengl-tutorial\\models\\revil\\partygirltorso_d.png");
    GLuint tex_leg = loadTexture("D:\\Git\\opengl-tutorial\\models\\revil\\partygirlleg_d.png");
    GLuint tex_hair = loadTexture("D:\\Git\\opengl-tutorial\\models\\revil\\partygirlhair_d.png");
    GLuint tex_head = loadTexture("D:\\Git\\opengl-tutorial\\models\\revil\\partygirlhead_d.png");

    const Point3 eye(0, 0, 4);
    const Vector3 up(1, 0, 0);
    const Point3 target(1, 0, 0);

    float FieldOfView = 0.7f;

    while (!glfwWindowShouldClose(window))
    {
        auto currentTime = GetMicroseconds();
        auto deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        glViewport(0, 0, PezGetConfig().Width, PezGetConfig().Height);

        Matrices.Modelview = Matrix4::lookAt(eye, target, up);
        Matrices.Modelview *= Matrix4::rotationX(ThetaX);
        Matrices.Modelview *= Matrix4::rotationY(ThetaY);
        Matrices.Modelview *= Matrix4::rotationZ(-Pi / 2.0f);
        
        Matrices.Projection = Matrix4::perspective(
            FieldOfView,
            float(PezGetConfig().Width) / PezGetConfig().Height, // Aspect Ratio
            0.0f,   // Near Plane
            1.0f);  // Far Plane
        Matrices.ModelviewProjection = Matrices.Projection * Matrices.Modelview;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        SetUniform("ModelviewProjection", Matrices.ModelviewProjection);

        glBindVertexArray(VAO);

        size_t offset = 0;

        for (auto i = 0; i < shapes.size(); i++)
        {
            const auto shape = shapes[i];

            glBindTexture(GL_TEXTURE_2D, tex_hair);
            glDrawElements(GL_TRIANGLES, shape.mesh.indices.size(), GL_UNSIGNED_INT, (void*)(offset * sizeof(GLuint)));

            offset += shape.mesh.indices.size();

            // THIS CAUSES a full screen malfunction! (could be a virus)
            // glDrawElements(GL_TRIANGLES, shape.mesh.indices.size(), GL_UNSIGNED_INT, (void*)prev_shape_size);
        }

        // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex
        // glDrawElementsBaseVertex(GL_TRIANGLES, shapes[0].mesh.indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, 0, 0);

        //PezUpdate(deltaTime.count());
        //PezRender();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}