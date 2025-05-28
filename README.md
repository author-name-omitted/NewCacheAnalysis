# NewCacheAnalysis

This is a multi-core WCET tool base on [LLVM-TA](https://gitlab.cs.uni-saarland.de/reineke/llvmta) built by University of Saarland  

## Get Started

Please ensure that Docker is installed in the system before proceeding with the following steps for building:

### Build the Docker Image  
This project provides two Docker image build files, based on Arch and Ubuntu 22.04 respectively. Please select according to your needs. The build command is:  
```bash
docker build -t llvmtadocker:latest - < .devcontainer/Dockerfile
```  
Note: To facilitate users in China, the build process will replace the image source. If you need to use your own image source, please modify the content of [Dockerfile](.devcontainer/Dockerfile) accordingly.

### Run the Docker Container  
Execute the following command in the root directory of the project repository:  
```bash
docker run -i -d \
    -v `pwd`:/workspaces/llvmta:rw \
    -v `pwd`/build:/workspaces/llvmta/build:rw \
    --name NCA llvmtadocker:latest
```

### Compile the Project  
Use the following command to enter the container:  
```bash
docker exec -it NCA /bin/bash
```  
> Note: The default user after entering the container is root. To avoid permission issues, it is recommended to switch to an ordinary user for compilation. The container comes with an ordinary user named `vscode`, which can be switched to using the `su vscode` command. Use the `sudo chown -R vscode:vscode /workspaces/llvmta` command to modify the permissions of the project folder.  
> Additionally, you may need to reference some environment variables of the root user, such as `$PATH`, `$LD_LIBRARY_PATH`, etc., to avoid compilation issues.  

Then proceed with the compilation according to the following steps:  
```bash
cd /workspaces/llvmta
./config.sh [dev|rel] # dev is recommended
cd build
ninja -j $(nproc)
```  
Among them, `./config.sh dev` uses Debug mode for compilation, while `./config.sh rel` uses Release mode. Please set according to your needs. The dev mode supports debugging but is expected to occupy 70GB of storage. The number of compilation cores can be selected according to your computer configuration, such as `ninja -j12`. Using all cores may affect the normal use of the computer during compilation, and please pay attention to memory load for multi-core compilation.  


### Run the Experiment

```sh
pip install -r requirements.txt  
cd our_experiment
```

#### lab1: Ratio of WCEET

You can modify `MAX_THREAD_HYPER` on line 114 of `lab1.py` to specify the maximum number of benchmarks to run in parallel according to your needs. The following commands can be used to run both the `zhang` and `our` methods:

```bash
./lab1.py -s lab1/0516_2c_zw -p -ops options/lab1_l1_82.txt -m zhangw
./lab1.py -s lab1/0516_2c_our -p -ops options/lab1_l1_82.txt -m our
```

After the execution, you can use the following scripts to aggregate the results:

```bash
./post_run.py -s lab1/0516_2c_zw -t lab1/0516_2c_zw -m zhangw
./post_run.py -s lab1/0516_2c_our -t lab1/0516_2c_our -m our
```

The results can be found in `wceet.csv` under the corresponding folders, and `wceet_runtime.csv` shows the time taken for the analysis.

#### lab2: Varied Cache Associativity

Similarly, you can modify `MAX_THREAD_HYPER` on line 114 of `lab2.py`.

```bash
./run_lab2.sh # this script will call lab2.py
```

After execution, the following scripts can be used for result aggregation:

```bash
./post_lab2.sh  
./lab2_summary_0514.py
```

The summarized results can be found in `lab2/lab2_summary.csv`.
