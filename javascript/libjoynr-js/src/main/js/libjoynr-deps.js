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

var libjoynrExports, // will receive the module names that should be exported
// this instrumentation of the define method is there to...
// ...leave the code block afterwards fully intact, so that static code analysis of require.js optimizer can still find all dependencies in
// the array of the call to define below and
// ...we can tap the dependency array with the full module names including the namespaces we need for exporting
libjoynrAmdDefine = define;

/**
 * @private
 */
define = function(name, dependencies, callback) {
    if (name === "libjoynr-deps") { // so that it works in the tests with real async AMD loading
        libjoynrExports = dependencies;
    }
    libjoynrAmdDefine(name, dependencies, callback);
};

define.amd = libjoynrAmdDefine.amd;

// place the require.js module name of all modules that are part of the external joynr API.
define("libjoynr-deps", [
    "joynr/Runtime",
    "joynr/buildSignature",
    "joynr/messaging/MessagingQos",
    "joynr/proxy/PeriodicSubscriptionQos",
    "joynr/proxy/OnChangeSubscriptionQos",
    "joynr/proxy/MulticastSubscriptionQos",
    "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
    "joynr/proxy/BroadcastFilterParameters",
    "joynr/types/ArbitrationStrategyCollection",
    "joynr/util/Util",
    "joynr/exceptions/JoynrException",
    "joynr/exceptions/JoynrRuntimeException",
    "joynr/exceptions/ApplicationException",
    "joynr/exceptions/DiscoveryException",
    "joynr/exceptions/IllegalAccessException",
    "joynr/exceptions/MethodInvocationException",
    "joynr/exceptions/NoCompatibleProviderFoundException",
    "joynr/exceptions/ProviderRuntimeException",
    "joynr/exceptions/PublicationMissedException",
    "joynr/exceptions/SubscriptionException",
    "joynr/types/ProviderQos",
    "joynr/types/ProviderScope",
    "joynr/types/CustomParameter",
    "joynr/proxy/DiscoveryQos",
    "joynr/types/DiscoveryScope",
    "joynr/system/RoutingTypes/BrowserAddress",
    "joynr/system/RoutingTypes/ChannelAddress",
    "joynr/system/RoutingTypes/CommonApiDbusAddress",
    "joynr/system/RoutingTypes/WebSocketAddress",
    "joynr/system/RoutingTypes/WebSocketClientAddress",
    "joynr/util/LongTimer"
], function() { // load all external modules
    var nsContext, nsElem, nsElems, i, value;
    var root = {};

    // cycle over all exports
    for (i = 0; i < libjoynrExports.length; ++i) {
        // Window in case of a Browser or DedicatedWebWorkerContext in a WebWorker Environment
        nsContext = root;
        nsElems = libjoynrExports[i].split("/").reverse();
        nsElems.pop(); //remove "joynr";
        // go through namespace elements of require.js namespace, i.e. "some/namespace/NameSpaceTest"
        while (nsElems.length) {
            // translate namespace elements to objects on window or the current WebWorkerContext,
            // e.g. "some/namespace/NameSpaceTest" is then usable as "some.namespace.NameSpaceTest"
            // as in "console.log(new some.namespace.NameSpaceTest().msg);", but don"t overwrite
            // already existing namespaces, make module publicly available on the window variable
            // or the current WebWorkerContext, which in fact defines a global variable with the
            // name <module.name> and assigns the module to it
            nsElem = nsElems.pop();
            if (nsElems.length) {
                value = nsContext[nsElem] || {};
            } else {
                // make all members of the module read-only
                value = Object.freeze(require(libjoynrExports[i]));
            }

            // export namespace fragment or module read-only to the parent namespace
            Object.defineProperty(nsContext, nsElem, {
                readable : true,
                enumerable : true,
                configurable : false,
                writable : false,
                value : value
            });
            nsContext = value;
        }
    }

    return root;
});
