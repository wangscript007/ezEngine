#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/ModelImporterDLL.h>

class ezLogInterface;
class ezProgress;
class ezEditableSkeleton;
class ezMeshResourceDescriptor;

namespace ezModelImporter2
{
  struct ImportOptions
  {
    bool m_bImportSkinningData = false;
    bool m_bImportAnimations = false;

    ezMeshResourceDescriptor* m_pMeshOutput = nullptr;
    ezEditableSkeleton* m_pSkeletonOutput = nullptr;

    ezString m_sSourceFile;
  };

  class EZ_MODELIMPORTER2_DLL Importer
  {
  public:
    Importer();
    virtual ~Importer();

    ezResult Import(const ImportOptions& options, ezLogInterface* pLogInterface = nullptr, ezProgress* pProgress = nullptr);

  protected:
    virtual ezResult DoImport() = 0;

    ImportOptions m_Options;
    ezProgress* m_pProgress = nullptr;
  };

} // namespace ezModelImporter2
