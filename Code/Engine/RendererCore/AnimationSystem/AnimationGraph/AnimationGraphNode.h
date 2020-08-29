#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezSkeletonResource;
class ezAnimationGraph;
using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

class EZ_RENDERERCORE_DLL ezAnimationGraphNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationGraphNode, ezReflectedClass);

public:
  ezAnimationGraphNode();
  virtual ~ezAnimationGraphNode();

  virtual float UpdateWeight(ezTime tDiff) = 0;
  virtual void Step(ezTime tDiff, const ezSkeletonResource* pSkeleton) = 0;

protected:
  friend ezAnimationGraph;

  ezAnimationGraph* m_pOwner = nullptr;
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
};

class EZ_RENDERERCORE_DLL ezSampleAnimGraphNode : public ezAnimationGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleAnimGraphNode, ezAnimationGraphNode);

public:
  virtual float UpdateWeight(ezTime tDiff) override;
  virtual void Step(ezTime ov, const ezSkeletonResource* pSkeleton) override;

  void SetAnimationClip(const char* szFile); // [ property ]
  const char* GetAnimationClip() const;      // [ property ]

  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

  ezTime m_RampUp;   // [ property ]
  ezTime m_RampDown; // [ property ]

  ezAnimationClipResourceHandle m_hAnimationClip;

private:
  ezHashedString m_sBlackboardEntry;
  ezTime m_PlaybackTime;
  ozz::animation::SamplingCache m_ozzSamplingCache;
  float m_fCurWeight = 0.0f;
};
