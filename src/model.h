#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "stb_image.h"

namespace mar
{
    namespace
    {
        // --- Some forwards
        struct OBJObject;
        struct OBJFace;
        struct OBJFaceItem;
        struct OBJMaterial;

        struct OBJFace
        {
            std::vector<OBJFaceItem> items;
            std::string              material;
        };

        struct OBJFaceItem
        {
            OBJFaceItem()
            {
                vertexIndex = 0;
                normalIndex = 0;
                textureIndex = 0;
            }

            unsigned int vertexIndex;
            unsigned int normalIndex;
            unsigned int textureIndex;
        };

        struct OBJMaterial
        {
            float  Ka[4];
            float  Kd[4];
            float  Ks[4];

            GLuint texture;

            float texScaleU;
            float texScaleV;
        };

        struct OBJObject
        {
            std::vector<glm::vec3>             vertices;
            std::vector<glm::vec3>             normals;
            std::vector<glm::vec2>             textures;
            std::vector<OBJFace>               faces;
            std::map<std::string, OBJMaterial> materials;
        };
    }

    /**
     * \brief This class handles a 3D model. Only the OBJ file format is supported.
     */
    class Model
    {
    public:
        /**
         * \brief Create a new, empty model.
         */
        OBJObject getModelObject();

    public:
        /**
         * \brief Load an OBJ model.
         * \param data_path         Path from where to look for textures (when processing MTL)
         * \param mesh_filename     Path to the OBJ file
         * \param material_filename Path to the MTL file
         * \return true if the model was loaded successfully, false otherwise
         */
        bool load(const std::string& data_path, const std::string& mesh_filename, const std::string& material_filename = "");

    protected:
        /**
         * \brief Process a single line of an OBJ file.
         */
        void processMeshLine(const std::string& line);

        /**
         * \brief Process a single line of a MTL file.
         */
        void processMaterialLine(const std::string& line);

    private:
        /**
         * \brief OBJ structure containing the model.
         */
        OBJObject object_;

        /**
         * \brief Name of the current material during parsing.
         */
        std::string currentMaterial_;

        /**
         * \brief Path to the texture files.
         */
        std::string dataPath_;
    };
}

#endif // MODEL_H

#ifdef MODEL_IMPLEMENTATION

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <limits>

namespace
{
    void chop(std::string& str)
    {
        std::string whitespaces(" \t\f\v\n\r");
        size_t found = str.find_last_not_of(whitespaces);
        if (found != std::string::npos)
            str.erase(found + 1);
        else
            str.clear();
    }

    void split(const std::string& str, char delim, std::vector<std::string>& tokens)
    {
        std::stringstream iss(str);
        std::string item;
        while (std::getline(iss, item, delim))
        {
            if (item != " " && !item.empty())
            {
                chop(item);
                tokens.push_back(item);
            }
        }
    }
}

namespace mar
{
    GLuint loadTexture(const std::string& path)
    {
        GLuint texture = 0;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* image = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        return texture;
    }

    bool Model::load(const std::string& data_path, const std::string& mesh_filename, const std::string& material_filename)
    {
        dataPath_ = data_path;

        std::ifstream file_mesh;
        std::ifstream file_material;

        file_mesh.open((data_path + mesh_filename).c_str());
        if (!material_filename.empty())
            file_material.open((data_path + material_filename).c_str());

        if (!file_mesh.good() || (!material_filename.empty() && !file_material.good()))
            return false;

        std::string line;

        OBJMaterial& defaultMaterial = object_.materials[""];
        defaultMaterial.Ka[0] = 1;
        defaultMaterial.Ka[1] = 1;
        defaultMaterial.Ka[2] = 1;
        defaultMaterial.Ka[3] = 1;

        defaultMaterial.Kd[0] = 1;
        defaultMaterial.Kd[1] = 1;
        defaultMaterial.Kd[2] = 1;
        defaultMaterial.Kd[3] = 1;

        defaultMaterial.Ks[0] = 1;
        defaultMaterial.Ks[1] = 1;
        defaultMaterial.Ks[2] = 1;
        defaultMaterial.Ks[3] = 1;

        defaultMaterial.texture = 0;

        defaultMaterial.texScaleU = 1;
        defaultMaterial.texScaleV = 1;

        if (!material_filename.empty())
        {
            while (!file_material.eof())
            {
                std::getline(file_material, line);
                processMaterialLine(line);
            }
            file_material.close();
        }

        while (!file_mesh.eof())
        {
            std::getline(file_mesh, line);
            processMeshLine(line);
        }
        file_mesh.close();

        return true;
    }

    void Model::processMaterialLine(const std::string& line)
    {
        if (line.find("#") == 0) // A comment
        {
            return;
        }
        else if (line.find("newmtl ") == 0) // A new material
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 2)
                return;

            currentMaterial_ = tokens[1];
        }
        else if (line.find("Ka ") == 0) // Ambient color
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 4)
                return;

            OBJMaterial& material = object_.materials[currentMaterial_];

            material.Ka[0] = std::strtod(tokens.at(1).c_str(), 0);
            material.Ka[1] = std::strtod(tokens.at(2).c_str(), 0);
            material.Ka[2] = std::strtod(tokens.at(3).c_str(), 0);
        }
        else if (line.find("Kd ") == 0) // Diffuse color
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 4)
                return;

            OBJMaterial& material = object_.materials[currentMaterial_];

            material.Kd[0] = std::strtod(tokens.at(1).c_str(), 0);
            material.Kd[1] = std::strtod(tokens.at(2).c_str(), 0);
            material.Kd[2] = std::strtod(tokens.at(3).c_str(), 0);
        }
        else if (line.find("Ks ") == 0) // Specular color
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 4)
                return;

            OBJMaterial& material = object_.materials[currentMaterial_];

            material.Ks[0] = std::strtod(tokens.at(1).c_str(), 0);
            material.Ks[1] = std::strtod(tokens.at(2).c_str(), 0);
            material.Ks[2] = std::strtod(tokens.at(3).c_str(), 0);
        }
        else if (line.find("map_Kd ") == 0) // Texture mapping
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            OBJMaterial& material = object_.materials[currentMaterial_];

            if (tokens.size() == 2)
            {
                material.texture = loadTexture(dataPath_ + tokens[1]);
                material.texScaleU = 1;
                material.texScaleV = 1;
            }
            else if (tokens.size() == 6)
            {
                material.texture = loadTexture(dataPath_ + tokens[5]);
                material.texScaleU = std::strtod(tokens.at(2).c_str(), 0);
                material.texScaleV = std::strtod(tokens.at(3).c_str(), 0);
            }
            else
            {
                return;
            }
        }
        else // Whatever
        {
            return;
        }
    }

    void Model::processMeshLine(const std::string& line)
    {
        if (line.find("#") == 0) // A comment
        {
            return;
        }
        else if (line.find("v ") == 0) // A vertex
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 4) return;

            glm::vec3 vertex;
            vertex.x = strtod(tokens.at(1).c_str(), 0);
            vertex.y = strtod(tokens.at(2).c_str(), 0);
            vertex.z = strtod(tokens.at(3).c_str(), 0);
            object_.vertices.push_back(vertex);
        }
        else if (line.find("usemtl ") == 0) // A material usage
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 2) return;

            currentMaterial_ = tokens[1];
        }
        else if (line.find("f ") == 0) // A face
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            OBJFace face;

            for (int i = 1; i < tokens.size(); ++i) // Each item of the face
            {
                OBJFaceItem item;

                item.vertexIndex = -1;
                item.normalIndex = -1;
                item.textureIndex = -1;

                std::vector<std::string> items;
                split(tokens.at(i), '/', items);

                switch (items.size())
                {
                case 1:
                {
                    if (!items.at(0).empty()) item.vertexIndex = std::strtol(items.at(0).c_str(), 0, 10);
                    break;
                }
                case 2:
                {
                    if (!items.at(0).empty()) item.vertexIndex = std::strtol(items.at(0).c_str(), 0, 10);
                    if (!items.at(1).empty()) item.textureIndex = std::strtol(items.at(1).c_str(), 0, 10);
                    break;
                }
                case 3:
                {
                    if (!items.at(0).empty()) item.vertexIndex = std::strtol(items.at(0).c_str(), 0, 10);
                    if (!items.at(1).empty()) item.textureIndex = std::strtol(items.at(1).c_str(), 0, 10);
                    if (!items.at(2).empty()) item.normalIndex = std::strtol(items.at(2).c_str(), 0, 10);
                    break;
                }
                }

                face.items.push_back(item);
            }

            face.material = currentMaterial_;

            object_.faces.push_back(face);
        }
        else if (line.find("vn ") == 0) // A normal
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 4) return;

            glm::vec3 normal;
            normal.x = std::strtod(tokens.at(1).c_str(), 0);
            normal.y = std::strtod(tokens.at(2).c_str(), 0);
            normal.z = std::strtod(tokens.at(3).c_str(), 0);

            object_.normals.push_back(normal);
        }
        else if (line.find("vt ") == 0) // A texture coordinate
        {
            std::vector<std::string> tokens;
            split(line, ' ', tokens);

            if (tokens.size() != 3) return;

            glm::vec2 texture;
            texture.x = std::strtod(tokens.at(1).c_str(), 0);
            texture.y = std::strtod(tokens.at(2).c_str(), 0);

            object_.textures.push_back(texture);
        }
        else // Whatever
        {
            return;
        }
    }

    OBJObject Model::getModelObject()
    {
        return object_;
    }

    //void Model::build()
    //{
    //    glEnable(GL_TEXTURE_2D);

    //    static float ambient[4];
    //    static float diffuse[4];
    //    static float specular[4];

    //    for (int i = 0; i < object_.faces.size(); ++i) // Each face
    //    {
    //        OBJFace& face = object_.faces.at(i);
    //        OBJMaterial& material = object_.materials[face.material];

    //        //glMaterialfv( GL_FRONT, GL_AMBIENT,  material.Ka );
    //        //glMaterialfv( GL_FRONT, GL_DIFFUSE,  material.Kd );
    //        //glMaterialfv( GL_FRONT, GL_SPECULAR, material.Ks );

    //        glBindTexture(GL_TEXTURE_2D, material.texture);

    //        for (int j = 0; j < object_.faces.at(i).items.size(); ++j) // Each vertex
    //        {
    //            OBJFaceItem& item = object_.faces.at(i).items.at(j);

    //            if (item.textureIndex > 0) // Texture coordinates are provided
    //                glTexCoord2f(object_.textures.at(item.textureIndex).coords[0] * material.texScaleU,
    //                    object_.textures.at(item.textureIndex).coords[1] * material.texScaleV);

    //            glVertex3fv(object_.vertices.at(item.vertexIndex).coords);
    //        }
    //    }
    //}

}

#endif