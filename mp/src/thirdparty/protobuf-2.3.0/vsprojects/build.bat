@echo off
setlocal

@rem Move to the batch file directory so we can run it from anywhere
cd %~dp0

call "%VS110COMNTOOLS%vsvars32.bat"
set dirsuffix=\2012

@rem Note that we no longer build separate debug libraries
for /d %%d in ( "release|Win32" "release|x64") do (
	@rem Note that this message seems to display out of order for some reason...
	echo Building %%d
	devenv protobuf_2010.sln /clean %%d
	devenv protobuf_2010.sln /build %%d /project libprotobuf-lite
	@rem Note that building libprotoc also ensures that libprotobuf builds
	devenv protobuf_2010.sln /build %%d /project libprotoc
)

p4 edit ..\..\..\lib\public%dirsuffix%\libproto*.lib
p4 edit ..\..\..\lib\public\x64%dirsuffix%\libproto*.lib

copy /y Release\libproto*.lib ..\..\..\lib\public%dirsuffix%
copy /y x64\Release\libproto*.lib ..\..\..\lib\public\x64%dirsuffix%

@echo Check in the changed libraries in src\lib if you are done.
