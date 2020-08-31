#include <ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>

#include <RendererCore/AnimationSystem/EditableSkeleton.h>
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
    if (m_Options.m_bImportMeshes)
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

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::TraverseAiScene()
  {
    m_pSkeletonResult->m_Children.PushBack(EZ_DEFAULT_NEW(ezEditableSkeletonJoint));

    EZ_SUCCEED_OR_RETURN(TraverseAiNode(m_aiScene->mRootNode, ezMat4::IdentityMatrix(), m_pSkeletonResult->m_Children.PeekBack()));

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::TraverseAiNode(aiNode* pNode, const ezMat4& parentTransform, ezEditableSkeletonJoint* pCurJoint)
  {
    const ezMat4 localTransform = ConvertAssimpType(pNode->mTransformation);
    const ezMat4 globalTransform = parentTransform * localTransform;

    pCurJoint->m_sName.Assign(pNode->mName.C_Str());
    pCurJoint->m_Transform.SetFromMat4(localTransform);

    if (pNode->mNumMeshes > 0)
    {
      for (ezUInt32 meshIdx = 0; meshIdx < pNode->mNumChildren; ++meshIdx)
      {
        EZ_SUCCEED_OR_RETURN(ProcessAiMesh(m_aiScene->mMeshes[pNode->mMeshes[meshIdx]], globalTransform));
      }
    }

    for (ezUInt32 childIdx = 0; childIdx < pNode->mNumChildren; ++childIdx)
    {
      pCurJoint->m_Children.PushBack(EZ_DEFAULT_NEW(ezEditableSkeletonJoint));

      EZ_SUCCEED_OR_RETURN(TraverseAiNode(pNode->mChildren[childIdx], globalTransform, pCurJoint->m_Children.PeekBack()));
    }

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::ProcessAiMesh(aiMesh* pMesh, const ezMat4& transform)
  {
    if (m_Options.m_bImportSkeleton && pMesh->HasBones())
    {
      // pMesh->mBones
    }

    return EZ_SUCCESS;
  }

} // namespace ezModelImporter2
