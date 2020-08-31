#include <ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace ezModelImporter2
{
  ezColor ConvertAssimpType(const aiColor3D& value, bool invert = false)
  {
    if (invert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return ezColor(value.r, value.g, value.b);
  }

  ezMat4 ConvertAssimpType(const aiMatrix4x4& value)
  {
    ezMat4 mTransformation;
    mTransformation.SetFromArray(&value.a1, ezMatrixLayout::RowMajor);
    return mTransformation;
  }

  ezVec3 ConvertAssimpType(const aiVector3D& value) { return ezVec3(value.x, value.y, value.z); }

  ezQuat ConvertAssimpType(const aiQuaternion& value) { return ezQuat(value.x, value.y, value.z, value.w); }

} // namespace ezModelImporter2
