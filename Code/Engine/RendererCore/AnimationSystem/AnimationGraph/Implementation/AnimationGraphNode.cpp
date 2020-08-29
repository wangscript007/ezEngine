#include <RendererCorePCH.h>

#include <AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationGraphNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

ezAnimationGraphNode::ezAnimationGraphNode() = default;
ezAnimationGraphNode::~ezAnimationGraphNode() = default;

void ezSampleAnimGraphNode::Step(ezTime tDiff, const ezSkeletonResource* pSkeleton)
{
  if (!m_hAnimationClip.IsValid())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  const auto& animDesc = pAnimClip->GetDescriptor();

  m_PlaybackTime += tDiff;
  if (m_PlaybackTime > animDesc.GetDuration())
  {
    m_PlaybackTime -= animDesc.GetDuration();
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
