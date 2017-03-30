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
        "joynr/provider/ProviderAttributeNotifyReadWrite",
        [ "joynr/provider/ProviderAttribute"
        ],
        function(ProviderAttribute) {

            /**
             * Constructor of ProviderAttribute* object that is used in the generation of provider
             * objects
             *
             * @constructor
             * @name ProviderAttributeNotifyReadWrite
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
            function ProviderAttributeNotifyReadWrite(
                    parent,
                    implementation,
                    attributeName,
                    attributeType) {
                if (!(this instanceof ProviderAttributeNotifyReadWrite)) {
                    // in case someone calls constructor without new keyword
                    // (e.g. var c = Constructor({..}))
                    return new ProviderAttributeNotifyReadWrite(
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
                                "NOTIFYREADWRITE");

                /**
                 * Registers the getter function for this attribute
                 *
                 * @name ProviderAttributeNotifyReadWrite#registerGetter
                 * @function
                 *
                 * @param {Function} getterFunc the getter function with the
                 *            signature '{?} getterFunc() {..}'
                 */
                this.registerGetter = function registerGetter(getterFunc) {
                    return providerAttribute.registerGetter(getterFunc);
                };

                /**
                 * Calls through the Getter registered with registerGetter
                 *
                 * @name ProviderAttributeNotifyReadWrite#get
                 * @function
                 *
                 * @returns {?} the attribute value
                 *
                 * @see ProviderAttributeNotifyReadWrite#registerGetter
                 */
                this.get = function get() {
                    return providerAttribute.get();
                };

                /**
                 * Registers the setter function for this attribute
                 *
                 * @name ProviderAttributeNotifyReadWrite#registerSetter
                 * @function
                 *
                 * @param {Function} setterFunc the setter function with the
                 *            signature'void setterFunc({?}value) {..}'
                 */
                this.registerSetter = function registerSetter(setterFunc) {
                    return providerAttribute.registerSetter(setterFunc);
                };

                /**
                 * Calls through the setter registered with registerSetter with the same arguments
                 * as this function
                 *
                 * @name ProviderAttributeNotifyReadWrite#set
                 * @function
                 *
                 * @param {?}
                 *            value the new value of the attribute
                 *
                 * @see ProviderAttributeNotifyReadWrite#registerSetter
                 */
                this.set = function set(value) {
                    return providerAttribute.set(value);
                };

                /**
                 * If this attribute is changed the application must call this function with the
                 * new value, which causes a publication with the new value to be sent to all
                 * subscribers
                 *
                 * @name ProviderAttributeNotifyReadWrite#valueChanged
                 * @function
                 *
                 * @param {?}
                 *            value the new value of the attribute
                 * @see ProviderAttributeNotifyReadWrite#registerObserver
                 * @see ProviderAttributeNotifyReadWrite#unregisterObserver
                 */
                this.valueChanged = function valueChanged(value) {
                    providerAttribute.valueChanged(value);
                };

                /**
                 * Registers an Observer for value changes
                 *
                 * @name ProviderAttributeNotifyReadWrite#registerObserver
                 * @function
                 *
                 * @param {Function}
                 *            observer the callback function with the signature "function(value){..}"
                 * @see ProviderAttributeNotifyReadWrite#valueChanged
                 * @see ProviderAttributeNotifyReadWrite#unregisterObserver
                 */
                this.registerObserver = function registerObserver(observer) {
                    providerAttribute.registerObserver(observer);
                };

                /**
                 * Unregisters an Observer for value changes
                 *
                 * @name ProviderAttributeNotifyReadWrite#unregisterObserver
                 * @function
                 *
                 * @param {Function}
                 *            observer the callback function with the signature "function(value){..}"
                 * @see ProviderAttributeNotifyReadWrite#valueChanged
                 * @see ProviderAttributeNotifyReadWrite#registerObserver
                 */
                this.unregisterObserver = function unregisterObserver(observer) {
                    providerAttribute.unregisterObserver(observer);
                };

                /**
                 * Check Getter and Setter functions.
                 * See [ProviderAttribute.checkGet]{@link ProviderAttribute#checkGet}
                 * and [ProviderAttribute.checkSet]{@link ProviderAttribute#checkSet}
                 *
                 * @function ProviderAttributeNotifyReadWrite#check
                 *
                 * @returns {Boolean}
                 */
                this.check = function check() {
                    return providerAttribute.checkGet() && providerAttribute.checkSet();
                };

                return Object.freeze(this);
            }

            return ProviderAttributeNotifyReadWrite;

        });