cmake_minimum_required(VERSION 3.13)
project(EndlessSky)
add_compile_options(-std=c++11 -Wall)
add_link_options(-Wl,--subsystem,windows)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -msse3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

# Add Endless Sky source CMakeLists.txt file
set(SOURCE_FILES
        main.cpp
        Account.cpp
        AI.cpp
        Angle.cpp
        Armament.cpp
        AsteroidField.cpp
        Audio.cpp
        BankPanel.cpp
        BatchDrawList.cpp
        BatchShader.cpp
        BoardingPanel.cpp
        Body.cpp
        CaptureOdds.cpp
        CargoHold.cpp
        CollisionSet.cpp
        Color.cpp
        Command.cpp
        ConditionSet.cpp
        Conversation.cpp
        ConversationPanel.cpp
        DataFile.cpp
        DataNode.cpp
        DataWriter.cpp
        Date.cpp
        Depreciation.cpp
        Dialog.cpp
        Dictionary.cpp
        DistanceMap.cpp
        DrawList.cpp
        Effect.cpp
        Engine.cpp
        EscortDisplay.cpp
        File.cpp
        Files.cpp
        FillShader.cpp
        Fleet.cpp
        Flotsam.cpp
        FogShader.cpp
        Font.cpp
        FontSet.cpp
        Format.cpp
        FrameTimer.cpp
        Galaxy.cpp
        GameData.cpp
        GameEvent.cpp
        GameWindow.cpp
        Government.cpp
        HailPanel.cpp
        Hardpoint.cpp
        HiringPanel.cpp
        ImageBuffer.cpp
        ImageSet.cpp
        Information.cpp
        Interface.cpp
        ItemInfoDisplay.cpp
        LineShader.cpp
        LoadPanel.cpp
        LocationFilter.cpp
        LogbookPanel.cpp
        main.cpp
        MainPanel.cpp
        MapDetailPanel.cpp
        MapOutfitterPanel.cpp
        MapPanel.cpp
        MapSalesPanel.cpp
        MapShipyardPanel.cpp
        Mask.cpp
        MenuPanel.cpp
        Messages.cpp
        Minable.cpp
        Mission.cpp
        MissionAction.cpp
        MissionPanel.cpp
        Mortgage.cpp
        Music.cpp
        News.cpp
        NPC.cpp
        Outfit.cpp
        OutfitInfoDisplay.cpp
        OutfitterPanel.cpp
        OutlineShader.cpp
        Panel.cpp
        Person.cpp
        Personality.cpp
        Phrase.cpp
        Planet.cpp
        PlanetLabel.cpp
        PlanetPanel.cpp
        PlayerInfo.cpp
        PlayerInfoPanel.cpp
        Point.cpp
        PointerShader.cpp
        Politics.cpp
        Preferences.cpp
        PreferencesPanel.cpp
        Projectile.cpp
        Radar.cpp
        Random.cpp
        Rectangle.cpp
        RingShader.cpp
        SavedGame.cpp
        Screen.cpp
        Shader.cpp
        Ship.cpp
        ShipEvent.cpp
        ShipInfoDisplay.cpp
        ShipInfoPanel.cpp
        ShipyardPanel.cpp
        ShopPanel.cpp
        Sound.cpp
        SpaceportPanel.cpp
        Sprite.cpp
        SpriteQueue.cpp
        SpriteSet.cpp
        SpriteShader.cpp
        StarField.cpp
        StartConditions.cpp
        StellarObject.cpp
        System.cpp
        Table.cpp
        Trade.cpp
        TradingPanel.cpp
        UI.cpp
        Visual.cpp
        Weapon.cpp
        WrappedText.cpp
        )

list(TRANSFORM SOURCE_FILES PREPEND ${CMAKE_SOURCE_DIR}/source/)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(EndlessSky ${SOURCE_FILES})

# Add the include directory for the libraries
target_include_directories(EndlessSky PRIVATE ${CMAKE_SOURCE_DIR}/dev64/include)

# List the libraries required to compile Endless Sky
set(ES_DEV64_LIB
        libsdl2main.a
        libsdl2.dll.a
        libpng.dll.a
        libturbojpeg.dll.a
        libjpeg.dll.a
        libmad.dll.a
        libopenal32.dll.a
        libglew32.dll.a
        )

# Prepend each library entry with the path to the library directory
list(TRANSFORM ES_DEV64_LIB PREPEND ${CMAKE_SOURCE_DIR}/dev64/lib/)

# Set MINGW install path to explicitly include libmingw32.a and libopengl32.a
set (MINGW_INSTALL_PATH "C:/Program Files/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/x86_64-w64-mingw32/lib")

# Add libraries to be linked when compiling
target_link_libraries(EndlessSky -lwinmm ${MINGW_INSTALL_PATH}/libmingw32.a ${ES_DEV64_LIB} ${MINGW_INSTALL_PATH}/libopengl32.a)

# Place the finished executable in <this_folder>/obj/<release_type>/EndlessSky.exe
set_target_properties(EndlessSky PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})