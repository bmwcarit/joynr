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

import TestEnd2EndDatatypesTestData from "../TestEnd2EndDatatypesTestData";
import * as IntegrationUtils from "../IntegrationUtils";
import End2EndAbstractTest from "../End2EndAbstractTest";
describe("libjoynr-js.integration.end2end.datatypes", () => {
    let datatypesProxy: any;
    const abstractTest = new End2EndAbstractTest("End2EndDatatypesTest", "TestEnd2EndDatatypesProviderProcess");

    beforeAll(async () => {
        const settings = await abstractTest.beforeEach();
        datatypesProxy = settings.dataProxy;
    });

    it("supports all datatypes in attributes get/set", async () => {
        for (let i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
            const test = TestEnd2EndDatatypesTestData[i];
            for (let j = 0; j < test.values.length; ++j) {
                const attributeName = test.attribute;
                const attributeValue = test.values[j];
                const attribute = datatypesProxy[attributeName];
                await attribute.set({
                    value: attributeValue
                });
                const value = await attribute.get();
                expect(value).toEqual(attributeValue);
                IntegrationUtils.checkValueAndType(value, attributeValue);
            }
        }
    });

    it("supports all datatypes as operation arguments", async () => {
        for (let i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
            const test = TestEnd2EndDatatypesTestData[i];
            const arg = test.values[0];
            const expectedReturnValue = test.jsRuntimeType;
            const result = await datatypesProxy.getJavascriptType({
                arg
            });
            expect(result).toEqual({
                javascriptType: expectedReturnValue
            });
        }
    });

    it("supports all datatypes as operation argument and return value", async () => {
        for (let i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
            const arg = TestEnd2EndDatatypesTestData[i].values[0];
            const value = await datatypesProxy.getArgumentBack({
                arg
            });
            expect(value).toEqual({ returnValue: arg });
            IntegrationUtils.checkValueAndType(value.returnValue, arg);
        }
    });

    it("supports multiple operation arguments", async () => {
        const opArgs: Record<string, any> = {};
        for (let i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
            const test = TestEnd2EndDatatypesTestData[i];
            /* replace all dots with _ */
            let paramName = `${test.joynrType.replace(/\./g, "_")}Arg`;
            paramName = paramName.slice(0, 1).toLowerCase() + paramName.slice(1);
            opArgs[paramName] = test.values[0];
        }
        const result = await datatypesProxy.multipleArguments(opArgs);
        expect(result).toEqual({
            serialized: JSON.stringify(opArgs)
        });
    });

    afterAll(abstractTest.afterEach);
});
