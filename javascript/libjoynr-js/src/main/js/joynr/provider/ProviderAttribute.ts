/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import UtilInternal = require("../util/UtilInternal");
import Typing = require("../util/Typing");
import ProviderRuntimeException = require("../exceptions/ProviderRuntimeException");

type ProviderAttributeConstructor<T = ProviderAttribute> = new (...args: any[]) => T;

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function Notifiable<TBase extends ProviderAttributeConstructor, T = unknown>(superclass: TBase) {
    class Notifiable extends superclass {
        public callbacks: ((value: T) => void)[] = [];

        public constructor(...args: any[]) {
            super(...args);
            this.hasNotify = true;
            if (this.implementation) {
                this.implementation.valueChanged = this.valueChanged.bind(this);
            }
        }

        /**
         * If this attribute is changed the application should call this function with the new value,
         * whereafter the new value gets published
         *
         * @param value - the new value of the attribute
         */
        public valueChanged(value: T): void {
            UtilInternal.fire(this.callbacks, [value]);
        }

        /**
         * Registers an Observer for value changes
         *
         * @param observer - the callback function with the signature "function(value){..}"
         *
         * @see ProviderAttribute#valueChanged
         * @see ProviderAttribute#unregisterObserver
         */
        public registerObserver(observer: (value: T) => void): void {
            this.callbacks.push(observer);
        }

        /**
         * Unregisters an Observer for value changes
         *
         * @param observer - the callback function with the signature "function(value){..}"
         *
         * @see ProviderAttribute#valueChanged
         * @see ProviderAttribute#registerObserver
         */
        public unregisterObserver(observer: (value: T) => void): void {
            UtilInternal.removeElementFromArray(this.callbacks, observer);
        }
    }
    return Notifiable;
}

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function Writable<TBase extends ProviderAttributeConstructor, T = unknown>(superclass: TBase) {
    return class Writable extends superclass {
        public constructor(...args: any[]) {
            super(...args);
            this.hasWrite = true;
        }

        /**
         * Registers the setter function for this attribute
         *
         * @param setterFunc - the setter function with the signature
         *          'void setterFunc({?}value) {..}'
         * @returns fluent interface to call multiple methods
         */
        public registerSetter(setterFunc: (value: T) => void): ProviderAttribute {
            this.privateSetterFunc = setterFunc;
            return this;
        }

        /**
         * Calls through the setter registered with registerSetter with the same arguments as this
         * function
         *
         * @param value - the new value of the attribute
         *
         * @throws {Error} if no setter function was registered before calling it
         *
         * @see ProviderAttribute#registerSetter
         */
        public async set(value: T): Promise<[]> {
            if (!this.privateSetterFunc) {
                throw new Error("no setter function registered for provider attribute");
            }
            if (!this.privateGetterFunc) {
                throw new Error("no getter function registered for provider attribute");
            }

            const setterParams = Typing.augmentTypes(value, this.attributeType);
            const originalValue = await this.privateGetterFunc();
            await this.privateSetterFunc(setterParams);

            if (originalValue !== value && this.hasNotify) {
                (this as any).valueChanged(value);
            }
            return [];
        }
    };
}

function toArray<T>(returnValue: T): T[] {
    return [returnValue];
}

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function Readable<TBase extends ProviderAttributeConstructor, T = unknown>(superclass: TBase) {
    return class Readable extends superclass {
        public constructor(...args: any[]) {
            super(...args);
            this.hasRead = true;
        }

        public registerGetter(getterFunc: () => T | Promise<T>): void {
            this.privateGetterFunc = async () => getterFunc();
        }

        /**
         * Calls through the getter registered with registerGetter with the same arguments as this
         * function
         *
         * @returns a Promise which resolves the attribute value
         *
         * rejects {Error} if no getter function was registered before calling it
         * rejects {Error} if registered getter returns a compound type with incorrect values
         */
        public get(): Promise<T[]> {
            try {
                if (!this.privateGetterFunc) {
                    return Promise.reject(
                        new Error(`no getter function registered for provider attribute: ${this.attributeName}`)
                    );
                }
                return Promise.resolve(this.privateGetterFunc())
                    .then(toArray)
                    .catch(error => {
                        if (error instanceof ProviderRuntimeException) {
                            throw error;
                        }
                        throw new ProviderRuntimeException({
                            detailMessage: `getter method for attribute ${this.attributeName} reported an error`
                        });
                    });
            } catch (e) {
                return Promise.reject(e);
            }
        }
    };
}

interface JoynrProvider {
    interfaceName: string;
}

export class ProviderAttribute<T = any> {
    public hasNotify = false;
    public attributeType: string;
    public attributeName: string;
    /** protected property, needs to be public for tsc -d */
    public implementation: {
        get?: (key: string) => any | Promise<any>;
        set?: (value: T) => void | Promise<void>;
        valueChanged?: (value: T) => void;
    };
    public parent: JoynrProvider;
    public hasRead = false;
    public hasWrite = false;
    /** protected property, needs to be public for tsc -d */
    public privateGetterFunc?: Function;
    /** protected property, needs to be public for tsc -d */
    public privateSetterFunc?: (value: T) => void | Promise<void>;

    /**
     * Constructor of ProviderAttribute object that is used in the generation of provider attributes
     *
     * @constructor
     *
     * @param parent - the provider object that contains this attribute
     * @param [implementation] - the definition of attribute implementation
     * @param {Function} [implementation.set] - the getter function with the signature "function(value){}" that
     *          stores the given attribute value
     * @param {Function} [implementation.get] the getter function with the signature "function(){}" that returns the
     *          current attribute value
     * @param attributeName - the name of the attribute
     * @param attributeType - the type of the attribute
     */
    public constructor(
        parent: JoynrProvider,
        implementation: {
            get?: (key: string) => T | Promise<T>;
            set?: (value: T) => void | Promise<void>;
        },
        attributeName: string,
        attributeType: string
    ) {
        this.parent = parent;
        this.implementation = implementation;
        this.attributeName = attributeName;
        this.attributeType = attributeType;

        // place these functions after the forwarding we don't want them public
        if (this.implementation && typeof this.implementation.get === "function") {
            this.privateGetterFunc = this.implementation.get;
        }
        if (this.implementation && typeof this.implementation.set === "function") {
            this.privateSetterFunc = this.implementation.set;
        }
    }

    public isNotifiable(): boolean {
        return this.hasNotify;
    }

    /**
     * Check Getter and Setter functions.
     */
    public check(): boolean {
        return (
            (!this.hasRead || typeof this.privateGetterFunc === "function") &&
            (!this.hasWrite || typeof this.privateSetterFunc === "function")
        );
    }
}
/*
    The empty container classes below are declared for typing purposes.
    Unfortunately declarations need to be repeated since it's impossible to give template parameters
    to mixin classes. See https://github.com/Microsoft/TypeScript/issues/26154
 */
const ProviderReadAttributeImpl = Readable<ProviderAttributeConstructor>(ProviderAttribute);
export class ProviderReadAttribute<T> extends ProviderReadAttributeImpl {
    public get!: () => Promise<T[]>;
    public registerGetter!: (getterFunc: () => T | Promise<T>) => void;
}
const ProviderReadWriteAttributeImpl = Writable<ProviderAttributeConstructor>(ProviderReadAttribute);
export class ProviderReadWriteAttribute<T> extends ProviderReadWriteAttributeImpl {
    public get!: () => Promise<T[]>;
    public registerGetter!: (getterFunc: () => T | Promise<T>) => void;

    public registerSetter!: (setterFunc: (value: T) => void) => ProviderReadWriteAttribute<T>;
    public set!: (value: T) => Promise<[]>;
}
const ProviderReadWriteNotifyAttributeImpl = Notifiable<ProviderAttributeConstructor>(ProviderReadWriteAttribute);
export class ProviderReadWriteNotifyAttribute<T> extends ProviderReadWriteNotifyAttributeImpl {
    public get!: () => Promise<T[]>;
    public registerGetter!: (getterFunc: () => T | Promise<T>) => void;

    public registerSetter!: (setterFunc: (value: T) => void) => ProviderReadWriteAttribute<T>;
    public set!: (value: T) => Promise<[]>;

    public registerObserver!: (observer: (value: T) => void) => void;
    public unregisterObserver!: (observer: (value: T) => void) => void;
    public valueChanged!: (value: T) => void;
}
const ProviderReadNotifyAttributeImpl = Notifiable<ProviderAttributeConstructor>(ProviderReadAttribute);
export class ProviderReadNotifyAttribute<T> extends ProviderReadNotifyAttributeImpl {
    public get!: () => Promise<T[]>;
    public registerGetter!: (getterFunc: () => T | Promise<T>) => void;

    public registerObserver!: (observer: (value: T) => void) => void;
    public unregisterObserver!: (observer: (value: T) => void) => void;
    public valueChanged!: (value: T) => void;
}
