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

import End2EndAbstractTest from "../End2EndAbstractTest";

import NoCompatibleProviderFoundException from "../../../../main/js/joynr/exceptions/NoCompatibleProviderFoundException";
import testUtil = require("../../testUtil");

const abstractTest = new End2EndAbstractTest("GeneratorVersionMismatchTest", "TestMultipleVersionsInterfaceProcess", {
    versioning: "packageVersion1"
});

describe("Incompatible provider test", () => {
    it("Proxy version greater than provider version", async () => {
        const error = await testUtil.reversePromise(abstractTest.beforeEach());
        expect(error instanceof NoCompatibleProviderFoundException).toBe(true);
        expect(error.discoveredVersions.length).toEqual(1);
        expect(error.discoveredVersions[0].majorVersion).toEqual(1);
        await abstractTest.afterEach();
    });
});
