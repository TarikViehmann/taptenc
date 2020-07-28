Requirements:
 - utap lib (https://people.cs.aau.dk/~marius/utap/, developed with v0.94)
   - The char arrays for capturing names are to small for taptenc as state
     names tend to become large during construction of the transformation TA
     - therefore, change the name[32] array inside the loadIF function of src/tracer.cpp to a larger number (10000 is likely way more than necessary)
     - also replace all occurrences of 31 in that function by the same number - 1 (it is used as formatting string in sscanf to restrict the number of read characters)

 - uppaal (tested with uppaal 4.1.24)
   - set the variable VERIFYTA_DIR to the directory containing verifyta executable
     (e.g. export VERIFYTA_DIR=~/uppaal64-4.1.24/bin-Linux)

Installation:
Run
```
make
```
If it fails, run it again. There are probably some bugs in the build system...
