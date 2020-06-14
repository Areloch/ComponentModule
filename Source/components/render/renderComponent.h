#pragma once

#include "Components/Source/components/component.h"
#include "scene/sceneRenderState.h"

#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif

class renderComponent : public Component
{
public:
   renderComponent();

   virtual void render(SceneRenderState* state);

   virtual bool castRayRendered(const Point3F& start, const Point3F& end, RayInfo* info) = 0;

   virtual MatrixF getNodeTransform(S32 nodeIdx) {  return MatrixF::Identity; }
   virtual S32 getNodeByName(String nodeName) { return -1; }

   virtual TSShape* getShape() { return nullptr; }
   virtual TSShapeInstance* getShapeInstance() { return nullptr; }
};
