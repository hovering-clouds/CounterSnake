## About

This repo contains the C++ implementation for CounterSnake, which is a hierarchical compression framework that reduces memory consumption of sketch counters. We integrate CounterSnake with nine sketches, and also implement several SOTA frameworks for comparison. We also design a unified testing framework to run these frameworks and sketches on different datasets, which automatically gather evaluation results of their performance.

The directories looks like this graph:

`CounterSnake`
├── `data`: datasets put here
├── `doc`
│   ├── `image`
│   └── `tutorial`
├── `exp`: configuration files used in our experiments
│   ├── `old`
│   └── `vldb`
├── `src`
│   ├── `common`: header files for our testing framework
│   ├── `driver`: automatically generated driver codes
│   ├── `impl`: implementation of some methods in `commom`
│   ├── `layer_counter`: source codes of CounterSnake and compared frameworks
│   ├── `pcap_parser`: scripts that convert datasets to a unified format
│   ├── `sketch`: implementation of sketches used in our experiments 
│   └── `sketch_test`: codes that run sketch instances and gather results
├── `test`: unit tests to verify the implementation correctness 
└── `third_party`: dependencies



## Build

### Install Dependencies

We require the following dependencies to build this project on Linux or Mac.

| Dependency | Installation (on Linux) | Installation (on Mac) |
|---|---|---|
|  **Cmake** (>=3.20) | `sudo apt-get install cmake -y` | `brew install cmake` |
| **boost** (>=1.75) | `sudo apt-get install libboost-all-dev -y` | `brew install boost` |
| **libpcap** (>=1.9) | `sudo apt-get install libpcap-dev -y` | `brew install libpcap` |
| **PcapPlusPlus** (>=21.05) | Method 1. Build from [source](https://pcapplusplus.github.io/docs/install#build-from-source) using its default configuration and installation directory <br />Method 2. `brew install pcapplusplus`  | `brew install pcapplusplus` |

This repo also depends on three third-party libraries, namely [eigen](https://gitlab.com/libeigen/eigen), [fmt](https://github.com/fmtlib/fmt), and [tomlplusplus](https://github.com/marzer/tomlplusplus). They are maintained as git submodules. Hence, don't forget to clone them with `git submodule update --init`. Besides, make sure your C++ compiler supports C++17 and the python interpreter version is at least 3.7 to enable essential library features.

### Build Steps
The following shell script builds the testing framework. As long as the dependencies are correctly installed, the script should run successfully. 
```shell
mkdir build && cd build
cmake ..
make
```

After executing the script, we will see a number of executable files in the `build/` directory, including:
- vanilla sketches without optimization: CM, DT, ES, FR, HP, MV, PR, SL (no need to run them directly in our experiments)
- compression frameworks that are integrated with the sketches: BitMatcher, Bitsense, Diamond, Dway, DwayNeg, Pyramid, Sac, Stingy
- dataset parsers: parser, parser-kosarak, synthesizer

Note that the executable files of our CounterSnake framework is named as Dway and DwayNeg, the latter of which support negative counters with the sign-bit encoding technique.

## Run

### Dataset Preparation

Before running the executable files, we should prepare the stream dataset and covert them into a format that can be accepted by the testing framework. 

Currently we provide scripts for three datasets, namely [Caida](https://www.caida.org/catalog/datasets/passive_dataset/), [Kosarak](http://fimi.uantwerpen.be/data/) and Zipf. The first two can be download at the corresponding websites, and the last one is generated with our scripts.

For Caida trace:
- You should get a bunch of `.pcap` files, whose name start with 'equinix-nyc.dira.20190117-130000' or something similar. Choose one of them and put it into `data/`. 
- We will trunctate it and only use the the first 1.0M disctinct items. To do this, modify `input` entry in `src/pcap_parser/parser.toml` as your pcap file name, and then go to `build` directory and run `./parser -c ../src/pcap_parser/parser.toml`.
- After this, you should see a new file `data-1000K.bin` in `data/`, and this is the parsed Caida dataset.

For Kosarak trace:
- You should get a file `kosarak.dat`. 
- Put it into `data/` and run `./parser_kosarak -i ../data/kosarak.dat -o ../data/kosarak.bin` to parse it.
- After this, you should see a new file `kosarak.bin` in `data/`, and this is the parsed Caida dataset.


For Zipf datasets:
- Go to `src/pcap_parser/parser.toml` and change the `skew` there. You should also adjust the `flow_number` (number of disctinct items) to let the total number of items be roughly 25M.
- Run `./synthesizer -c ../src/pcap_parser/parser.toml` to generate the file.
- You should properly set the `output_file` name so that we end up with the following files: `zipf-000.bin`, `zipf-025.bin`, `zipf-050.bin`, `zipf-075.bin`, `zipf-100.bin`, `zipf-125.bin`, `zipf-150.bin`, `zipf-175.bin`, `zipf-200.bin`, which corresponds to skewness $0.0\sim 2.0$.

### Configuration Files

The executable files of all the compared frameworks can be run by `exec_file -c config_file`. Therefore, we must provide configuration files to each of the frameworks. It's tedious work to set all these frameworks' diverse parameters, so we have prepared these configurations files in `exp/vldb/`. 

For example, to run CounterSnake on frequency estimation tasks (Exp\#1) with 1MB memory budget, try `./Dway -c ../exp/vldb/exp1-2-4-5-freq/config_1M/dway_1M.toml` in `build/` directory.

### Run experiments
We also provide shell scripts to run a bunch of related experiments at a time. These scripts are in the subdirectories of `exp/vldb/`. 

For example, to test CounterSnake on frequency estimation tasks on all Zipf datasets (Exp\#3), try `../exp/vldb/exp3-zipf-freq/run.bash ./Dway dway` in `build/` directory. The first parameter is the path of the executable file, and the second parameter is the prefix of the configuration files (go to `../exp/vldb/exp3-zipf-freq/config_000/` and you'll see all the possible prefixes).

### Gather Results
Unfortunately, we record all results manually so there is no automatic scripts that can gather the results and turn them into nice figures presented in the paper. However, the output of the executable files should be clear to read, and the related metrics can be extracted from there.
