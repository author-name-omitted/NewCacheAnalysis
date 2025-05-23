# NOTE

This is a fork of LLVMTA built by University of Saarland, you can find their version at [here](https://gitlab.cs.uni-saarland.de/reineke/llvmta)

# LLVMTA

This Repository contains LLVMTA a static timing analysis tool based on the LLVM compiler ecosystem.

## Setup with VS Code and Docker development container

1.) Get [VS CODE](https://code.visualstudio.com/) and necessary [extensions](https://code.visualstudio.com/docs/remote/remote-overview) for remote container development.

![alt text](dependencies/img/extension.gif)

2.a.) This repository supports VS code development containers. Just press F1 inside VS code and execute "Remote-Containers: Open Worksapce in Container..". Now continue with [building](#build-the-project)  the Project. Should you prefere to build the container yourself continue with the next steps.

![alt text](dependencies/img/open.gif)

2.b.) Build the Docker container from the Docker file. The default Dockerfile uses Ubuntu and Dockerfile.Arch uses Arch Linux.

```
docker build -t llvmtadocker:latest - < .devcontainer/Dockerfile
```

3.) Run the Docker dev container with Repository as Volume. The third line defines an extra Volume for the build folder and is optional.

```
docker run -i -d \
	-v {$path_to_this_repository}:/workspaces/llvmta:rw \
	-v {$path_to_build_bir}:/workspaces/llvmta/build:rw \
	--name $name_of_instance llvmtadocker:latest
```

4.) Now VS Code can be attached to the container.

## Build the project

If using VS code all config and build tasks can be triggered by shift+ctr+b.

1.) Config the project with the ./config.sh script. Be aware that llvm and LLVMTA requires a lot or RAM, should your System have only 16GB of RAM choose the "lowRes" configuration.

```
Script to configure llvm, clang and LLVMTA:
  dev | development          Configure for development.
  rel | release              Configure for Release.
  lowRes | lowResources      Configure for low Ram PC.
  distributed | dis          Configure for icecc distributed compiler.
  clean                      Removes build folder.
```

You will be asked to download llvm and clang at the first setup. The script will do so automatically.

![alt text](dependencies/img/config.gif)

3.) Build llvm, clang and LLVMTA with ninja. This is necessary at least once, so that the llvm and LLVMTA toolchain are of the same version.

```
cd build
ninja -j [#CPUs]
```

For later rebuilds it is enough to just build LLVMTA.

```
cd build
ninja -j [#CPUs] llvmta
```

![alt text](dependencies/img/build.gif)

## Using LLVMTA

This section showcases how to use LLVMTA on a simple test case of a nested loop example. Make sure LLVMTA is compiled and in your $PATH variable.

1.) Change into the testcases folder.

```
cd testcases
```

2.) Run LLVMTA on the benchmark using the script

```
./runTestcase loopexamples/nested
```

All test cases are in the Benchmarks folder and only their relative path has to be handed. The full path for the command above ist "testcases/Benchmarks/loopexamples/nested", keep this in mind while using the script.

## Using Gurobi

To use Gurobi, simply place your gurobi.lic file in the dependencies folder.
The license will not be found, if its not named gurobi.lic! Otherwise LLVMTA will fallback to using LPsolve.


## LAB 1

```bash
./lab1.py -s lab1/0516_2c_zw -p -ops options/lab1_l1_82.txt -m zhangw
./lab1.py -s lab1/0516_2c_our -p -ops options/lab1_l1_82.txt -m our
```


### lab1 Data processing

```bash
./post_run.py -s lab1/0516_2c_zw -t lab1/0516_2c_zw -m zhangw
./post_run.py -s lab1/0516_2c_our -t lab1/0516_2c_our -m our
```


## LAB 2
```bash
./lab2.py -s lab2/0515_2c_ly_2 -p -ops options/lab2_l1_82.txt -m liangy -a 2
./lab2.py -s lab2/0515_2c_zw_2 -p -ops options/lab2_l1_82.txt -m zhangw -a 2
./lab2.py -s lab2/0515_2c_our_2 -p -ops options/lab2_l1_82.txt -m our -a 2
```

```bash
./lab2.py -s lab2/0515_2c_ly_4 -p -ops options/lab2_l1_82.txt -m liangy -a 4
./lab2.py -s lab2/0515_2c_zw_4 -p -ops options/lab2_l1_82.txt -m zhangw -a 4
./lab2.py -s lab2/0515_2c_our_4 -p -ops options/lab2_l1_82.txt -m our -a 4
```

```bash
./lab2.py -s lab2/0515_2c_ly_8 -p -ops options/lab2_l1_82.txt -m liangy -a 8
./lab2.py -s lab2/0515_2c_zw_8 -p -ops options/lab2_l1_82.txt -m zhangw -a 8
./lab2.py -s lab2/0515_2c_our_8 -p -ops options/lab2_l1_82.txt -m our -a 8
```
or run it with a helper script   
```bash
./run_lab2.sh
```
### lab2 Data processing

```bash
./post_run.py -s lab2/0515_2c_ly_2 -t lab2/0515_2c_ly_2 -m liangy
./post_run.py -s lab2/0515_2c_zw_2 -t lab2/0515_2c_zw_2 -m zhangw
./post_run.py -s lab2/0515_2c_our_2 -t lab2/0515_2c_our_2 -m our
```

```bash
./post_run.py -s lab2/0515_2c_ly_4 -t lab2/0515_2c_ly_4 -m liangy
./post_run.py -s lab2/0515_2c_zw_4 -t lab2/0515_2c_zw_4 -m zhangw
./post_run.py -s lab2/0515_2c_our_4 -t lab2/0515_2c_our_4 -m our
```

```bash
./post_run.py -s lab2/0515_2c_ly_8 -t lab2/0515_2c_ly_8 -m liangy
./post_run.py -s lab2/0515_2c_zw_8 -t lab2/0515_2c_zw_8 -m zhangw
./post_run.py -s lab2/0515_2c_our_8 -t lab2/0515_2c_our_8 -m our
```

or run it with a helper script   
```bash
./post_run.sh
```
