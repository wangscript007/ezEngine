#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

#include <GameEngine/Path/PathComponent.h>
#include <GameEngine/Path/PathVisualizeComponent.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>


// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPathVisualizeComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on


ezPathVisualizeComponent::ezPathVisualizeComponent() = default;
ezPathVisualizeComponent::~ezPathVisualizeComponent() = default;

ezResult ezPathVisualizeComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  // TODO: Use bounds of Bezier path of component
  bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezPathVisualizeComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  const ezPathComponent* pPathComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<ezPathComponent>(pPathComponent))
  {
    //auto knots = pPathComponent->GetKnots();

    ezHybridArray<ezDebugRenderer::Line, 64> lines;

    ezTransform t = GetOwner()->GetGlobalTransform();


    auto segments = pPathComponent->GetBezierPath().GetSegments();

    const ezUInt32 numSteps = 16;
    const float tStep = 1.0f / numSteps;

    for (auto segment : segments)
    {
      ezVec3 start = segment.EvaluateAt(0.0f);

      for (ezUInt32 i = 1; i <= numSteps; ++i)
      {
        ezVec3 current = segment.EvaluateAt(tStep * i);

        lines.PushBack({t.TransformPosition(start), t.TransformPosition(current)});

        start = current;
      }
    }

    if (!segments.IsEmpty())
    {
      ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::GreenYellow);
    }
  }
}
