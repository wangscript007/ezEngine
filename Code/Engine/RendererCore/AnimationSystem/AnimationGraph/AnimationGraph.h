#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationGraphNode.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezGameObject;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

class EZ_RENDERERCORE_DLL ezAnimationGraph
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimationGraph);

public:
  ezAnimationGraph();
  ~ezAnimationGraph();

  void Update(ezTime tDiff);
  void SendResultTo(ezGameObject* pObject);

  ezDynamicArray<ezUniquePtr<ezAnimationGraphNode>> m_Nodes;

  ezSkeletonResourceHandle m_hSkeleton;

private:
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_ModelSpaceTransforms;

  bool m_bFinalized = false;
  void Finalize(const ezSkeletonResource* pSkeleton);
};
