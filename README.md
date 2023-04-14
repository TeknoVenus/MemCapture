# MemCapture
Memory capture and analysis tool for RDK

## Build
```shell
$ mkdir build && cd ./build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j$(nproc)
```

## Run
```shell
Usage: MemCapture <option(s)>
    Utility to capture memory statistics

    -h, --help          Print this help and exit
    -o, --output-dir    Directory to save results in
    -r, --report        Type of report to generate. Supported options = ['CSV', 'TABLE']. Defaults to TABLE
    -d, --duration      Amount of time (in seconds) to capture data for. Default 30 seconds
    -p, --platform      Platform we're running on. Supported options = ['AMLOGIC', 'REALTEK', 'BROADCOM']. Defaults to Amlogic
```

Example:
```shell
$ ./MemCapture --platform AMLOGIC --duration 30 --report CSV --output-dir /tmp/memcapture_results/
```
Averages are calculated over the specified duration.

### Results

By default, results are saved into a single `report.txt` file in `<current-directory>/<timestamp>/report.txt`. To change the output directory, provide a valid path to the `-o` argument.

By selecting the CSV option, each report will be saved as its own CSV file which can then be imported into Excel for analysis.

### Notes

Tool currently supports three platforms - `AMLOGIC` (default), `REALTEK` and `BROADCOM`. Note Realtek platforms don't expose performance metrics in the same way as Amlogic, so some stats are not available
