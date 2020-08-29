#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

class ezSkeletonResource;
class ezAnimationController;
class ezStreamWriter;
class ezStreamReader;
using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

class EZ_RENDERERCORE_DLL ezAnimationControllerNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationControllerNode, ezReflectedClass);

public:
  ezAnimationControllerNode();
  virtual ~ezAnimationControllerNode();

  virtual float UpdateWeight(ezTime tDiff) = 0;
  virtual void Step(ezTime tDiff, const ezSkeletonResource* pSkeleton) = 0;

  virtual ezResult SerializeNode(ezStreamWriter& stream) const = 0;
  virtual ezResult DeserializeNode(ezStreamReader& stream) = 0;

protected:
  friend ezAnimationController;

  ezAnimationController* m_pOwner = nullptr;
  ozz::vector<ozz::math::SoaTransform> m_ozzLocalTransforms;
};

class EZ_RENDERERCORE_DLL ezSampleAnimGraphNode : public ezAnimationControllerNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleAnimGraphNode, ezAnimationControllerNode);

public:
  virtual float UpdateWeight(ezTime tDiff) override;
  virtual void Step(ezTime ov, const ezSkeletonResource* pSkeleton) override;

  void SetAnimationClip(const char* szFile); // [ property ]
  const char* GetAnimationClip() const;      // [ property ]

  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  ezTime m_RampUp;   // [ property ]
  ezTime m_RampDown; // [ property ]

  ezAnimationClipResourceHandle m_hAnimationClip;

private:
  ezHashedString m_sBlackboardEntry;
  ezTime m_PlaybackTime;
  ozz::animation::SamplingCache m_ozzSamplingCache;
  float m_fCurWeight = 0.0f;
};
