/*jslint es5: true, node: true, node: true */
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
var Promise = require("../../global/Promise");
var Util = require("../util/UtilInternal");
var Typing = require("../util/Typing");
var TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");

var typeRegistry = TypeRegistrySingleton.getInstance();

// prettier-ignore
var asNotify = (function() {

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
    function valueChanged(value) {
        Util.fire(this.callbacks, [ value
        ]);
    }

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
    function registerObserver(observer) {
        this.callbacks.push(observer);
    }

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
    function unregisterObserver(observer) {
        Util.removeElementFromArray(this.callbacks, observer);
    }

    return function() {
        this.valueChanged = valueChanged;
        this.registerObserver = registerObserver;
        this.unregisterObserver = unregisterObserver;
        this.callbacks = [];
    };

}());

// prettier-ignore
var asWrite =
        (function() {

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
            function registerSetter(setterFunc) {
                this.privateSetterFunc = setterFunc;
                return this;
            }

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
            function set(value) {
                var originalValue;
                var that = this;
                if (!this.privateSetterFunc) {
                    throw new Error("no setter function registered for provider attribute");
                }
                return Promise.resolve(this.privateGetterFunc()).then(
                        function(getterValue) {
                            originalValue = getterValue;
                            return this.privateSetterFunc(Typing.augmentTypes(
                                    value,
                                    typeRegistry,
                                    this.attributeType));

                        }.bind(this)).then(function() {
                    if (originalValue !== value && that.valueChanged instanceof Function) {
                        that.valueChanged(value);
                    }
                    return [];
                });
            }

            return function() {
                this.set = set;
                this.registerSetter = registerSetter;
            };

        }());

// prettier-ignore
var asRead = (function() {

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
    function registerGetter(getterFunc) {
        this.privateGetterFunc = getterFunc;
    }

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
    function get() {
        var value;
        if (!this.privateGetterFunc) {
            throw new Error("no getter function registered for provider attribute");
        }
        // call getter function with the same arguments as this function
        value = this.privateGetterFunc();

        function toArray(returnValue) {
            return [ returnValue
            ];
        }

        if (Util.isPromise(value)) {
            return value.then(toArray);
        }
        return [ value
        ];
    }

    return function() {
        this.get = get;
        this.registerGetter = registerGetter;
    };
}());

// prettier-ignore
var asReadOrWrite =
        (function() {

            /**
             * Check Getter and Setter functions.
             *
             * @function ProviderAttributeNotifyReadWrite#check
             *
             * @returns {Boolean}
             */
            function check() {
                return (!this.hasRead || (typeof this.privateGetterFunc === "function"))
                    && (!this.hasWrite || (typeof this.privateSetterFunc === "function"));
            }

            return function() {
                this.check = check;
            };

        }());

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
function ProviderAttribute(parent, implementation, attributeName, attributeType, attributeCaps) {
    // a function from the publication manager to be called when the attribute value changes
    if (!(this instanceof ProviderAttribute)) {
        // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
        return new ProviderAttribute(parent, implementation, attributeName, attributeType, attributeCaps);
    }

    this.parent = parent;
    this.implementation = implementation;
    this.attributeName = attributeName;
    this.attributeType = attributeType;

    this.hasNotify = false;

    if (attributeCaps.match(/READ/)) {
        this.hasRead = true;
        asRead.call(this);
    }
    if (attributeCaps.match(/WRITE/)) {
        this.hasWrite = true;
        asWrite.call(this);
    }
    if (attributeCaps.match(/NOTIFY/)) {
        this.hasNotify = true;
        asNotify.call(this);
    }

    if (this.hasRead || this.hasWrite) {
        asReadOrWrite.call(this);
    }

    var publicProviderAttribute = Util.forward({}, this);
    publicProviderAttribute.isNotifiable = this.isNotifiable.bind(this);

    // place these functions after the forwarding we don't want them public
    this.privateGetterFunc = implementation ? implementation.get : undefined;
    this.privateSetterFunc = implementation ? implementation.set : undefined;

    return Object.freeze(publicProviderAttribute);
}

ProviderAttribute.prototype.isNotifiable = function() {
    return this.hasNotify;
};

module.exports = ProviderAttribute;
