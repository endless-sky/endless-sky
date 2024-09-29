cmake_minimum_required(VERSION 3.0)

project(endless_mobile)

# We aren't using a new enough cmake to use c++20 this way. Hardwire the
# standard directly instead.
# set(CMAKE_CXX_STANDARD 20 CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c11")

# SDL requires c89 compliance, but their android code is written against
# C99. This isn't fixed until 2.30
set(HAVE_GCC_WDECLARATION_AFTER_STATEMENT 0)

option(ENABLE_SHARED "" FALSE)
option(ENABLED_STATIC "" TRUE)
option(WITH_TURBOJPEG "" FALSE)
add_subdirectory(libjpeg-turbo-2.1.3 EXCLUDE_FROM_ALL)

option(PNG_SHARED "" OFF)
option(PNG_SHARED "" ON)
option(PNG_TESTS "" OFF)
add_subdirectory(libpng-1.6.37       EXCLUDE_FROM_ALL)

option(ALSOFT_DLOPEN "" OFF)
option(ALSOFT_WERROR "" OFF)
option(ALSOFT_UTILS "" OFF)
option(ALSOFT_EXAMPLES "" OFF)
option(ALSOFT_EAX "" OFF)
# For android, we have opensl, oboe, or sdl2.
# opensl is deprecated, and sdl2 uses the android.media api. oboe uses either
# opensl or aaudio, depending on the android version, but isn't included in the
# ndk.
# for now, using the opensl backend, since it still seems to be present in the
# ndk, and doesn't require the jni layer used by sdl2. AAudio might be
# preferable in the future, if openal-soft ever adds support for it.
OPTION(ALSOFT_REQUIRE_OPENSL "" ON)
OPTION(ALSOFT_BACKEND_OBOE "" OFF)
OPTION(ALSOFT_BACKEND_SDL2 "" OFF)
OPTION(ALSOFT_BACKEND_PIPEWIRE "" OFF)
set(LIBTYPE STATIC)
add_definitions("-DAL_LIBTYPE_STATIC")
add_subdirectory(openal-soft-1.22.0  EXCLUDE_FROM_ALL)

OPTION(SDL_STATIC_PIC "" ON)
set(SDL_SHARED OFF) # doesn't seem to work right... For now passing in via gradle
set(SDL_STATIC ON)
add_subdirectory(SDL2-2.0.20          EXCLUDE_FROM_ALL)

include_directories(
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/src/libpng-1.6.37/
   ${CMAKE_BINARY_DIR}/src/libpng-1.6.37/
   ${CMAKE_SOURCE_DIR}/src/libjpeg-turbo-2.1.3/
   ${CMAKE_BINARY_DIR}/src/libjpeg-turbo-2.1.3/
   ${CMAKE_SOURCE_DIR}/src/openal-soft-1.22.0/include/
   ${CMAKE_BINARY_DIR}/src/openal-soft-1.22.0/include/
)

add_definitions("-DES_GLES")

# Generate a version string
# Extract the version from the credits.txt
set(version "")
file(STRINGS "${CMAKE_SOURCE_DIR}/../../../credits.txt" matched)
if(matched)
	string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" version ${matched})
endif()

# Append the git tag
execute_process(COMMAND git describe --tags
						OUTPUT_VARIABLE mobile_version
						OUTPUT_STRIP_TRAILING_WHITESPACE)

set(ENDLESS_SKY_VERSION "${version}-${mobile_version}")
add_definitions(-DENDLESS_SKY_VERSION="${ENDLESS_SKY_VERSION}")

add_library(main SHARED
   ${CMAKE_SOURCE_DIR}/../../../source/Account.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/AI.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/AlertLabel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/AmmoDisplay.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Angle.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Armament.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/AsteroidField.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/audio/Audio.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/BankPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/BatchDrawList.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/BatchShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Bitset.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/BoardingPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Body.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/CaptureOdds.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/CargoHold.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/CategoryList.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/CrashState.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Collision.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/CollisionSet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Color.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Command.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ConditionSet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ConditionsStore.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Conversation.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ConversationPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/CoreStartData.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DamageProfile.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DataFile.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DataNode.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DataWriter.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Date.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Depreciation.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Dialog.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Dictionary.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DistanceCalculationSettings.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DistanceMap.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Distribution.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/DrawList.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Dropdown.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Effect.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Engine.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/EscortDisplay.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/EsUuid.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/File.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Files.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FillShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FireCommand.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Fleet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FleetCargo.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Flotsam.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FogShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FormationPattern.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FormationPositioner.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/FrameTimer.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Galaxy.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GameAction.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GameData.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GameEvent.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GameLoadingPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Gamerules.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GameWindow.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GamePad.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GamepadCursor.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/GamepadPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Gesture.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Government.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/HailPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Hardpoint.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Hazard.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/HiringPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/ImageBuffer.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/ImageSet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/InfoPanelState.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Information.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Interface.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ItemInfoDisplay.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/KtxFile.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/LineShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/LoadPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/LocationFilter.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/LogbookPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Logger.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/main.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MainPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MapDetailPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MapOutfitterPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MapPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MapPlanetCard.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MapSalesPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MapShipyardPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/Mask.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/MaskManager.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MenuAnimationPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MenuPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MessageLogPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Messages.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Minable.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Mission.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MissionAction.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/MissionPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Mortgage.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/audio/Music_stub.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/News.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/NPC.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/NPCAction.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/opengl.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Orders.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Outfit.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/OutfitInfoDisplay.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/OutfitterPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/OutlineShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Panel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Paragraphs.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Person.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Personality.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Phrase.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Planet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PlanetLabel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PlanetPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PlayerInfo.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PlayerInfoPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Plugins.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Point.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PointerShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Politics.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Port.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Preferences.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PreferencesPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/PrintData.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Projectile.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Radar.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/RaidFleet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/RadialSelectionPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Random.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Rectangle.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/RenderBuffer.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/RingShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/SavedGame.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Screen.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ScrollBar.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Shader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Ship.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipEvent.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipInfoDisplay.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipInfoPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipJumpNavigation.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipManager.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipyardPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ship/ShipAICache.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShopPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/audio/Sound.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/SpaceportPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/Sprite.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/image/SpriteSet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/SpriteShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ShipNameDialog.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/StarField.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/StartConditions.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/StartConditionsPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/StellarObject.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/StringInterner.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/System.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/TaskQueue.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/test/Test.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/test/TestContext.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/test/TestData.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/TouchScreen.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/DisplayText.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/Font.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/FontSet.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/Format.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/Table.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/Utf8.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/text/WrappedText.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/TextArea.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/TextReplacements.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Trade.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/TradingPanel.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/UI.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/UiRectShader.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/UniverseObjects.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Variant.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Visual.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Weapon.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Weather.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/Wormhole.cpp
   ${CMAKE_SOURCE_DIR}/../../../source/ZoomGesture.cpp
)

target_link_libraries(main
   SDL2main
   SDL2-static
   OpenAL
   png_static
   z
   jpeg-static
   GLESv3
)


