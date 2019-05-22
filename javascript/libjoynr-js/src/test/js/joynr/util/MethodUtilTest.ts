/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
import * as MethodUtil from "../../../../main/js/joynr/util/MethodUtil";

describe("libjoynr-js.joynr.MethodUtil", () => {
    it("transformParameterMapToArray throws for null values", () => {
        const operationArguments = { bool: null };
        const parameters = [{ name: "bool", type: "Boolean" }];
        expect(() => {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws for undefined values", () => {
        const operationArguments = { bool: undefined };
        const parameters = [{ name: "bool", type: "Boolean" }];
        expect(() => {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match number of arguments", () => {
        const operationArguments = {};
        const parameters = [{ name: "bool", type: "Boolean" }];
        expect(() => {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match arguments (name mismatch)", () => {
        const operationArguments = { int: true };
        const parameters = [{ name: "bool", type: "Boolean" }];
        expect(() => {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match arguments (type mismatch)", () => {
        const operationArguments = { bool: 2 };
        const parameters = [{ name: "bool", type: "Boolean" }];
        expect(() => {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray throws if signature does not match arguments (type mismatch (no array))", () => {
        const operationArguments = { bool: [true] };
        const parameters = [{ name: "bool", type: "Boolean" }];
        expect(() => {
            MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        }).toThrow();
    });

    it("transformParameterMapToArray can convert map to array", () => {
        const expectedObject = { paramDatatypes: ["Boolean"], params: [true] };
        const operationArguments = { bool: true };
        const parameters = [{ name: "bool", type: "Boolean" }];
        const result = MethodUtil.transformParameterMapToArray(operationArguments, parameters);
        expect(result).toEqual(expectedObject);
    });
});
