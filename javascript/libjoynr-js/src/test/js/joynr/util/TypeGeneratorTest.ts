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

import EnumInsideTypeCollectionWithoutVersion from "../../../generated/joynr/types/TestTypesWithoutVersion/EnumInsideTypeCollectionWithoutVersion";
import MapInsideTypeCollectionWithoutVersion from "../../../generated/joynr/types/TestTypesWithoutVersion/MapInsideTypeCollectionWithoutVersion";
import StructInsideTypeCollectionWithoutVersion from "../../../generated/joynr/types/TestTypesWithoutVersion/StructInsideTypeCollectionWithoutVersion";
import TEnum from "../../../generated/joynr/types/TestTypes/TEnum";
import TStringKeyMap from "../../../generated/joynr/types/TestTypes/TStringKeyMap";
import TStruct from "../../../generated/joynr/types/TestTypes/TStruct";
import TStructWithTypedefMembers from "../../../generated/joynr/types/TestTypes/TStructWithTypedefMembers";
import * as Typing from "../../../../main/js/joynr/util/Typing";

describe("libjoynr-js.joynr.TypeGenerator.Enum", () => {
    it("type collection enum default version is set correctly", () => {
        expect(EnumInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toBeDefined();
        expect(EnumInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toEqual(0);
        expect(EnumInsideTypeCollectionWithoutVersion.MINOR_VERSION).toBeDefined();
        expect(EnumInsideTypeCollectionWithoutVersion.MINOR_VERSION).toEqual(0);
    });
    it("type collection enum version is set correctly", () => {
        expect(TEnum.MAJOR_VERSION).toBeDefined();
        expect(TEnum.MAJOR_VERSION).toEqual(49);
        expect(TEnum.MINOR_VERSION).toBeDefined();
        expect(TEnum.MINOR_VERSION).toEqual(13);
    });
}); // describe Enum

describe("libjoynr-js.joynr.TypeGenerator.Map", () => {
    it("type collection map default version is set correctly", () => {
        expect(MapInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toBeDefined();
        expect(MapInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toEqual(0);
        expect(MapInsideTypeCollectionWithoutVersion.MINOR_VERSION).toBeDefined();
        expect(MapInsideTypeCollectionWithoutVersion.MINOR_VERSION).toEqual(0);
    });
    it("type collection map version is set correctly", () => {
        expect(TStringKeyMap.MAJOR_VERSION).toBeDefined();
        expect(TStringKeyMap.MAJOR_VERSION).toEqual(49);
        expect(TStringKeyMap.MINOR_VERSION).toBeDefined();
        expect(TStringKeyMap.MINOR_VERSION).toEqual(13);
    });
}); // describe Map

describe("libjoynr-js.joynr.TypeGenerator.Compound", () => {
    let testStructWithTypeDefMembers: any;
    let testStructWithObjectNullMaps: any;

    beforeEach(() => {
        testStructWithTypeDefMembers = new TStructWithTypedefMembers({
            typeDefForPrimitive: 42,
            typeDefForTStruct: new TStruct({
                tDouble: 47.11,
                tInt64: 2323,
                tString: "testString"
            }),
            typeDefForTStringKeyMap: new TStringKeyMap({
                key1: "value1",
                key2: "value2"
            }),
            typeDefForTEnum: TEnum.TLITERALA,
            arrayOfTypeDefForPrimitive: [],
            arrayOfTypeDefForTStruct: [],
            arrayOfTypeDefForTStringKeyMap: [],
            arrayOfTypeDefForTEnum: []
        });

        const testObjectNullTStruct = Object.create(null);
        testObjectNullTStruct["tDouble"] = 47.11;
        testObjectNullTStruct["tInt64"] = 2323;
        testObjectNullTStruct["tString"] = "testString";

        const testObjectNullStringKeyMap = Object.create(null);
        testObjectNullStringKeyMap["key1"] = "value1";
        testObjectNullStringKeyMap["key2"] = "value2";

        testStructWithObjectNullMaps = new TStructWithTypedefMembers({
            typeDefForPrimitive: 42,
            typeDefForTStruct: new TStruct(testObjectNullTStruct),
            typeDefForTStringKeyMap: new TStringKeyMap(testObjectNullStringKeyMap),
            typeDefForTEnum: TEnum.TLITERALA,
            arrayOfTypeDefForPrimitive: [],
            arrayOfTypeDefForTStruct: [],
            arrayOfTypeDefForTStringKeyMap: [],
            arrayOfTypeDefForTEnum: []
        });
    });

    it("type collection struct default version is set correctly", () => {
        expect(StructInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toBeDefined();
        expect(StructInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toEqual(0);
        expect(StructInsideTypeCollectionWithoutVersion.MINOR_VERSION).toBeDefined();
        expect(StructInsideTypeCollectionWithoutVersion.MINOR_VERSION).toEqual(0);
    });
    it("type collection struct version is set correctly", () => {
        expect(TStruct.MAJOR_VERSION).toBeDefined();
        expect(TStruct.MAJOR_VERSION).toEqual(49);
        expect(TStruct.MINOR_VERSION).toBeDefined();
        expect(TStruct.MINOR_VERSION).toEqual(13);
    });

    it("StructWithTypedefMembers: checkMembers accepts correct types", () => {
        expect(() =>
            TStructWithTypedefMembers.checkMembers(testStructWithTypeDefMembers, Typing.checkPropertyIfDefined)
        ).not.toThrow();
    });

    it("StructWithTypedefMembers: checkMembers detects wrong types", () => {
        let testStruct: any;

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.typeDefForPrimitive = "string";
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForPrimitive is not of type Number. Actual type is String"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.typeDefForTStruct = "string";
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForTStruct is not of type TStruct. Actual type is String"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.typeDefForTStringKeyMap = "string";
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForTStringKeyMap is not of type TStringKeyMap. Actual type is String"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.typeDefForTEnum = 44;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForTEnum is not of type TEnum. Actual type is Number"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.arrayOfTypeDefForPrimitive = testStruct.typeDefForPrimitive;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForPrimitive is not of type Array. Actual type is Number"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.arrayOfTypeDefForTStruct = testStruct.typeDefForTStruct;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForTStruct is not of type Array. Actual type is TStruct"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.arrayOfTypeDefForTStringKeyMap = testStruct.typeDefForTStringKeyMap;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForTStringKeyMap is not of type Array. Actual type is TStringKeyMap"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithTypeDefMembers);
        testStruct.arrayOfTypeDefForTEnum = testStruct.typeDefForTEnum;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForTEnum is not of type Array. Actual type is TEnum"
        );
    });

    it("StructWithTypedefMembers with ObjectNullMaps: checkMembers detects wrong types", () => {
        let testStruct: any;

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.typeDefForPrimitive = "string";
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForPrimitive is not of type Number. Actual type is String"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.typeDefForTStruct = "string";
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForTStruct is not of type TStruct. Actual type is String"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.typeDefForTStringKeyMap = "string";
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForTStringKeyMap is not of type TStringKeyMap. Actual type is String"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.typeDefForTEnum = 44;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.typeDefForTEnum is not of type TEnum. Actual type is Number"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.arrayOfTypeDefForPrimitive = testStruct.typeDefForPrimitive;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForPrimitive is not of type Array. Actual type is Number"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.arrayOfTypeDefForTStruct = testStruct.typeDefForTStruct;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForTStruct is not of type Array. Actual type is TStruct"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.arrayOfTypeDefForTStringKeyMap = testStruct.typeDefForTStringKeyMap;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForTStringKeyMap is not of type Array. Actual type is TStringKeyMap"
        );

        testStruct = new TStructWithTypedefMembers(testStructWithObjectNullMaps);
        testStruct.arrayOfTypeDefForTEnum = testStruct.typeDefForTEnum;
        expect(() => TStructWithTypedefMembers.checkMembers(testStruct, Typing.checkPropertyIfDefined)).toThrow(
            "members.arrayOfTypeDefForTEnum is not of type Array. Actual type is TEnum"
        );
    });
});
