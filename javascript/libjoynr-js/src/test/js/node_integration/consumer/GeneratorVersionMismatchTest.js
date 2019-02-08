/*global fail: true*/
/*jslint es5: true*/

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

const End2EndAbstractTest = require("../End2EndAbstractTest");
const NoCompatibleProviderFoundException = require("../../../../main/js/joynr/exceptions/NoCompatibleProviderFoundException");

const abstractTest = new End2EndAbstractTest("GeneratorVersionMismatchTest", "TestMultipleVersionsInterfaceProcess", {
    versioning: "packageVersion1"
});

describe("Incompatible provider test", () => {
    it("Proxy version greater than provider version", done => {
        abstractTest
            .beforeEach()
            .then(() => {
                fail("Expected NoCompatibleProviderFoundException was not thrown");
            })
            .catch(error => {
                expect(error instanceof NoCompatibleProviderFoundException).toBe(true);
                expect(error.discoveredVersions.length).toEqual(1);
                expect(error.discoveredVersions[0].majorVersion).toEqual(1);
            })
            .then(abstractTest.afterEach)
            .then(done)
            .catch(fail);
    });
});
