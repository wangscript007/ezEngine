#include <ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>

#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace ezModelImporter2
{
  ImporterAssimp::ImporterAssimp() = default;
  ImporterAssimp::~ImporterAssimp() = default;

  class aiLogStreamError : public Assimp::LogStream
  {
  public:
    void write(const char* message) { ezLog::Error("AssImp: {0}", message); }
  };

  class aiLogStreamWarning : public Assimp::LogStream
  {
  public:
    void write(const char* message) { ezLog::Warning("AssImp: {0}", message); }
  };

  class aiLogStreamInfo : public Assimp::LogStream
  {
  public:
    void write(const char* message) { ezLog::Dev("AssImp: {0}", message); }
  };

  ezResult ImporterAssimp::DoImport()
  {
    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);

    Assimp::DefaultLogger::get()->attachStream(new aiLogStreamError(), Assimp::Logger::Err);
    Assimp::DefaultLogger::get()->attachStream(new aiLogStreamWarning(), Assimp::Logger::Warn);
    Assimp::DefaultLogger::get()->attachStream(new aiLogStreamInfo(), Assimp::Logger::Info);

    // Note: ReadFileFromMemory is not able to read dependent files even if we use our own Assimp::IOSystem.
    // It is possible to use ReadFile instead but this leads to a lot of code...
    // Triangulate:           Our mesh format cannot handle anything else.
    // JoinIdenticalVertices: Assimp doesn't use index buffer at all if this is not specified.
    // TransformUVCoords:     As of now we do not have a concept for uv transforms.
    // Process_FlipUVs:       Assimp assumes OpenGl style UV coordinate system otherwise.
    // ImproveCacheLocality:  Reorders triangles for better vertex cache locality.

    ezUInt32 uiAssimpFlags = 0;
    if (m_Options.m_pMeshOutput != nullptr)
    {
      uiAssimpFlags |= aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_TransformUVCoords | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality;
    }

    m_aiScene = m_aiImporter.ReadFile(m_Options.m_sSourceFile, uiAssimpFlags);
    if (m_aiScene == nullptr)
    {
      ezLog::Error("Assimp failed to import '{}'", m_Options.m_sSourceFile);
      return EZ_FAILURE;
    }

    if (m_aiScene->mMetaData != nullptr)
    {
      // TODO: not yet sure this will work as expected with animations
      // if (m_aiScene->mMetaData->Get("UnitScaleFactor", m_fUnitScale))
      //{
      //  // Only FBX files have this unit scale factor and the default unit for FBX is cm. We want meters.
      //  m_fUnitScale = m_fUnitScale / 100.0f;

      //  if (aiNode* node = m_aiScene->mRootNode)
      //  {
      //    aiMatrix4x4 scaleMat;
      //    aiMatrix4x4::Scaling(aiVector3t(m_fUnitScale), scaleMat);

      //    node->mTransformation = scaleMat * node->mTransformation;
      //  }
      //}
    }

    EZ_SUCCEED_OR_RETURN(TraverseAiScene());

    EZ_SUCCEED_OR_RETURN(PrepareOutputMesh());

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::TraverseAiScene()
  {
    if (m_Options.m_pSkeletonOutput != nullptr)
    {
      m_Options.m_pSkeletonOutput->m_Children.PushBack(EZ_DEFAULT_NEW(ezEditableSkeletonJoint));
      EZ_SUCCEED_OR_RETURN(TraverseAiNode(m_aiScene->mRootNode, ezMat4::IdentityMatrix(), m_Options.m_pSkeletonOutput->m_Children.PeekBack()));
    }
    else
    {
      EZ_SUCCEED_OR_RETURN(TraverseAiNode(m_aiScene->mRootNode, ezMat4::IdentityMatrix(), nullptr));
    }

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::TraverseAiNode(aiNode* pNode, const ezMat4& parentTransform, ezEditableSkeletonJoint* pCurJoint)
  {
    const ezMat4 localTransform = ConvertAssimpType(pNode->mTransformation);
    const ezMat4 globalTransform = parentTransform * localTransform;

    if (pCurJoint)
    {
      pCurJoint->m_sName.Assign(pNode->mName.C_Str());
      pCurJoint->m_Transform.SetFromMat4(localTransform);
    }

    if (pNode->mNumMeshes > 0)
    {
      for (ezUInt32 meshIdx = 0; meshIdx < pNode->mNumMeshes; ++meshIdx)
      {
        EZ_SUCCEED_OR_RETURN(ProcessAiMesh(m_aiScene->mMeshes[pNode->mMeshes[meshIdx]], globalTransform));
      }
    }

    for (ezUInt32 childIdx = 0; childIdx < pNode->mNumChildren; ++childIdx)
    {
      if (pCurJoint)
      {
        pCurJoint->m_Children.PushBack(EZ_DEFAULT_NEW(ezEditableSkeletonJoint));

        EZ_SUCCEED_OR_RETURN(TraverseAiNode(pNode->mChildren[childIdx], globalTransform, pCurJoint->m_Children.PeekBack()));
      }
      else
      {
        EZ_SUCCEED_OR_RETURN(TraverseAiNode(pNode->mChildren[childIdx], globalTransform, nullptr));
      }
    }

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::ProcessAiMesh(aiMesh* pMesh, const ezMat4& transform)
  {
    if (pMesh->mPrimitiveTypes != aiPrimitiveType::aiPrimitiveType_TRIANGLE)
      return EZ_SUCCESS;

    if (m_Options.m_pSkeletonOutput != nullptr && pMesh->HasBones())
    {
      // pMesh->mBones
    }

    {
      auto& mi = m_MeshInstances[pMesh->mMaterialIndex].ExpandAndGetRef();
      mi.m_GlobalTransform = transform;
      mi.m_pMesh = pMesh;

      m_uiTotalMeshVertices += pMesh->mNumVertices;
      m_uiTotalMeshTriangles += pMesh->mNumFaces;
    }

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::PrepareOutputMesh()
  {
    if (m_Options.m_pMeshOutput == nullptr)
      return EZ_SUCCESS;

    auto& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    // TODO: precision
    const ezUInt32 uiMeshPositionsStream = mb.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    const ezUInt32 uiMeshNormalsStream = mb.AddStream(ezGALVertexAttributeSemantic::Normal, ezMeshNormalPrecision::ToResourceFormatNormal(ezMeshNormalPrecision::_32Bit));
    const ezUInt32 uiMeshTexCoordsStream = mb.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::_32Bit));
    const ezUInt32 uiMeshTangentsStream = mb.AddStream(ezGALVertexAttributeSemantic::Tangent, ezMeshNormalPrecision::ToResourceFormatTangent(ezMeshNormalPrecision::_32Bit));

    mb.AllocateStreams(m_uiTotalMeshVertices, ezGALPrimitiveTopology::Triangles, m_uiTotalMeshTriangles);

    ezUInt32 uiMeshPrevTriangleIdx = 0;
    ezUInt32 uiMeshCurVertexIdx = 0;
    ezUInt32 uiMeshCurTriangleIdx = 0;
    ezUInt32 uiMeshCurMaterialIdx = 0;

    for (auto itMesh : m_MeshInstances)
    {
      for (const auto& mi : itMesh.Value())
      {
        for (ezUInt32 vertIdx = 0; vertIdx < mi.m_pMesh->mNumVertices; ++vertIdx)
        {
          const ezUInt32 finalVertIdx = uiMeshCurVertexIdx + vertIdx;

          const ezVec3 position = mi.m_GlobalTransform * ConvertAssimpType(mi.m_pMesh->mVertices[vertIdx]);
          mb.SetVertexData(uiMeshPositionsStream, finalVertIdx, position);

          if (mi.m_pMesh->HasNormals())
          {
            // TODO: mi.m_GlobalTransform

            const ezVec3 normal = ConvertAssimpType(mi.m_pMesh->mNormals[vertIdx]);
            mb.SetVertexData(uiMeshNormalsStream, finalVertIdx, normal);
          }

          if (mi.m_pMesh->HasTextureCoords(0))
          {
            const ezVec2 texcoord = ConvertAssimpType(mi.m_pMesh->mTextureCoords[0][vertIdx]).GetAsVec2();
            mb.SetVertexData(uiMeshTexCoordsStream, finalVertIdx, texcoord);
          }

          if (mi.m_pMesh->HasTangentsAndBitangents())
          {
            // TODO: mi.m_GlobalTransform

            const ezVec3 tangent = ConvertAssimpType(mi.m_pMesh->mTangents[vertIdx]);
            const ezVec3 bitangent = ConvertAssimpType(mi.m_pMesh->mBitangents[vertIdx]);

            mb.SetVertexData(uiMeshTexCoordsStream, finalVertIdx, tangent);
          }
        }

        for (ezUInt32 triIdx = 0; triIdx < mi.m_pMesh->mNumFaces; ++triIdx)
        {
          const ezUInt32 finalTriIdx = uiMeshCurTriangleIdx + triIdx;

          const ezUInt32 f0 = mi.m_pMesh->mFaces[triIdx].mIndices[0];
          const ezUInt32 f1 = mi.m_pMesh->mFaces[triIdx].mIndices[1];
          const ezUInt32 f2 = mi.m_pMesh->mFaces[triIdx].mIndices[2];

          mb.SetTriangleIndices(finalTriIdx, uiMeshCurVertexIdx + f0, uiMeshCurVertexIdx + f1, uiMeshCurVertexIdx + f2);
        }

        uiMeshCurTriangleIdx += mi.m_pMesh->mNumFaces;
        uiMeshCurVertexIdx += mi.m_pMesh->mNumVertices;
      }

      m_Options.m_pMeshOutput->SetMaterial(uiMeshCurMaterialIdx, "invalid material");
      m_Options.m_pMeshOutput->AddSubMesh(uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx, uiMeshPrevTriangleIdx, uiMeshCurMaterialIdx);

      uiMeshPrevTriangleIdx = uiMeshCurTriangleIdx;
      ++uiMeshCurMaterialIdx;
    }

    m_Options.m_pMeshOutput->ComputeBounds();

    return EZ_SUCCESS;
  }

} // namespace ezModelImporter2
