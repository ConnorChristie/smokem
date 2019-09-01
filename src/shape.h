#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "utility.h"
#include "stb_image.h"
#include "tiny_obj_loader.h"

#include <iostream>
#include <sstream>
#include <vector>

constexpr auto VALS_PER_VERT = 3;
constexpr auto VALS_PER_NORM = 3;
constexpr auto VALS_PER_TEXCOORD = 2;
constexpr auto VALS_PER_MTL_SURFACE = 3;

constexpr auto VERTICES_BUF_POS = 0; // position of vertex data in the buffer
constexpr auto INDICES_BUF_POS = 1; // position of index data in the buffer
constexpr auto NORMALS_BUF_POS = 2; // position of normal data in the buffer
constexpr auto TEXCOORDS_BUF_POS = 3; // position of texcoord data in the buffer

class Shape {
public:
    /* Only Shape constructor.
     * @param	programID	the ID of the program to which the data will be buffered.
     * @param	shape		The shape data, in the form specified in the tinyobj library.
     * @param	material	The material data, in the form specified in the tiny obj library.
     * @param	directory	The directory where shape & material data can be found, relative to the directory in which this executable was called.
     */
    Shape(GLuint programID, tinyobj::shape_t shape, tinyobj::material_t material, std::string directory);

    /* Basic Shape destructor. */
    ~Shape();

    /* Render the shape and its associated texture data.
     * @param	programID	The ID of the program to which the data will be buffered.
     */
    void render(GLuint programID, bool ignoreChecks = false);

    /* Accessors for data sizes */
    unsigned int getVerticesSize() const;
    unsigned int getIndicesSize() const;
    unsigned int getNormalsSize() const;
    unsigned int getTexCoordsSize() const;

private:
    /* We keep this constructor private since it's of no use to an external user. */
    Shape();

    /* The function that actually initialises the shape data.
     * @param	programID	the ID of the program to which the data will be buffered.
     * @param	shape		The shape data, in the form specified in the tinyobj library.
     * @param	material	The material data, in the form specified in the tiny obj library.
     * @param	directory	The directory where shape & material data can be found, relative to the directory in which this executable was called.
     */
    void shapeInit(GLuint programID, tinyobj::shape_t shape, tinyobj::material_t material, std::string directory);

    /* Generates a new texture, and returns the handle for this texture.
     * @param	filename	The filepath, relative to the directory where the executable was called, where the image file can be found.
     * @param 	texCount 	The offset for the active texture, 0 for the regular texture and 1 for the bumpmap of the texture
     * @return	A handle to the texture data.
     */
    unsigned int generateTexture(const char* filename, const unsigned int texCount);

    static const unsigned int mBufSize = 4; // size of the buffer
    GLuint mBuffer[mBufSize]; // the buffer
    
    /* surface lighting data */
    float mAmbient[VALS_PER_MTL_SURFACE];
    float mDiffuse[VALS_PER_MTL_SURFACE];
    float mSpecular[VALS_PER_MTL_SURFACE];
    float mEmission[VALS_PER_MTL_SURFACE];
    float mShininess;

    /* Sizes of buffered data */
    unsigned int mVerticesSize;
    unsigned int mIndicesSize;
    unsigned int mNormalsSize;
    unsigned int mTexCoordsSize;

    unsigned int mVertexVaoHandle; // VAO handle for vertices
    unsigned int mTextureHandle; // handle for the texture data
    unsigned int mTextureNormHandle; // handle for the bumpmap data for the texture
};

#endif
