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

#include "console/engineAPI.h"
#include "shapeCollisionComponent.h"
#include "materials/baseMatInstance.h"

DefineEngineMethod(CollisionComponent, getNumberOfContacts, S32, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   return object->getCollisionList()->getCount();
}

DefineEngineMethod(CollisionComponent, getBestContact, S32, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   return 0;
}

DefineEngineMethod(CollisionComponent, getContactNormal, Point3F, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   if (object->getContactInfo())
   {
      if (object->getContactInfo()->contactObject)
      {
         return object->getContactInfo()->contactNormal;
      }
   }

   return Point3F::Zero;
}

DefineEngineMethod(CollisionComponent, getContactMaterial, S32, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   if (object->getContactInfo())
   {
      if (object->getContactInfo()->contactObject)
      {
         if (object->getContactInfo()->contactMaterial != NULL)
            return object->getContactInfo()->contactMaterial->getMaterial()->getId();
      }
   }

   return 0;
}

DefineEngineMethod(CollisionComponent, getContactObject, S32, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   if (object->getContactInfo())
   {
      return object->getContactInfo()->contactObject != NULL ? object->getContactInfo()->contactObject->getId() : 0;
   }

   return 0;
}

DefineEngineMethod(CollisionComponent, getContactPoint, Point3F, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   if (object->getContactInfo())
   {
      if (object->getContactInfo()->contactObject)
      {
         return object->getContactInfo()->contactPoint;
      }
   }

   return Point3F::Zero;
}

DefineEngineMethod(CollisionComponent, getContactTime, S32, (), ,
   "Gets the number of contacts this collider has hit.\n"
   "@return The number of static fields defined on the object.")
{
   if (object->getContactInfo())
   {
      if (object->getContactInfo()->contactObject)
      {
         return object->getContactInfo()->contactTimer;
      }
   }

   return 0;
}

DefineEngineMethod(ShapeCollisionComponent, hasContact, bool, (), ,
   "@brief Apply an impulse to this object as defined by a world position and velocity vector.\n\n"

   "@param pos impulse world position\n"
   "@param vel impulse velocity (impulse force F = m * v)\n"
   "@return Always true\n"

   "@note Not all objects that derrive from GameBase have this defined.\n")
{
   return object->hasContact();
}

DefineEngineMethod(ShapeCollisionComponent, getCollisionCount, S32, (), ,
   "@brief Apply an impulse to this object as defined by a world position and velocity vector.\n\n"

   "@param pos impulse world position\n"
   "@param vel impulse velocity (impulse force F = m * v)\n"
   "@return Always true\n"

   "@note Not all objects that derrive from GameBase have this defined.\n")
{
   return object->getCollisionCount();
}

DefineEngineMethod(ShapeCollisionComponent, getCollisionNormal, Point3F, (S32 collisionIndex), ,
   "@brief Apply an impulse to this object as defined by a world position and velocity vector.\n\n"

   "@param pos impulse world position\n"
   "@param vel impulse velocity (impulse force F = m * v)\n"
   "@return Always true\n"

   "@note Not all objects that derrive from GameBase have this defined.\n")
{
   return object->getCollisionNormal(collisionIndex);
}

DefineEngineMethod(ShapeCollisionComponent, getCollisionAngle, F32, (S32 collisionIndex, VectorF upVector), ,
   "@brief Apply an impulse to this object as defined by a world position and velocity vector.\n\n"

   "@param pos impulse world position\n"
   "@param vel impulse velocity (impulse force F = m * v)\n"
   "@return Always true\n"

   "@note Not all objects that derrive from GameBase have this defined.\n")
{
   return object->getCollisionAngle(collisionIndex, upVector);
}

DefineEngineMethod(ShapeCollisionComponent, getBestCollisionAngle, F32, (VectorF upVector), ,
   "@brief Apply an impulse to this object as defined by a world position and velocity vector.\n\n"

   "@param pos impulse world position\n"
   "@param vel impulse velocity (impulse force F = m * v)\n"
   "@return Always true\n"

   "@note Not all objects that derrive from GameBase have this defined.\n")
{
   return object->getBestCollisionAngle(upVector);
}
