#include "smokem.h"

using namespace vmath;
using std::string;

static Program* ModelProgram;
static Camera* camera;

static std::vector<Object*> objects;
static std::vector<Light> lights;

void Smokem::initialize(GLFWwindow* window)
{
    Config cfg = getConfig();

    camera = new Camera(cfg.Width, cfg.Height, glm::vec3(-70, 35, 70));

    ModelProgram = new Program({
        Shader(GL_VERTEX_SHADER, "shaders/model.vert"),
        Shader(GL_FRAGMENT_SHADER, "shaders/model.frag")
    });

    assert(checkError());

    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\blender\\untitled.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(0, 0, 0), 1.0f));
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\sonic\\sonic-the-hedgehog.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(-30, 1, 0), 0.3f));
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\medieval-house\\medieval-house-2.obj", glm::vec3(0, 0, -Pi / 2.0f), glm::vec3(10, 4.5f, 0), 3.0f));
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\shuttle\\space-shuttle-orbiter.obj", glm::vec3(0, 0, -Pi / 2.0f), glm::vec3(50, 8, 65), 0.04f));

    lights.push_back({
        glm::vec3(20, 100, 50), // position of the light in the world space.
        glm::vec3(0.1f), // ambient light
        glm::vec3(0.09f), // diffuse light
        glm::vec3(0.04f), // specular light
        1000.0f // brightness
    });

    assert(checkError());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    const char* glsl_version = "#version 150";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void Smokem::update(GLFWwindow* window, long long dt)
{
    assert(checkError());
    Config cfg = getConfig();

    camera->update(window, dt);

    glm::mat4 viewMatrix = camera->getViewMatrix();
    glm::mat4 projectionMatrix = camera->getProjectionMatrix();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> ambients;
    std::vector<glm::vec3> diffuses;
    std::vector<glm::vec3> speculars;
    std::vector<float> brightnesses;

    for (int i = 0; i < lights.size(); i++)
    {
        positions.push_back(lights.at(i).position);
        ambients.push_back(lights.at(i).ambient);
        diffuses.push_back(lights.at(i).diffuse);
        speculars.push_back(lights.at(i).specular);
        brightnesses.push_back(lights.at(i).brightness);
    }

    glUseProgram(ModelProgram->id());
    {
        Program* p = ModelProgram;
        {
            // Camera
            glUniformMatrix4fv(glGetUniformLocation(p->id(), "view_matrix"), 1, false, glm::value_ptr(viewMatrix));
            glUniformMatrix4fv(glGetUniformLocation(p->id(), "projection_matrix"), 1, false, glm::value_ptr(projectionMatrix));
        }
        {
            // Lights
            glUniform3fv(glGetUniformLocation(p->id(), "lightPositions"), lights.size(), glm::value_ptr(positions.front()));
            glUniform3fv(glGetUniformLocation(p->id(), "lightAmbients"), lights.size(), glm::value_ptr(ambients.front()));
            glUniform3fv(glGetUniformLocation(p->id(), "lightDiffuses"), lights.size(), glm::value_ptr(diffuses.front()));
            glUniform3fv(glGetUniformLocation(p->id(), "lightSpeculars"), lights.size(), glm::value_ptr(speculars.front()));
            glUniform1fv(glGetUniformLocation(p->id(), "lightBrightnesses"), lights.size(), &brightnesses.front());
        }
    }

    glEnable(GL_DEPTH_TEST);
    assert(checkError());

    updateGui();
}

void Smokem::updateGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Smokem");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        if (ImGui::CollapsingHeader("Camera"))
        {
            glm::vec3 pos = camera->getPosition();

            ImGui::PushItemWidth(100);
            bool changed = ImGui::DragFloat("X", &pos.x, 0.2f, NULL, NULL); ImGui::SameLine();
            changed = changed || ImGui::DragFloat("Y", &pos.y, 0.2f, NULL, NULL); ImGui::SameLine();
            changed = changed || ImGui::DragFloat("Z", &pos.z, 0.2f, NULL, NULL);
            ImGui::PopItemWidth();

            if (changed)
            {
                camera->setPosition(pos);
            }
        }

        if (ImGui::CollapsingHeader("Objects"))
        {
            for (auto i = 0; i < objects.size(); i++)
            {
                std::string objName = "Object" + std::to_string(i);

                if (ImGui::TreeNode(objName.c_str()))
                {
                    glm::vec3 pos = objects.at(i)->getTranslation();

                    ImGui::PushItemWidth(100);
                    bool changed = ImGui::DragFloat("X", &pos.x, 0.2f, NULL, NULL); ImGui::SameLine();
                    changed = changed || ImGui::DragFloat("Y", &pos.y, 0.2f, NULL, NULL); ImGui::SameLine();
                    changed = changed || ImGui::DragFloat("Z", &pos.z, 0.2f, NULL, NULL);
                    ImGui::PopItemWidth();

                    if (changed)
                    {
                        objects.at(i)->setTranslation(pos);
                    }

                    ImGui::TreePop();
                }
            }
        }

        ImGui::End();
    }

    //ImGui::ShowDemoWindow((bool*)true);
}

void Smokem::render()
{
    assert(checkError());
    Config cfg = getConfig();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, cfg.Width, cfg.Height);
    glClearColor(0, 0, 0, 1);

    Program* p = ModelProgram;
    glUseProgram(p->id());

    for (const auto &obj : objects)
    {
        glm::mat3 normMtx = glm::transpose(glm::inverse(glm::mat3(obj->getModelMatrix() * camera->getViewMatrix())));
        glUniformMatrix3fv(glGetUniformLocation(p->id(), "normal_matrix"), 1, false, glm::value_ptr(normMtx));
    
        obj->render(p->id());
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    assert(checkError());
}
