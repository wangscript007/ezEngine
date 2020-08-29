#include <GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimationGraphComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimationGraphComponent, 1, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_ACCESSOR_PROPERTY("AnimClip0", GetAnimClip0, SetAnimClip0)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimClip1", GetAnimClip1, SetAnimClip1)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("AnimClip2", GetAnimClip2, SetAnimClip2)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
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

ezAnimationGraphComponent::ezAnimationGraphComponent() = default;
ezAnimationGraphComponent::~ezAnimationGraphComponent() = default;

void ezAnimationGraphComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hSkeleton;
  s << m_hAnimationClip0;
  s << m_hAnimationClip1;
  s << m_hAnimationClip2;
}

void ezAnimationGraphComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hSkeleton;
  s >> m_hAnimationClip0;
  s >> m_hAnimationClip1;
  s >> m_hAnimationClip2;
}

void ezAnimationGraphComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  m_hSkeleton = hResource;
}

const ezSkeletonResourceHandle& ezAnimationGraphComponent::GetSkeleton() const
{
  return m_hSkeleton;
}

void ezAnimationGraphComponent::SetSkeletonFile(const char* szFile)
{
  ezSkeletonResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* ezAnimationGraphComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}


void ezAnimationGraphComponent::SetAnimClip0(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClip0 = hResource;
}


const char* ezAnimationGraphComponent::GetAnimClip0() const
{
  if (!m_hAnimationClip0.IsValid())
    return "";

  return m_hAnimationClip0.GetResourceID();
}

void ezAnimationGraphComponent::SetAnimClip1(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClip1 = hResource;
}

const char* ezAnimationGraphComponent::GetAnimClip1() const
{
  if (!m_hAnimationClip1.IsValid())
    return "";

  return m_hAnimationClip1.GetResourceID();
}


void ezAnimationGraphComponent::SetAnimClip2(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClip2 = hResource;
}

const char* ezAnimationGraphComponent::GetAnimClip2() const
{
  if (!m_hAnimationClip2.IsValid())
    return "";

  return m_hAnimationClip2.GetResourceID();
}

void ezAnimationGraphComponent::Update()
{
  if (!m_hSkeleton.IsValid())
    return;

  m_AnimationGraph.Update(GetWorld()->GetClock().GetTimeDiff());
  m_AnimationGraph.SendResultTo(GetOwner());

  float fValue;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegX, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Left", fValue);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosX, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Right", fValue);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegY, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Backwards", fValue);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosY, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Forwards", fValue);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonA, &fValue);
  m_AnimationGraph.m_Blackboard.SetEntryValue("Idle", fValue);
}

void ezAnimationGraphComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezHashedString hs;

  m_AnimationGraph.m_hSkeleton = m_hSkeleton;

  hs.Assign("Left");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Right");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Forwards");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Backwards");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);
  hs.Assign("Idle");
  m_AnimationGraph.m_Blackboard.RegisterEntry(hs, 0.0f);

  if (m_hAnimationClip0.IsValid())
  {
    ezUniquePtr<ezSampleAnimGraphNode> pNode = EZ_DEFAULT_NEW(ezSampleAnimGraphNode);
    pNode->m_hAnimationClip = m_hAnimationClip0;
    pNode->m_RampUp = ezTime::Seconds(0.2);
    pNode->m_RampDown = ezTime::Seconds(0.4);
    pNode->m_sBlackboardEntry = "Forwards";
    m_AnimationGraph.m_Nodes.PushBack(std::move(pNode));
  }

  if (m_hAnimationClip1.IsValid())
  {
    ezUniquePtr<ezSampleAnimGraphNode> pNode = EZ_DEFAULT_NEW(ezSampleAnimGraphNode);
    pNode->m_hAnimationClip = m_hAnimationClip1;
    pNode->m_RampUp = ezTime::Seconds(0.2);
    pNode->m_RampDown = ezTime::Seconds(0.4);
    pNode->m_sBlackboardEntry = "Backwards";
    m_AnimationGraph.m_Nodes.PushBack(std::move(pNode));
  }

  if (m_hAnimationClip2.IsValid())
  {
    ezUniquePtr<ezSampleAnimGraphNode> pNode = EZ_DEFAULT_NEW(ezSampleAnimGraphNode);
    pNode->m_hAnimationClip = m_hAnimationClip2;
    pNode->m_RampUp = ezTime::Seconds(0.5);
    pNode->m_RampDown = ezTime::Seconds(0.3);
    pNode->m_sBlackboardEntry = "Idle";
    m_AnimationGraph.m_Nodes.PushBack(std::move(pNode));
  }
}
