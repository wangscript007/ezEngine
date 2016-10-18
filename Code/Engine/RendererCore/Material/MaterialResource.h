#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;

struct ezMaterialResourceDescriptor
{
  struct Parameter
  {
    ezTempHashedString m_Name;
    ezVariant m_Value;
  };

  struct TextureBinding
  {
    ezHashedString m_Name;
    ezTextureResourceHandle m_Value;
  };

  void Clear()
  {
    m_hBaseMaterial.Invalidate();
    m_hShader.Invalidate();
    m_PermutationVars.Clear();
    m_Parameters.Clear();
    m_TextureBindings.Clear();
  }

  ezMaterialResourceHandle m_hBaseMaterial;
  ezShaderResourceHandle m_hShader;
  ezDynamicArray<ezPermutationVar> m_PermutationVars;
  ezDynamicArray<Parameter> m_Parameters;
  ezDynamicArray<TextureBinding> m_TextureBindings;
};

class EZ_RENDERERCORE_DLL ezMaterialResource : public ezResource<ezMaterialResource, ezMaterialResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource, ezResourceBase);

public:
  ezMaterialResource();
  ~ezMaterialResource();

  ezHashedString GetPermutationValue(const ezTempHashedString& sName);

  void SetParameter(const ezTempHashedString& sName, const ezVariant& value);
  ezVariant GetParameter(const ezTempHashedString& sName);

  void SetTextureBinding(const ezHashedString& sName, ezTextureResourceHandle value);
  ezTextureResourceHandle GetTextureBinding(const ezTempHashedString& sName);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMaterialResourceDescriptor& descriptor) override;

private:
  ezMaterialResourceDescriptor m_Desc;

  friend class ezRenderContext;

  ezEvent<const ezMaterialResource*> m_ModifiedEvent;
  void OnBaseMaterialModified(const ezMaterialResource* pModifiedMaterial);
  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  ezAtomicInteger32 m_iLastModified;
  ezAtomicInteger32 m_iLastConstantsModified;
  ezInt32 m_iLastUpdated;
  ezInt32 m_iLastConstantsUpdated;

  bool IsModified();
  bool AreContantsModified();

  void UpdateCaches();
  void UpdateConstantBuffer(ezShaderPermutationResource* pShaderPermutation);

  ezConstantBufferStorageHandle m_hConstantBufferStorage;

  ezMutex m_CacheMutex;
  ezShaderResourceHandle m_hCachedShader;
  ezHashTable<ezHashedString, ezHashedString> m_CachedPermutationVars;
  ezHashTable<ezTempHashedString, ezVariant> m_CachedParameters;
  ezHashTable<ezHashedString, ezTextureResourceHandle> m_CachedTextureBindings;
};
