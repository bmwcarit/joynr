
# Joynr Javascript testing

## Testing environment

All tests are run through the [jest](https://jestjs.io/) testing framework. The test runner is
configured to be ``jest-circus``, which is also developed by the ``jest`` team.  
Tests are run directly against the typescript files without triggering a manual compilation to js.
The compilation is embedded into the process by the ``ts-jest`` addon.

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

Start joynr mqtt infrastructure:

Please refer to the
[starting joynr backend instructions](../wiki/StartingJoynrBackend.md)

Then run:

```
cd <JOYNR_REPO>
cd javascript/libjoynr-js
mvn clean install -DskipTests=false
```

**Note:** Running the maven commands does a lot more than running the tests (e.g. generating files
from fidls, run the formatter), which makes it slow. Thus it should only be done once to prepare the
environment. The preferred way to run tests is through npm. The commands are explained in the
following sections.

### Running tests
**Note:** Here and in all following sections all paths (also in commands) are relative to <JOYNR_REPO>javascript/libjoynr-js.

For running all tests

```
npm test
```

has to be run.

#### Testing framework configuration

The unit tests are configured via ``jest.config.js`` and the integration test are configured via
``jest.config.integration.js``.

#### Selection of test cases

If only a single-test should be run, the easiest way is to use the ```fit()``` (to enable just a
single test-case) or ```fdescribe()``` API (to enable all tests for that section) instead
of the regular ```it()``` or ```describe()``` ones. All other tests will be automatically
disabled. <br />

#### Test execution

```
npm run ts:test
```
will run the unit tests without coverage

```
npm run ts:test:ci
```
will run the unit tests with coverage

```
npm ts:test:sit
```

will run the sytem integration tests.

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

### Multi language tests

JS is tested in combination with CPP, Java and JEE.
These tests are available:  
* Inter Language Test (ILT)
* Performance Test
* System Integration Test
* Robustness Test

The tests and their respective documentation can be found in ``<JOYNR_REPO>/tests/``.

#### Test/release package

The test/release package can be built using
``npm run ts:package``

This package is file referenced by the test package.json files of the multi language tests.

#### Special case ILT:

The ILT is further tested using [browserify](http://browserify.org/)
which bundles a javascript application into a single file.
``browserify`` compatibility is a requirement for joynr which imposes additional
constraints on the codebase like non dynamic imports. Including ``browserify`` into the ILT test
ensures that apps using joynr won't face any difficulties including ``browserify`` into their build
chains.

### (JS) joynr-generator npm module

The joynr-generator npm module is a javascript wrapper script around
``joynr-generator-standalone.jar``.  
It builds into the npm eco system and provides compilation of the generated ``.ts`` files to ``.js``.

The joynr-generator is tested in <JOYNR_REPO>/javascript/joynr-generator-test.
Use ``npm test`` to run all related tests.

