/*jslint es5: true*/
/*global fail: true*/

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

const End2EndAbstractTest = require("./End2EndAbstractTest");

describe("Compatibility tests for providers created by new generator", () => {
    it("Provider with version in package name and original proxy", done => {
        const abstractTest = new End2EndAbstractTest(
            "ProviderWithVersionedPackageNameTest",
            "TestMultipleVersionsInterfaceProcess",
            "packageVersion2"
        );

        abstractTest
            .beforeEach()
            .then(abstractTest.afterEach)
            .then(done)
            .catch(fail);
    });

    it("Provider with version in name and original proxy", done => {
        const abstractTest = new End2EndAbstractTest(
            "ProviderWithVersionedNameTest",
            "TestMultipleVersionsInterfaceProcess",
            "nameVersion2"
        );

        abstractTest
            .beforeEach()
            .then(abstractTest.afterEach)
            .then(done)
            .catch(fail);
    });
});
