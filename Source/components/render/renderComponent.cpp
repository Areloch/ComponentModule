#include "renderComponent.h"

renderComponent::renderComponent()
{
   mComponentType = StringTable->insert("renderComponent");
}

void renderComponent::render(SceneRenderState* state)
{
}
