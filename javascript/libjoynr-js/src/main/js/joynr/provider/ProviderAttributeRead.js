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

define(
        "joynr/provider/ProviderAttributeRead",
        [ "joynr/provider/ProviderAttribute"
        ],
        function(ProviderAttribute) {

            /**
             * Constructor of ProviderAttribute* object that is used in the generation of provider
             * objects
             *
             * @constructor
             * @name ProviderAttributeRead
             *
             * @param {Object} parent is the provider object that contains this attribute
             * @param {Object}
             *            [implementation] the definition of attribute implementation
             * @param {Function}
             *            [implementation.set] the getter function with the signature
             *            "function(value){}" that stores the given attribute value
             * @param {Function}
             *            [implementation.get] the getter function with the signature "function(){}"
             *            that returns the current attribute value
             * @param {String} attributeName the name of the attribute
             * @param {String} attributeType the type of the attribute
             */
            function ProviderAttributeRead(parent, implementation, attributeName, attributeType) {
                if (!(this instanceof ProviderAttributeRead)) {
                    // in case someone calls constructor without new keyword
                    // (e.g. var c = Constructor({..}))
                    return new ProviderAttributeRead(
                            parent,
                            implementation,
                            attributeName,
                            attributeType);
                }

                var providerAttribute =
                        new ProviderAttribute(
                                parent,
                                implementation,
                                attributeName,
                                attributeType,
                                "READ");

                /**
                 * Registers the getter function for this attribute
                 *
                 * @name ProviderAttributeRead#registerGetter
                 * @function
                 *
                 * @param {Function} getterFunc the getter function with the signature
                 *            '{?} getterFunc() {..}'
                 */
                this.registerGetter = function registerGetter(getterFunc) {
                    return providerAttribute.registerGetter(getterFunc);
                };

                /**
                 * Calls through the getter registered with registerGetter
                 *
                 * @name ProviderAttributeRead#get
                 * @function
                 *
                 * @returns {?} the attribute value
                 *
                 * @see ProviderAttributeRead#registerGetter
                 */
                this.get = function get() {
                    return providerAttribute.get();
                };

                /**
                 * See [ProviderAttribute.checkGet]{@link ProviderAttribute#checkGet}
                 *
                 * @function ProviderAttributeRead#check
                 *
                 * @returns {Boolean}
                 */
                this.check = function check() {
                    return providerAttribute.checkGet();
                };

                return Object.freeze(this);
            }

            return ProviderAttributeRead;

        });