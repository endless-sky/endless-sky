/*
Copyright (c) 2022 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/


#include <jni.h>
#include <string>

/**
 * Retrieve android assets via the jni.
 */
class AndroidAsset
{
public:
   AndroidAsset()
   {
      // The initial implementation of this class used the aassetmanager api,
      // but AAssetDir_getNextFileName does not return directory names, which
      // meant that we could not recursively parse directories. This class will
      // instead use native jni calls to access the
      // android.content.res.AssetManager class, which *does* return directory
      // names as well as file names.
      m_env = (JNIEnv*)SDL_AndroidGetJNIEnv();
      m_env->PushLocalFrame(16);

      /* context = SDLActivity.getContext(); */
      jobject context = (jobject)SDL_AndroidGetActivity();

      /* assetManager = context.getAssets(); */
      jmethodID get_assets_mid = m_env->GetMethodID(m_env->GetObjectClass(context),
               "getAssets", "()Landroid/content/res/AssetManager;");
      // TODO: do I need to increment some sort of reference on assetManager or something?
      m_asset_manager = m_env->CallObjectMethod(context, get_assets_mid);
   }

   /// Open a directory and retrieve everything in it
   std::vector<std::string> DirectoryList(const std::string& dir_name)
   {
      std::vector<std::string> ret;
      jmethodID list_mid = m_env->GetMethodID(m_env->GetObjectClass(m_asset_manager),
            "list", "(Ljava/lang/String;)[Ljava/lang/String;");
      jstring path_object = m_env->NewStringUTF(dir_name.c_str());
      jobjectArray file_list = (jobjectArray)m_env->CallObjectMethod(m_asset_manager, list_mid, path_object);
      m_env->DeleteLocalRef(path_object);

      size_t length = m_env->GetArrayLength(file_list);
      for (size_t i = 0; i < length; ++i)
      {
         jstring entry_name_object = (jstring)m_env->GetObjectArrayElement(file_list, i);
         const char* entry_name = m_env->GetStringUTFChars(entry_name_object, nullptr);
         if (entry_name)
         {
            ret.push_back(entry_name);
            m_env->ReleaseStringUTFChars(entry_name_object, entry_name);
         }
         m_env->DeleteLocalRef(entry_name_object);
      }
      return ret;
   }

   ~AndroidAsset()
   {
      m_env->PopLocalFrame(nullptr);
   }

private:
   JNIEnv* m_env = nullptr;
   jobject m_asset_manager = nullptr;
};