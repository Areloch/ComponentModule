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

#include "behavior.h"
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

Behavior::Behavior() : Component()
{
   mCanSaveFieldDictionary = false;
   mComponentType = StringTable->insert("behavior");
}

Behavior::~Behavior()
{
   for (S32 i = 0; i < mFields.size(); ++i)
   {
      BehaviorField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(Behavior);

//////////////////////////////////////////////////////////////////////////

void Behavior::initPersistFields()
{
   Parent::initPersistFields();
}

//////////////////////////////////////////////////////////////////////////
bool Behavior::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void Behavior::onRemove()
{
   Parent::onRemove();
}

void Behavior::onComponentAdd()
{
   Parent::onComponentAdd();

   if (isServerObject())
   {
      if (isMethod("onAdd"))
         Con::executef(this, "onAdd");
   }
}

void Behavior::onComponentRemove()
{
   if (isServerObject())
   {
      if (isMethod("onRemove"))
         Con::executef(this, "onRemove");
   }

   Parent::onComponentRemove();
}

U32 Behavior::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
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

void Behavior::unpackUpdate(NetConnection *con, BitStream *stream)
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

void Behavior::packToStream(Stream &stream, U32 tabStop, S32 behaviorID, U32 flags /* = 0  */)
{
   char buffer[1024];

   writeFields(stream, tabStop);

   // Write out the fields which the behavior template knows about
   for (int i = 0; i < getBehaviorFieldCount(); i++)
   {
      BehaviorField *field = getBehaviorField(i);
      const char *objFieldValue = getDataField(field->mFieldName, NULL);

      // If the field holds the same value as the template's default value than it
      // will get initialized by the template, and so it won't be included just
      // to try to keep the object files looking as non-horrible as possible.
      if (dStrcmp(field->mDefaultValue, objFieldValue) != 0)
      {
         dSprintf(buffer, sizeof(buffer), "%s = \"%s\";\n", field->mFieldName, (dStrlen(objFieldValue) > 0 ? objFieldValue : "0"));

         stream.writeTabs(tabStop);
         stream.write(dStrlen(buffer), buffer);
      }
   }
}

void Behavior::processTick()
{
   Parent::processTick();

   if (isServerObject() && mEnabled)
   {
      if (mOwner != NULL && isMethod("Update"))
         Con::executef(this, "Update");
   }
}

void Behavior::setDataField(StringTableEntry slotName, const char *array, const char *value)
{
   Parent::setDataField(slotName, array, value);

   onDataSet.trigger(this, slotName, value);
}

StringTableEntry Behavior::getComponentName()
{
   return getNamespace()->getName();
}

//catch any behavior field updates
void Behavior::onStaticModified(const char* slotName, const char* newValue)
{
   Parent::onStaticModified(slotName, newValue);

   //If we don't have an owner yet, then this is probably the initial setup, so we don't need the console callbacks yet.
   if (!mOwner)
      return;

   onDataSet.trigger(this, slotName, newValue);

   checkBehaviorFieldModified(slotName, newValue);
}

void Behavior::onDynamicModified(const char* slotName, const char* newValue)
{
   Parent::onDynamicModified(slotName, newValue);

   //If we don't have an owner yet, then this is probably the initial setup, so we don't need the console callbacks yet.
   if (!mOwner)
      return;

   checkBehaviorFieldModified(slotName, newValue);
}

void Behavior::checkBehaviorFieldModified(const char* slotName, const char* newValue)
{
   StringTableEntry slotNameEntry = StringTable->insert(slotName);

   //find if it's a behavior field
   for (int i = 0; i < mFields.size(); i++)
   {
      BehaviorField *field = getBehaviorField(i);
      if (field->mFieldName == slotNameEntry)
      {
         //we have a match, do the script callback that we updated a field
         if (isMethod("onInspectorUpdate"))
            Con::executef(this, "onInspectorUpdate", slotName);

         return;
      }
   }
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void Behavior::addBehaviorField(const char *fieldName, const char *desc, const char *type, const char *defaultValue /* = NULL */, const char *userData /* = NULL */, /*const char* dependency /* = NULL *//*,*/ bool hidden /* = false */, const char* customLabel /* = ""*/)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
         return;
   }

   BehaviorField field;

   if (customLabel != "")
      field.mFieldLabel = customLabel;
   else
      field.mFieldLabel = stFieldName;

   field.mFieldName = stFieldName;

   //find the field type
   S32 fieldTypeMask = -1;
   StringTableEntry fieldType = StringTable->insert(type);

   if (fieldType == StringTable->insert("int"))
      fieldTypeMask = TypeS32;
   else if (fieldType == StringTable->insert("float"))
      fieldTypeMask = TypeF32;
   else if (fieldType == StringTable->insert("vector"))
      fieldTypeMask = TypePoint3F;
   //else if (fieldType == StringTable->insert("material"))
   //   fieldTypeMask = TypeMaterialAssetPtr;
   else if (fieldType == StringTable->insert("image"))
      fieldTypeMask = TypeImageFilename;
   else if (fieldType == StringTable->insert("shape"))
      fieldTypeMask = TypeShapeFilename;
   else if (fieldType == StringTable->insert("bool"))
      fieldTypeMask = TypeBool;
   else if (fieldType == StringTable->insert("object"))
      fieldTypeMask = TypeSimObjectPtr;
   else if (fieldType == StringTable->insert("string"))
      fieldTypeMask = TypeString;
   else if (fieldType == StringTable->insert("colorI"))
      fieldTypeMask = TypeColorI;
   else if (fieldType == StringTable->insert("colorF"))
      fieldTypeMask = TypeColorF;
   else if (fieldType == StringTable->insert("ease"))
      fieldTypeMask = TypeEaseF;
   else if (fieldType == StringTable->insert("gameObject"))
      fieldTypeMask = TypeGameObjectAssetPtr;
   else
      fieldTypeMask = TypeString;
   field.mFieldTypeName = fieldType;

   field.mFieldType = fieldTypeMask;

   field.mUserData = StringTable->insert(userData ? userData : "");
   field.mDefaultValue = StringTable->insert(defaultValue ? defaultValue : "");
   field.mFieldDescription = getDescriptionText(desc);

   field.mGroup = mComponentGroup;

   field.mHidden = hidden;

   mFields.push_back(field);

   //Before we set this, we need to do a test to see if this field was already set, like from the mission file or a taml file
   const char* curFieldData = getDataField(field.mFieldName, NULL);

   if (dStrIsEmpty(curFieldData))
      setDataField(field.mFieldName, NULL, field.mDefaultValue);
}

BehaviorField* Behavior::getBehaviorField(const char *fieldName)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
         return &mFields[i];
   }

   return NULL;
}

//////////////////////////////////////////////////////////////////////////
void Behavior::beginFieldGroup(const char* groupName)
{
   if (dStrcmp(mComponentGroup, ""))
   {
      Con::errorf("Behavior: attempting to begin new field group with a group already begun!");
      return;
   }

   mComponentGroup = StringTable->insert(groupName);
}

void Behavior::endFieldGroup()
{
   mComponentGroup = StringTable->insert("");
}

//////////////////////////////////////////////////////////////////////////
// Console Methods
//////////////////////////////////////////////////////////////////////////
DefineEngineMethod(Behavior, beginGroup, void, (String groupName),,
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

DefineEngineMethod(Behavior, endGroup, void, (),,
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

DefineEngineMethod(Behavior, addBehaviorField, void, (String fieldName, String fieldDesc, String fieldType, String defValue, String userData, bool hidden),
   ("", "", "", "", "", false),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   object->addBehaviorField(fieldName, fieldDesc, fieldType, defValue, userData, hidden);
}

DefineEngineMethod(Behavior, getBehaviorFieldCount, S32, (),, 
   "@brief Get the number of BehaviorField's on this object\n"
   "@return Returns the number of BehaviorFields as a nonnegative integer\n")
{
   return object->getBehaviorFieldCount();
}

// [tom, 1/12/2007] Field accessors split into multiple methods to allow space
// for long descriptions and type data.

DefineEngineMethod(Behavior, getBehaviorField, const char *, (S32 index),, 
   "@brief Gets a Tab-Delimited list of information about a BehaviorField specified by Index\n"
   "@param index The index of the behavior\n"
   "@return FieldName, FieldType and FieldDefaultValue, each separated by a TAB character.\n")
{
   BehaviorField *field = object->getBehaviorField(index);
   if (field == NULL)
      return "";

   char *buf = Con::getReturnBuffer(1024);
   dSprintf(buf, 1024, "%s\t%s\t%s\t%s", field->mFieldName, field->mFieldType, field->mDefaultValue, field->mGroup);

   return buf;
}

/*DefineEngineMethod(Behavior, setBehaviorField, const char *, (S32 index),, 
   "@brief Gets a Tab-Delimited list of information about a BehaviorField specified by Index\n"
   "@param index The index of the behavior\n"
   "@return FieldName, FieldType and FieldDefaultValue, each separated by a TAB character.\n")
{
   BehaviorField *field = object->getBehaviorField(index);
   if (field == NULL)
      return "";

   char *buf = Con::getReturnBuffer(1024);
   dSprintf(buf, 1024, "%s\t%s\t%s", field->mFieldName, field->mFieldType, field->mDefaultValue);

   return buf;
}*/

DefineEngineMethod(Behavior, getBehaviorFieldType, const char *, (String fieldName), ,
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   BehaviorField *field = object->getBehaviorField(fieldName);
   if (field == NULL)
      return "";

   return field->mFieldTypeName;;
}

DefineEngineMethod(Behavior, getBehaviorFieldUserData, const char *, (S32 index),, 
   "@brief Gets the UserData associated with a field by index in the field list\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the user data of this field\n")
{
   BehaviorField *field = object->getBehaviorField(index);
   if (field == NULL)
      return "";

   return field->mUserData;
}

DefineEngineMethod(Behavior, getBehaviorFieldDescription, const char *, (S32 index),, 
   "@brief Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   BehaviorField *field = object->getBehaviorField(index);
   if (field == NULL)
      return "";

   return field->mFieldDescription ? field->mFieldDescription : "";
}
