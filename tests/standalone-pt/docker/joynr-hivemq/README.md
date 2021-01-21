# HiveMQ Base Image

## Basic Single Instance

To start a single HiveMQ instance and allow access to the MQTT port as well as the Control Center,
[get Docker](https://www.docker.com/get-started) and run the following command:

`docker run --ulimit nofile=500000:500000 -p 8080:8080 -p 8000:8000 -p 1883:1883 hivemq/hivemq4`

You can connect to the broker via MQTT (1883) or Websockets (8000) or the Control Center (8080) via the respective ports.

## Clustering and Configuration

For clustering and configuration please refer to the [readme](./README.md).
