# Endless Sky installer for 64-bit Windows

# To use this script:
## Verify the GCC and DEV64 paths below are correct for your system
## Verify git executable is in your windows PATH, otherwise version will be
##   blank
## Install NSIS, then right click this file and select "Compile NSIS Script"


# Environmental Configuration
!define GCC "C:\Program Files\mingw-w64\x86_64-6.2.0-posix-seh-rt_v5-rev1\mingw64"
!define DEV64 "C:\dev64"

# Constants
!define PROGRAM_NAME "Endless Sky"
!define COMPANY_NAME "Michael Zahniser"
!system "git describe --long --dirty > git_desc.txt"
!define /file PROGRAM_VERSION git_desc.txt

# Libraries
!include nsDialogs.nsh
!include LogicLib.nsh

# Name of installer (Windows title bar)
Name "Endless Sky"

# File name of installer
OutFile "endless-sky-win64-${PROGRAM_VERSION}-setup.exe"

DirText "This setup will install Endless Sky. Choose a directory."

# Default installation directory
InstallDir "$PROGRAMFILES64\Endless Sky"
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "InstallLocation"

# For writing to program files folder
RequestExecutionLevel admin

# Variables
Var Dialog
Var CheckBox
Var UserDataCheckBox

# Pages
Page directory directory_pre directory_show directory_leave
Page instfiles
UninstPage uninstConfirm
UninstPage custom un.removeUserDataPage un.removeUserDataLeave ": User Data"
UninstPage instfiles


# For systems with UAC disabled
!macro VerifyUserIsAdmin
	UserInfo::GetAccountType
	pop $0
	${If} $0 != "admin" ;Require admin rights on NT4+
		messageBox mb_iconstop "Administrator rights required!"
		setErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
		quit
	${EndIf}
!macroend


Section "uninstall previous" SecUninstallPrevious
	# On 64-bit systems, the registry entries will be in "Wow6432Node"
	ReadRegStr $R0 HKLM \
		"Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
		"UninstallString"
	StrCmp $R0 "" Done
	
	DetailPrint "Removing old version."
	ClearErrors
	ExecWait '$R0 /S _?=$INSTDIR' # Do not copy uninstaller to a temp file
	
	Done:
SectionEnd


Section "install" section_install_id
	# $INSTDIR is defined by InstallDir above
	SetOutPath $INSTDIR
	
	# Installation Files
	File "..\bin\Release\EndlessSky.exe"
	
	File "${GCC}\bin\libgcc_s_seh-1.dll"
	File "${GCC}\bin\libstdc++-6.dll"
	File "${GCC}\bin\libwinpthread-1.dll"
	
	File "${DEV64}\bin\glew32.dll"
	File "${DEV64}\bin\libjpeg-62.dll"
	File "${DEV64}\bin\libmad-0.dll"
	File "${DEV64}\bin\libpng15-15.dll"
	File "${DEV64}\bin\libturbojpeg.dll"
	File "${DEV64}\bin\OpenAL32.dll"
	File "${DEV64}\bin\SDL2.dll"
	File "${DEV64}\bin\soft_oal.dll"
	File "${DEV64}\bin\zlib1.dll"
	
	File ..\changelog
	File ..\copyright
	File ..\credits.txt
	File ..\keys.txt
	File ..\license.txt
	
	SetOutPath $INSTDIR\data
	File /r ..\data\*
	SetOutPath $INSTDIR\images
	File /r ..\images\*
	SetOutPath $INSTDIR\sounds
	File /r ..\sounds\*
	
	# Create uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"
	
	# Write registry keys for the Add/Remove programs control panel
	SectionGetSize ${section_install_id} $0
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "DisplayIcon" "$INSTDIR\EndlessSky.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "DisplayName" "${PROGRAM_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "Publisher" "${COMPANY_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "DisplayVersion" "${PROGRAM_VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "InstallLocation" "$INSTDIR"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
                 "EstimatedSize" $0
	
	# change the context of $SMPROGRAMS and other shell variables to all users
	SetShellVarContext all
	CreateDirectory "$SMPROGRAMS\${PROGRAM_NAME}"
	CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME}\${PROGRAM_NAME}.lnk" "$INSTDIR\EndlessSky.exe"
	CreateShortCut "$DESKTOP\${PROGRAM_NAME}.lnk" "$INSTDIR\EndlessSky.exe"
	
SectionEnd


Section "uninstall"
	# Remove files
	Delete "$INSTDIR\EndlessSky.exe"
	
	Delete "$INSTDIR\libgcc_s_seh-1.dll"
	Delete "$INSTDIR\libstdc++-6.dll"
	Delete "$INSTDIR\libwinpthread-1.dll"
	
	Delete "$INSTDIR\glew32.dll"
	Delete "$INSTDIR\libjpeg-62.dll"
	Delete "$INSTDIR\libmad-0.dll"
	Delete "$INSTDIR\libpng15-15.dll"
	Delete "$INSTDIR\libturbojpeg.dll"
	Delete "$INSTDIR\OpenAL32.dll"
	Delete "$INSTDIR\SDL2.dll"
	Delete "$INSTDIR\soft_oal.dll"
	Delete "$INSTDIR\zlib1.dll"
	
	Delete $INSTDIR\changelog
	Delete $INSTDIR\copyright
	Delete $INSTDIR\credits.txt
	Delete $INSTDIR\keys.txt
	Delete $INSTDIR\license.txt
	
	RMDir /r $INSTDIR\data
	RMDir /r $INSTDIR\images
	RMDir /r $INSTDIR\sounds
	
	# Remove shortcuts
	Delete "$SMPROGRAMS\${PROGRAM_NAME}\${PROGRAM_NAME}.lnk"
	RMDir "$SMPROGRAMS\${PROGRAM_NAME}"
	Delete "$DESKTOP\${PROGRAM_NAME}.lnk"
	
	# Delete user data if selected
	Call un.removeUserData
	
	# Delete uninstaller last
	Delete "$INSTDIR\uninstall.exe"
	
	# Will only delete install directory if empty (e.g. no plugins folder)
	RMDir $INSTDIR

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}"

SectionEnd


Function directory_pre
FunctionEnd
Function directory_show
FunctionEnd
Function directory_leave
	Call get_uninstall_string
	Pop $R0
	StrCmp $R0 "" Done
	
	MessageBox MB_OKCANCEL "Upgrade your existing installation of \
		${PROGRAM_NAME}?" IDOK Done
	Abort
	
	Done:
FunctionEnd


Function get_uninstall_string
	#Puts uninstall string from registry on stack.
	Push $R0
	# On 64-bit systems, the registry entries will be in "Wow6432Node"
	ReadRegStr $R0 HKLM \
		"Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}" \
		"UninstallString"
	Exch $R0
FunctionEnd


Function un.removeUserDataPage
	nsDialogs::Create 1018
	Pop $Dialog
	
	${If} $Dialog == error
		Abort
	${EndIf}
	
	${NSD_CreateLabel} 0 0 100% 24u "Check the box below to delete all saved games, plugins, and modifications for the current user."
	Pop $0
	
	${NSD_CreateCheckBox} 0 25u 100% 12u "Permanently delete user data"
	Pop $CheckBox
	
	nsDialogs::Show
FunctionEnd


Function un.removeUserDataLeave
	${NSD_GetState} $CheckBox $UserDataCheckBox
FunctionEnd


Function un.removeUserData
	${If} $UserDataCheckBox == ${BST_CHECKED}
		RMDir /r "$APPDATA\endless-sky"
	${EndIf}
FunctionEnd


Function un.onGUIInit
	SetShellVarContext all
 
	#Verify the uninstaller - last chance to back out
	MessageBox MB_OKCANCEL "Permanently remove ${PROGRAM_NAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
FunctionEnd
