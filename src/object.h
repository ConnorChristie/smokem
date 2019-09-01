#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_obj_loader.h"
#include "shape.h"

#include <iostream>
#include <sstream>
#include <vector>

class Object {
public:
    Object(GLuint programID, const char* objfile);
    Object(GLuint programID, const char* objfile, glm::vec3 rotate, glm::vec3 translate, float scale);

    ~Object();

    /** Renders the vertex data associated with this Object.
     * @param   programID   The shader program to buffer the object data to.
	 * @param	ignoreChecks True if checks for sending texture data etc to the GLSL shaders should be ignored.
	 */
    void render(GLuint programID, bool ignoreChecks = false);

    void setRotation(glm::vec3 rotation);
    void setTranslation(glm::vec3 translation);
    void setScale(float scale);

    unsigned int getVerticesSize() const { return mVerticesSize; };
    unsigned int getIndicesSize() const { return mIndicesSize; };
	unsigned int getNormalsSize() const { return mNormalsSize; };
    unsigned int getTexCoordsSize() const { return mTexCoordsSize; };

    glm::vec3 getTranslation() const { return mTranslate; };
    glm::vec3 getRotation() const { return mRotate; };
    float getScaleFactor() const { return mScale; };

    glm::vec3 getPosition();
    glm::mat4 getModelMatrix();

protected:
    /** The actual routine called by constructors etc to set up data, textures, etc on creation.
     * Thus it should be called ONLY ONCE, and will be done by all constructors.
     * @param   programID   The shader program to buffer the object data to.
     * @param   objfile     The filename of where the data of an OBJ file is stored. 
     * @param   rotate		The amount in the x, y, and z planes that the object will be rotated by.
     * @param   translate	The amount in the x, y, and z planes that the object will be translated, relative to the origin of the world.
     * @param   scale		Amount to scale the object by, as a percentage of its original size. Defaults to its initial size if not specified.
     */
    void objectInit(GLuint programID, const char* objfile, glm::vec3 rotate, glm::vec3 translate, float scale);

    void calcModelMatrix();

    unsigned int mVerticesSize;
    unsigned int mIndicesSize;
    unsigned int mNormalsSize;
    unsigned int mTexCoordsSize;

    std::vector<Shape> mShapes;
    glm::vec3 mCentres;

    glm::vec3 mInitialRotate;
    glm::vec3 mRotate;
    glm::vec3 mTranslate;
    float mScale;

    bool mModelMatrixChanged;
    glm::mat4 mModelMatrix;
};

#endif
