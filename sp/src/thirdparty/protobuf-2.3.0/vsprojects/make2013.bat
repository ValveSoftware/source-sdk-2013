@echo off
setlocal

@rem Move to the batch file directory so we can run it from anywhere
cd %~dp0

call "%VS120COMNTOOLS%vsvars32.bat"
set dirsuffix=\2013

p4 edit ..\gtest\msvc\*.vcxproj
p4 edit *.vcxproj
p4 edit *.sln

@rem This will upgrade from v110_xp to v120_xp
devenv protobuf_2010.sln /upgrade
@rem Remove upgrade files
rmdir Backup /s/q
rmdir _UpgradeReport_Files /s/q
del UpgradeLog*.*

@rem Note that we no longer build separate debug libraries
for /d %%d in ( "release|Win32" "release|x64") do (
	@rem Note that this message seems to display out of order for some reason...
	echo Building %%d
	devenv protobuf_2010.sln /clean %%d
	devenv protobuf_2010.sln /build %%d /project libprotobuf-lite
	@rem Note that building libprotoc also ensures that libprotobuf builds
	devenv protobuf_2010.sln /build %%d /project libprotoc
)

@rem Make the destination directories in case they don't already exist.
mkdir ..\..\..\lib\public%dirsuffix%
mkdir ..\..\..\lib\public\x64%dirsuffix%

@rem Edit the destination files if they do exist
p4 edit ..\..\..\lib\public%dirsuffix%\libproto*.lib
p4 edit ..\..\..\lib\public\x64%dirsuffix%\libproto*.lib

copy /y Release\libproto*.lib ..\..\..\lib\public%dirsuffix%
copy /y x64\Release\libproto*.lib ..\..\..\lib\public\x64%dirsuffix%

@rem Auto-add the files if they aren't added yet. Too bad we can't use wildcards here.
p4 add ..\..\..\lib\public%dirsuffix%\libprotobuf-lite.lib
p4 add ..\..\..\lib\public%dirsuffix%\libprotobuf.lib
p4 add ..\..\..\lib\public%dirsuffix%\libprotoc.lib
p4 add ..\..\..\lib\public\x64%dirsuffix%\libprotobuf-lite.lib
p4 add ..\..\..\lib\public\x64%dirsuffix%\libprotobuf.lib
p4 add ..\..\..\lib\public\x64%dirsuffix%\libprotoc.lib

p4 revert ..\gtest\msvc\*.vcxproj
p4 revert *.vcxproj
p4 revert *.sln

@echo Check in the changed libraries in src\lib if you are done.
