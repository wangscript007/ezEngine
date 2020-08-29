#include <RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>

ezAnimationGraph::ezAnimationGraph() = default;
ezAnimationGraph::~ezAnimationGraph() = default;

void ezAnimationGraph::Update(ezTime tDiff)
{
  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  ezHybridArray<ozz::animation::BlendingJob::Layer, 8> layers;

  for (const auto& pNode : m_Nodes)
  {
    pNode->Step(tDiff, pSkeleton.GetPointer());

    auto& l = layers.ExpandAndGetRef();
    l.transform = make_span(pNode->m_ozzLocalTransforms);
    l.weight = 1.0f / m_Nodes.GetCount();
  }

  {
    m_ozzLocalTransforms.resize(pOzzSkeleton->num_soa_joints());

    ozz::animation::BlendingJob job;
    job.threshold = 0.1f;
    job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(layers), end(layers));
    job.bind_pose = pOzzSkeleton->joint_bind_poses();
    job.output = make_span(m_ozzLocalTransforms);
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }

  m_bFinalized = false;
}

void ezAnimationGraph::Finalize(const ezSkeletonResource* pSkeleton)
{
  if (m_bFinalized)
    return;

  m_bFinalized = true;

  const ozz::animation::Skeleton* pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  {
    m_ModelSpaceTransforms.SetCountUninitialized(pOzzSkeleton->num_joints());

    ozz::animation::LocalToModelJob job;
    job.input = make_span(m_ozzLocalTransforms);
    job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(begin(m_ModelSpaceTransforms)), reinterpret_cast<ozz::math::Float4x4*>(end(m_ModelSpaceTransforms)));
    job.skeleton = pOzzSkeleton;
    EZ_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }
}

void ezAnimationGraph::SendResultTo(ezGameObject* pObject)
{
  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  Finalize(pSkeleton.GetPointer());

  ezMsgAnimationPoseUpdated msg;
  msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_ModelSpaceTransforms;

  pObject->SendMessageRecursive(msg);
}
