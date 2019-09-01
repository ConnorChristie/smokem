#include "object.h"

Object::Object(GLuint programID, const char* objfile)
{
    objectInit(programID, objfile, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
}

Object::Object(GLuint programID, const char* objfile, glm::vec3 rotate, glm::vec3 translate, float scale)
{
    objectInit(programID, objfile, rotate, translate, scale);
}

Object::~Object()
{
    //?? TODO: stub
}

void Object::objectInit(GLuint programID, const char* objfile, glm::vec3 rotate, glm::vec3 translate, float scale)
{
    setScale(scale);
    setTranslation(translate);
    mInitialRotate = rotate;

    std::string directory = objfile;

#ifdef _WIN32
    directory = directory.substr(0, directory.find_last_of('\\'));
    directory += '\\';
#else
    directory = directory.substr(0, directory.find_last_of('/'));
    directory += '/';
#endif

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string result = LoadObj(shapes, materials, objfile, directory.c_str());

    for (const auto& shape : shapes)
    {
        // materials are usually on a per-shape basis so let's grab the first one and use that
        int materialId = shape.mesh.material_ids.at(0);

        Shape sh(programID, shape, materials.at(materialId), directory);
        mShapes.push_back(sh);
    }
}

void Object::calcModelMatrix()
{
    if (mModelMatrixChanged)
    {
        mModelMatrix = glm::mat4();
        mModelMatrix = glm::translate(mModelMatrix, mTranslate);
        mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mScale));
        mModelMatrix = glm::rotate(mModelMatrix, mRotate.x, glm::vec3(1.0f, 0.0f, 0.0f));
        mModelMatrix = glm::rotate(mModelMatrix, mRotate.y, glm::vec3(0.0f, 1.0f, 0.0f));
        mModelMatrix = glm::rotate(mModelMatrix, mRotate.z, glm::vec3(0.0f, 0.0f, 1.0f));

        mModelMatrixChanged = false;
    }
}

void Object::render(GLuint programID)
{
    glUseProgram(programID);
    glUniformMatrix4fv(glGetUniformLocation(programID, "model_matrix"), 1, false, glm::value_ptr(getModelMatrix()));

    for (int i = 0; i < mShapes.size(); i++)
    {
        mShapes.at(i).render(programID);
    }
}

void Object::setRotation(glm::vec3 rotation)
{
    mRotate = mInitialRotate + rotation;
    mModelMatrixChanged = true;
}

void Object::setScale(float scale)
{
    mScale = scale;
    mModelMatrixChanged = true;
}

void Object::setTranslation(glm::vec3 translation)
{
    mTranslate = translation;
    mModelMatrixChanged = true;
}

glm::mat4 Object::getModelMatrix()
{
    calcModelMatrix();
    return mModelMatrix;
}