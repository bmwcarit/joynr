#!/bin/bash

set -x
pkill -f -9 cluster-controller
pkill -f -9 ilt-provider
pkill -f -9 ilt-consumer
pkill -f -9 java
