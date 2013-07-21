=====================================
Updating GCCXML
=====================================
1. Go to https://github.com/gccxml/gccxml and clone latest
2. Use CMake to generate project files and build
3. Update Support folder
	- Create a virtual machine containing a 32 bit installation of a linux distribution (e.g. Ubuntu)
	- Install GCC and G++
	- Copy usr/lib and usr/include to the Support folder when parsing on other platforms than Linux

	
=====================================
Updating PyPlusPlus
=====================================
https://bitbucket.org/ompl/pyplusplus

=====================================
Updating src_module_builder Settings
=====================================
1. Open srcpy/src_module_builder.py
2. Update include paths
	- `gcc -print-prog-name=cc1plus` -v
	- `gcc -print-prog-name=cc1` -v
3. Update symbols gccxml_gcc_options file
	- gcc -dM -E - < /dev/null