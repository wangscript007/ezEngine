#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/ModelImporterDLL.h>

class ezLogInterface;
class ezProgress;
class ezEditableSkeleton;

namespace ezModelImporter2
{
  struct ImportOptions
  {
    bool m_bImportMeshes = false;
    bool m_bImportSkinningData = false;
    bool m_bImportSkeleton = false;
    bool m_bImportAnimations = false;

    ezString m_sSourceFile;
  };

  class EZ_MODELIMPORTER2_DLL Importer
  {
  public:
    Importer();
    virtual ~Importer();

    ezResult Import(const ImportOptions& options, ezLogInterface* pLogInterface = nullptr, ezProgress* pProgress = nullptr);

    ezUniquePtr<ezEditableSkeleton> m_pSkeletonResult;

  protected:
    virtual ezResult DoImport() = 0;

    ImportOptions m_Options;
    ezProgress* m_pProgress = nullptr;
  };

} // namespace ezModelImporter2
