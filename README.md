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
$ ./MemCapture --duration 30 > /tmp/memory.txt
```
Duration is the amount of time to capture data for. Averages calculated over this duration.

Check the contents of `/tmp/memory.txt` to see the memory report