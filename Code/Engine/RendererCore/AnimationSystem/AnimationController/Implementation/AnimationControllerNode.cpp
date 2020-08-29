#include <RendererCorePCH.h>

#include <AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationController/AnimationController.h>
#include <RendererCore/AnimationSystem/AnimationController/AnimationControllerNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerNode, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSampleAnimGraphNode, 1, ezRTTIDefaultAllocator<ezSampleAnimGraphNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClip, SetAnimationClip)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("RampUpTime", m_RampUp),
    EZ_MEMBER_PROPERTY("RampDownTime", m_RampDown),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimationControllerNode::ezAnimationControllerNode() = default;
ezAnimationControllerNode::~ezAnimationControllerNode() = default;

ezResult ezAnimationControllerNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);
  return EZ_SUCCESS;
}

ezResult ezAnimationControllerNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);
  return EZ_SUCCESS;
}

float ezSampleAnimGraphNode::UpdateWeight(ezTime tDiff)
{
  const ezVariant vValue = m_pOwner->m_Blackboard.GetEntryValue(m_sBlackboardEntry);

  if (!vValue.IsFloatingPoint())
    return 0.0f;

  const float fValue = vValue.ConvertTo<float>();

  if (m_fCurWeight < fValue)
  {
    m_fCurWeight += tDiff.AsFloatInSeconds() / m_RampUp.AsFloatInSeconds();
    m_fCurWeight = ezMath::Min(m_fCurWeight, fValue);
  }
  else if (m_fCurWeight > fValue)
  {
    m_fCurWeight -= tDiff.AsFloatInSeconds() / m_RampDown.AsFloatInSeconds();
    m_fCurWeight = ezMath::Max(0.0f, m_fCurWeight);
  }

  if (m_fCurWeight <= 0.0f)
  {
    m_PlaybackTime.SetZero();
  }

  return m_fCurWeight;
}

void ezSampleAnimGraphNode::Step(ezTime tDiff, const ezSkeletonResource* pSkeleton)
{
  if (!m_hAnimationClip.IsValid())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const auto& animDesc = pAnimClip->GetDescriptor();

  m_PlaybackTime += tDiff * m_fSpeed;
  if (m_fSpeed > 0 && m_PlaybackTime > animDesc.GetDuration())
  {
    m_PlaybackTime -= animDesc.GetDuration();
  }
  else if (m_fSpeed < 0 && m_PlaybackTime < ezTime::Zero())
  {
    m_PlaybackTime += animDesc.GetDuration();
  }

  const ozz::animation::Animation* pOzzAnimation = &animDesc.GetMappedOzzAnimation(*pSkeleton);

  if (m_ozzSamplingCache.max_tracks() != animDesc.GetNumJoints())
  {
    m_ozzSamplingCache.Resize(animDesc.GetNumJoints());
    m_ozzLocalTransforms.resize(pOzzSkeleton->num_soa_joints());
  }

  {
    ozz::animation::SamplingJob job;
    job.animation = pOzzAnimation;
    job.cache = &m_ozzSamplingCache;
    job.ratio = m_PlaybackTime.AsFloatInSeconds() / animDesc.GetDuration().AsFloatInSeconds();
    job.output = make_span(m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }
}

void ezSampleAnimGraphNode::SetAnimationClip(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  m_hAnimationClip = hResource;
}

const char* ezSampleAnimGraphNode::GetAnimationClip() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}

void ezSampleAnimGraphNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezSampleAnimGraphNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

ezResult ezSampleAnimGraphNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_RampUp;
  stream << m_RampDown;
  stream << m_PlaybackTime;
  stream << m_hAnimationClip;
  stream << m_fSpeed;

  return EZ_SUCCESS;
}

ezResult ezSampleAnimGraphNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_RampUp;
  stream >> m_RampDown;
  stream >> m_PlaybackTime;
  stream >> m_hAnimationClip;
  stream >> m_fSpeed;

  return EZ_SUCCESS;
}
