## ttBld

The original of this tool was written in 2002, long before CMake became popular/standard. It's been maintained and used internally within **KeyWorks Software** ever since. While it still has a few advantages over **CMake**, it's not going to be enough to justify switching to this app. This project remains as a public repository for historical purposes, and it is still maintained since it can be used to create or maintain a `CMakeLists.txt` file, but it should no longer be used as the primary build system for a project.

This tool is used with C++ projects to create **ninja** scripts that can either be run directly, or they can be run by the **makefile** that **ttBld** will generate. To convert a `.srcfiles.yaml` that ttBld creates into a CMake file, run the following in the root of your project:

```
	ttBld -cmake
```

If **CMakeLists.txt** doesn't exist (and .srcfiles.yaml can be found), it will be created. If it already exists, a `CMakeLists.ttbld` file will be created so that you can run a difference between the two files.

Note that **ttBld** can import most Visual Studio projects, so if you want to convert a .vcproj or .vcxproj into a `CMakeLists.txt` file, first run `ttBld` to convert the VS project into .srcfiles.yaml, and then run ttBld again with the `-cmake` command.

## Cloning

To clone this repository, use:

```
git clone --recurse-submodules https://github.com/KeyWorksRW/ttBld
```

If you already cloned the repository without using the --recurse command, then enter:

```
	git submodule init
	git submodule update
```

## Building

Because **wxWidgets** is required, you will need to build in a shell where the the `$(WXWIN)` environment variable has been set so that the appropriate include and library files can be found.

At a minimum, you will need a C++17 or later compiler, and CMake version 3.20 or later. It is strongly recommended that you use a multi-config generator as in the following:

```
	cmake -G "Ninja Multi-Config" . -B build
```

After the above step, building can be done with the following command line (from the root of the project):

```
	cmake.exe --build build --config Release
```

## License

There are multiple licenses used by this project.

All KeyWorks Software contributions are under Apache License 2.0 [LICENSE](LICENSE).

PugiXML code is under MIT [LICENSE](pugixml_license).
