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
const Mutex = require("await-semaphore").Mutex;
const RequireUtil = require("../js/node_integration/RequireUtil.js");

console.log("joynr Jasmine 2.x system integration tests");

const requirePaths = [
    "../js/node_integration/CompatibleProvidersTest.js",
    "../js/node_integration/End2EndRPCTest.js",
    "../js/node_integration/End2EndSubscriptionTest.js",
    "../js/node_integration/End2EndDatatypesTest.js",
    "../js/node_integration/IncompatibleProviderTest.js",
    "../js/node_integration/LocalDiscoveryTest.js",
    "../js/node_integration/MultipleVersionsTest.js"
];

async function executeTests() {
    const pathsNum = requirePaths.length;
    for (let i = 0; i < pathsNum; i++) {
        const Jasmine = require("jasmine");
        const jasmine = new Jasmine();
        jasmine.loadConfigFile(`${__dirname}/spec/support/jasmine.json`);
        const path = requirePaths[i];
        require(path);
        const mutex = new Mutex();
        const release = await mutex.acquire();
        //jasmine.onComplete(callback) prevents the process from exiting.
        //So it should not be used for the last test.
        if (i !== pathsNum - 1) {
            jasmine.onComplete(() => {
                release();
            });
        }
        jasmine.execute();
        await mutex.acquire();
        RequireUtil.deleteModuleAndNewChildrenFromCache(require.resolve(path));
        RequireUtil.deleteModuleAndNewChildrenFromCache(require.resolve("jasmine"));
    }
}

executeTests();