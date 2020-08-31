#pragma once

#include <ModelImporter2/Importer/Importer.h>

#include <assimp/Importer.hpp>

class ezEditableSkeletonJoint;
struct aiNode;
struct aiMesh;

namespace ezModelImporter2
{
  class ImporterAssimp : public Importer
  {
  public:
    ImporterAssimp();
    ~ImporterAssimp();

  protected:
    virtual ezResult DoImport() override;

  private:
    ezResult TraverseAiScene();
    ezResult TraverseAiNode(aiNode* pNode, const ezMat4& parentTransform, ezEditableSkeletonJoint* pCurJoint);
    ezResult ProcessAiMesh(aiMesh* pMesh, const ezMat4& transform);

    Assimp::Importer m_aiImporter;
    const aiScene* m_aiScene = nullptr;
    float m_fUnitScale = 1.0f;
  };

  extern ezColor ConvertAssimpType(const aiColor3D& value, bool invert = false);
  extern ezMat4 ConvertAssimpType(const aiMatrix4x4& value);
  extern ezVec3 ConvertAssimpType(const aiVector3D& value);
  extern ezQuat ConvertAssimpType(const aiQuaternion& value);

} // namespace ezModelImporter2
