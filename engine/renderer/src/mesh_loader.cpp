#include "renderer/mesh_loader.h"
#include "renderer/opengl_render_device.h"

#include "core/logging.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
                auto fullPath = modelDir / texPath.C_Str();
                mat.diffuseTexturePath = fullPath.string();
            }
        }

        result.materials.push_back(std::move(mat));
    }

    result.mesh = std::make_unique<Mesh>(Mesh::create(m_device, meshData));

    logger->info("Loaded model '{}': {} vertices, {} indices, {} submeshes, {} materials",
                 path, meshData.vertices.size(), meshData.indices.size(),
                 meshData.subMeshes.size(), result.materials.size());

    return result;
}

} // namespace placeholder::renderer
