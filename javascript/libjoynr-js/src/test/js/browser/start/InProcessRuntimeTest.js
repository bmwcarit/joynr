/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
const provisioning = require("../../../resources/joynr/provisioning/provisioning_cc");
const InProcessRuntime = require("../../../../main/js/joynr/start/InProcessRuntime");

function outputPromiseError(error) {
    expect(error.toString()).toBeFalsy();
}

describe("libjoynr-js.joynr.start.TestInProcessRuntime", () => {
    let runtime;

    function startInProcessRuntime() {
        return runtime.start(provisioning).catch(outputPromiseError);
    }

    function shutdownInProcessRuntime() {
        return runtime.shutdown().catch(outputPromiseError);
    }

    beforeEach(done => {
        runtime = new InProcessRuntime();
        done();
    });

    it("is of correct type and has all members", done => {
        expect(InProcessRuntime).toBeDefined();
        expect(runtime).toBeDefined();
        expect(runtime instanceof InProcessRuntime).toBeTruthy();
        expect(runtime.logging).toBeDefined();
        expect(runtime.typeRegistry).toBeDefined();

        expect(runtime.registration).toBeNull();
        expect(runtime.proxyBuilder).toBeNull();

        startInProcessRuntime()
            .then(() => {
                expect(runtime.typeRegistry).toBeDefined();
                expect(runtime.registration).toBeDefined();
                expect(runtime.proxyBuilder).toBeDefined();
                done();
                return null;
            })
            .catch(fail);
    });

    it("can be started and shutdown successfully", done => {
        startInProcessRuntime()
            .then(shutdownInProcessRuntime)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    const nrRestarts = 3;
    it(`can be started and shut down successfully ${nrRestarts} times`, done => {
        let i;

        function createFunc(promiseChain) {
            return promiseChain.then(shutdownInProcessRuntime).then(startInProcessRuntime);
        }

        let promiseChain = startInProcessRuntime();
        for (i = 1; i < nrRestarts; ++i) {
            promiseChain = createFunc(promiseChain);
        }
        promiseChain
            .then(shutdownInProcessRuntime)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("rejects Promise when started in state STARTED", done => {
        startInProcessRuntime()
            .then(() => runtime.start(provisioning))
            .then(fail)
            .catch(() => done());
    });

    it("throws when shutdown in state SHUTDOWN", () => {
        return startInProcessRuntime()
            .then(shutdownInProcessRuntime)
            .then(() => {
                return runtime.shutdown().then(fail, () => {});
            });
    });
});
