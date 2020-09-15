## ttBld

This tool is used with C++ projects to create **ninja** scripts that can either be run directly, or they can be run by the **makefile** that **ttBld** will generate.


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

At an absolute minimum, you will need to have the following in your PATH:

- [Ninja.exe](https://github.com/ninja-build/ninja)

- MSVC compiler toolchain

- [wxWidgets](https://www.wxwidgets.org/) -- The debug build requires the DLL versions, the release build links to static libraries

Before building, you will need to start one of the Visual Studio command shells, preferably one that builds 64-bit targets. From within that shell, you can use the following commands to build the library:

	cd src
	nmake

By default, this will build a 64-bit release version of the `ttLibwx.lib` library followed by a 64-bit version of `ttBld.exe`.

Once you have built `ttLib.exe` or you have downloaded a binary release version, you can use **ttBld** to generate ninja scripts for compiling with `clang-cl`. If you have MINGW installed and both MINGW and the **clang-cl** compiler are in your `$PATH`, you can use the following commands to build:

	cd src
	mingw32-make

Note: currently, **ttBld** can only be built as a **Windows** console app (it does have UI for setting options, converting other projects, etc.). A **UNIX** version is planned.

## License

There are multiple licenses used by this project.

All KeyWorks Software contributions are under Apache License 2.0 [LICENSE](LICENSE).

PugiXML code is under MIT [LICENSE](pugixml_license).

## Creating a new project

To create a new **ttBld** project, first **cd** to where your source files are located and then run **ttBld.exe**. **ttBld** will display a dialog where you can either convert from an existing Visual Studio project (any version), or you can simply use the source files in the current directory. You will the be presented with options for setting up a **.vscode/** directory with all the **.json** files needed for **Visual Studio Code** to be able to build your project. Finally, you will be displayed a dialog allowing you to customize build options (later, you can always get to this dialog by running `ttBld -options`).

If you are using **VS Code**, you can now run it and choose one of the build commands on the Task menu. **ttBld** always generates a makefile in the same directory as the **.ninja** scripts. If you would prefer a **makefile** that doesn't use **ttBld** to automatically update the **.ninja** scripts, then run `ttBld -makefile` in the same directory where **ttBld** created the `.srcfiles.yaml` file (see above paragraph).
