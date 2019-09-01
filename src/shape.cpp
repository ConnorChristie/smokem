#include "shape.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Shape::Shape(GLuint programID, tinyobj::shape_t shape, tinyobj::material_t material, std::string directory)
{
    shapeInit(programID, shape, material, directory);
}

Shape::~Shape()
{
    //?? TODO: stub
}

void Shape::shapeInit(GLuint programID, tinyobj::shape_t shape, tinyobj::material_t material, std::string directory)
{
    glUseProgram(programID);
    assert(checkError());

    mVerticesSize = shape.mesh.positions.size();
    mIndicesSize = shape.mesh.indices.size();
    mNormalsSize = shape.mesh.normals.size();
    mTexCoordsSize = shape.mesh.texcoords.size();

    for (int i = 0; i < VALS_PER_MTL_SURFACE; i++)
    {
        mAmbient[i] = material.ambient[i];
        mDiffuse[i] = material.diffuse[i];
        mSpecular[i] = material.specular[i];
        mEmission[i] = material.emission[i];
    }
    mShininess = material.shininess;

    GLuint buffers[mBufSize];

    glGenVertexArrays(1, &mVertexVaoHandle);
    glBindVertexArray(mVertexVaoHandle);
    glGenBuffers(mBufSize, buffers);

    {
        // Vertices
        glBindBuffer(GL_ARRAY_BUFFER, buffers[VERTICES_BUF_POS]);
        glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(float), &shape.mesh.positions.front(), GL_STATIC_DRAW);

        auto vertLoc = glGetAttribLocation(programID, "a_vertex");
        glEnableVertexAttribArray(vertLoc);
        glVertexAttribPointer(vertLoc, VALS_PER_VERT, GL_FLOAT, GL_FALSE, 0, 0);
    }
    {
        // Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[INDICES_BUF_POS]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize * sizeof(unsigned int), &shape.mesh.indices.front(), GL_STATIC_DRAW);
    }
    {
        // Normals
        glBindBuffer(GL_ARRAY_BUFFER, buffers[NORMALS_BUF_POS]);
        glBufferData(GL_ARRAY_BUFFER, mNormalsSize * sizeof(float), &shape.mesh.normals.front(), GL_STATIC_DRAW);

        auto normLoc = glGetAttribLocation(programID, "a_normal");
        glEnableVertexAttribArray(normLoc);
        glVertexAttribPointer(normLoc, VALS_PER_NORM, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (mTexCoordsSize > 0)
    {
        // Texture
        glBindBuffer(GL_ARRAY_BUFFER, buffers[TEXCOORDS_BUF_POS]);
        glBufferData(GL_ARRAY_BUFFER, mTexCoordsSize * sizeof(float), &shape.mesh.texcoords.front(), GL_STATIC_DRAW);

        auto texLoc = glGetAttribLocation(programID, "a_tex_coord");
        glEnableVertexAttribArray(texLoc);
        glVertexAttribPointer(texLoc, VALS_PER_TEXCOORD, GL_FLOAT, GL_FALSE, 0, 0);

        mTextureHandle = generateTexture((directory + material.diffuse_texname).c_str(), 0);
        mTextureNormHandle = generateTexture((directory + material.normal_texname + "normTex.png").c_str(), 1);
    }
    else
    {
        mTextureHandle = generateTexture("", 0);
        mTextureNormHandle = generateTexture("", 1);
    }

    assert(checkError());
}

void Shape::render(GLuint programID)
{
    glUseProgram(programID);

    {
        // Texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTextureHandle);
        glUniform1i(glGetUniformLocation(programID, "tex_map"), 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    {
        // Normals
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mTextureNormHandle);
        glUniform1i(glGetUniformLocation(programID, "tex_norm"), 1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    {
        // Material
        glUniform3fv(glGetUniformLocation(programID, "mtl_ambient"), 1, mAmbient);
        glUniform3fv(glGetUniformLocation(programID, "mtl_diffuse"), 1, mDiffuse);
        glUniform3fv(glGetUniformLocation(programID, "mtl_specular"), 1, mSpecular);
        glUniform3fv(glGetUniformLocation(programID, "mtl_emission"), 1, mEmission);
        glUniform1f(glGetUniformLocation(programID, "mtl_shininess"), mShininess);
    }

    glBindVertexArray(mVertexVaoHandle);
    glDrawElements(GL_TRIANGLES, mIndicesSize, GL_UNSIGNED_INT, 0);
}

GLuint Shape::generateTexture(const char* filename, const unsigned int texCount)
{
    GLuint texture = 0;

    switch (texCount)
    {
    case 0:	glActiveTexture(GL_TEXTURE0); break;
    case 1: glActiveTexture(GL_TEXTURE1); break;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb);

    if (channels == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    }
    else if (channels == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    }
    else
    {
        std::string fn = filename;
        if (!fn.empty()) std::cerr << "file '" << filename << "' is not a valid image file. Creating default image file as substitute." << std::endl;
        unsigned char def[3] = { 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, def);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(image);
    return texture;
}
