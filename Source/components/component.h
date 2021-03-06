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

#pragma once

#ifndef COMPONENT_H
#define COMPONENT_H

#ifndef _NETOBJECT_H_
#include "sim/netObject.h"
#endif
#ifndef ENTITY_H
#include "../entity.h"
#endif
#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif 

class Entity;
class Namespace;

//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class Component : public SimObject
{
   typedef SimObject Parent;

protected:
   StringTableEntry mFriendlyName;
   StringTableEntry mDescription;

   StringTableEntry mFromResource;
   StringTableEntry mComponentGroup;
   StringTableEntry mComponentType;
   StringTableEntry mNetworkType;

   Vector<StringTableEntry> mDependencies;

   bool mNetworked;

   U32 componentIdx;

   Entity*              mOwner;
   bool					   mHidden;
   bool					   mEnabled;

   U32                  mDirtyMaskBits;

   bool                 mIsServerObject;

public:
   Component();
   virtual ~Component();
   DECLARE_CONOBJECT(Component);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void packToStream(Stream &stream, U32 tabStop, S32 behaviorID, U32 flags = 0);

   //This is called when we are added to an entity
   virtual void onComponentAdd();            
   //This is called when we are removed from an entity
   virtual void onComponentRemove();         

   //This is called when a different component is added to our owner entity
   virtual void componentAddedToOwner(Component *comp);  
   //This is called when a different component is removed from our owner entity
   virtual void componentRemovedFromOwner(Component *comp);  

   //Overridden by components that actually care
   virtual void ownerTransformSet(MatrixF *mat) {}

   void setOwner(Entity* pOwner);
   inline Entity *getOwner() { return mOwner ? mOwner : NULL; }
   static bool setOwner(void *object, const char *index, const char *data) { return true; }

   bool	isEnabled() { return mEnabled; }
   void  setEnabled(bool toggle) { mEnabled = toggle; setMaskBits(EnableMask); }

   bool isActive() { return mEnabled && mOwner != NULL; }

   static bool _setEnabled(void *object, const char *index, const char *data);

   virtual void processTick();
   virtual void interpolateTick(F32 dt){}
   virtual void advanceTime(F32 dt){}

   StringTableEntry getComponentType() { return mComponentType; }

   const char *getDescriptionText(const char *desc);

   const char *getFriendlyName() { return mFriendlyName; }

   bool isNetworked() { return mNetworked; }

   void beginFieldGroup(const char* groupName);
   void endFieldGroup();

   void addDependency(StringTableEntry name);
   /// @}

   /// @name Description
   /// @{
   static bool setDescription(void *object, const char *index, const char *data);
   static const char* getDescription(void* obj, const char* data);

   /// @Primary usage functions
   /// @These are used by the various engine-based behaviors to integrate with the component classes
   enum NetMaskBits
   {
      InitialUpdateMask = BIT(0),
      OwnerMask = BIT(1),
      UpdateMask = BIT(2),
      EnableMask = BIT(3),
      NamespaceMask = BIT(4),
      NextFreeMask = BIT(5)
   };

   virtual void setMaskBits(U32 orMask);
   virtual void clearMaskBits() {
      mDirtyMaskBits = 0;
   }

   bool isServerObject() { return mIsServerObject; }
   bool isClientObject() { return !mIsServerObject; }

   void setIsServerObject(bool isServerObj) { mIsServerObject = isServerObj; }

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);
   /// @}

   Signal< void(SimObject*, String, String) > onDataSet;
   virtual void setDataField(StringTableEntry slotName, const char *array, const char *value);

   virtual void checkDependencies(){}

   StringTableEntry getComponentName();

   virtual void onInspect() {}
   virtual void onEndInspect() {}

};

#endif // COMPONENT_H
