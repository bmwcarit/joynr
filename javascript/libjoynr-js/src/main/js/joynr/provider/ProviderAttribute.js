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
const UtilInternal = require("../util/UtilInternal");
const Typing = require("../util/Typing");
const ProviderRuntimeException = require("../exceptions/ProviderRuntimeException");

// prettier-ignore
const asNotify = (function() {
    /**
     * If this attribute is changed the application should call this function with the new value,
     * whereafter the new value gets published
     *
     * @name ProviderAttribute#valueChanged
     * @function
     *
     * @param {?}
     *      value - the new value of the attribute
     *
     * @see ProviderAttribute#registerObserver
     * @see ProviderAttribute#unregisterObserver
     */
    function valueChanged(value) {
        UtilInternal.fire(this.callbacks, [value]);
    }

    /**
     * Registers an Observer for value changes
     *
     * @name ProviderAttribute#registerObserver
     * @function
     *
     * @param {Function}
     *      observer - the callback function with the signature "function(value){..}"
     *
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
     *
     * @param {Function}
     *      observer - the callback function with the signature "function(value){..}"
     *
     * @see ProviderAttribute#valueChanged
     * @see ProviderAttribute#registerObserver
     */
    function unregisterObserver(observer) {
        UtilInternal.removeElementFromArray(this.callbacks, observer);
    }

    return function() {
        // since ValueChanged is copied to the implementation bind is necessary here. (or in the generated code)
        this.valueChanged = valueChanged.bind(this);
        this.registerObserver = registerObserver;
        this.unregisterObserver = unregisterObserver;
        this.callbacks = [];
    };
}());

// prettier-ignore
const asWrite = (function() {
    /**
     * Registers the setter function for this attribute
     *
     * @name ProviderAttribute#registerSetter
     * @function
     *
     * @param {Function}
     *      setterFunc - the setter function with the signature
     *          'void setterFunc({?}value) {..}'
     * @returns {ProviderAttribute} fluent interface to call multiple methods
     */
    function registerSetter(setterFunc) {
        this.privateSetterFunc = setterFunc;
        return this;
    }

    /**
     * Calls through the setter registered with registerSetter with the same arguments as this
     * function
     *
     * @name ProviderAttribute#set
     * @function
     *
     * @param {?}
     *      value - the new value of the attribute
     *
     * @throws {Error} if no setter function was registered before calling it
     *
     * @see ProviderAttribute#registerSetter
     */
    async function set(value) {
        if (!this.privateSetterFunc) {
            throw new Error("no setter function registered for provider attribute");
        }

        const setterParams = Typing.augmentTypes(value, this.attributeType);
        const originalValue = await Promise.resolve(this.privateGetterFunc());
        await this.privateSetterFunc(setterParams);

        if (originalValue !== value && this.valueChanged instanceof Function) {
            this.valueChanged(value);
        }
        return [];
    }

    return function() {
        this.set = set;
        this.registerSetter = registerSetter;
    };
}());

function toArray(returnValue) {
    return [returnValue];
}

// prettier-ignore
const asRead = (function() {
    /**
     * Registers the getter function for this attribute
     *
     * @name ProviderAttribute#registerGetter
     * @function
     *
     * @param {Function}
     *      getterFunc - the getter function with the signature '{?} getterFunc() {..}'
     * @returns {ProviderAttribute} fluent interface to call multiple methods
     */
    function registerGetter(getterFunc) {
        this.privateGetterFunc = function(){
            return Promise.resolve().then(getterFunc);
        };
    }

    function curryCreateError(context){
        return function createError(error){
            if (error instanceof ProviderRuntimeException) {
                throw error;
            }
            throw new ProviderRuntimeException({
                detailMessage: `getter method for attribute ${  context.attributeName  } reported an error`
            });
        };
    }

    /**
     * Calls through the getter registered with registerGetter with the same arguments as this
     * function
     *
     * @name ProviderAttribute#get
     * @function
     *
     * @returns {?} a Promise which resolves the attribute value
     *
     * rejects {Error} if no getter function was registered before calling it
     * rejects {Error} if registered getter returns a compound type with incorrect values
     *
     * @see ProviderAttribute#registerGetter
     */
    function get() {
        try{
            if (!this.privateGetterFunc) {
                return Promise.reject(new Error(`no getter function registered for provider attribute: ${  this.attributeName}`));
            }
            return Promise.resolve(this.privateGetterFunc()).then(toArray).catch(this._createError);

        } catch (e){
            return Promise.reject(e);
        }
    }

    return function() {
        this._createError = curryCreateError(this);
        this.get = get;
        this.registerGetter = registerGetter;
    };
}());

// prettier-ignore
const asReadOrWrite = (function() {
    /**
     * Check Getter and Setter functions.
     *
     * @name ProviderAttribute#check
     * @function
     *
     * @returns {Boolean}
     */
    function check() {
        return (
            (!this.hasRead || typeof this.privateGetterFunc === "function") &&
            (!this.hasWrite || typeof this.privateSetterFunc === "function")
        );
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
 *      parent - the provider object that contains this attribute
 * @param {Object}
 *      [implementation] - the definition of attribute implementation
 * @param {Function}
 *      [implementation.set] - the getter function with the signature "function(value){}" that
 *          stores the given attribute value
 * @param {Function}
 *      [implementation.get] the getter function with the signature "function(){}" that returns the
 *          current attribute value
 * @param {String}
 *      attributeName - the name of the attribute
 * @param {String}
 *      attributeType - the type of the attribute
 * @param {String}
 *      attributeCaps - the capabilities of the attribute:
 *          [NOTIFY][READWRITE|READONLY|WRITEONLY], e.g. NOTIFYREADWRITE, if the string contains
 *          'NOTIFY' this attribute receives the valueChanged functions, if the string contains
 *          'READ' or 'WRITE' this attribute receives the registration functions for getter and
 *          setters respectively
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

    // place these functions after the forwarding we don't want them public
    if (implementation && typeof implementation.get === "function") {
        this.privateGetterFunc = implementation.get;
    }
    if (implementation && typeof implementation.set === "function") {
        this.privateSetterFunc = implementation.set;
    }
}

ProviderAttribute.prototype.isNotifiable = function() {
    return this.hasNotify;
};

module.exports = ProviderAttribute;
