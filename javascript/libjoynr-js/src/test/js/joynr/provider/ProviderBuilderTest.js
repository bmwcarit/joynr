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
var RadioProvider = require('../../../test-classes/joynr/vehicle/RadioProvider');
var ProviderBuilder = require('../../../classes/joynr/provider/ProviderBuilder');
var ProviderOperation = require('../../../classes/joynr/provider/ProviderOperation');

describe("libjoynr-js.joynr.provider.ProviderBuilder", function() {
    var providerBuilder = null;
    var implementation = null;

    beforeEach(function() {
        providerBuilder = new ProviderBuilder();
        implementation = {
            isOn : {
                value : false,
                get : function() {
                    return this.value;
                },
                set : function(newValue) {
                    this.value = newValue;
                }
            },
            numberOfStations : {
                value : 0,
                get : function() {
                    return this.value;
                },
                set : function(newValue) {
                    this.value = newValue;
                }
            },
            mixedSubscriptions : {
                value : "testvalue",
                get : function() {
                    return this.value;
                },
                set : function(newValue) {
                    this.value = newValue;
                }
            },
            attrProvidedImpl : {
                value : "testValue2",
                get : function() {
                    return this.value;
                },
                set : function(newValue) {
                    this.value = newValue;
                }
            },
            attributeTestingProviderInterface : {
                get : function() {
                    return undefined;
                }
            },
            addFavoriteStation : jasmine.createSpy("addFavoriteStation"),
            weakSignal : jasmine.createSpy("weakSignal"),
            methodProvidedImpl : jasmine.createSpy("methodProvidedImpl")
        };
    });

    it("is defined and of correct type", function() {
        expect(providerBuilder).toBeDefined();
        expect(typeof providerBuilder.build === "function").toBe(true);
    });

    it("returns a provider of the given type", function() {
        var radioProvider = providerBuilder.build(RadioProvider, implementation);
        expect(radioProvider).toBeDefined();
        expect(radioProvider).not.toBeNull();
        expect(radioProvider instanceof RadioProvider).toBeTruthy();
        expect(radioProvider.interfaceName).toBeDefined();
        expect(radioProvider.isOn).toBeDefined();
        expect(radioProvider.addFavoriteStation).toBeDefined();
        expect(radioProvider.addFavoriteStation instanceof ProviderOperation).toBeTruthy();

    });

});
