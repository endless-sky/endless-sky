
LOCAL_PATH := $(call my-dir)
BASE_LOCAL_PATH := $(LOCAL_PATH)

include $(LOCAL_PATH)/SDL2-2.0.20/Android.mk

include $(CLEAR_VARS)
# TODO: how to trigger cmake instead of manually integrating it here? it may be
# worth pre-compiling this in a shell script, and specifying it as 
# $(PREBUILT_STATIC_LIBRARY) here
# TODO: is simd worth it here? We don't use any of this in performance critical
# code.

LOCAL_PATH := $(BASE_LOCAL_PATH)/libjpeg-turbo-2.1.3
LOCAL_C_INCLUDES := $(BASE_LOCAL_PATH)
LOCAL_MODULE := jpeg
LOCAL_SRC_FILES := \
	jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
	jcicc.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
	jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c \
	jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdicc.c jdinput.c \
	jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c \
	jdtrans.c jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c \
	jidctint.c jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c \
	jaricom.c jcarith.c jdarith.c jsimd_none.c

# COMPILE_TARGET := arm-linux-androidapi16
# $(LOCAL_PATH)/libjpeg-turbo-2.1.3/build/install/lib/libjpeg.a:
# 	mkdir build
# 	cd build
# 	cmake .. -DCMAKE_INSTALL_PREFIX=install \
# 		      -DCMAKE_BUILD_TYPE=Release \
# 				-DENABLE_SHARED=OFF \
# 				-DWITH_TURBOJPEG=OFF \
# 				-DANDROID_ABI=$(TARGET_ARCH_ABI) \
# 				-DANDROID_ARM_MODE=arm \
# 				-DANDROID_PLATFORM=$(TARGET_PLATFORM) \
# 				-DCMAKE_ASM_FLAGS="--target=$(COMPILE_TARGET)" \
# 				-DCMAKE_TOOLCHAIN_FILE=$(ANDROID_NDK_HOME)
# 	make && make install
# 
# .PHONY: $(LOCAL_PATH)/libjpeg-turbo-2.1.3/build/install/lib/libjpeg.a

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
# TODO: how to trigger cmake instead of manually integrating it here? it may be
# worth pre-compiling this in a shell script, and specifying it as 
# $(PREBUILT_STATIC_LIBRARY) here

LOCAL_PATH := $(BASE_LOCAL_PATH)/libpng-1.6.37
LOCAL_MODULE := png16
LOCAL_C_INCLUDES := $(BASE_LOCAL_PATH)
LOCAL_SRC_FILES := \
	png.c pngerror.c \
	pngget.c pngmem.c pngpread.c pngread.c pngrio.c pngrtran.c pngrutil.c \
	pngset.c pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c \
	png.h pngconf.h pngdebug.h pnginfo.h pngpriv.h pngstruct.h pngusr.dfa \


# $(LOCAL_PATH)/libpng-1.6.37/.libs/libpng16.a: 
# 	PREFIX=$(LOCAL_PATH)/libpng-1.6.37/ \
# 			 LINK_TARGET=$(TARGET_ABI) \
# 			 APP_PLATFORM=$(APP_PLATFORM) \
# 			 SYSROOT=$(ANDROID_NDK_HOME)/platforms/$(APP_PLATFORM)/arch-$(TARGET_ARCH) \
# 			 TOOLCHAIN_PATH=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuild/linux-x86_64 \
# 			 CPP=${TOOLCHAIN_PATH}/$(COMPILE_TARGET)-clang\ -E \
# 			 AR=${TOOLCHAIN_PATH}/$(TARGET_ABI)-ar \
# 			 AS=${TOOLCHAIN_PATH}/$(TARGET_ABI)-as \
# 			 NM=${TOOLCHAIN_PATH}/$(TARGET_ABI)-nm \
# 			 CC=${TOOLCHAIN_PATH}/$(COMPILE_TARGET)-clang \
# 			 CXX=${TOOLCHAIN_PATH}/$(COMPILE_TARGET)-clang++ \
# 			 LD=${TOOLCHAIN_PATH}/$(TARGET_ABI)-ld \
# 			 RANLIB=${TOOLCHAIN_PATH}/$(TARGET_ABI)-ranlib \
# 			 STRIP=${TOOLCHAIN_PATH}/$(TARGET_ABI)-strip \
# 			 OBJDUMP=${TOOLCHAIN_PATH}/$(TARGET_ABI)-objdump \
# 			 OBJCOPY=${TOOLCHAIN_PATH}/$(TARGET_ABI)-objcopy \
# 			 ADDR2LINE=${TOOLCHAIN_PATH}/$(TARGET_ABI)-addr2line \
# 			 READELF=${TOOLCHAIN_PATH}/$(TARGET_ABI)-readelf \
# 			 SIZE=${TOOLCHAIN_PATH}/$(TARGET_ABI)-size \
# 			 STRINGS=${TOOLCHAIN_PATH}/$(TARGET_ABI)-strings \
# 			 ELFEDIT=${TOOLCHAIN_PATH}/$(TARGET_ABI)-elfedit \
# 			 GCOV=${TOOLCHAIN_PATH}/$(TARGET_ABI)-gcov \
# 			 GDB=${TOOLCHAIN_PATH}/$(TARGET_ABI)-gdb \
# 			 GPROF=${TOOLCHAIN_PATH}/$(TARGET_ABI)-gprof \
# 			 PKG_CONFIG_PATH=$(LOCAL_PATH)/lib/pkgconfig \
# 			 CFLAGS="${CFLAGS} --sysroot=${SYSROOT} --sysroot=${ANDROID_PREFIX}/sysroot -I${SYSROOT}/usr/include -I${ANDROID_PREFIX}/include" \
# 			 CPPFLAGS="${CFLAGS}" \
# 			 LDFLAGS="${LDFLAGS} -L${SYSROOT}/usr/lib -L${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/lib" \
# 			 ./configure --host=$(TARGET_ABI) --prefix=${PREFIX} 
# 	make && make install
# 
# .PHONY: $(LOCAL_PATH)/libpng-1.6.37/.libs/libpng16.a 

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PATH := $(BASE_LOCAL_PATH)
LOCAL_MODULE := main

SDL_PATH := SDL2-2.0.20

OUR_SRCS := $(LOCAL_PATH)/../../..

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
   $(OUR_SRCS)/src \
	$(LOCAL_PATH)/libpng-1.6.37/ \
	$(LOCAL_PATH)/libjpeg-turbo-2.1.3/

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/LineShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MainPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/DistanceMap.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ItemInfoDisplay.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ShipInfoPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Mortgage.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ImageSet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/RingShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Minable.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Test.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Body.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/EscortDisplay.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Sound_stub.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/OutfitInfoDisplay.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/BatchShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/SpriteSet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/BatchDrawList.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Effect.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/DataFile.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Messages.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/StarField.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Weapon.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Interface.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/CargoHold.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MapSalesPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/BoardingPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Point.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Conversation.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/UniverseObjects.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/opengl.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ConditionSet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/GameEvent.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ShipEvent.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Rectangle.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/GameLoadingPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MaskManager.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/WrappedText.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/Utf8.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/Table.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/Font.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/FontSet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/Format.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/text/DisplayText.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Command.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Planet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/LogbookPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MenuPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/DrawList.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Mask.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/SpaceportPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MissionAction.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Armament.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/System.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Radar.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Account.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Dictionary.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Information.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Sprite.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/DamageProfile.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Weather.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/PointerShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/BankPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/CaptureOdds.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Fleet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ImageBuffer.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MapDetailPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/main.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Date.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MissionPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Preferences.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Person.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/FireCommand.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/PlanetPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/LoadPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/SpriteShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/NPC.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/GameAction.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MapOutfitterPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/StartConditions.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/HiringPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/TestData.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/PlayerInfo.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Projectile.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/CoreStartData.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Shader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/EsUuid.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Mission.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/PlanetLabel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/AsteroidField.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/OutlineShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Politics.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/GameWindow.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Random.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/News.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Files.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Depreciation.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/File.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/FrameTimer.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/SpriteQueue.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/LocationFilter.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Flotsam.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MapPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/TextReplacements.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/UI.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/StellarObject.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/DataNode.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ShipyardPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Phrase.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/FogShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Hardpoint.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/SavedGame.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Audio_stub.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Dialog.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ShipInfoDisplay.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Government.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Panel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Visual.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/PreferencesPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Engine.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Galaxy.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/GameData.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ConversationPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Trade.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/AI.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/FillShader.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Color.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/CollisionSet.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Ship.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/DataWriter.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Hazard.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/TestContext.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/PlayerInfoPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/ShopPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Outfit.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/OutfitterPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Angle.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/HailPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/StartConditionsPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Music_stub.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/TradingPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MapShipyardPanel.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Personality.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Screen.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/Bitset.cpp) \
   $(subst $(LOCAL_PATH)/,, $(OUR_SRCS)/source/MenuAnimationPanel.cpp) \


LOCAL_STATIC_LIBRARIES := SDL2_static png16 jpeg

LOCAL_LDLIBS := -lGLESv3 -lOpenSLES -llog -lz -landroid
LOCAL_CPPFLAGS += -std=c++11 -frtti -fexceptions -DES_GLES -flto
LOCAL_LDFLAGS += -flto 

include $(BUILD_SHARED_LIBRARY)
