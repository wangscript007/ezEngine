#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/AnimationSystem/AnimationController/AnimationController.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezAnimationControllerResourceHandle = ezTypedResourceHandle<class ezAnimationControllerResource>;

using ezAnimationControllerComponentManager = ezComponentManagerSimple<class ezAnimationControllerComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezAnimationControllerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimationControllerComponent, ezComponent, ezAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimationControllerComponent

public:
  ezAnimationControllerComponent();
  ~ezAnimationControllerComponent();

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  const ezSkeletonResourceHandle& GetSkeleton() const;

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetAnimationControllerFile(const char* szFile); // [ property ]
  const char* GetAnimationControllerFile() const;      // [ property ]

protected:
  void Update();

  ezAnimationControllerResourceHandle m_hAnimationController;
  ezSkeletonResourceHandle m_hSkeleton;
  ezAnimationController m_AnimationGraph;
};
