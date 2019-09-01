#include "shape.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Shape::Shape()
{
    //?? TODO: stub
}

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

    // generate the buffer, with appropriate size
    glGenBuffers(mBufSize, mBuffer);

    assert(checkError());

    // generate vertex array for the vertex data
    glGenVertexArrays(1, &mVertexVaoHandle);
    glBindVertexArray(mVertexVaoHandle);

    assert(checkError());

    // get attribute locations
    int vertLoc = glGetAttribLocation(programID, "a_vertex");
    int normLoc = glGetAttribLocation(programID, "a_normal");
    int texLoc = glGetAttribLocation(programID, "a_tex_coord");

    // buffer vertices
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer[VERTICES_BUF_POS]);
    glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(float), &shape.mesh.positions.front(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(vertLoc);
    glVertexAttribPointer(vertLoc, VALS_PER_VERT, GL_FLOAT, GL_FALSE, 0, 0);

    assert(checkError());

    // buffer indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffer[INDICES_BUF_POS]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize * sizeof(unsigned int), &shape.mesh.indices.front(), GL_STATIC_DRAW);

    assert(checkError());

    // buffer normals
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer[NORMALS_BUF_POS]);
    glBufferData(GL_ARRAY_BUFFER, mNormalsSize * sizeof(float), &shape.mesh.normals.front(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(normLoc);
    glVertexAttribPointer(normLoc, VALS_PER_NORM, GL_FLOAT, GL_FALSE, 0, 0);

    assert(checkError());

    if (mTexCoordsSize > 0)
    {
        // buffer texcoords
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer[TEXCOORDS_BUF_POS]);
        glBufferData(GL_ARRAY_BUFFER, mTexCoordsSize * sizeof(float), &shape.mesh.texcoords.front(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(texLoc);
        glVertexAttribPointer(texLoc, VALS_PER_TEXCOORD, GL_FLOAT, GL_FALSE, 0, 0);

        assert(checkError());

        // bind texture
        glActiveTexture(GL_TEXTURE0);
        mTextureHandle = generateTexture((directory + material.diffuse_texname).c_str(), 0);
        mTextureNormHandle = generateTexture((directory + material.normal_texname + "normTex.png").c_str(), 1);
    }
    else
    {
        // no texture file given, so we create a default
        glActiveTexture(GL_TEXTURE0);
        mTextureHandle = generateTexture("", 0);
        mTextureNormHandle = generateTexture("", 1);
    }

    assert(checkError());

    // unbind vertex array
    glBindVertexArray(0);
    // unbind the buffer too
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // and we're done!
    assert(checkError());
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

    // Linear interpolation filter gives a smoothed out appearance the square pixels
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // Clamp to edge can help to remove visible seams when lookups are at the edge of the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(image);
    return texture;
}

void Shape::render(GLuint programID, bool ignoreChecks)
{
    glUseProgram(programID);

    int texMapHandle = glGetUniformLocation(programID, "tex_map");
    int texNormHandle = glGetUniformLocation(programID, "tex_norm");
    if (texMapHandle == -1 && !ignoreChecks)
    {
        std::cerr << "Could not find uniform variable 'tex_map'" << std::endl;
        exit(1);
    }
    if (texNormHandle == -1 && !ignoreChecks)
    {
        std::cerr << "Could not find uniform variable 'tex_norm'" << std::endl;
        exit(1);
    }

    int mtlAmbientHandle = glGetUniformLocation(programID, "mtl_ambient");
    int mtlDiffuseHandle = glGetUniformLocation(programID, "mtl_diffuse");
    int mtlSpecularHandle = glGetUniformLocation(programID, "mtl_specular");
    int	mtlEmissionHandle = glGetUniformLocation(programID, "mtl_emission");
    int mtlShininessHandle = glGetUniformLocation(programID, "mtl_shininess");
    if (!ignoreChecks)
    {
        if ((mtlAmbientHandle == -1) || (mtlDiffuseHandle == -1) || (mtlSpecularHandle == -1) || (mtlEmissionHandle == -1) || (mtlShininessHandle == -1))
        {
            std::cerr << "Could not find light material uniform variables." << std::endl;
            exit(1);
        }
    }

    assert(checkError());

    // send texture handle
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(texMapHandle, 0); // sending main texture data to GL_TEXTURE0.

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glActiveTexture(GL_TEXTURE1);
    glUniform1i(texNormHandle, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, mTextureNormHandle);

    // send light material data
    glUniform3fv(mtlAmbientHandle, 1, mAmbient);
    glUniform3fv(mtlDiffuseHandle, 1, mDiffuse);
    glUniform3fv(mtlSpecularHandle, 1, mSpecular);
    glUniform3fv(mtlEmissionHandle, 1, mEmission);
    glUniform1f(mtlShininessHandle, mShininess);

    assert(checkError());

    // buffer the data proper
    glBindVertexArray(mVertexVaoHandle);
    glDrawElements(GL_TRIANGLES, mIndicesSize, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // unbind VAO

    assert(checkError());
}

unsigned int Shape::getVerticesSize() const {
    return mVerticesSize;
}

unsigned int Shape::getIndicesSize() const {
    return mIndicesSize;
}

unsigned int Shape::getNormalsSize() const {
    return mNormalsSize;
}

unsigned int Shape::getTexCoordsSize() const {
    return mTexCoordsSize;
}