//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef STATIC_MESH_COMPONENT_H
#define STATIC_MESH_COMPONENT_H

#include "renderComponent.h"

#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _SCENERENDERSTATE_H_
#include "scene/sceneRenderState.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef ENTITY_H
   #include "../../entity.h"
#endif
#ifndef _NETSTRINGTABLE_H_
   #include "sim/netStringTable.h"
#endif

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif 
#ifndef _SHAPE_ASSET_H_
#include "T3D/assets/ShapeAsset.h"
#endif 
#ifndef _GFXVERTEXFORMAT_H_
#include "gfx/gfxVertexFormat.h"
#endif

class TSShapeInstance;
class SceneRenderState;
//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class MeshComponent : public renderComponent
{
   typedef renderComponent Parent;

protected:
   enum
   {
      ShapeMask = Parent::NextFreeMask,
      MaterialMask = Parent::NextFreeMask << 1,
      NextFreeMask = Parent::NextFreeMask << 2,
   };

   DECLARE_SHAPEASSET(MeshComponent, Mesh);

   Resource<TSShape> mShape;
   TSShapeInstance* mShapeInstance;
   //Box3F						mShapeBounds;
   Point3F					mCenterOffset;

   struct matMap
   {
      MaterialAsset* matAsset;
      String assetId;
      U32 slot;
   };

   Vector<matMap>  mChangingMaterials;
   Vector<matMap>  mMaterials;
   Box3F mBounds;
   Point3F mScale;
   MatrixF mTransform;

public:
   MeshComponent();
   virtual ~MeshComponent();
   DECLARE_CONOBJECT(MeshComponent);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void inspectPostApply();

   virtual void prepRenderImage(SceneRenderState *state);

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   Box3F getShapeBounds() { return mBounds; }

   virtual MatrixF getNodeTransform(S32 nodeIdx);
   S32 getNodeByName(String nodeName);

   void setupShape();
   void updateShape();
   void updateMaterials();

   void _shapeAssetUpdated(ShapeAsset* asset);

   virtual void onComponentRemove();
   virtual void onComponentAdd();

   virtual void ownerTransformSet(MatrixF *mat);

   static bool writeShape(void* obj, StringTableEntry pFieldName) { return static_cast<MeshComponent*>(obj)->mMeshAsset.notNull(); }

   virtual TSShape* getShape() { if (mMeshAsset)  return mMeshAsset->getShape(); else return NULL; }
   virtual TSShapeInstance* getShapeInstance() { return mShapeInstance; }

   Resource<TSShape> getShapeResource() { return mMeshAsset->getShapeResource(); }

   void _onResourceChanged(const Torque::Path &path);

   virtual bool castRayRendered(const Point3F &start, const Point3F &end, RayInfo *info);

   void mountObjectToNode(SceneObject* objB, String node, MatrixF txfm);

   virtual void onDynamicModified(const char* slotName, const char* newValue);

   void changeMaterial(U32 slot, MaterialAsset* newMat);
   bool setMatInstField(U32 slot, const char* field, const char* value);

   virtual void onInspect();
   virtual void onEndInspect();

   virtual Vector<MatrixF> getNodeTransforms()
   {
      Vector<MatrixF> bob;
      return bob;
   }

   virtual void setNodeTransforms(Vector<MatrixF> transforms)
   {
      return;
   }

   Signal< void(MeshComponent*) > onShapeChanged;
   Signal< void(MeshComponent*) > onShapeInstanceChanged;
};

#endif
