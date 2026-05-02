#include "renderer/mesh_loader.h"
#include "renderer/opengl_render_device.h"

#include "core/logging.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stb_image.h>

#include <filesystem>

namespace placeholder::renderer
{

MeshLoader::MeshLoader(OpenGLRenderDevice& device)
    : m_device(device)
{
}

LoadedModel MeshLoader::loadFromFile(const std::string& path)
{
    auto logger = core::getLogger("renderer");
    LoadedModel result;

    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate
                       | aiProcess_GenNormals
                       | aiProcess_FlipUVs
                       | aiProcess_CalcTangentSpace
                       | aiProcess_JoinIdenticalVertices
                       | aiProcess_OptimizeMeshes;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        logger->error("Assimp failed to load '{}': {}", path, importer.GetErrorString());
        return result;
    }

    MeshData meshData;
    std::filesystem::path modelDir = std::filesystem::path(path).parent_path();

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* aiMesh = scene->mMeshes[i];

        SubMesh subMesh;
        subMesh.indexOffset = static_cast<uint32_t>(meshData.indices.size());
        subMesh.materialIndex = aiMesh->mMaterialIndex;

        uint32_t baseVertex = static_cast<uint32_t>(meshData.vertices.size());

        for (unsigned int v = 0; v < aiMesh->mNumVertices; ++v)
        {
            MeshVertex vertex;
            vertex.position = {aiMesh->mVertices[v].x,
                               aiMesh->mVertices[v].y,
                               aiMesh->mVertices[v].z};

            if (aiMesh->HasNormals())
            {
                vertex.normal = {aiMesh->mNormals[v].x,
                                 aiMesh->mNormals[v].y,
                                 aiMesh->mNormals[v].z};
            }

            if (aiMesh->HasTextureCoords(0))
            {
                vertex.texCoord = {aiMesh->mTextureCoords[0][v].x,
                                   aiMesh->mTextureCoords[0][v].y};
            }

            meshData.vertices.push_back(vertex);
        }

        for (unsigned int f = 0; f < aiMesh->mNumFaces; ++f)
        {
            const aiFace& face = aiMesh->mFaces[f];
            for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
            {
                meshData.indices.push_back(baseVertex + face.mIndices[idx]);
            }
        }

        subMesh.indexCount = static_cast<uint32_t>(meshData.indices.size()) - subMesh.indexOffset;
        meshData.subMeshes.push_back(subMesh);
    }

    // Extract embedded textures (glTF binary, FBX, etc.)
    for (unsigned int i = 0; i < scene->mNumTextures; ++i)
    {
        const aiTexture* aiTex = scene->mTextures[i];
        EmbeddedTexture embTex;

        if (aiTex->mHeight == 0)
        {
            // Compressed format (PNG/JPG stored as raw bytes)
            int width, height, channels;
            stbi_set_flip_vertically_on_load(true);
            unsigned char* pixels = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(aiTex->pcData),
                static_cast<int>(aiTex->mWidth),
                &width, &height, &channels, 4);

            if (pixels)
            {
                embTex.width = width;
                embTex.height = height;
                embTex.channels = 4;
                embTex.compressed = false;
                embTex.data.assign(pixels, pixels + width * height * 4);
                stbi_image_free(pixels);
                logger->debug("Decoded embedded texture {} ({}x{}, format: {})",
                              i, width, height, aiTex->achFormatHint);
            }
            else
            {
                logger->warn("Failed to decode embedded texture {}", i);
            }
        }
        else
        {
            // Uncompressed ARGB8888
            embTex.width = static_cast<int>(aiTex->mWidth);
            embTex.height = static_cast<int>(aiTex->mHeight);
            embTex.channels = 4;
            embTex.compressed = false;
            size_t pixelCount = static_cast<size_t>(embTex.width) * embTex.height;
            embTex.data.resize(pixelCount * 4);
            for (size_t p = 0; p < pixelCount; ++p)
            {
                embTex.data[p * 4 + 0] = aiTex->pcData[p].r;
                embTex.data[p * 4 + 1] = aiTex->pcData[p].g;
                embTex.data[p * 4 + 2] = aiTex->pcData[p].b;
                embTex.data[p * 4 + 3] = aiTex->pcData[p].a;
            }
        }

        result.embeddedTextures.push_back(std::move(embTex));
    }

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        const aiMaterial* aiMat = scene->mMaterials[i];
        LoadedMaterial mat;

        aiString name;
        if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        {
            mat.name = name.C_Str();
        }

        aiColor3D color;
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            mat.diffuseColor = {color.r, color.g, color.b};
        }

        float opacity = 1.0f;
        if (aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
        {
            mat.opacity = opacity;
        }

        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texPath;
            if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
            {
                std::string texPathStr = texPath.C_Str();

                if (texPathStr.size() > 1 && texPathStr[0] == '*')
                {
                    int texIdx = std::atoi(texPathStr.c_str() + 1);
                    mat.embeddedTextureIndex = texIdx;
                }
                else
                {
                    auto fullPath = modelDir / texPathStr;
                    mat.diffuseTexturePath = fullPath.string();
                }
            }
        }

        // Fall back to base color texture for PBR materials (glTF)
        if (mat.diffuseTexturePath.empty() && mat.embeddedTextureIndex < 0)
        {
            if (aiMat->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
            {
                aiString texPath;
                if (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS)
                {
                    std::string texPathStr = texPath.C_Str();
                    if (texPathStr.size() > 1 && texPathStr[0] == '*')
                    {
                        mat.embeddedTextureIndex = std::atoi(texPathStr.c_str() + 1);
                    }
                    else
                    {
                        auto fullPath = modelDir / texPathStr;
                        mat.diffuseTexturePath = fullPath.string();
                    }
                }
            }
        }

        result.materials.push_back(std::move(mat));
    }

    result.mesh = std::make_unique<Mesh>(Mesh::create(m_device, meshData));

    if (!meshData.vertices.empty())
    {
        result.bounds.min = meshData.vertices[0].position;
        result.bounds.max = meshData.vertices[0].position;
        for (const auto& v : meshData.vertices)
        {
            result.bounds.min = glm::min(result.bounds.min, v.position);
            result.bounds.max = glm::max(result.bounds.max, v.position);
        }
    }

    logger->info("Loaded model '{}': {} vertices, {} indices, {} submeshes, {} materials, {} embedded textures",
                 path, meshData.vertices.size(), meshData.indices.size(),
                 meshData.subMeshes.size(), result.materials.size(),
                 result.embeddedTextures.size());

    return result;
}

} // namespace placeholder::renderer
