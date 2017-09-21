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
     * @name ProviderAttributeReadWrite
     *
     * @param {Object} parent is the provider object that contains this attribute
     * @param {Object}
     *            [implementation] the definition of attribute implementation
     * @param {Function}
     *            [implementation.set] the getter function with the signature "function(value){}"
     *            that stores the given attribute value
     * @param {Function}
     *            [implementation.get] the getter function with the signature "function(){}" that
     *            returns the current attribute value
     * @param {String} attributeName the name of the attribute
     * @param {String} attributeType the type of the attribute
     */
    function ProviderAttributeReadWrite(parent, implementation, attributeName, attributeType) {
        if (!(this instanceof ProviderAttributeReadWrite)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new ProviderAttributeReadWrite(
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
                        "READWRITE");

        /**
         * Registers the getter function for this attribute
         *
         * @name ProviderAttributeReadWrite#registerGetter
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
         * @name ProviderAttributeReadWrite#get
         * @function
         *
         * @returns {?} the attribute value
         *
         * @see ProviderAttributeReadWrite#registerGetter
         */
        this.get = function get() {
            return providerAttribute.get();
        };

        /**
         * Registers the setter function for this attribute
         *
         * @name ProviderAttributeReadWrite#registerSetter
         * @function
         *
         * @param {Function} setterFunc the setter function with the signature
         *            'void setterFunc({?}value) {..}'
         */
        this.registerSetter = function registerSetter(setterFunc) {
            return providerAttribute.registerSetter(setterFunc);
        };

        /**
         * Calls through the setter registered with registerSetter with the same arguments as this
         * function
         *
         * @name ProviderAttributeReadWrite#set
         * @function
         *
         * @param {?}
         *            value the new value of the attribute
         *
         * @see ProviderAttributeReadWrite#registerSetter
         */
        this.set = function set(value) {
            return providerAttribute.set(value);
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

    return ProviderAttributeReadWrite;

        }(ProviderAttribute));