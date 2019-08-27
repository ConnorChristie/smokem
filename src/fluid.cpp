#include "fluid.h"
#include <iostream>

using namespace vmath;
using std::string;

static const Point3 EyePosition = Point3(0, 0, 2);
static GLuint RaycastProgram;
static GLuint LightProgram;
static GLuint BlurProgram;
static float FieldOfView = 0.7f;
static bool SimulateFluid = true;
static const float DefaultThetaX = 0;
static const float DefaultThetaY = 0.75f;
static float ThetaX = DefaultThetaX;
static float ThetaY = DefaultThetaY;
static int ViewSamples = 512;
static int LightSamples = 256;

const float ImpulseTemperature = 5.0f;
const float ImpulseDensity = 1.25f;
const float TemperatureDissipation = 0.99f;
const float VelocityDissipation = 0.99f;
const float DensityDissipation = 0.999f;

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

void PezInitialize()
{
    RaycastProgram = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/raycast/raycast.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/raycast/raycast.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/raycast/raycast.frag")
    });

    LightProgram = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/light/cache.frag")
    });

    BlurProgram = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/light/blur.frag")
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

    assert(checkError());
}

void PezUpdate(long long dt)
{
    assert(checkError());
    PezConfig cfg = PezGetConfig();

    Vector3 up(1, 0, 0); Point3 target(0);
    Matrices.View = Matrix4::lookAt(EyePosition, target, up);

    Matrix4 modelMatrix = Matrix4::identity();
    modelMatrix *= Matrix4::rotationX(ThetaX);
    modelMatrix *= Matrix4::rotationY(ThetaY);

    Matrices.Modelview = Matrices.View * modelMatrix;
    Matrices.Projection = Matrix4::perspective(
        FieldOfView,
        float(cfg.Width) / cfg.Height, // Aspect Ratio
        0.0f,   // Near Plane
        1.0f);  // Far Plane
    Matrices.ModelviewProjection = Matrices.Projection * Matrices.Modelview;

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

void PezRender()
{
    assert(checkError());
    PezConfig cfg = PezGetConfig();

    glActiveTexture(GL_TEXTURE0);

    // Blur and brighten the density map:
    bool blurAndBrighten = true;
    if (blurAndBrighten)
    {
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, Surfaces.BlurredDensity.FboHandle);
        glViewport(0, 0, Slabs.Density.Ping.Width, Slabs.Density.Ping.Height);
        glBindVertexArray(Vaos.FullscreenQuad);
        glBindTexture(GL_TEXTURE_3D, Slabs.Density.Ping.ColorTexture);

        glUseProgram(BlurProgram);
        SetUniform("DensityScale", 5.0f);
        SetUniform("StepSize", sqrtf(2.0) / float(ViewSamples));
        SetUniform("InverseSize", recipPerElem(Vector3(float(GridWidth), float(GridHeight), float(GridDepth))));

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GridDepth);
    }
    assert(checkError());

    // Generate the light cache:
    bool cacheLights = true;
    if (cacheLights)
    {
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, Surfaces.LightCache.FboHandle);
        glViewport(0, 0, Surfaces.LightCache.Width, Surfaces.LightCache.Height);
        glBindVertexArray(Vaos.FullscreenQuad);
        glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity.ColorTexture);

        glUseProgram(LightProgram);
        SetUniform("LightStep", sqrtf(2.0) / float(LightSamples));
        SetUniform("LightSamples", LightSamples);
        SetUniform("InverseSize", recipPerElem(Vector3(float(GridWidth), float(GridHeight), float(GridDepth))));

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GridDepth);
    }

    // Perform raycasting:
    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, cfg.Width, cfg.Height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(Vaos.CubeCenter);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity.ColorTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, Surfaces.LightCache.ColorTexture);

    glUseProgram(RaycastProgram);
    SetUniform("ModelviewProjection", Matrices.ModelviewProjection);
    SetUniform("Modelview", Matrices.Modelview);
    SetUniform("ViewMatrix", Matrices.View);
    SetUniform("ProjectionMatrix", Matrices.Projection);
    SetUniform("ViewSamples", ViewSamples);
    SetUniform("EyePosition", EyePosition);
    SetUniform("Density", 0);
    SetUniform("LightCache", 1);
    SetUniform("RayOrigin", Vector4(transpose(Matrices.Modelview) * EyePosition).getXYZ());
    SetUniform("FocalLength", 1.0f / std::tan(FieldOfView / 2));
    SetUniform("WindowSize", float(cfg.Width), float(cfg.Height));
    SetUniform("StepSize", sqrtf(2.0) / float(ViewSamples));

    glDrawArrays(GL_POINTS, 0, 1);

    assert(checkError());
}
