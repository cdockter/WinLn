WinLn 0.1
========================================

### INFO
I recently was trying to run some scripts that rely on ln on a Windows machine.  I attempted use use ln implementations from [Msys](http://www.mingw.org/wiki/msys) and [GOW](https://github.com/bmatzelle/gow) only to discovery they both use a copy mechanic in their mimic symlinks creation.

The goal here is to provide a ln compatible implementation which actually creates symlinks on Windows machines.

#### Features
* synlinks for files and directories
* '-f' and '--force' switches for overwriting existing symlinks
* accepts all ln options a silently ignores them
* works on Windows Vista and later

### INSTALLATION
There are two options:
1. Download the latest version of [ln.exe](https://github.com/cdockter/WinLn/tree/master/bins) and put it on your path.
2. Pull the latest source and build.  Put the resulting ln.exe on your path.
Happy ln-ing! :D

### SOURCE
(https://github.com/cdockter/WinLn

### CONTRIBUTING
This was really just to solve my problem and I was suprised that it didn't already exist (as far as I can tell).  To that end, Please feel free to improve! :)

### LICENSE
See the included license agreement in the repository. 