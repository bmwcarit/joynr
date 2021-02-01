# Performance measurement environment - Java

This application enables to perform requests in two different ways:
* perform N-number of loookup requests asynchronously with a particular max number
  of inflight requests
* perform N-number of lookup requests asynchronously with a particular max number
  of inflight requests and also perform asynchronous N-number of proxy creation
  operations in a separate thread

During the performance measurement of RPC calls as described above, multiple statistics
are created (total duration of the test case, average request response time, max response
time, etc.) and written to the particular csv-file ("PerformanceMeasurementTest.csv" by
default).

## Application goal

This application has been implemented in order to measure:
* an average duration of the request
* a number of the completed requests per second
with and without parallel proxy creation in a separate thread.
The collected data can be used for the evaluation of the joynr performance
after an impmenetation of particular changes (for example, implementation
of Util.ObjectMapper).

## Build

Execute the following command in the terminal in tests/performance-measurement-environment:

`mvn clean install`

## Run the application

Run the following script in the terminal in tests/performance-measurement-environment:

`./run-performance-test.sh`

## Configuration of the "run-performance-test.sh" script

The script "run-performance-test.sh" is created to configure and to perform two types of
test cases (asynchronous request performance with and without parallel proxy creation) consecutive.

Example of the test performance configuration:
```
NUM_REQUESTS=10000
NUM_MAXINFLIGHT=100
NUM_PROXIES=2000
FILENAME="PerformanceMeasurementTest.csv"
NUM_ITERATIONS=10

for testcase in 'REQUESTS_ONLY' 'REQUESTS_WITH_PROXY'; do
    echo "Testcase: $testcase"
    performPerformanceMeasurementTest $testcase $NUM_REQUESTS $NUM_MAXINFLIGHT $NUM_PROXIES $FILENAME $NUM_ITERATIONS
done
```
where
* **NUM_REQUESTS**: number of requests to perform asynchronously in the context of a particular
  test case.
* **NUM_MAXINFLIGHT**: number of requests that are handled asynchronously in parallel in the
  context of a particular test case.
* **NUM_PROXIES**: number of proxy creation operation to perform in a separate thread while
  requests handling in the context of a particular test case.
* **FILENAME**: name of an output-file to save all collected test data.
* **NUM_ITERATIONS**: number of iteration to perform a particlar test case.
* **testcase**: type of a test case to perform, it can has one the following values:
  `REQUESTS_ONLY` to perform requests without a parallel proxy creation, `REQUESTS_WITH_PROXY`
  to perform requests alongwith a parallel proxy creation in a separate thread.

Several exectutions of the performance measurement test will accumulate results in the same
output `csv` file. A resulting csv-file will be saved in under the following path:
`$JOYNR_SRC/tests/performance-measurement-test`

NOTE: a resulting csv-file will not be created again if it already exists, all new collected
data will be appended to the pre-existed `csv` file.

## Measured metrics

After the execution of "run-performance-test.sh" the following parameters are written
to an output `csv` file that are relevant for the further performance evaluation:

* **NumOfRequests**: number of performed asynchronous requests in test case (with maxInflightNumber 100)
* **NumOfProxies**: number of proxy creation operation performed in a separate thread while
  requests handling
* **NumberOfReqsPerSec**: number of requests performed per second in the context of a particular test case
* **AverageResponseTime[ms]**: average duration of a performed request (from its sending to its completion) in
  the context of a particular test case
