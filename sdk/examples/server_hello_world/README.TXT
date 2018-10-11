CMAKE BUILD INSTRUCTIONS
------------------------

CMake is a simple Makefile generator to generate Makefiles for various plattforms.
Syntax: cmake <-DOPTION=VALUE> <PATH_TO_CMAKELISTS.TXT>
Important options:
  -DCMAKE_INSTALL_PREFIX: Base path where the install target will copy the files (make install).
                          You should use the SDK base directory for that.
  -DCMAKE_BUILD_TYPE: Can either be 'Debug' or 'Release'.

The quick approach - Compile and install in 2 lines:
----------------------------------------------------

cmake -DCMAKE_INSTALL_PREFIX=../.. .
make install

Using a separate build directory:
---------------------------------

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../../.. ..
make
make install

This way the source directory stays clean and you can simply remove all build files
by deleting the build folder.

rm -rf build

Windows notices:
----------------

By default cmake will create Visual Studio projects.
This is most likely what you want, but if you prefer also building on the commandline
e.g. for batch builds you should modify the cmake command like this:
CMD> cmake -G "NMake Makfiles" -DCMAKE_INSTALL_PREFIX=../../.. ..
This will create Makefiles useable for MS nmake.
Building works simalar to normal make command.
CMD> nmake
CMD> nmake install

