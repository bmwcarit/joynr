/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
        "joynr/provider/ProviderAttribute",
        [ "joynr/util/UtilInternal"
        ],
        function(Util) {

            /**
             * Constructor of ProviderAttribute object that is used in the generation of provider attributes
             *
             * @name ProviderAttribute
             * @constructor
             *
             * @param {Provider}
             *            parent is the provider object that contains this attribute
             * @param {Object}
             *            [implementation] the definition of attribute implementation
             * @param {Function}
             *            [implementation.set] the getter function with the signature "function(value){}" that stores the given
             *            attribute value
             * @param {Function}
             *            [implementation.get] the getter function with the signature "function(){}" that returns the current attribute
             *            value
             * @param {String}
             *            attributeName the name of the attribute
             * @param {String}
             *            attributeType the type of the attribute
             * @param {String}
             *            attributeCaps the capabilities of the attribute: [NOTIFY][READWRITE|READONLY|WRITEONLY], e.g. NOTIFYREADWRITE, if the
             *            string contains 'NOTIFY' this attribute receives the valueChanged functions, if the string contains 'READ' or 'WRITE' this
             *            attribute receives the registration functions for getter and setters respectively
             */
            function ProviderAttribute(
                    parent,
                    implementation,
                    attributeName,
                    attributeType,
                    attributeCaps) {
                // a function from the publication manager to be called when the attribute value changes
                if (!(this instanceof ProviderAttribute)) {
                    // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                    return new ProviderAttribute(
                            parent,
                            implementation,
                            attributeName,
                            attributeType,
                            attributeCaps);
                }

                // TODO: should we do this more strictly, like attributeCaps.match('^(NOTIFY)?(READWRITE|((READ|WRITE)ONLY))?$') ?
                var privateGetterFunc = (implementation ? implementation.get : undefined);
                if (attributeCaps.match(/READ/)) {
                    /**
                     * Registers the getter function for this attribute
                     *
                     * @name ProviderAttribute#registerGetter
                     * @function
                     *
                     * @param {Function}
                     *            getterFunc the getter function with the signature '{?} getterFunc() {..}'
                     * @returns {ProviderAttribute} fluent interface to call multiple methods
                     */
                    this.registerGetter = function registerGetter(getterFunc) {
                        privateGetterFunc = getterFunc;
                        return this;
                    };

                    /**
                     * Calls through the getter registered with registerGetter with the same arguments as this function
                     *
                     * @name ProviderAttribute#get
                     * @function
                     *
                     * @returns {?} the attribute value
                     *
                     * @throws {Error} if no getter function was registered before calling it
                     * @throws {Error} if registered getter returns a compound type with incorrect values
                     * @see ProviderAttribute#registerGetter
                     */
                    this.get =
                            function get() {
                                var value;
                                if (!privateGetterFunc) {
                                    throw new Error(
                                            "no getter function registered for provider attribute");
                                }
                                // call getter function with the same arguments as this function
                                value = privateGetterFunc();

                                return value;
                            };
                }
                var privateSetterFunc = (implementation ? implementation.set : undefined);
                if (attributeCaps.match(/WRITE/)) {

                    /**
                     * Registers the setter function for this attribute
                     *
                     * @name ProviderAttribute#registerSetter
                     * @function
                     *
                     * @param {Function}
                     *            setterFunc the setter function with the signature 'void setterFunc({?}value) {..}'
                     * @returns {ProviderAttribute} fluent interface to call multiple methods
                     */
                    this.registerSetter = function registerSetter(setterFunc) {
                        privateSetterFunc = setterFunc;
                        return this;
                    };

                    /**
                     * Calls through the setter registered with registerSetter with the same arguments as this function
                     *
                     * @name ProviderAttribute#set
                     * @function
                     *
                     * @param {?}
                     *            value the new value of the attribute
                     *
                     * @throws {Error} if no setter function was registered before calling it
                     * @see ProviderAttribute#registerSetter
                     */
                    this.set =
                            function set(value) {
                                if (!privateSetterFunc) {
                                    throw new Error(
                                            "no setter function registered for provider attribute");
                                }
                                var oldValue = privateGetterFunc();
                                // call setter function with the same arguments as this function
                                privateSetterFunc(value);
                                var newValue = privateGetterFunc();
                                if (newValue !== oldValue) {
                                    if (this.valueChanged !== undefined) {
                                        this.valueChanged(newValue);
                                    }
                                }
                            };
                }
                if (attributeCaps.match(/NOTIFY/)) {
                    var callbacks = [];

                    /**
                     * If this attribute is changed the application should call this function with the new value, whereafter the new value gets
                     * published
                     *
                     * @name ProviderAttribute#valueChanged
                     * @function
                     *
                     * @param {?}
                     *            value the new value of the attribute
                     * @see ProviderAttribute#registerObserver
                     * @see ProviderAttribute#unregisterObserver
                     */
                    this.valueChanged = function valueChanged(value) {
                        Util.fire(callbacks, value);
                    };

                    /**
                     * Registers an Observer for value changes
                     *
                     * @name ProviderAttribute#registerObserver
                     * @function
                     * @param {Function}
                     *            observer the callback function with the signature "function(value){..}"
                     * @see ProviderAttribute#valueChanged
                     * @see ProviderAttribute#unregisterObserver
                     */
                    this.registerObserver = function registerObserver(observer) {
                        callbacks.push(observer);
                    };

                    /**
                     * Unregisters an Observer for value changes
                     *
                     * @name ProviderAttribute#unregisterObserver
                     * @function
                     * @param {Function}
                     *            observer the callback function with the signature "function(value){..}"
                     * @see ProviderAttribute#valueChanged
                     * @see ProviderAttribute#registerObserver
                     */
                    this.unregisterObserver = function unregisterObserver(observer) {
                        Util.removeElementFromArray(callbacks, observer);
                    };

                    /**
                     * Check if setter function is defined.
                     * @function ProviderAttribute#checkSet
                     *
                     * @returns {Boolean}
                     */
                    this.checkSet = function checkSet() {
                        return typeof privateSetterFunc === "function";
                    };

                    /**
                     * Check if getter function is defined.
                     * @function ProviderAttribute#checkGet
                     *
                     * @returns {Boolean}
                     */
                    this.checkGet = function checkGet() {
                        return typeof privateGetterFunc === "function";
                    };
                }

            }

            return ProviderAttribute;

        });
