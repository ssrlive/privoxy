# Microsoft Developer Studio Project File - Name="vc_privoxy" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vc_privoxy - Win32 Debug with Win32 threads
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vc_privoxy.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vc_privoxy.mak"\
 CFG="vc_privoxy - Win32 Debug with Win32 threads"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vc_privoxy - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vc_privoxy - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vc_privoxy - Win32 Release with Win32 threads" (based on\
 "Win32 (x86) Application")
!MESSAGE "vc_privoxy - Win32 Debug with Win32 threads" (based on\
 "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vc_privoxy - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc_release"
# PROP BASE Intermediate_Dir "vc_release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc_release"
# PROP Intermediate_Dir "vc_release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "..\pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib pthreadVC.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "vc_privoxy - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vc_debug"
# PROP BASE Intermediate_Dir "vc_debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vc_debug"
# PROP Intermediate_Dir "vc_debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\pcre" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib pthreadVC.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "vc_privoxy - Win32 Release with Win32 threads"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc_junkb"
# PROP BASE Intermediate_Dir "vc_junkb"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc_release_winthr"
# PROP Intermediate_Dir "vc_release_winthr"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "..\pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STATIC" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "..\pcre" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib pthreadVC.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "vc_privoxy - Win32 Debug with Win32 threads"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vc_junk0"
# PROP BASE Intermediate_Dir "vc_junk0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vc_debug_winthr"
# PROP Intermediate_Dir "vc_debug_winthr"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /O2 /I "..\pcre" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "STATIC" /YX /FD /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /Zi /Od /I "..\pcre" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib pthreadVC.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "vc_privoxy - Win32 Release"
# Name "vc_privoxy - Win32 Debug"
# Name "vc_privoxy - Win32 Release with Win32 threads"
# Name "vc_privoxy - Win32 Debug with Win32 threads"
# Begin Group "Privoxy"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\actionlist.h
# End Source File
# Begin Source File

SOURCE=..\src\actions.c
# End Source File
# Begin Source File

SOURCE=..\src\actions.h
# End Source File
# Begin Source File

SOURCE=..\src\cgi.c
# End Source File
# Begin Source File

SOURCE=..\src\cgi.h
# End Source File
# Begin Source File

SOURCE=..\src\cgiedit.c
# End Source File
# Begin Source File

SOURCE=..\src\cgiedit.h
# End Source File
# Begin Source File

SOURCE=..\src\cgisimple.c
# End Source File
# Begin Source File

SOURCE=..\src\cgisimple.h
# End Source File
# Begin Source File

SOURCE=..\src\config.h
# End Source File
# Begin Source File

SOURCE=..\src\deanimate.c
# End Source File
# Begin Source File

SOURCE=..\src\deanimate.h
# End Source File
# Begin Source File

SOURCE=..\src\errlog.c
# End Source File
# Begin Source File

SOURCE=..\src\errlog.h
# End Source File
# Begin Source File

SOURCE=..\src\filters.c
# End Source File
# Begin Source File

SOURCE=..\src\filters.h
# End Source File
# Begin Source File

SOURCE=..\src\jcc.c
# End Source File
# Begin Source File

SOURCE=..\src\jcc.h
# End Source File
# Begin Source File

SOURCE=..\src\killpopup.c
# End Source File
# Begin Source File

SOURCE=..\src\killpopup.h
# End Source File
# Begin Source File

SOURCE=..\src\loadcfg.c
# End Source File
# Begin Source File

SOURCE=..\src\loadcfg.h
# End Source File
# Begin Source File

SOURCE=..\src\loaders.c
# End Source File
# Begin Source File

SOURCE=..\src\loaders.h
# End Source File
# Begin Source File

SOURCE=..\src\parsers.c
# End Source File
# Begin Source File

SOURCE=..\src\parsers.h
# End Source File
# Begin Source File

SOURCE=..\src\project.h
# End Source File
# Begin Source File

SOURCE=..\src\urlmatch.c
# End Source File
# Begin Source File

SOURCE=..\src\urlmatch.h
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\cygwin.h
# End Source File
# Begin Source File

SOURCE=..\src\w32log.c
# End Source File
# Begin Source File

SOURCE=..\src\w32log.h
# End Source File
# Begin Source File

SOURCE=..\src\w32res.h
# End Source File
# Begin Source File

SOURCE=..\src\w32taskbar.c
# End Source File
# Begin Source File

SOURCE=..\src\w32taskbar.h
# End Source File
# Begin Source File

SOURCE=..\src\win32.c
# End Source File
# Begin Source File

SOURCE=..\src\win32.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter "rc,ico,bmp"
# Begin Source File

SOURCE=..\src\icons\ico00001.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00002.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00003.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00004.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00005.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00006.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00007.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\ico00008.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\idle.ico
# End Source File
# Begin Source File

SOURCE=..\src\icons\privoxy.ico
# End Source File
# Begin Source File

SOURCE=..\src\w32res.rc
# End Source File
# End Group
# Begin Group "PCRE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\pcre\chartables.c

!IF  "$(CFG)" == "vc_privoxy - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vc_privoxy - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vc_privoxy - Win32 Release with Win32 threads"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vc_privoxy - Win32 Debug with Win32 threads"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\pcre\config.h
# End Source File
# Begin Source File

SOURCE=..\pcre\get.c
# End Source File
# Begin Source File

SOURCE=..\pcre\internal.h
# End Source File
# Begin Source File

SOURCE=..\pcre\maketables.c
# End Source File
# Begin Source File

SOURCE=..\pcre\pcre.c
# End Source File
# Begin Source File

SOURCE=..\pcre\pcre.h
# End Source File
# Begin Source File

SOURCE=..\pcre\pcreposix.c
# End Source File
# Begin Source File

SOURCE=..\pcre\pcreposix.h
# End Source File
# Begin Source File

SOURCE=..\pcre\study.c
# End Source File
# End Group
# Begin Group "PCRS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\pcrs.c
# End Source File
# Begin Source File

SOURCE=..\src\pcrs.h
# End Source File
# End Group
# Begin Group "Sockets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\gateway.c
# End Source File
# Begin Source File

SOURCE=..\src\gateway.h
# End Source File
# Begin Source File

SOURCE=..\src\jbsockets.c
# End Source File
# Begin Source File

SOURCE=..\src\jbsockets.h
# End Source File
# End Group
# Begin Group "Utilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\encode.c
# End Source File
# Begin Source File

SOURCE=..\src\encode.h
# End Source File
# Begin Source File

SOURCE=..\src\list.c
# End Source File
# Begin Source File

SOURCE=..\src\list.h
# End Source File
# Begin Source File

SOURCE=..\src\miscutil.c
# End Source File
# Begin Source File

SOURCE=..\src\miscutil.h
# End Source File
# Begin Source File

SOURCE=..\src\ssplit.c
# End Source File
# Begin Source File

SOURCE=..\src\ssplit.h
# End Source File
# End Group
# End Target
# End Project
