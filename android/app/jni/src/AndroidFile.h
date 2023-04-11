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
 * Save or retrieve a file via android intents. This will open a gui and prompt
 * the user for a file name.
 */
class AndroidFile
{
public:
	AndroidFile()
	{
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

   /// Prompt the user for a place to save this file. Errors are not returned
   /// here, but instead displayed to the user.
   void SaveFile(const std::string& filename, const std::string& content)
   {
		jobject context = (jobject)SDL_AndroidGetActivity();
      // void saveFile(String filename, byte[] content)
      jmethodID saveFile = m_env->GetMethodID(m_env->GetObjectClass(context),
					"saveFile", "(Ljava/lang/String;[B)V");

      jbyteArray arr = m_env->NewByteArray(content.size());
      m_env->SetByteArrayRegion(arr, 0, content.size(), (jbyte*)content.c_str());

		jstring str = m_env->NewStringUTF(filename.c_str());

      m_env->CallVoidMethod(context, saveFile,
         str,
         arr
      );
		m_env->DeleteLocalRef(str);
		m_env->DeleteLocalRef(arr);
   }

	/// Prompt the user for a file, and then return its contents. If the user
	/// cancels or something, the returned string will be empty. Any errors will
	/// be displayed to the user in the java code.
	std::string GetFile(const std::string& prompt, const std::string& mime_type="text/plain")
	{
		jobject context = (jobject)SDL_AndroidGetActivity();
      // void getFile(java.lang.String, java.lang.String)
		jmethodID getFile = m_env->GetMethodID(m_env->GetObjectClass(context),
				"getFile", "(Ljava/lang/String;Ljava/lang/String;)[B");

		// This method is blocking while the user selects or cancels the file.
		jstring prompt_str = m_env->NewStringUTF(prompt.c_str());
		jstring mime_type_str = m_env->NewStringUTF(mime_type.c_str());
		jbyteArray data = (jbyteArray)m_env->CallObjectMethod(context, getFile, prompt_str, mime_type_str);
		m_env->DeleteLocalRef(prompt_str);
		m_env->DeleteLocalRef(mime_type_str);

		if (m_env->IsSameObject(data, NULL))
		{
			// user canceled the dialog, or some other (already reported) error
			// occurred
			return "";
		}
		size_t len = m_env->GetArrayLength(data);
		std::vector<char> tmp;
		tmp.resize(len);
		m_env->GetByteArrayRegion(data, 0, len, (jbyte*)tmp.data());
		m_env->DeleteLocalRef(data);
		return std::string(tmp.begin(), tmp.end());
	}

	/// Prompt the user for a zip file, and then unpack it at the given path.
	bool GetAndUnzipPlugin(const std::string& prompt, const std::string& zip_path)
	{
		jobject context = (jobject)SDL_AndroidGetActivity();
      // void getFile(java.lang.String, java.lang.String)
      jmethodID promptUserAndUnzipPlugin = m_env->GetMethodID(m_env->GetObjectClass(context),
					"promptUserAndUnzipPlugin", "(Ljava/lang/String;Ljava/lang/String;)Z");

		// This method is blocking while the user selects or cancels the file.
		jstring prompt_str = m_env->NewStringUTF(prompt.c_str());
		jstring zip_path_str = m_env->NewStringUTF(zip_path.c_str());
		bool success = (bool)m_env->CallBooleanMethod(context, promptUserAndUnzipPlugin, prompt_str, zip_path_str);
		m_env->DeleteLocalRef(prompt_str);
		m_env->DeleteLocalRef(zip_path_str);

		return success;
	}

	~AndroidFile()
	{
		m_env->PopLocalFrame(nullptr);
	}

private:
	JNIEnv* m_env = nullptr;
	jobject m_asset_manager = nullptr;
};
