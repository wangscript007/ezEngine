#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezSkeletonResource;
using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

class EZ_RENDERERCORE_DLL ezAnimationGraphNode
{
public:
  ezAnimationGraphNode();
  virtual ~ezAnimationGraphNode();

  virtual void Step(ezTime tDiff, const ezSkeletonResource* pSkeleton) = 0;

  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
};

class EZ_RENDERERCORE_DLL ezSampleAnimGraphNode : public ezAnimationGraphNode
{
public:
  virtual void Step(ezTime tDiff, const ezSkeletonResource* pSkeleton);

  ezTime m_PlaybackTime;
  ezAnimationClipResourceHandle m_hAnimationClip;
  ozz::animation::SamplingCache m_ozzSamplingCache;
};
