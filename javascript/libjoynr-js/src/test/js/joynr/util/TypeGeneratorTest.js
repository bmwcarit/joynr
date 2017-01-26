/*jslint newcap: true, nomen: true */

/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define([
    "joynr/types/TestTypesWithoutVersion/EnumInsideTypeCollectionWithoutVersion",
    "joynr/types/TestTypesWithoutVersion/MapInsideTypeCollectionWithoutVersion",
    "joynr/types/TestTypesWithoutVersion/StructInsideTypeCollectionWithoutVersion",
    "joynr/types/TestTypes/TEnum",
    "joynr/types/TestTypes/TStringKeyMap",
    "joynr/types/TestTypes/TStruct"
], function(
        EnumInsideTypeCollectionWithoutVersion,
        MapInsideTypeCollectionWithoutVersion,
        StructInsideTypeCollectionWithoutVersion,
        TEnum,
        TStringKeyMap,
        TStruct) {

    describe("libjoynr-js.joynr.TypeGenerator", function() {
        it("type collection enum default version is set correctly", function() {
            expect(EnumInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toBeDefined();
            expect(EnumInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toEqual(0);
            expect(EnumInsideTypeCollectionWithoutVersion.MINOR_VERSION).toBeDefined();
            expect(EnumInsideTypeCollectionWithoutVersion.MINOR_VERSION).toEqual(0);
        });
        it("type collection enum version is set correctly", function() {
            expect(TEnum.MAJOR_VERSION).toBeDefined();
            expect(TEnum.MAJOR_VERSION).toEqual(49);
            expect(TEnum.MINOR_VERSION).toBeDefined();
            expect(TEnum.MINOR_VERSION).toEqual(13);
        });

        it("type collection map default version is set correctly", function() {
            expect(MapInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toBeDefined();
            expect(MapInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toEqual(0);
            expect(MapInsideTypeCollectionWithoutVersion.MINOR_VERSION).toBeDefined();
            expect(MapInsideTypeCollectionWithoutVersion.MINOR_VERSION).toEqual(0);
        });
        it("type collection map version is set correctly", function() {
            expect(TStringKeyMap.MAJOR_VERSION).toBeDefined();
            expect(TStringKeyMap.MAJOR_VERSION).toEqual(49);
            expect(TStringKeyMap.MINOR_VERSION).toBeDefined();
            expect(TStringKeyMap.MINOR_VERSION).toEqual(13);
        });

        it("type collection struct default version is set correctly", function() {
            expect(StructInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toBeDefined();
            expect(StructInsideTypeCollectionWithoutVersion.MAJOR_VERSION).toEqual(0);
            expect(StructInsideTypeCollectionWithoutVersion.MINOR_VERSION).toBeDefined();
            expect(StructInsideTypeCollectionWithoutVersion.MINOR_VERSION).toEqual(0);
        });
        it("type collection struct version is set correctly", function() {
            expect(TStruct.MAJOR_VERSION).toBeDefined();
            expect(TStruct.MAJOR_VERSION).toEqual(49);
            expect(TStruct.MINOR_VERSION).toBeDefined();
            expect(TStruct.MINOR_VERSION).toEqual(13);
        });
    });

});