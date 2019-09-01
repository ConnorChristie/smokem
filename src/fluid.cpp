#include "fluid.h"

using namespace vmath;
using std::string;

static Program* ModelProgram;
static Camera* camera;

static std::vector<Object*> objects;
static std::vector<Light> lights;

PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = "Fluid3d";
    config.Width = 1920;
    config.Height = 1080;
    config.Multisampling = 4;
    config.VerticalSync = 1;
    return config;
}

void PezInitialize(GLFWwindow* window)
{
    PezConfig cfg = PezGetConfig();

    camera = new Camera(cfg.Width, cfg.Height);

    ModelProgram = new Program({
        Shader(GL_VERTEX_SHADER, "shaders/model.vert"),
        Shader(GL_FRAGMENT_SHADER, "shaders/model.frag")
    });

    assert(checkError());

    glGenVertexArrays(1, &Vaos.CubeCenter);
    glBindVertexArray(Vaos.CubeCenter);
    CreatePointVbo(0, 0, 0);
    glEnableVertexAttribArray(SlotPosition);
    glVertexAttribPointer(SlotPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGenVertexArrays(1, &Vaos.FullscreenQuad);
    glBindVertexArray(Vaos.FullscreenQuad);
    CreateQuadVbo();
    glEnableVertexAttribArray(SlotPosition);
    glVertexAttribPointer(SlotPosition, 2, GL_SHORT, GL_FALSE, 2 * sizeof(short), 0);

    //obj = new Object(ModelProgram, "D:\\Git\\opengl-tutorial\\models\\torch\\torch.obj", glm::vec3(0), glm::vec3(2, 0, 5), 2.0f);
    //obj = new Object(ModelProgram, "D:\\Git\\opengl-tutorial\\models\\shuttle\\space-shuttle-orbiter.obj", glm::vec3(0), glm::vec3(0, 0, 8), 0.1f);
    //obj = new Object(ModelProgram, "D:\\Git\\opengl-tutorial\\models\\standard-male\\standard-male-figure.obj", glm::vec3(0), glm::vec3(0, 0, 8), 0.1f);
    //obj = new Object(ModelProgram, "D:\\Git\\opengl-tutorial\\models\\tenryuu\\light-cruiser-tenryuu.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(0, 0, 5), 1.5f);
    //obj = new Object(ModelProgram, "D:\\Git\\opengl-tutorial\\models\\revil\\resident-evil-racoon-city-party-girl.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(0, 0, 5), 1.0f);
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\blender\\untitled.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(0, 0, 0), 1.0f));
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\sonic\\sonic-the-hedgehog.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(-27, -13, 0), 0.3f));
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\medieval-house\\medieval-house-2.obj", glm::vec3(0, 0, -Pi / 2.0f), glm::vec3(10, 1.5f, 0), 3.0f));
    objects.push_back(new Object(ModelProgram->id(), "D:\\Git\\opengl-tutorial\\models\\shuttle\\space-shuttle-orbiter.obj", glm::vec3(0, 0, -Pi / 2.0f), glm::vec3(42, -48, 280), 0.04f));

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

void PezUpdate(GLFWwindow* window, long long dt)
{
    assert(checkError());
    PezConfig cfg = PezGetConfig();

    camera->update(window, dt);

    glm::mat4 viewMatrix = camera->getView();
    glm::mat4 projectionMatrix = camera->getProjection();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, cfg.Width, cfg.Height);
    glClearColor(0, 0, 0, 1);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> ambients;
    std::vector<glm::vec3> diffuses;
    std::vector<glm::vec3> speculars;
    std::vector<float> brightnesses;

    for (int i = 0; i < lights.size(); i++)
    {
        positions.push_back(lights.at(i).mPosition);
        ambients.push_back(lights.at(i).mAmbient);
        diffuses.push_back(lights.at(i).mDiffuse);
        speculars.push_back(lights.at(i).mSpecular);
        brightnesses.push_back(lights.at(i).mBrightness);
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

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Smokem");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        if (ImGui::CollapsingHeader("Objects"))
        {
            for (auto i = 0; i < objects.size(); i++)
            {
                std::string objName = "Object" + std::to_string(i);

                if (ImGui::TreeNode(objName.c_str()))
                {
                    glm::vec3 pos = objects.at(i)->getTranslation();
                    //float x = pos.x, y = pos.y, z = pos.z;

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

    assert(checkError());
}

void PezRender()
{
    assert(checkError());
    PezConfig cfg = PezGetConfig();

    Program* p = ModelProgram;
    glUseProgram(p->id());

    for (const auto obj : objects)
    {
        glm::mat3 normMtx = glm::transpose(glm::inverse(glm::mat3(obj->getModelMatrix() * camera->getView())));
        glUniformMatrix3fv(glGetUniformLocation(p->id(), "normal_matrix"), 1, false, glm::value_ptr(normMtx));
    
        obj->render(p->id(), true);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    assert(checkError());
}
