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
        "joynr/provider/ProviderAttributeWrite",
        [ "joynr/provider/ProviderAttribute"
        ],
        function(ProviderAttribute) {

            /**
             * Constructor of ProviderAttribute* object that is used in the generation of provider objects
             *
             * @constructor
             * @name ProviderAttributeWrite
             *
             * @param {Object} parent is the provider object that contains this attribute
             * @param {Object}
             *            [implementation] the definition of attribute implementation
             * @param {Function}
             *            [implementation.set] the getter function with the signature
             *            "function(value){}" that stores the given attribute value
             * @param {Function}
             *            [implementation.get] the getter function with the signature
             *            "function(){}" that returns the current attribute value
             * @param {String} attributeName the name of the attribute
             * @param {String} attributeType the type of the attribute
             */
            function ProviderAttributeWrite(parent, implementation, attributeName, attributeType) {
                if (!(this instanceof ProviderAttributeWrite)) {
                    // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                    return new ProviderAttributeWrite(
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
                                "WRITE");

                /**
                 * Registers the setter function for this attribute
                 *
                 * @name ProviderAttributeWrite#registerSetter
                 * @function
                 *
                 * @param {Function} setterFunc the setter function with the signature
                 *            'void setterFunc({?}value) {..}'
                 */
                this.registerSetter = function registerSetter(setterFunc) {
                    return providerAttribute.registerSetter(setterFunc);
                };

                /**
                 * Calls through the setter registered with registerSetter with the same arguments
                 * as this function
                 *
                 * @name ProviderAttributeWrite#set
                 * @function
                 *
                 * @param {?}
                 *            value the new value of the attribute
                 *
                 * @see ProviderAttributeWrite#registerSetter
                 */
                this.set = function set(value) {
                    return providerAttribute.set(value);
                };

                /**
                 * See [ProviderAttribute.checkSet]{@link ProviderAttribute#checkSet}
                 *
                 * @function ProviderAttributeWrite#check
                 *
                 * @returns {Boolean}
                 */
                this.check = function check() {
                    return providerAttribute.checkSet();
                };

                return Object.freeze(this);
            }

            return ProviderAttributeWrite;

        });