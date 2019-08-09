
# Joynr Javascript testing

## Testing environment

The libjoynr Javascript tests use the
[Jasmine 2.4](http://jasmine.github.io/2.4/introduction.html) test
framework.

Unit tests are run as nodejs tests. (See section [Nodejs tests](#nodejs_tests) for details.)<br />
For browser tests (e.g. InProcessRuntimeTest.js in <JOYNR_REPO>/javascript/libjoynr-js/src/test/js/joynr/start)
we use [Karma](https://karma-runner.github.io) as test runner.
(See section [Karma test configuration](#karma_test_configuration) for details)

### Prerequisites

In order to run the tests, the following environment must be built:

```
cd <JOYNR_REPO>
mvn clean install -P javascript
cd javascript/libjoynr-js
```

In addition, a MQTT broker (e.g. [Mosquitto](http://mosquitto.org)) must be running locally
supporting web socket connections on port 9001. An example mosquitto configuration can be found
under javascript/libjoynr-js/src/test/resources/mosquitto-test.conf, allowing to start the broker
via the following command:

```
cd <JOYNR_REPO>/javascript/libjoynr-js
mosquitto -c src/test/resources/mosquitto-test.conf
```

The joynr mqtt infrastructure also has to be started by deploying discovery-directory-jee and
domain-access-controller-jee to a Java EE application server (for the configuration of the
application server see [Infrastructure](infrastructure.md) and [JEE Developer Guide](jee.md)
or the [Radio Tutorial](Tutorial.md)).

Start joynr mqtt infrastructure:

```
asadmin deploy <JOYNR_REPO>/java/backend-services/discovery-directory-jee/target/discovery-directory-jee-<JOYNR_VERSION>.war
asadmin deploy <JOYNR_REPO>/java/backend-services/domain-access-controller-jee/target/domain-access-controller-jee-<JOYNR_VERSION>.war
```

Then run:

```
cd <JOYNR_REPO>
cd javascript/libjoynr-js
mvn clean install -DskipTests=false
```

(This will also run all tests in src/tests/resources/node-run-unit-tests.js and src/tests/resources/node-run-system-integration-tests.js with node
and src/test/js/joynr/start/InProcessRuntimeTest.js with karma.)

**Note:** Running the maven commands does a lot more than running the tests (e.g. generating files from fidls, run the formatter), which makes it slow.
Thus it should only be done once to prepare the environment.
The preferred way to run tests is through npm. The commands are explained in the following sections.

### Running tests
**Note:** Here and in all following sections all paths (also in commands) are relative to < JOYNR_REPO >javascript/libjoynr-js.

For running all tests (all tests in src/tests/resources/node-run-unit-tests.js and src/tests/resources/node-run-system-integration-tests.js with node
and src/test/js/joynr/start/InProcessRuntimeTest.js with karma) simply

```
npm run test
```

has to be run.

How to execute single tests with node or karma can be read the following sections.

#### <a id="karma_test_configuration"></a>Karma test configuration

The karma test environment is configured using the following file:

```
## Integration test configuration
src/test/karma/karma.integration.conf.js
```

It specifies
* which files are made available (files to be tested, environment, test cases)
* which port is used for debugging
* which reporters are used (e.g. junit to create XML output)
* the output location of the test reports
* that the tests are run with the help of browserify

The last point means: A browserify build is started (requires all modules in the browser) and then tested.

##### Selection of test cases

The test cases can be expanded or shrinked by

* reducing the number of files provided for loading in the browser through configuration change in
  the ```karma.integration.conf.js```

If only a single-test should be run, the easiest way is to use the ```fit()``` (to enable just a
single test-case) or ```fdescribe()``` API (to enable all tests for that section) instead
of the regular ```it()``` or ```describe()``` ones. All other tests will be automatically
disabled. <br />
But for a single-test it might actually be easier to run it as a node test. For node the execution of one test at a time is
simplified by the node-unit-test-helper. (See section [Nodejs tests](#nodejs_tests) for details.)

If multiple tests should be run, then either the pattern (```TEST_REGEXP```) needs to be
modified or the list of loaded files must be explictly specified (instead of using wildcard
syntax), so that tests that should not be run are not loaded and consequently cannot be found
by the pattern match.

##### Debugging

Karma tests can be debugged as follows:

In the required ```karma.<config>.js``` temporarily change the following entry:

* change ```browsers``` to the favored browser (e.g. ```Chrome```)

Example:

```
    ...
    browsers: ['Chrome'],
    ...
```

Running the tests will then open the specified browser if the option
--single-run was set to false in the invocation.

The command should look like this:

```
node_modules/.bin/karma start src/test/karma/karma.integration.conf.js --single-run=false
```

If running a browser with visible UI just hit the debug button once the test has run once.
If using a headless browser (e.g. ```ChromeCustom```) please run a browser (e.g. ```Google-Chrome```)
and load the following URI:

```
http://localhost:9876/debug.html
```

Activate the Browser console / debug window. (In Chrome for example the developer tools have to be opened in the browser.)

Browse the ```Files``` section or alike in order to locate the test-case in ```test-classes``` hierarchy
or the test source (e.g. in ```classes```). Set breakpoints as needed.
Reload the page (e.g. using ```Ctrl-R```). The tests should be executed again.

##### Starting karma from the command line

Command line for karma test execution:

```
npm run karma
```

#### <a id="nodejs_tests"></a>Nodejs tests

##### Selection of test cases

The configuration of the nodejs tests is part of the files

 ```
 /src/test/resources/node-run-unit-tests.js
 /src/test/resources/node-run-integration-tests.js and
 /src/test/resources/node-run-system-integration-tests.js.
 ```

All modules that are required here will be tested.
In order to run a different selection of tests this list can be adapted accordingly.

##### Test execution

```
npm run test:unit
```

will run the unit tests listed in node-run-unit-tests.js.

```
npm test:sit
```

will run the sytem integration tests.

##### Manual execution

The node tests can also be run using

```
cd <TESTDIR>
node <TESTCASENAME>
```

< TESTCASENAME > can for example be node-run-unit-tests.js in src/tests/resources and the command will execute all tests (also the required ones) in the file.

(use --inspect for debugging with chrome dev tools for node - Note: Debugging doesn't work with forked processes in nodejs because then the debug port is already in use!)

Note that each test that should be run in automatic mode with node has to be embedded in a jasmine
environment and be followed by jasmine.execute() (cp. node-run-unit-tests.js). <br \>
Or src/test/js/joynr/node-unit-test-helper can be required at the beginning of the file, which makes it easy to run single tests.

This is exactly what the command npm run test:unit/test:sit does.

### Limitations

Joynr supported browser and node as environments and thus there are tests for both.
But since the introduction of smrf the browser support is broken and therefore browser tests are limited.

### More helpful tools for test writing

#### Formatting of tests

```
npm run format
```

will automatically format all js files in the libjoynr-js project using the formatter prettier.

#### Static code analysis

Running the command

```
npm run lint
```

will automatically run a static code analysis on all files in the libjoynr-js project and
show when certain rules are violated (e.g. default case in a switch is omitted).

When running

```
npm run lint:fix
```

instead, eslint will automatically fix as many issues as possible

### Speed up mvn build for manual test execution

As already mentioned the mvn build is only needed once in the beginning (unless fidl files have changed or the general structure of the project...)
for running tests. <br \>
If a rerun is still wished for some reason, it can be sped up.

#### Skip automatic test execution during build with tests enabled
Automatic test execution during the maven build can be skipped while still preparing the tests for later execution.
This allows to run only the required tests or run them manually later (npm run test) without having to run the
whole maven build.

In order to build the tests without executing them automatically, run:

```
cd <JOYNR_REPO>
cd javascript/libjoynr-js
mvn clean install
```
