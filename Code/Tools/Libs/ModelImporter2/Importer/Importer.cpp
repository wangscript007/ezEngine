#include <ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

namespace ezModelImporter2
{
  Importer::Importer() = default;
  Importer::~Importer() = default;

  ezResult Importer::Import(const ImportOptions& options, ezLogInterface* pLogInterface /*= nullptr*/, ezProgress* pProgress /*= nullptr*/)
  {
    ezResult res = EZ_FAILURE;

    ezLogInterface* pPrevLogSystem = ezLog::GetThreadLocalLogSystem();

    if (pLogInterface)
    {
      ezLog::SetThreadLocalLogSystem(pLogInterface);
    }

    {
      m_pProgress = pProgress;
      m_Options = options;

      EZ_LOG_BLOCK("ModelImport", m_Options.m_sSourceFile);

      m_pSkeletonResult = EZ_DEFAULT_NEW(ezEditableSkeleton);

      res = DoImport();
    }


    ezLog::SetThreadLocalLogSystem(pPrevLogSystem);

    return res;
  }

} // namespace ezModelImporter2
