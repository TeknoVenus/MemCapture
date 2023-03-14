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
$ ./MemCapture --platform AMLOGIC --duration 30 > /tmp/memory.txt
```
```shell
$ ./MemCapture --platform REALTEK --duration 30 > /tmp/memory.txt
```
```shell
$ ./MemCapture --platform BROADCOM --duration 30 > /tmp/memory.txt
```

Duration is the amount of time to capture data for. Averages calculated over this duration.

Tool currently supports thre platforms - `AMLOGIC` (default), `REALTEK` and `BROADCOM`. Note Realtek platforms don't expose performance metrics in the same way as Amlogic, so some stats are not available

Check the contents of `/tmp/memory.txt` to see the memory report