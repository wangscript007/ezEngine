#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationControllerNode.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezGameObject;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

class EZ_RENDERERCORE_DLL ezAnimationController
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAnimationController);

public:
  ezAnimationController();
  ~ezAnimationController();

  void Update(ezTime tDiff);
  void SendResultTo(ezGameObject* pObject);

  ezDynamicArray<ezUniquePtr<ezAnimationControllerNode>> m_Nodes;

  ezSkeletonResourceHandle m_hSkeleton;

  ezBlackboard m_Blackboard;

private:
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_ModelSpaceTransforms;

  bool m_bFinalized = false;
  void Finalize(const ezSkeletonResource* pSkeleton);
};
