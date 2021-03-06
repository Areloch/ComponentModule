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

#include "shapeCollisionComponent.h"
#include "shapeCollisionComponent_ScriptBinding.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"
#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/physicsBody.h"
#include "T3D/physics/physicsCollision.h"
#include "T3D/gameBase/gameConnection.h"
#include "collision/extrudedPolyList.h"
#include "math/mathIO.h"
#include "gfx/sim/debugDraw.h"  
#include "collision/concretePolyList.h"

#include "T3D/trigger.h"
#include "opcode/Opcode.h"
#include "opcode/Ice/IceAABB.h"
#include "opcode/Ice/IcePoint.h"
#include "opcode/OPC_AABBTree.h"
#include "opcode/OPC_AABBCollider.h"

#include "math/mathUtils.h"
#include "materials/baseMatInstance.h"
#include "collision/vertexPolyList.h"

extern bool gEditingMission;

static bool sRenderColliders = false;

//Docs
ConsoleDocClass(ShapeCollisionComponent,
   "@brief The Box Collider component uses a box or rectangular convex shape for collisions.\n\n"

   "Colliders are individualized components that are similarly based off the CollisionInterface core.\n"
   "They are basically the entire functionality of how Torque handles collisions compacted into a single component.\n"
   "A collider will both collide against and be collided with, other entities.\n"
   "Individual colliders will offer different shapes. This box collider will generate a box/rectangle convex, \n"
   "while the mesh collider will take the owner Entity's rendered shape and do polysoup collision on it, etc.\n\n"

   "The general flow of operations for how collisions happen is thus:\n"
   "  -When the component is added(or updated) prepCollision() is called.\n"
   "    This will set up our initial convex shape for usage later.\n\n"

   "  -When we update via processTick(), we first test if our entity owner is mobile.\n"
   "    If our owner isn't mobile(as in, they have no components that provide it a velocity to move)\n"
   "    then we skip doing our active collision checks. Collisions are checked by the things moving, as\n"
   "    opposed to being reactionary. If we're moving, we call updateWorkingCollisionSet().\n"
   "    updateWorkingCollisionSet() estimates our bounding space for our current ticket based on our position and velocity.\n"
   "    If our bounding space has changed since the last tick, we proceed to call updateWorkingList() on our convex.\n"
   "    This notifies any object in the bounding space that they may be collided with, so they will call buildConvex().\n"
   "    buildConvex() will set up our ConvexList with our collision convex info.\n\n"

   "  -When the component that is actually causing our movement, such as SimplePhysicsBehavior, updates, it will check collisions.\n"
   "    It will call checkCollisions() on us. checkCollisions() will first build a bounding shape for our convex, and test\n"
   "    if we can early out because we won't hit anything based on our starting point, velocity, and tick time.\n"
   "    If we don't early out, we proceed to call updateCollisions(). This builds an ExtrudePolyList, which is then extruded\n"
   "    based on our velocity. We then test our extruded polies on our working list of objects we build\n"
   "    up earlier via updateWorkingCollisionSet. Any collisions that happen here will be added to our mCollisionList.\n"
   "    Finally, we call handleCollisionList() on our collisionList, which then queues out the colliison notice\n"
   "    to the object(s) we collided with so they can do callbacks and the like. We also report back on if we did collide\n"
   "    to the physics component via our bool return in checkCollisions() so it can make the physics react accordingly.\n\n"

   "One interesting point to note is the usage of mBlockColliding.\n"
   "This is set so that it dictates the return on checkCollisions(). If set to false, it will ensure checkCollisions()\n"
   "will return false, regardless if we actually collided. This is useful, because even if checkCollisions() returns false,\n"
   "we still handle the collisions so the callbacks happen. This enables us to apply a collider to an object that doesn't block\n"
   "objects, but does have callbacks, so it can act as a trigger, allowing for arbitrarily shaped triggers, as any collider can\n"
   "act as a trigger volume(including MeshCollider).\n\n"

   "@tsexample\n"
   "new ShapeCollisionComponentInstance()\n"
   "{\n"
   "   template = ShapeCollisionComponentTemplate;\n"
   "   colliderSize = \"1 1 2\";\n"
   "   blockColldingObject = \"1\";\n"
   "};\n"
   "@endtsexample\n"

   "@see SimplePhysicsBehavior\n"
   "@ingroup Collision\n"
   "@ingroup Components\n"
   );
//Docs

/////////////////////////////////////////////////////////////////////////
ImplementEnumType(CollisionMeshMeshType,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ShapeCollisionComponent::None, "None", "No mesh data." },
{ ShapeCollisionComponent::Bounds, "Bounds", "Bounding box of the shape." },
{ ShapeCollisionComponent::CollisionMesh, "Collision Mesh", "Specifically desingated \"collision\" meshes." },
{ ShapeCollisionComponent::VisibleMesh, "Visible Mesh", "Rendered mesh polygons." },
EndImplementEnumType;

//
ShapeCollisionComponent::ShapeCollisionComponent() : CollisionComponent()
{
   mFriendlyName = "Shape Collision Component";

   mFriendlyName = "Shape Collision";

   mDescription = getDescriptionText("A stub component class that physics components should inherit from.");

   mOwnerShapeComponent = NULL;
   mOwnerPhysicsComp = NULL;

   mBlockColliding = true;

   mCollisionType = CollisionMesh;
   mLOSType = CollisionMesh;
   mDecalType = CollisionMesh;

   colisionMeshPrefix = StringTable->insert("Collision");

   CollisionMoveMask = (TerrainObjectType | PlayerObjectType |
      StaticShapeObjectType | VehicleObjectType |
      VehicleBlockerObjectType | DynamicShapeObjectType | StaticObjectType | EntityObjectType | TriggerObjectType);

   mAnimated = false;

   mCollisionInited = false;
}

ShapeCollisionComponent::~ShapeCollisionComponent()
{
   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CONOBJECT(ShapeCollisionComponent);

void ShapeCollisionComponent::onComponentAdd()
{
   Parent::onComponentAdd();

   MeshComponent* meshComponent = mOwner->getComponent<MeshComponent>(StringTable->insert("renderComponent"));
   if (meshComponent)
   {
      meshComponent->onShapeInstanceChanged.notify(this, &ShapeCollisionComponent::targetShapeChanged);
      mOwnerShapeComponent = meshComponent;
   }

   //physicsInterface
   PhysicsComponent *physicsComp = mOwner->getComponent<PhysicsComponent>(StringTable->insert("physicsComponent"));
   if (physicsComp)
   {
      mOwnerPhysicsComp = physicsComp;
   }
   else
   {
      if (PHYSICSMGR)
      {
         mPhysicsRep = PHYSICSMGR->createBody();
      }
   }

   prepCollision();
}

void ShapeCollisionComponent::onComponentRemove()
{
   SAFE_DELETE(mPhysicsRep);

   mOwnerPhysicsComp = nullptr;

   mCollisionInited = false;

   Parent::onComponentRemove();
}

void ShapeCollisionComponent::componentAddedToOwner(Component *comp)
{
   if (comp->getId() == getId())
      return;

   //test if this is a shape component!
   MeshComponent* meshComponent = dynamic_cast<MeshComponent*>(comp);
   if (meshComponent)
   {
      meshComponent->onShapeInstanceChanged.notify(this, &ShapeCollisionComponent::targetShapeChanged);
      mOwnerShapeComponent = meshComponent;
      prepCollision();
   }

   PhysicsComponent *physicsComp = dynamic_cast<PhysicsComponent*>(comp);
   if (physicsComp)
   {
      if (mPhysicsRep)
         SAFE_DELETE(mPhysicsRep);

      mOwnerPhysicsComp = physicsComp;

      prepCollision();
   }
}

void ShapeCollisionComponent::componentRemovedFromOwner(Component *comp)
{
   if (comp->getId() == getId()) //?????????
      return;

   //test if this is a shape component!
   MeshComponent* meshComponent = dynamic_cast<MeshComponent*>(comp);
   if (meshComponent)
   {
      meshComponent->onShapeInstanceChanged.remove(this, &ShapeCollisionComponent::targetShapeChanged);
      mOwnerShapeComponent = NULL;
      prepCollision();
   }

   //physicsInterface
   PhysicsComponent *physicsComp = dynamic_cast<PhysicsComponent*>(comp);
   if (physicsComp)
   {
      mPhysicsRep = PHYSICSMGR->createBody();

      mCollisionInited = false;

      mOwnerPhysicsComp = nullptr;

      prepCollision();
   }
}

void ShapeCollisionComponent::checkDependencies()
{
}

void ShapeCollisionComponent::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Collision");

      addField("CollisionType", TypeCollisionMeshMeshType, Offset(mCollisionType, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("LineOfSightType", TypeCollisionMeshMeshType, Offset(mLOSType, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("DecalType", TypeCollisionMeshMeshType, Offset(mDecalType, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("CollisionMeshPrefix", TypeString, Offset(colisionMeshPrefix, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("BlockCollisions", TypeBool, Offset(mBlockColliding, ShapeCollisionComponent), "");

   endGroup("Collision");
}

void ShapeCollisionComponent::inspectPostApply()
{
   // Apply any transformations set in the editor
   Parent::inspectPostApply();

   if (isServerObject())
   {
      setMaskBits(ColliderMask);
      prepCollision();
   }
}

U32 ShapeCollisionComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (stream->writeFlag(mask & (ColliderMask | InitialUpdateMask)))
   {
      stream->write((U32)mCollisionType);
      stream->writeString(colisionMeshPrefix);
   }

   return retMask;
}

void ShapeCollisionComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag()) // UpdateMask
   {
      U32 collisionType = CollisionMesh;

      stream->read(&collisionType);

      // Handle it if we have changed CollisionType's
      if ((MeshType)collisionType != mCollisionType)
      {
         mCollisionType = (MeshType)collisionType;

         prepCollision();
      }

      char readBuffer[1024];

      stream->readString(readBuffer);
      colisionMeshPrefix = StringTable->insert(readBuffer);
   }
}

void ShapeCollisionComponent::ownerTransformSet(MatrixF *mat)
{
   /*bool isSrv = isServerObject();
   if (mPhysicsRep && mCollisionInited)
      mPhysicsRep->setTransform(mOwner->getTransform());*/
}

//Setup
void ShapeCollisionComponent::targetShapeChanged(MeshComponent* shapeComponent)
{
   prepCollision();
}

void ShapeCollisionComponent::prepCollision()
{
   if (!mOwner)
      return;

   // Let the client know that the collision was updated
   setMaskBits(ColliderMask);

   mOwner->disableCollision();

   if (mCollisionType == None)
      return;

   //Physics API
   PhysicsCollision *colShape = NULL;

   if (mCollisionType == Bounds)
   {
      MatrixF offset(true);

      if (mOwnerShapeComponent && mOwnerShapeComponent->getShape())
         offset.setPosition(mOwnerShapeComponent->getShape()->center);

      colShape = PHYSICSMGR->createCollision();
      colShape->addBox(mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset);
   }
   else if (mCollisionType == CollisionMesh || (mCollisionType == VisibleMesh /*&& !mOwner->getComponent<AnimatedMesh>()*/))
   {
      colShape = buildColShapes();
   }

   if (colShape)
   {
      mPhysicsWorld = PHYSICSMGR->getWorld(isServerObject() ? "server" : "client");

      if (mPhysicsRep)
      {
         if (mBlockColliding)
            mPhysicsRep->init(colShape, 0, 0, mOwner, mPhysicsWorld);
         else
            mPhysicsRep->init(colShape, 0, PhysicsBody::BF_TRIGGER, mOwner, mPhysicsWorld);

         mPhysicsRep->setTransform(mOwner->getTransform());

         mCollisionInited = true;
      }
   }

   mOwner->enableCollision();

   onCollisionChanged.trigger(colShape);
}

//Update
void ShapeCollisionComponent::processTick()
{
   if (!isActive())
      return;

   //callback if we have a persisting contact
   if (mContactInfo.contactObject)
   {
      if (mContactInfo.contactTimer > 0)
      {
         if (isMethod("updateContact"))
            Con::executef(this, "updateContact");

         if (mOwner->isMethod("updateContact"))
            Con::executef(mOwner, "updateContact");
      }

      ++mContactInfo.contactTimer;
   }
   else if (mContactInfo.contactTimer != 0)
      mContactInfo.clear();
}

void ShapeCollisionComponent::updatePhysics()
{
   
}

PhysicsCollision* ShapeCollisionComponent::getCollisionData()
{
   if ((!PHYSICSMGR || mCollisionType == None) || mOwnerShapeComponent == NULL)
      return NULL;

   PhysicsCollision *colShape = NULL;
   if (mCollisionType == Bounds)
   {
      MatrixF offset(true);
      offset.setPosition(mOwnerShapeComponent->getShape()->center);
      colShape = PHYSICSMGR->createCollision();
      colShape->addBox(mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset);
   }
   else if (mCollisionType == CollisionMesh || (mCollisionType == VisibleMesh/* && !mOwner->getComponent<AnimatedMesh>()*/))
   {
      colShape = buildColShapes();
      //colShape = mOwnerShapeInstance->getShape()->buildColShape(mCollisionType == VisibleMesh, mOwner->getScale());
   }
   /*else if (mCollisionType == VisibleMesh && !mOwner->getComponent<AnimatedMesh>())
   {
   //We don't have support for visible mesh collisions with animated meshes currently in the physics abstraction layer
   //so we don't generate anything if we're set to use a visible mesh but have an animated mesh component.
   colShape = mOwnerShapeInstance->getShape()->buildColShape(mCollisionType == VisibleMesh, mOwner->getScale());
   }*/
   else if (mCollisionType == VisibleMesh/* && mOwner->getComponent<AnimatedMesh>()*/)
   {
      Con::printf("ShapeCollisionComponent::updatePhysics: Cannot use visible mesh collisions with an animated mesh!");
   }

   return colShape;
}

bool ShapeCollisionComponent::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (!mCollisionType == None)
   {
      if (mPhysicsWorld)
      {
         return mPhysicsWorld->castRay(start, end, info, Point3F::Zero);
      }
   }

   return false;
}

PhysicsCollision* ShapeCollisionComponent::buildColShapes()
{
   PROFILE_SCOPE(ShapeCollisionComponent_buildColShapes);

   PhysicsCollision *colShape = NULL;
   U32 surfaceKey = 0;

   TSShape* shape = mOwnerShapeComponent->getShape();

   if (shape == nullptr)
      return nullptr;

   if (mCollisionType == VisibleMesh)
   {
      // Here we build triangle collision meshes from the
      // visible detail levels.

      // A negative subshape on the detail means we don't have geometry.
      const TSShape::Detail &detail = shape->details[0];
      if (detail.subShapeNum < 0)
         return NULL;

      // We don't try to optimize the triangles we're given
      // and assume the art was created properly for collision.
      ConcretePolyList polyList;
      polyList.setTransform(&MatrixF::Identity, mOwner->getScale());

      // Create the collision meshes.
      S32 start = shape->subShapeFirstObject[detail.subShapeNum];
      S32 end = start + shape->subShapeNumObjects[detail.subShapeNum];
      for (S32 o = start; o < end; o++)
      {
         const TSShape::Object &object = shape->objects[o];
         if (detail.objectDetailNum >= object.numMeshes)
            continue;

         // No mesh or no verts.... nothing to do.
         TSMesh *mesh = shape->meshes[object.startMeshIndex + detail.objectDetailNum];
         if (!mesh || mesh->mNumVerts == 0)
            continue;

         // Gather the mesh triangles.
         polyList.clear();
         mesh->buildPolyList(0, &polyList, surfaceKey, NULL);

         // Create the collision shape if we haven't already.
         if (!colShape)
            colShape = PHYSICSMGR->createCollision();

         // Get the object space mesh transform.
         MatrixF localXfm;
         shape->getNodeWorldTransform(object.nodeIndex, &localXfm);

         colShape->addTriangleMesh(polyList.mVertexList.address(),
            polyList.mVertexList.size(),
            polyList.mIndexList.address(),
            polyList.mIndexList.size() / 3,
            localXfm);
      }

      // Return what we built... if anything.
      return colShape;
   }
   else if (mCollisionType == CollisionMesh)
   {

      // Scan out the collision hulls...
      //
      // TODO: We need to support LOS collision for physics.
      //
      for (U32 i = 0; i < shape->details.size(); i++)
      {
         const TSShape::Detail &detail = shape->details[i];
         const String &name = shape->names[detail.nameIndex];

         // Is this a valid collision detail.
         if (!dStrStartsWith(name, colisionMeshPrefix) || detail.subShapeNum < 0)
            continue;

         // Now go thru the meshes for this detail.
         S32 start = shape->subShapeFirstObject[detail.subShapeNum];
         S32 end = start + shape->subShapeNumObjects[detail.subShapeNum];
         if (start >= end)
            continue;

         for (S32 o = start; o < end; o++)
         {
            const TSShape::Object &object = shape->objects[o];
            const String &meshName = shape->names[object.nameIndex];

            if (object.numMeshes <= detail.objectDetailNum)
               continue;

            // No mesh, a flat bounds, or no verts.... nothing to do.
            TSMesh *mesh = shape->meshes[object.startMeshIndex + detail.objectDetailNum];
            if (!mesh || mesh->getBounds().isEmpty() || mesh->mNumVerts == 0)
               continue;

            // We need the default mesh transform.
            MatrixF localXfm;
            shape->getNodeWorldTransform(object.nodeIndex, &localXfm);

            // We have some sort of collision shape... so allocate it.
            if (!colShape)
               colShape = PHYSICSMGR->createCollision();

            // Any other mesh name we assume as a generic convex hull.
            //
            // Collect the verts using the vertex polylist which will 
            // filter out duplicates.  This is importaint as the convex
            // generators can sometimes fail with duplicate verts.
            //
            VertexPolyList polyList;
            MatrixF meshMat(localXfm);

            Point3F t = meshMat.getPosition();
            t.convolve(mOwner->getScale());
            meshMat.setPosition(t);

            polyList.setTransform(&MatrixF::Identity, mOwner->getScale());
            mesh->buildPolyList(0, &polyList, surfaceKey, NULL);
            colShape->addConvex(polyList.getVertexList().address(),
               polyList.getVertexList().size(),
               meshMat);
         } // objects
      } // details
   }

   return colShape;
}
