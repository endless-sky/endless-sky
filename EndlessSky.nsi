; Endless Sky, Nullsoft Installer Script
;
;--------------------------------
; Include stuff, Modern UI
!include MUI2.nsh

;---------------------------------

; The name of the installer
Name "Endless Sky"

; The file to write
OutFile "Endless Sky Setup 0.9.6.exe"

; The default installation directory
InstallDir $PROGRAMFILES\EndlessSky

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "SOFTWARE\EndlessSky" "Install_Dir"

; Start Menu Config
Var StartMenuFolder
  
; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Checked and this is the best compressor
SetCompressor /SOLID lzma

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

;Start Menu Page Config
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "SOFTWARE\EndlessSky" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Start Endless Sky"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; The stuff to install
Section "Endless Sky (required)" EndlessSky

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Files to be written to dir
  File /r "data"
  File /r "images"
  File /r "sounds"
  File "changelog"
  File "copyright"
  File "credits.txt"
  File "EndlessSky.exe"
  File "glew32.dll"
  File "icon.png"
  File "keys.txt"
  File "libgcc_s_seh-1.dll"
  File "libjpeg-62.dll"
  File "libmad-0.dll"
  File "libpng15-15.dll"
  File "libstdc++-6.dll"
  File "libturbojpeg.dll"
  File "libwinpthread-1.dll"
  File "license.txt"
  File "OpenAL32.dll"
  File "SDL2.dll"
  File "soft_oal.dll"
  File "zlib1.dll"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\EndlessSky "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EndlessSky" "DisplayName" "Endless Sky"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EndlessSky" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EndlessSky" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EndlessSky" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  ;Start Menu Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Endless Sky.lnk" "$INSTDIR\EndlessSky.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall Endless Sky.lnk" "$INSTDIR\uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END

  
SectionEnd

;--------------------------------
;Languages
  !insertmacro MUI_LANGUAGE "English"
;Descriptions
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${EndlessSky} "The program files required to make Endless Sky work"
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller

Section "Uninstall"
  
  ; Remove files, uninstaller and dir
  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\images"
  RMDir /r "$INSTDIR\sounds"
  Delete "$INSTDIR\uninstall.exe"
  Delete "$INSTDIR\changelog"
  Delete "$INSTDIR\copyright"
  Delete "$INSTDIR\credits.txt"
  Delete "$INSTDIR\EndlessSky.exe"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\icon.png"
  Delete "$INSTDIR\keys.txt"
  Delete "$INSTDIR\libgcc_s_seh-1.dll"
  Delete "$INSTDIR\libjpeg-62.dll"
  Delete "$INSTDIR\libmad-0.dll"
  Delete "$INSTDIR\libpng15-15.dll"
  Delete "$INSTDIR\libstdc++-6.dll"
  Delete "$INSTDIR\libturbojpeg.dll"
  Delete "$INSTDIR\libwinpthread-1.dll"
  Delete "$INSTDIR\license.txt"
  Delete "$INSTDIR\OpenAL32.dll"
  Delete "$INSTDIR\SDL2.dll"
  Delete "$INSTDIR\soft_oal.dll"
  Delete "$INSTDIR\zlib1.dll"
  RMDir "$INSTDIR"
  
  ; Remove Start Menu Shortcuts
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall Endless Sky.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Endless Sky.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EndlessSky"
  DeleteRegKey HKLM SOFTWARE\EndlessSky
  DeleteRegKey /ifempty HKLM SOFTWARE\EndlessSky
  DeleteRegKey HKCU "SOFTWARE\EndlessSky"
  DeleteRegKey /ifempty HKCU SOFTWARE\EndlessSky
  
SectionEnd

Function LaunchLink
  ExecShell "" "$SMPROGRAMS\$StartMenuFolder\Endless Sky.lnk"
FunctionEnd
