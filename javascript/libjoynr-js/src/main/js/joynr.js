/*eslint global-require: "off"*/
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

function populateJoynrApi(joynr, api) {
    let key;
    for (key in api) {
        if (api.hasOwnProperty(key)) {
            joynr[key] = api[key];
        }
    }
}

function freeze(joynr, capabilitiesWritable) {
    Object.defineProperties(joynr, {
        messaging: {
            writable: false
        },
        types: {
            writable: false
        },
        start: {
            writable: false
        },
        shutdown: {
            writable: true
        },
        typeRegistry: {
            writable: false
        },
        capabilities: {
            writable: capabilitiesWritable === true
        },
        proxy: {
            writable: false
        },
        proxyBuilder: {
            writable: true
        },
        providerBuilder: {
            writable: false
        }
    });
}

const GenerationUtil = require("./joynr/util/GenerationUtil");

/**
 * @name joynr
 * @class
 */
const joynr = {
    loaded: false,
    /**
     * @name joynr#load
     * @function
     * @param provisioning
     * @param capabilitiesWriteable
     * @return Promise object being resolved in case all libjoynr dependencies are loaded
     */
    load: function load(provisioning, capabilitiesWritable) {
        joynr.loaded = true;
        const joynrapi = require("./libjoynr-deps");
        const runtime = new joynrapi.Runtime();
        return runtime
            .start(provisioning)
            .then(() => {
                populateJoynrApi(joynr, joynrapi);
                //remove Runtime, as it is not required for the end user
                delete joynr.Runtime;
                populateJoynrApi(joynr, runtime);
                freeze(joynr, capabilitiesWritable);

                // make sure the runtime is shutdown when process.exit(...)
                // gets called since otherwise the process might not
                // terminate. Ignore any exception thrown in case shutdown
                // had already been invoked manually before reaching this
                // point.
                if (typeof process === "object" && typeof process.on === "function") {
                    process.on("exit", () => {
                        try {
                            joynr.shutdown();
                        } catch (error) {
                            // ignore
                        }
                    });
                }
                return joynr;
            })
            .catch(error => {
                return Promise.reject(error);
            });
    },
    /**
     * Adds a typeName to constructor entry in the type registry.
     *
     * @name joynr#addType
     * @function
     * @see TypeRegistry#addType
     *
     * @param {String}
     *            joynrTypeName - the joynr type name that is sent on the wire.
     * @param {Function}
     *            typeConstructor - the corresponding JavaScript constructor for this type.
     * @param {boolean}
     *            isEnum - optional flag if the added type is an enumeration type
     */
    addType: function registerType(name, type, isEnum) {
        const TypeRegistrySingleton = require("./joynr/types/TypeRegistrySingleton");
        TypeRegistrySingleton.getInstance().addType(name, type, isEnum);
    },
    JoynrObject: function JoynrObject() {},
    util: { GenerationUtil }
};

if (typeof window === "object") {
    // export namespace fragment or module read-only to the parent namespace
    Object.defineProperty(window, "joynr", {
        enumerable: true,
        configurable: false,
        writable: false,
        value: joynr
    });
}

joynr.selectRuntime = function selectRuntime(runtime) {
    if (joynr.loaded) {
        throw new Error("joynr.selectRuntime: this method must be invoked before calling joynr.load()");
    }
    joynr._selectedRuntime = runtime;
};
joynr.selectRuntime("websocket.libjoynr");

if (module !== undefined && module.exports) {
    exports.joynr = module.exports = joynr;
} else {
    // support CommonJS module 1.1.1 spec (`exports` cannot be a function)
    exports.joynr = joynr;
}

/* jslint nomen: false */
