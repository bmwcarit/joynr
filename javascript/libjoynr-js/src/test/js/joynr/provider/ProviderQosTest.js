/*jslint node: true */

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
var ProviderQos = require('../../../classes/joynr/types/ProviderQos');
var ProviderScope = require('../../../classes/joynr/types/ProviderScope');
var CustomParameter = require('../../../classes/joynr/types/CustomParameter');

describe("libjoynr-js.joynr.types.ProviderQos", function() {
    it("is instantiable", function() {
        expect(new ProviderQos({
            customParameters : [ new CustomParameter({
                name : "theName",
                value : "theValue"
            })
            ],
            priority : 1234,
            scope : ProviderScope.LOCAL,
            supportsOnChangeSubscriptions : true
        })).toBeDefined();
    });

    it("is of correct type", function() {
        var providerQos = new ProviderQos({
            customParameters : [ new CustomParameter({
                name : "theName",
                value : "theValue"
            })
            ],
            priority : 1234,
            scope : ProviderScope.LOCAL,
            supportsOnChangeSubscriptions : true
        });
        expect(providerQos).toBeDefined();
        expect(providerQos).not.toBeNull();
        expect(typeof providerQos === "object").toBeTruthy();
        expect(providerQos instanceof ProviderQos).toEqual(true);
    });

    function testValues(customParameters, version, priority, scope, supportsOnChangeSubscriptions) {
        var providerQos = new ProviderQos({
            customParameters : customParameters,
            priority : priority,
            scope : scope,
            supportsOnChangeSubscriptions : supportsOnChangeSubscriptions
        });
        expect(providerQos.customParameters).toEqual(customParameters);
        expect(providerQos.priority).toEqual(priority);
        expect(providerQos.scope).toEqual(scope);
        expect(providerQos.supportsOnChangeSubscriptions).toEqual(supportsOnChangeSubscriptions);
    }

    /*
     * This test has been disabled, as
     it("throws exceptions upon missing or wrongly typed arguments", function () {
     // settings is undefined
     expect(function () {
     var providerQos = new ProviderQos();
     }).toThrow();
     // version is missing => exception
     expect(function () {
     var providerQos = new ProviderQos([new CustomParameter("theName","theValue")],1234, false, true);
     }).toThrow();
     // priority is missing => exception
     expect(function () {
     var providerQos = new ProviderQos([new CustomParameter("theName","theValue")],123, false, true);
     }).toThrow();
     // qos is missing => OK
     expect(function () {
     var providerQos = new ProviderQos([],123,1234,false,true);
     }).not.toThrow();
     // isLocalOnly  is missing => OK
     expect(function () {
     var providerQos = new ProviderQos([new CustomParameter("theName","theValue")],123,1234,true);
     }).not.toThrow();
     // all parameters
     expect(function () {
     var providerQos = new ProviderQos([new CustomParameter("theName","theValue")],123,1234,false,true);
     }).not.toThrow();
     });
     */

    it("constructs with correct member values", function() {
        testValues([], 0, 0, ProviderScope.LOCAL);
        testValues([
            new CustomParameter({
                name : "theName",
                value : "theValue"
            }),
            new CustomParameter({
                name : "anotherName",
                value : "anotherValue"
            })
        ], 123, 1234, ProviderScope.GLOBAL);
    });
});
