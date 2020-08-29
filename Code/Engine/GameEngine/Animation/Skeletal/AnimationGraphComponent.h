#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimationController/AnimationController.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

using ezAnimationGraphComponentManager = ezComponentManagerSimple<class ezAnimationGraphComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezAnimationGraphComponent : public ezAnimationControllerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimationGraphComponent, ezAnimationControllerComponent, ezAnimationGraphComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJointAttachmentComponent

public:
  ezAnimationGraphComponent();
  ~ezAnimationGraphComponent();

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  const ezSkeletonResourceHandle& GetSkeleton() const;

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetAnimClip0(const char* szFile); // [ property ]
  const char* GetAnimClip0() const;      // [ property ]
  void SetAnimClip1(const char* szFile); // [ property ]
  const char* GetAnimClip1() const;      // [ property ]
  void SetAnimClip2(const char* szFile); // [ property ]
  const char* GetAnimClip2() const;      // [ property ]

protected:
  void Update();

  ezSkeletonResourceHandle m_hSkeleton;

  ezAnimationController m_AnimationGraph;

  ezAnimationClipResourceHandle m_hAnimationClip0;
  ezAnimationClipResourceHandle m_hAnimationClip1;
  ezAnimationClipResourceHandle m_hAnimationClip2;
};
