## Getting started

Before start running simulations, the following line has to be executed:

```
bash main-lib-install.sh
```

After that, build ns-3 as always. Now you are ready to go!

## Running a simulation

There are 2 options:
1. Running a sole simulation with cca-perf.sh file along the corresponding arguments.
2. Running multiple simulations configuring parameters.ccaperf and then executing parallel.sh.

Both files options and flags can be listed calling the script with the `--help` option.

### Examples:
Running the following line:

```
bash cca-perf.sh --verbose --useAI --enqueue "red22"
```
means that:

- verbose: verbose mode activated. Print status and which part were executed.
- useAI: AI mode activated. Use a Python defined AQM (which includes an AI one). Remember to call it alongside --enqueue and/or --dequeue.
- enqueue: Enqueue AQM to use (options available in cca-perf.py, default dummmy). In this case, the AQM for enqueue is RED with a Minimum threshold of 22%.

Running the following line:

```
bash parallel.sh --montecarlo 21 --outDir MY_SIMULATIONS --noBuild --maxSim 4
```
will run every simulation configured in parameters.ccaperf with the following options:

- montecarlo: Number of montecarlo iterations.
- outDir: Custom folder/directory name for the folder with the different simulations, the word "PAR-" will be prefixed. In this example: /PAR-MY_SIMULATIONS.
- noBuild: Skips the build step of ns3.
- maxSim: Maximum number of simultaneous simulations.

**IMPORTANT NOTE**: If the simulations configured in parameters.ccaperf have --useAI flag active, regardless of the AQM algorithm, the number of processors used by your machine will be doubled. For every simulation, there will be two parallel process, one for ns-3 in C++, and the other for AQM algorithms in Python.

### Information about AI/AQM integration

There are multiple AQMs available that work alongside the simulation and interact with the RLC buffer. They are listed inside the cca-perf.py file and can also be displayed calling `python cca-perf.py --help`.

## Building and Running Docker
The only files necessary are the docs folder with its contents and the Dockerfile.

To run ns3 inside a docker container, inside this folder run:

```shell
docker build -t cca-perf-image .
```

After the docker is built, run the next command to start a container in detached mode 

```shell
docker run -d --name cca-perf --hostname cca-perf-cont -v ./:/home/jsandova/ns-3-dev/scratch/cca-perf cca-perf-image
```

Finally, run the following command to enter the docker.
```shell
docker exec -it cca-perf bash
```
Remember to install QUIC and this repository inside the docker!!

## Configs

### Add vector components

Files to update/change:

1. `cca-perf-ai-py.cc`: Add/change the correct component with `.def_readwrite()`.
2. `lte-rlc-um.h`: Add/change in `struct dataToSend{}`.
3. `lte-rlc-um.cc`: Add/change in `msgInterface->GetCpp2PyVector()->at(m_my_num)`.
4. `cca-perf.py`: Add/change in `_name = vector_data.name`.

# Important
If ns3-ai **DOES NOT WORK** with multi-bss example is enabled, disable it by commenting the line `add_subdirectory(multi-bss)` in the file `<PATH>/ns-3-dev/contrib/ai/examples/CmakeLists.txt`.
