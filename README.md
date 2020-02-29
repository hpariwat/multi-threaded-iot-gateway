# Multi-threaded IoT Sensor Gateway

This is a sensor gateway for reading and storing sensor data from IoT networks with TCP connection using a shared memory space under a multi-threaded environment (POSIX threads) with gcc and SQLite.

## Running

Make the sensor gateway and run it

```bash
$ make run
```

Normally we use real sensor data, but for the testing purposes we can run our own dummy sensor nodes

```bash
$ ./sensor_node {sensor_id} {operating_time} {ip} {port}
```

And cut off the transmission (here it will destroy all sensor nodes)

```bash
$ make kill
```

## Test Scripts
Stress Test: running 8 sensors simultaneously

```bash
$ make test-stress
```

Scaling Test: running 8 sensors simultaneously with varying cut-off time

```bash
$ make test-scal
```

Endurance Test: running 8 sensors simultaneously for 1 hour

```bash
$ make test-endur
```

Timeout Test: running 8 sensors simultaneously and let each be killed one by one

```bash
$ make test-timeout
```
