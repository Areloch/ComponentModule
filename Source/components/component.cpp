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

#include "component.h"
#include "platform/platform.h"
#include "console/simBase.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "console/consoleInternal.h"
//#include "T3D/assets/MaterialAsset.h"

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

Component::Component()
{
   mFriendlyName = StringTable->EmptyString();
   mFromResource = StringTable->EmptyString();
   mComponentType = StringTable->EmptyString();
   mComponentGroup = StringTable->EmptyString();
   mNetworkType = StringTable->EmptyString();
   //mDependency = StringTable->EmptyString();

   mNetworked = false;

   // [tom, 1/12/2007] We manage the memory for the description since it
   // could be loaded from a file and thus massive. This is accomplished with
   // protected fields, but since they still call Con::getData() the field
   // needs to always be valid. This is pretty lame.
   mDescription = new char[1];
   ((char *)mDescription)[0] = 0;

   mOwner = NULL;

   mCanSaveFieldDictionary = false;

   //mOriginatingAssetId = StringTable->EmptyString();

   mIsServerObject = true;

   componentIdx = 0;

   mHidden = false;
   mEnabled = true;

   mDirtyMaskBits = 0;
}

Component::~Component()
{
   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(Component);

//////////////////////////////////////////////////////////////////////////

void Component::initPersistFields()
{
   addGroup("Component");
   addField("componentType", TypeCaseString, Offset(mComponentType, Component), "The type of behavior.", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
   addField("networkType", TypeCaseString, Offset(mNetworkType, Component), "The type of behavior.", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
   addField("friendlyName", TypeCaseString, Offset(mFriendlyName, Component), "Human friendly name of this behavior", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
      addProtectedField("description", TypeCaseString, Offset(mDescription, Component), &setDescription, &getDescription,
         "The description of this behavior which can be set to a \"string\" or a fileName\n", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

      addField("networked", TypeBool, Offset(mNetworked, Component), "Is this behavior ghosted to clients?", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

      addProtectedField("Owner", TypeSimObjectPtr, Offset(mOwner, Component), &setOwner, &defaultProtectedGetFn, "", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

      //addField("hidden", TypeBool, Offset(mHidden, Component), "Flags if this behavior is shown in the editor or not", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
      addProtectedField("enabled", TypeBool, Offset(mEnabled, Component), &_setEnabled, &defaultProtectedGetFn, "");

      //addField("originatingAsset", TypeComponentAssetPtr, Offset(mOriginatingAsset, Component),
      //   "Asset that spawned this component, used for tracking/housekeeping", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

   endGroup("Component");

   Parent::initPersistFields();

   //clear out irrelevent fields
   removeField("name");
   //removeField("internalName");
   removeField("parentGroup");
   //removeField("class");
   removeField("superClass");
   removeField("hidden");
   removeField("canSave");
   removeField("canSaveDynamicFields");
   removeField("persistentId");
}

bool Component::_setEnabled(void *object, const char *index, const char *data)
{
   Component *c = static_cast<Component*>(object);

   c->mEnabled = dAtob(data);
   c->setMaskBits(EnableMask);

   return true;
}

//////////////////////////////////////////////////////////////////////////

bool Component::setDescription(void *object, const char *index, const char *data)
{
   Component *bT = static_cast<Component *>(object);
   SAFE_DELETE_ARRAY(bT->mDescription);
   bT->mDescription = bT->getDescriptionText(data);

   // We return false since we don't want the console to mess with the data
   return false;
}

const char * Component::getDescription(void* obj, const char* data)
{
   Component *object = static_cast<Component *>(obj);

   return object->mDescription ? object->mDescription : "";
}

//////////////////////////////////////////////////////////////////////////
bool Component::onAdd()
{
   if (!Parent::onAdd())
      return false;

   setMaskBits(UpdateMask);
   setMaskBits(NamespaceMask);

   return true;
}

void Component::onRemove()
{
   onDataSet.removeAll();

   if (mOwner)
   {
      //notify our removal to the owner, so we have no loose ends
      mOwner->removeComponent(this, false);
   }

   Parent::onRemove();
}

void Component::onComponentAdd()
{
   mEnabled = true;
}

void Component::onComponentRemove()
{
   mEnabled = false;

   if (mOwner)
   {
      mOwner->onComponentAdded.remove(this, &Component::componentAddedToOwner);
      mOwner->onComponentRemoved.remove(this, &Component::componentRemovedFromOwner);
   }

   mOwner = NULL;
   setDataField("owner", NULL, "");
}

void Component::setOwner(Entity* owner)
{
   //first, catch if we have an existing owner, and we're changing from it
   if (mOwner && mOwner != owner)
   {
      mOwner->onComponentAdded.remove(this, &Component::componentAddedToOwner);
      mOwner->onComponentRemoved.remove(this, &Component::componentRemovedFromOwner);

      mOwner->removeComponent(this, false);
   }

   mOwner = owner;

   if (mOwner != NULL)
   {
      mOwner->onComponentAdded.notify(this, &Component::componentAddedToOwner);
      mOwner->onComponentRemoved.notify(this, &Component::componentRemovedFromOwner);
   }

   if (isServerObject())
   {
      setMaskBits(OwnerMask);

      //if we have any outstanding maskbits, push them along to have the network update happen on the entity
      if (mDirtyMaskBits != 0 && mOwner)
      {
         mOwner->setMaskBits(Entity::ComponentsUpdateMask);
      }
   }
}

void Component::componentAddedToOwner(Component *comp)
{
   return;
}

void Component::componentRemovedFromOwner(Component *comp)
{
   return;
}

void Component::setMaskBits(U32 orMask)
{
   AssertFatal(orMask != 0, "Invalid net mask bits set.");
   
   if (mOwner)
      mOwner->setComponentNetMask(this, orMask);
}

U32 Component::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = 0;

   /*if (mask & OwnerMask)
   {
      if (mOwner != NULL)
      {
         S32 ghostIndex = con->getGhostIndex(mOwner);

         if (ghostIndex == -1)
         {
            stream->writeFlag(false);
            retMask |= OwnerMask;
         }
         else
         {
            stream->writeFlag(true);
            stream->writeFlag(true);
            stream->writeInt(ghostIndex, NetConnection::GhostIdBitSize);
         }
      }
      else
      {
         stream->writeFlag(true);
         stream->writeFlag(false);
      }
   }
   else
      stream->writeFlag(false);*/

   if (stream->writeFlag(mask & EnableMask))
   {
      stream->writeFlag(mEnabled);
   }

   /*if (stream->writeFlag(mask & NamespaceMask))
   {
      const char* name = getName();
      if (stream->writeFlag(name && name[0]))
         stream->writeString(String(name));

      if (stream->writeFlag(mSuperClassName && mSuperClassName[0]))
         stream->writeString(String(mSuperClassName));

      if (stream->writeFlag(mClassName && mClassName[0]))
         stream->writeString(String(mClassName));
   }*/

   return retMask;
}

void Component::unpackUpdate(NetConnection *con, BitStream *stream)
{
   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         //we have an owner object, so fetch it
         S32 gIndex = stream->readInt(NetConnection::GhostIdBitSize);

         Entity *e = dynamic_cast<Entity*>(con->resolveGhost(gIndex));
         if (e)
            e->addComponent(this);
      }
      else
      {
         //it's being nulled out
         setOwner(NULL);
      }
   }*/

   if (stream->readFlag())
   {
      mEnabled = stream->readFlag();
   }

   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         char name[256];
         stream->readString(name);
         assignName(name);
      }

      if (stream->readFlag())
      {
         char superClassname[256];
         stream->readString(superClassname);
       mSuperClassName = superClassname;
      }

      if (stream->readFlag())
      {
         char classname[256];
         stream->readString(classname);
         mClassName = classname;
      }

      linkNamespaces();
   }*/
}

void Component::packToStream(Stream &stream, U32 tabStop, S32 behaviorID, U32 flags /* = 0  */)
{
   char buffer[1024];

   writeFields(stream, tabStop);
}

void Component::processTick()
{
}

void Component::setDataField(StringTableEntry slotName, const char *array, const char *value)
{
   Parent::setDataField(slotName, array, value);

   onDataSet.trigger(this, slotName, value);
}

StringTableEntry Component::getComponentName()
{
   return getNamespace()->getName();
}

//////////////////////////////////////////////////////////////////////////
const char * Component::getDescriptionText(const char *desc)
{
   if (desc == NULL)
      return NULL;

   char *newDesc = "";

   // [tom, 1/12/2007] If it isn't a file, just do it the easy way
   if (!Platform::isFile(desc))
   {
      dsize_t newDescLen = dStrlen(desc) + 1;
      newDesc = new char[newDescLen];
      dStrcpy(newDesc, desc, newDescLen);

      return newDesc;
   }

   FileStream str;
   str.open(desc, Torque::FS::File::Read);

   Stream *stream = &str;
   if (stream == NULL){
      str.close();
      return NULL;
   }

   U32 size = stream->getStreamSize();
   if (size > 0)
   {
      newDesc = new char[size + 1];
      if (stream->read(size, (void *)newDesc))
         newDesc[size] = 0;
      else
      {
         SAFE_DELETE_ARRAY(newDesc);
      }
   }

   str.close();
   //delete stream;

   return newDesc;
}
//////////////////////////////////////////////////////////////////////////
void Component::beginFieldGroup(const char* groupName)
{
   if (dStrcmp(mComponentGroup, ""))
   {
      Con::errorf("Component: attempting to begin new field group with a group already begun!");
      return;
   }

   mComponentGroup = StringTable->insert(groupName);
}

void Component::endFieldGroup()
{
   mComponentGroup = StringTable->insert("");
}

void Component::addDependency(StringTableEntry name)
{
   mDependencies.push_back_unique(name);
}

//////////////////////////////////////////////////////////////////////////
// Console Methods
//////////////////////////////////////////////////////////////////////////
DefineEngineMethod(Component, beginGroup, void, (String groupName),,
   "@brief Starts the grouping for following fields being added to be grouped into\n"
   "@param groupName The name of this group\n"
   "@param desc The Description of this field\n"
   "@param type The DataType for this field (default, int, float, Point2F, bool, enum, Object, keybind, color)\n"
   "@param defaultValue The Default value for this field\n"
   "@param userData An extra data field that can be used for custom data on a per-field basis<br>Usage for default types<br>"
   "-enum: a TAB separated list of possible values<br>"
   "-object: the T2D object type that are valid choices for the field.  The object types observe inheritance, so if you have a t2dSceneObject field you will be able to choose t2dStaticSrpites, t2dAnimatedSprites, etc.\n"
   "@return Nothing\n")
{
   object->beginFieldGroup(groupName);
}

DefineEngineMethod(Component, endGroup, void, (),,
   "@brief Ends the grouping for prior fields being added to be grouped into\n"
   "@param groupName The name of this group\n"
   "@param desc The Description of this field\n"
   "@param type The DataType for this field (default, int, float, Point2F, bool, enum, Object, keybind, color)\n"
   "@param defaultValue The Default value for this field\n"
   "@param userData An extra data field that can be used for custom data on a per-field basis<br>Usage for default types<br>"
   "-enum: a TAB separated list of possible values<br>"
   "-object: the T2D object type that are valid choices for the field.  The object types observe inheritance, so if you have a t2dSceneObject field you will be able to choose t2dStaticSrpites, t2dAnimatedSprites, etc.\n"
   "@return Nothing\n")
{
   object->endFieldGroup();
}

DefineEngineMethod(Component, addDependency, void, (String behaviorName),, 
   "@brief Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   object->addDependency(behaviorName);
}

DefineEngineMethod(Component, setDirty, void, (),,
   "@brief Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   object->setMaskBits(Component::OwnerMask);
}
