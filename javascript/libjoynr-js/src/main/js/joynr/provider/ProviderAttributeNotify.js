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
var ProviderAttribute = require('./ProviderAttribute');
module.exports =
        (function(ProviderAttribute) {

            /**
             * Constructor of ProviderAttribute* object that is used in the generation of provider objects
             *
             * @constructor
             * @name ProviderAttributeNotify
             *
             * @param {Object} parent is the provider object that contains this attribute
             * @param {Object}
             *            [implementation] the definition of attribute implementation
             * @param {Function}
             *            [implementation.set] the getter function with the signature "function(value){}" that stores the given
             *            attribute value
             * @param {Function}
             *            [implementation.get] the getter function with the signature "function(){}" that returns the current attribute
             *            value
             * @param {String} attributeName the name of the attribute
             * @param {String} attributeType the type of the attribute
             */
            function ProviderAttributeNotify(parent, implementation, attributeName, attributeType) {
                if (!(this instanceof ProviderAttributeNotify)) {
                    // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                    return new ProviderAttributeNotify(
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
                                "NOTIFY");

                /**
                 * If this attribute is changed the application should call this function with the
                 * new value, whereafter the new value gets published
                 *
                 * @name ProviderAttributeNotify#valueChanged
                 * @function
                 *
                 * @param {?}
                 *            value the new value of the attribute
                 * @see ProviderAttributeNotify#registerObserver
                 * @see ProviderAttributeNotify#unregisterObserver
                 */
                this.valueChanged = function valueChanged(value) {
                    providerAttribute.valueChanged(value);
                };

                /**
                 * Registers an Observer for value changes
                 *
                 * @name ProviderAttributeNotify#registerObserver
                 * @function
                 *
                 * @param {Function}
                 *            observer the callback function with the signature "function(value){..}"
                 * @see ProviderAttributeNotify#valueChanged
                 * @see ProviderAttributeNotify#unregisterObserver
                 */
                this.registerObserver = function registerObserver(observer) {
                    providerAttribute.registerObserver(observer);
                };

                /**
                 * Unregisters an Observer for value changes
                 *
                 * @name ProviderAttributeNotify#unregisterObserver
                 * @function
                 *
                 * @param {Function}
                 *            observer the callback function with the signature "function(value){..}"
                 * @see ProviderAttributeNotify#valueChanged
                 * @see ProviderAttributeNotify#registerObserver
                 */
                this.unregisterObserver = function unregisterObserver(observer) {
                    providerAttribute.unregisterObserver(observer);
                };

                /**
                 * See [ProviderAttribute.checkGet]{@link ProviderAttribute#checkGet}
                 *
                 * @function ProviderAttributeNotifyRead#check
                 *
                 * @returns {Boolean}
                 */
                this.check = function check() {
                    return providerAttribute.checkGet();
                };

                return Object.freeze(this);
            }

            return ProviderAttributeNotify;

        }(ProviderAttribute));