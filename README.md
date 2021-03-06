<h1 align="center">biot</h1>
<p align="center">BAM I/O Throughput Benchmark.</p>

# What is this about?
If you want measure the throughput of reading and writing BAM files, with
optional multithreaded BAM compression and decompression. `biot` reads in a
BAM file and writes the identical file out. Except for the (de)compression,
it's a serial workflow.

# How to compile

    mkdir build && cd build
    meson --prefix ~/mytools
    ninja
    ninja install

# Precompiled binary
You can find fully static binaries under releases. They are build with
 * gcc 10.3
 * glibc 2.33
 * mimalloc 1.63
 * zlib 1.2.11
 * htslib 1.13
# How to run

    biot movie.input.bam movie.output.bam --log-level INFO --compression-threads 8 --decompression-threads 8

# Interpret LOG output

    | 20210913 11:03:04.450 | INFO | Reads      : 9995            <- Number of input BAM records
    | 20210913 11:03:04.453 | INFO | Yield      : 159.3 MBases    <- Sum of all sequence lengths
    | 20210913 11:03:04.453 | INFO | Throughput : 6.2 GBases/min  <- Achieved throughput per minute
    | 20210913 11:03:04.453 | INFO | Run Time   : 1s 542ms        <- Wall time
    | 20210913 11:03:04.453 | INFO | CPU Time   : 17s 191ms       <- Consumed CPU time
    | 20210913 11:03:04.453 | INFO | Peak RSS   : 0.037 GB        <- Maximum occupied RAM

# Benchmarks

Hardware: 2x AMD 7702, Micron 9300 SSD w/ 3.5 GByte/s seq read/write

Throughput measures in **GBases per minute**:

| Threads |  CLR  | CCS+all | CCS+all+kinetics | HiFi only | HiFi+kinetics |
| :-----: | :---: | :-----: | :--------------: | :-------: | :-----------: |
|    1    |  0.7  |   2.2   |       0.5        |    1.5    |      0.4      |
|    2    |  1.8  |   5.4   |       1.1        |    3.9    |      0.9      |
|    4    |  3.4  |  10.6   |       2.2        |    7.6    |      1.6      |
|    8    |  6.4  |  19.8   |       4.2        |   14.3    |      3.2      |
|   16    | 11.4  |  35.9   |       7.5        |   25.8    |      5.8      |
|   32    | 19.0  |  58.8   |       12.4       |   41.5    |      9.6      |
|   64    | 24.8  |  76.8   |       16.0       |   56.3    |     12.8      |

No significant improvements beyond 64 threads.

Comparison NVMe Micron 9300 SSD vs spinning disk RAID 10 on 64 threads:
| Threads | CCS+all | CCS+all+kinetics | HiFi only | HiFi+kinetics |
| :-----: | :-----: | :--------------: | :-------: | :-----------: |
|  NVMe   |  76.8   |       16.0       |   56.3    |     12.8      |
|  HDDs   |  60.0   |       13.9       |   51.8    |     11.1      |
