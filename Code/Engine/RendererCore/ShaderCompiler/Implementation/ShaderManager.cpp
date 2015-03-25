#include <RendererCore/PCH.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererFoundation/Context/Context.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <RendererCore/Shader/ShaderStateResource.h>

const ezPermutationGenerator* ezRendererCore::GetGeneratorForShaderPermutation(ezUInt32 uiPermutationHash)
{
  auto it = s_PermutationHashCache.Find(uiPermutationHash);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void ezRendererCore::LoadShaderPermutationVarConfig(const char* szVariable)
{
  ezStringBuilder sPath;
  sPath.Format("%s/%s.ezPermVar", s_sPermVarSubDir.GetData(), szVariable);

  // clear earlier data
  s_AllowedPermutations.RemoveVariable(szVariable);

  if (s_AllowedPermutations.ReadFromFile(sPath, s_sPlatform).Failed())
    ezLog::Error("Could not read shader permutation variable '%s' from file '%s'", szVariable, sPath.GetData());
}

void ezRendererCore::PreloadShaderPermutations(ezShaderResourceHandle hShader, const ezPermutationGenerator& MainGenerator, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  ezPermutationGenerator Generator = MainGenerator;
  Generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;

  for (ezUInt32 p = 0; p < Generator.GetPermutationCount(); ++p)
  {
    Generator.GetPermutation(p, UsedPermVars);

    PreloadSingleShaderPermutation(hShader, UsedPermVars, tShouldBeAvailableIn);
  }
}

ezShaderPermutationResourceHandle ezRendererCore::PreloadSingleShaderPermutation(ezShaderResourceHandle hShader, const ezHybridArray<ezPermutationGenerator::PermutationVar, 16>& UsedPermVars, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  if (!pShader->IsShaderValid())
    return ezShaderPermutationResourceHandle();

  const ezUInt32 uiPermutationHash = ezPermutationGenerator::GetHash(UsedPermVars);

  /// \todo Mutex

  bool bExisted = false;
  auto itPermCache = s_PermutationHashCache.FindOrAdd(uiPermutationHash, &bExisted);

  if (!bExisted)
  {
    // store this set of permutations in a generator
    for (ezUInt32 pv = 0; pv < UsedPermVars.GetCount(); ++pv)
      itPermCache.Value().AddPermutation(UsedPermVars[pv].m_sVariable.GetData(), UsedPermVars[pv].m_sValue.GetData());
  }

  ezStringBuilder sShaderFile = GetShaderCacheDirectory();
  sShaderFile.AppendPath(GetActiveShaderPlatform().GetData());
  sShaderFile.AppendPath(pShader->GetResourceID().GetData());
  sShaderFile.ChangeFileExtension("");
  if (sShaderFile.EndsWith("."))
    sShaderFile.Shrink(0, 1);
  sShaderFile.AppendFormat("%08X.ezPermutation", uiPermutationHash);

  ezShaderPermutationResourceHandle hShaderPermutation = ezResourceManager::LoadResource<ezShaderPermutationResource>(sShaderFile.GetData());

  ezResourceManager::PreloadResource(hShaderPermutation, tShouldBeAvailableIn);

  return hShaderPermutation;
}

void ezRendererCore::ConfigureShaderSystem(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  s_ShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;

  ezStringBuilder s = szActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;

  s_ContextState.Clear();

  // initialize all permutation variables
  for (auto it = s_AllowedPermutations.GetPermutationSet().GetIterator(); it.IsValid(); ++it)
  {
    SetShaderPermutationVariable(it.Key().GetData(), it.Value().GetIterator().Key().GetData());
  }
}

void ezRendererCore::SetShaderPermutationVariable(const char* szVariable, const char* szValue, ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& state = s_ContextState[pContext];

  ezStringBuilder sVar, sVal;
  sVar = szVariable;
  sVal = szValue;

  sVar.ToUpper();
  sVal.ToUpper();

  /// \todo Could we use hashed variable names here ?
  bool bExisted = false;
  auto itVar = state.m_PermutationVariables.FindOrAdd(sVar, &bExisted);

  if (!bExisted)
  {
    LoadShaderPermutationVarConfig(sVar);
  }

  if (s_bEnableRuntimeCompilation && !s_AllowedPermutations.IsValueAllowed(sVar.GetData(), sVal.GetData()))
  {
    ezLog::Debug("Invalid Shader Permutation: '%s' cannot be set to value '%s' -> reloading config for variable", sVar.GetData(), sVal.GetData());
    LoadShaderPermutationVarConfig(sVar);

    if (!s_AllowedPermutations.IsValueAllowed(sVar.GetData(), sVal.GetData()))
    {
      ezLog::Error("Invalid Shader Permutation: '%s' cannot be set to value '%s'", sVar.GetData(), sVal.GetData());
      return;
    }
  }

  if (itVar.Value() != sVal)
    state.m_bShaderStateChanged = true;

  itVar.Value() = sVal;
}

void ezRendererCore::SetActiveShader(ezShaderResourceHandle hShader, ezGALContext* pContext, ezBitflags<ezShaderBindFlags> flags)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& state = s_ContextState[pContext];

  state.m_ShaderBindFlags = flags;

  if (flags.IsAnySet(ezShaderBindFlags::ForceRebind) || state.m_hActiveShader != hShader)
    state.m_bShaderStateChanged = true;

  state.m_hActiveShader = hShader;
}

ezGALShaderHandle ezRendererCore::GetActiveGALShader(ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& state = s_ContextState[pContext];

  // make sure the internal state is up to date
  SetShaderContextState(pContext, state, false);

  if (!state.m_bShaderStateValid)
    return ezGALShaderHandle(); // invalid handle

  return state.m_hActiveGALShader;
}

void ezRendererCore::SetShaderContextState(ezGALContext* pContext, ContextState& state, bool bForce)
{
  ezShaderPermutationResource* pShaderPermutation = nullptr;

  if (bForce || state.m_bShaderStateChanged)
  {
    state.m_bShaderStateChanged = false;
    state.m_bShaderStateValid = false;
    state.m_bTextureBindingsChanged = true;
    state.m_bConstantBufferBindingsChanged = true;

    if (!state.m_hActiveShader.IsValid())
      return;

    ezResourceLock<ezShaderResource> pShader(state.m_hActiveShader, ezResourceAcquireMode::AllowFallback);

    if (!pShader->IsShaderValid())
      return;

    state.m_PermGenerator.Clear();
    for (auto itPerm = state.m_PermutationVariables.GetIterator(); itPerm.IsValid(); ++itPerm)
      state.m_PermGenerator.AddPermutation(itPerm.Key().GetData(), itPerm.Value().GetData());

    state.m_PermGenerator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

    EZ_ASSERT_DEV(state.m_PermGenerator.GetPermutationCount() == 1, "Invalid shader setup");

    ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;
    state.m_PermGenerator.GetPermutation(0, UsedPermVars);

    state.m_hActiveShaderPermutation = PreloadSingleShaderPermutation(state.m_hActiveShader, UsedPermVars, ezTime::Seconds(0.0));

    if (!state.m_hActiveShaderPermutation.IsValid())
      return;

    pShaderPermutation = ezResourceManager::BeginAcquireResource(state.m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    if (!pShaderPermutation->IsShaderValid())
    {
      ezResourceManager::EndAcquireResource(pShaderPermutation);
      return;
    }

    state.m_hActiveGALShader = pShaderPermutation->GetGALShader();

    pContext->SetShader(state.m_hActiveGALShader);

    // Set render state from shader (unless they are all deactivated)
    if (!state.m_ShaderBindFlags.AreAllSet(ezShaderBindFlags::NoBlendState | ezShaderBindFlags::NoRasterizerState | ezShaderBindFlags::NoDepthStencilState))
    {
      ezResourceLock<ezShaderStateResource> StateRes(pShaderPermutation->GetShaderStateResource());

      if (!state.m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoBlendState))
        pContext->SetBlendState(StateRes->GetBlendState());

      if (!state.m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoRasterizerState))
        pContext->SetRasterizerState(StateRes->GetRasterizerState());

      if (!state.m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoDepthStencilState))
        pContext->SetDepthStencilState(StateRes->GetDepthStencilState());
    }

    state.m_bShaderStateValid = true;
  }

  if ((bForce || state.m_bTextureBindingsChanged) && state.m_hActiveShaderPermutation.IsValid())
  {
    state.m_bTextureBindingsChanged = false;

    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(state.m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyTextureBindings(pContext, (ezGALShaderStage::Enum) stage, pBin);
    }
  }

  UploadGlobalConstants(pContext);

  if ((bForce || state.m_bConstantBufferBindingsChanged) && state.m_hActiveShaderPermutation.IsValid())
  {
    state.m_bConstantBufferBindingsChanged = false;

    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(state.m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyConstantBufferBindings(pContext, pBin);
    }
  }

  if (pShaderPermutation != nullptr)
    ezResourceManager::EndAcquireResource(pShaderPermutation);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);

