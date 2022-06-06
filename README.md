# Prioritized-Planing-with-ML
Learning Agent Ordering for Prioritized Planning


The performance of prioritized planning (PP) is sensetive to its predefined priority ordering.
This works aims to find good priority orderings via machine learning.

The code requires the external library [BOOST](https://www.boost.org/). After you installed BOOST and downloaded the source code, go into the directory of the source code and compile it with CMake: 
```
cmake .
make
```

You also need to download the MAPF instances from the [MAPF benchmark](https://movingai.com/benchmarks/mapf/index.html) under the `map` folder.
Files under the `liblinear_weights_windows` folder are downloaded from the [LIBLINEAR github repo](https://github.com/cjlin1/liblinear). 
Files under the `svm_rank_linux` and `svm_rank_windows` folders are downloded from the [SVMRank library](https://www.cs.cornell.edu/people/tj/svm_light/svm_rank.html).


To generate training/test datasets, you'll need to edit some variables in `training/driver_train.cpp` on their corresponding lines between "CUSTOMIZE begin" and "CUSTOMIZE end". ML-T training files are created under `out/build/x64-Release/svm_rank_files/<map-name>/<num-of-agents>`; ML-P training files are created under `out/build/x64-Release/liblinear_files/<map-name>/<num-of-agents>`. The test result files will also be under those folders. 

If generating training datasets for ML-T:
```
string train_file = "train_1to25_raw.dat"; //ML-T training file must end with .dat
bool useLIBLINEAR = false;
bool generateTestFile = false && (!useLIBLINEAR);
```
The training file `train_1to25_norm_group5.dat` is created from `train_1to25_raw.dat` by normalizing the features and grouping the agents into priority groups of 5. 

If generating training datasets for ML-P:
```
string train_file = "train_25x1top5_commondep_liblinear";
bool useLIBLINEAR = true;
bool generateTestFile = false && (!useLIBLINEAR);
```
If generating test datasets, which will be under `out/build/x64-Release/liblinear_files/<map-name>/<num-of-agents>`:
```
string test_file = "test_1to25_fakelabel_raw.dat";
bool useLIBLINEAR = false;
bool generateTestFile = true && (!useLIBLINEAR);
bool solvePPforTestFile = false;
```

After setting the variables, you can run the command 
```
./pp_train -m ../map/random-32-32-20.map -a ../map/random-32-32-20.map-scen-random/random-32-32-20-random-1.scen -o test.csv -k 50
```
- m: the map file from the MAPF benchmark
- a: the scenario file from the MAPF benchmark
- k: the number of agents

You can find detailed explanation of the parameters with
```
./pp_train --help
```


If running test cases on LH, SH, ML-T/ML-P, RND, edit these variables in `test/driver_test.cpp` on their corresponding lines: 
```
vector<int> num_agents_to_train = { 50 }; //set this to the number of agents to train on
vector<int> num_agents_to_test = { 50 }; //set this to the number of agents to test on
...
vector<int> pp_runs = {0,1,2,3}; //0: LH; 1: SH; 2: ML; 3: R; must be ascending sorted
string map_name = "random-32-32-20"; //set to the map name
bool useLiblinear = true; //set to false if testing for ML-T instead of ML-P
string train_file = useLiblinear? "train_25x1top5_commondep_liblinear": "train_1to25_norm_group5.dat"; //set to the ML-P/NL-T training file name
string test_file = "test_1to25_fakelabel_norm.dat"; //set to the test file name
bool has_model_file = false;
bool rank_random_sampling = true; //set to false if running deterministic testing
bool random_restart = true; //set to false if running deterministic testing
bool fixedtime_restart = true; //set this to false if running deterministic testing
```

Then, run the command:
```
./pp_test
```
Note that every PP run in stochastic ranking with a fix time limit will take 1 minute for small maps (random-32-32-20, maze-32-32-2, room-32-32-4, warehouse-10-20-10-2-1) and 10 minutes for large maps (lak303d and ost003d).


## License
The software is released under USC – Research License. See license.md for further details.
 
## References

