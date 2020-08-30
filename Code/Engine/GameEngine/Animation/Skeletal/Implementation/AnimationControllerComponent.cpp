#include <GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimationController/AnimationControllerResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimationControllerComponent, 1, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimController", GetAnimationControllerFile, SetAnimationControllerFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Controller")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAnimationControllerComponent::ezAnimationControllerComponent() = default;
ezAnimationControllerComponent::~ezAnimationControllerComponent() = default;

void ezAnimationControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hAnimationController;
}

void ezAnimationControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hAnimationController;
}

void ezAnimationControllerComponent::SetAnimationControllerFile(const char* szFile)
{
  ezAnimationControllerResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationControllerResource>(szFile);
  }

  m_hAnimationController = hResource;
}


const char* ezAnimationControllerComponent::GetAnimationControllerFile() const
{
  if (!m_hAnimationController.IsValid())
    return "";

  return m_hAnimationController.GetResourceID();
}

void ezAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimationController.IsValid())
    return;

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezAnimationControllerResource> pAnimController(m_hAnimationController, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimController.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  pAnimController->DeserializeAnimationControllerState(m_AnimationGraph);

  m_AnimationGraph.m_hSkeleton = msg.m_hSkeleton;

  ezHashedString hs;
  hs.Assign("Idle");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Left");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Right");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Forwards");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Backwards");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("A");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("B");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("X");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Y");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
}

void ezAnimationControllerComponent::Update()
{
  m_AnimationGraph.Update(GetWorld()->GetClock().GetTimeDiff());
  m_AnimationGraph.SendResultTo(GetOwner());

  float fValue;
  bool bActive = false;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegX, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Left", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosX, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Right", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegY, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Backwards", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosY, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Forwards", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonA, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("A", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonB, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("B", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonX, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("X", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonY, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Y", fValue);
  bActive |= fValue != 0;

  m_AnimationGraph.m_Blackboard.SetEntryValue("Idle", bActive ? 0.0f : 1.0f);
}
