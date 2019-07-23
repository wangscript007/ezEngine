
#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

#include <RendererCore/Components/RenderComponent.h>

class ezPathVisualizeComponent;
struct ezMsgExtractRenderData;

typedef ezComponentManager<ezPathVisualizeComponent, ezBlockStorageType::Compact> ezPathVisualizeComponentManager;

/// \brief A simple path visualize component 
class EZ_GAMEENGINE_DLL ezPathVisualizeComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPathVisualizeComponent, ezRenderComponent, ezPathVisualizeComponentManager);

public:
  ezPathVisualizeComponent();
  ~ezPathVisualizeComponent();

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;


};
