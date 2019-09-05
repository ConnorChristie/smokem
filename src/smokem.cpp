#include "smokem.h"

using std::string;

static Program* ModelProgram;

static std::vector<Object*> objects;
static std::vector<Light> lights;

static glm::vec3 smokeTranslation(-10, 10, 20);

static Program* RaycastProgram;
static Program* LightProgram;
static Program* BlurProgram;

static bool SimulateFluid = true;
static int ViewSamples = GridWidth * 2;
static int LightSamples = GridWidth;

void Smokem::initialize(GLFWwindow* window)
{
    Config cfg = getConfig();

    this->window = window;
    this->camera = new Camera(window, cfg.Width, cfg.Height, glm::vec3(-3, 2, 12));

    ModelProgram = new Program({
        Shader(GL_VERTEX_SHADER, "shaders/model.vert"),
        Shader(GL_FRAGMENT_SHADER, "shaders/model.frag")
    });

    assert(checkError());

    objects.push_back(new Object(ModelProgram->id(), "models/blender/untitled.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(0, 0, 0), 1.0f));
    objects.push_back(new Object(ModelProgram->id(), "models/sonic/sonic-the-hedgehog.obj", glm::vec3(Pi, 0, -Pi / 2.0f), glm::vec3(-30, 1, 0), 0.3f));
    objects.push_back(new Object(ModelProgram->id(), "models/medieval-house/medieval-house-2.obj", glm::vec3(0, 0, -Pi / 2.0f), glm::vec3(10, 4.5f, 0), 3.0f));
    objects.push_back(new Object(ModelProgram->id(), "models/shuttle/space-shuttle-orbiter.obj", glm::vec3(0, 0, -Pi / 2.0f), glm::vec3(50, 8, 65), 0.04f));

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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    initSmoke();
}

static struct {
    SlabPod Velocity;
    SlabPod Density;
    SlabPod Pressure;
    SlabPod Temperature;
} Slabs;

static struct {
    SurfacePod Divergence;
    SurfacePod Obstacles;
    SurfacePod LightCache;
    SurfacePod BlurredDensity;
} Surfaces;

static struct {
    GLuint CubeCenter;
    GLuint FullscreenQuad;
} Vaos;

void Smokem::initSmoke()
{
    RaycastProgram = new Program({
        Shader(GL_VERTEX_SHADER, "shaders/raycast/raycast.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/raycast/raycast.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/raycast/raycast.frag")
    });

    LightProgram = new Program({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/light/cache.frag")
    });

    BlurProgram = new Program({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/light/blur.frag")
    });

    glGenVertexArrays(1, &Vaos.CubeCenter);
    glBindVertexArray(Vaos.CubeCenter);
    CreatePointVbo(0, 0, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGenVertexArrays(1, &Vaos.FullscreenQuad);
    glBindVertexArray(Vaos.FullscreenQuad);
    CreateQuadVbo();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, 2 * sizeof(short), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    InitializeSlabPrograms();

    Slabs.Velocity = CreateSlab(GridWidth, GridHeight, GridDepth, 3);
    Slabs.Density = CreateSlab(GridWidth, GridHeight, GridDepth, 1);
    Slabs.Pressure = CreateSlab(GridWidth, GridHeight, GridDepth, 1);
    Slabs.Temperature = CreateSlab(GridWidth, GridHeight, GridDepth, 1);

    Surfaces.Divergence = CreateVolume(GridWidth, GridHeight, GridDepth, 3);
    Surfaces.LightCache = CreateVolume(GridWidth, GridHeight, GridDepth, 1);
    Surfaces.BlurredDensity = CreateVolume(GridWidth, GridHeight, GridDepth, 1);
    Surfaces.Obstacles = CreateVolume(GridWidth, GridHeight, GridDepth, 3);

    CreateObstacles(Surfaces.Obstacles);
    ClearSurface(Slabs.Temperature.Ping, AmbientTemperature);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Smokem::updateSmoke(float dt)
{
    Config cfg = getConfig();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (SimulateFluid)
    {
        glBindVertexArray(Vaos.FullscreenQuad);
        glViewport(0, 0, GridWidth, GridHeight);

        Advect(Slabs.Velocity.Ping, Slabs.Velocity.Ping, Surfaces.Obstacles, Slabs.Velocity.Pong, VelocityDissipation);
        SwapSurfaces(&Slabs.Velocity);

        Advect(Slabs.Velocity.Ping, Slabs.Temperature.Ping, Surfaces.Obstacles, Slabs.Temperature.Pong, TemperatureDissipation);
        SwapSurfaces(&Slabs.Temperature);

        Advect(Slabs.Velocity.Ping, Slabs.Density.Ping, Surfaces.Obstacles, Slabs.Density.Pong, DensityDissipation);
        SwapSurfaces(&Slabs.Density);

        ApplyBuoyancy(Slabs.Velocity.Ping, Slabs.Temperature.Ping, Slabs.Density.Ping, Slabs.Velocity.Pong);
        SwapSurfaces(&Slabs.Velocity);

        ApplyImpulse(Slabs.Temperature.Ping, ImpulsePosition, ImpulseTemperature);
        ApplyImpulse(Slabs.Density.Ping, ImpulsePosition, ImpulseDensity);
        ComputeDivergence(Slabs.Velocity.Ping, Surfaces.Obstacles, Surfaces.Divergence);
        ClearSurface(Slabs.Pressure.Ping, 0);

        for (int i = 0; i < NumJacobiIterations; ++i)
        {
            Jacobi(Slabs.Pressure.Ping, Surfaces.Divergence, Surfaces.Obstacles, Slabs.Pressure.Pong);
            SwapSurfaces(&Slabs.Pressure);
        }

        assert(checkError());

        SubtractGradient(Slabs.Velocity.Ping, Slabs.Pressure.Ping, Surfaces.Obstacles, Slabs.Velocity.Pong);
        SwapSurfaces(&Slabs.Velocity);
    }
    assert(checkError());
}

void Smokem::renderSmoke()
{
    Config cfg = getConfig();

    glActiveTexture(GL_TEXTURE0);

    // Blur and brighten the density map
    bool BlurAndBrighten = true;
    if (BlurAndBrighten)
    {
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, Surfaces.BlurredDensity.FboHandle);
        glViewport(0, 0, Slabs.Density.Ping.Width, Slabs.Density.Ping.Height);
        glBindVertexArray(Vaos.FullscreenQuad);
        glBindTexture(GL_TEXTURE_3D, Slabs.Density.Ping.ColorTexture);

        GLuint pid = BlurProgram->id();
        glUseProgram(pid);
        SetUniform(pid, "DensityScale", 5.0f);
        SetUniform(pid, "StepSize", sqrtf(2.0) / float(ViewSamples));
        SetUniform(pid, "InverseSize", 1.0f / glm::vec3(GridWidth, GridHeight, GridDepth));

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GridDepth);
    }
    assert(checkError());

    // Generate the light cache
    bool CacheLights = true;
    if (CacheLights)
    {
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, Surfaces.LightCache.FboHandle);
        glViewport(0, 0, Surfaces.LightCache.Width, Surfaces.LightCache.Height);
        glBindVertexArray(Vaos.FullscreenQuad);
        glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity.ColorTexture);

        GLuint pid = LightProgram->id();
        glUseProgram(pid);
        SetUniform(pid, "LightStep", sqrtf(2.0) / float(LightSamples));
        SetUniform(pid, "LightSamples", LightSamples);
        SetUniform(pid, "InverseSize", 1.0f / glm::vec3(GridWidth, GridHeight, GridDepth));

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GridDepth);
    }
    assert(checkError());

    // Perform raycasting
    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, cfg.Width, cfg.Height);

    glBindVertexArray(Vaos.CubeCenter);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity.ColorTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, Surfaces.LightCache.ColorTexture);

    GLuint pid = RaycastProgram->id();
    glUseProgram(pid);
    SetUniform(pid, "Density", 0 /* DENSITY_TEXTURE_LOC */);
    SetUniform(pid, "LightCache", 1 /* LIGHT_CACHE_TEXTURE_LOC */);
    SetUniform(pid, "InverseProjectionMatrix", glm::inverse(camera->getProjectionMatrix()));
    SetUniform(pid, "InverseViewMatrix", glm::inverse(camera->getViewMatrix()));
    SetUniform(pid, "ViewSamples", ViewSamples);
    SetUniform(pid, "RayOrigin", camera->getTranslation());
    SetUniform(pid, "FocalLength", 1.0f / std::tan(camera->getFov() / 2));
    SetUniform(pid, "WindowSize", float(cfg.Width), float(cfg.Height));
    SetUniform(pid, "LightSamples", sqrtf(2) / ViewSamples);

    glDrawArrays(GL_POINTS, 0, 1);
}

void Smokem::update(float dt)
{
    Config cfg = getConfig();

    camera->update(dt);

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
            SetUniform(p->id(), "view_matrix", viewMatrix);
            SetUniform(p->id(), "projection_matrix", projectionMatrix);
        }
        {
            // Lights
            SetUniform(p->id(), "lightPositions", positions);
            SetUniform(p->id(), "lightAmbients", ambients);
            SetUniform(p->id(), "lightDiffuses", diffuses);
            SetUniform(p->id(), "lightSpeculars", speculars);
            SetUniform(p->id(), "lightBrightnesses", brightnesses);
        }
    }

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
            glm::vec3 pos = camera->getTranslation();
            glm::vec3 yawPitchFov = glm::vec3(camera->getYaw(), camera->getPitch(), camera->getFov());

            ImGui::BeginGroup();
            ImGui::PushItemWidth(100);

            bool changed = ImGui::DragFloat("X", &pos.x, 0.2f, NULL, NULL); ImGui::SameLine();
            changed |= ImGui::DragFloat("Y", &pos.y, 0.2f, NULL, NULL); ImGui::SameLine();
            changed |= ImGui::DragFloat("Z", &pos.z, 0.2f, NULL, NULL);

            changed |= ImGui::DragFloat("Yaw", &yawPitchFov.x, 0.2f, NULL, NULL); ImGui::SameLine();
            changed |= ImGui::DragFloat("Pitch", &yawPitchFov.y, 0.2f, NULL, NULL); ImGui::SameLine();
            changed |= ImGui::DragFloat("Fov", &yawPitchFov.z, 0.2f, NULL, NULL);

            ImGui::PopItemWidth();
            ImGui::EndGroup();

            if (changed)
            {
                camera->setTranslation(pos);

                camera->setYaw(yawPitchFov.x);
                camera->setPitch(yawPitchFov.y);
                camera->setFov(yawPitchFov.z);
            }
        }

        if (ImGui::CollapsingHeader("Smoke"))
        {
            ImGui::BeginGroup();
            ImGui::PushItemWidth(100);

            bool changed = ImGui::DragFloat("X", &smokeTranslation.x, 0.2f, NULL, NULL); ImGui::SameLine();
            changed |= ImGui::DragFloat("Y", &smokeTranslation.y, 0.2f, NULL, NULL); ImGui::SameLine();
            changed |= ImGui::DragFloat("Z", &smokeTranslation.z, 0.2f, NULL, NULL);

            ImGui::PopItemWidth();
            ImGui::EndGroup();
        }

        if (ImGui::CollapsingHeader("Objects"))
        {
            for (auto i = 0; i < objects.size(); i++)
            {
                std::string objName = "Object" + std::to_string(i);

                ImGui::BeginGroup();
                if (ImGui::TreeNode(objName.c_str()))
                {
                    glm::vec3 pos = objects.at(i)->getTranslation();

                    ImGui::PushItemWidth(100);
                    bool changed = ImGui::DragFloat("X", &pos.x, 0.2f, NULL, NULL); ImGui::SameLine();
                    changed |= ImGui::DragFloat("Y", &pos.y, 0.2f, NULL, NULL); ImGui::SameLine();
                    changed |= ImGui::DragFloat("Z", &pos.z, 0.2f, NULL, NULL);
                    ImGui::PopItemWidth();

                    if (changed)
                    {
                        objects.at(i)->setTranslation(pos);
                    }
                    ImGui::TreePop();
                }
                ImGui::EndGroup();
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, cfg.Width, cfg.Height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0);

    Program* p = ModelProgram;
    glUseProgram(p->id());

    for (const auto &obj : objects)
    {
        glm::mat3 normMtx = glm::transpose(glm::inverse(glm::mat3(obj->getModelMatrix() * camera->getViewMatrix())));
        SetUniform(p->id(), "normal_matrix", normMtx);
    
        obj->render(p->id());
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    assert(checkError());
}

void Smokem::exit()
{
    delete ModelProgram;
    delete RaycastProgram;
    delete LightProgram;
    delete BlurProgram;
}
