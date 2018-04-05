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
var libjoynrExports;
// will receive the module names that should be exported
// this instrumentation of the define method is there to...
// ...leave the code block afterwards fully intact, so that static code analysis of require.js optimizer can still find all dependencies in
// the array of the call to define below and
// ...we can tap the dependency array with the full module names including the namespaces we need for exporting

// place the require.js module name of all modules that are part of the external joynr API.
libjoynrExports = [
    "./joynr/Runtime",
    "./joynr/buildSignature",
    "./joynr/messaging/MessagingQos",
    "./joynr/proxy/PeriodicSubscriptionQos",
    "./joynr/proxy/OnChangeSubscriptionQos",
    "./joynr/proxy/MulticastSubscriptionQos",
    "./joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
    "./joynr/proxy/BroadcastFilterParameters",
    "./joynr/types/ArbitrationStrategyCollection",
    "./joynr/util/Util",
    "./joynr/util/GenerationUtil",
    "./joynr/exceptions/JoynrException",
    "./joynr/exceptions/JoynrRuntimeException",
    "./joynr/exceptions/ApplicationException",
    "./joynr/exceptions/DiscoveryException",
    "./joynr/exceptions/IllegalAccessException",
    "./joynr/exceptions/MethodInvocationException",
    "./joynr/exceptions/NoCompatibleProviderFoundException",
    "./joynr/exceptions/ProviderRuntimeException",
    "./joynr/exceptions/PublicationMissedException",
    "./joynr/exceptions/SubscriptionException",
    "./generated/joynr/types/ProviderQos",
    "./generated/joynr/types/ProviderScope",
    "./generated/joynr/types/CustomParameter",
    "./joynr/proxy/DiscoveryQos",
    "./generated/joynr/types/DiscoveryScope",
    "./generated/joynr/system/RoutingTypes/BrowserAddress",
    "./generated/joynr/system/RoutingTypes/ChannelAddress",
    "./generated/joynr/system/RoutingTypes/WebSocketAddress",
    "./generated/joynr/system/RoutingTypes/WebSocketClientAddress",
    "./joynr/util/LongTimer"
];
var Runtime = require("./joynr/Runtime");
var buildSignature = require("./joynr/buildSignature");
var MessagingQos = require("./joynr/messaging/MessagingQos");
var PeriodicSubscriptionQos = require("./joynr/proxy/PeriodicSubscriptionQos");
var OnChangeSubscriptionQos = require("./joynr/proxy/OnChangeSubscriptionQos");
var MulticastSubscriptionQos = require("./joynr/proxy/MulticastSubscriptionQos");
var OnChangeWithKeepAliveSubscriptionQos = require("./joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
var BroadcastFilterParameters = require("./joynr/proxy/BroadcastFilterParameters");
var ArbitrationStrategyCollection = require("./joynr/types/ArbitrationStrategyCollection");
var Util = require("./joynr/util/Util");
var GenerationUtil = require("./joynr/util/GenerationUtil");
var JoynrException = require("./joynr/exceptions/JoynrException");
var JoynrRuntimeException = require("./joynr/exceptions/JoynrRuntimeException");
var ApplicationException = require("./joynr/exceptions/ApplicationException");
var DiscoveryException = require("./joynr/exceptions/DiscoveryException");
var IllegalAccessException = require("./joynr/exceptions/IllegalAccessException");
var MethodInvocationException = require("./joynr/exceptions/MethodInvocationException");
var NoCompatibleProviderFoundException = require("./joynr/exceptions/NoCompatibleProviderFoundException");
var ProviderRuntimeException = require("./joynr/exceptions/ProviderRuntimeException");
var PublicationMissedException = require("./joynr/exceptions/PublicationMissedException");
var SubscriptionException = require("./joynr/exceptions/SubscriptionException");
var ProviderQos = require("./generated/joynr/types/ProviderQos");
var ProviderScope = require("./generated/joynr/types/ProviderScope");
var CustomParameter = require("./generated/joynr/types/CustomParameter");
var DiscoveryQos = require("./joynr/proxy/DiscoveryQos");
var DiscoveryScope = require("./generated/joynr/types/DiscoveryScope");
var BrowserAddress = require("./generated/joynr/system/RoutingTypes/BrowserAddress");
var ChannelAddress = require("./generated/joynr/system/RoutingTypes/ChannelAddress");
var WebSocketAddress = require("./generated/joynr/system/RoutingTypes/WebSocketAddress");
var WebSocketClientAddress = require("./generated/joynr/system/RoutingTypes/WebSocketClientAddress");
var LongTimer = require("./joynr/util/LongTimer");

// load all external modules
var nsContext, nsElem, nsElems, i, value;
var root = {};
// cycle over all exports
for (i = 0; i < libjoynrExports.length; ++i) {
    // Window in case of a Browser or DedicatedWebWorkerContext in a WebWorker Environment
    nsContext = root;
    nsElems = libjoynrExports[i]
        .replace(/^\.\/joynr\//, "")
        .replace(/\.\/generated\/joynr\//, "")
        .split("/")
        .reverse();
    //remove "joynr";
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
            readable: true,
            enumerable: true,
            configurable: false,
            writable: false,
            value: value
        });
        nsContext = value;
    }
}
module.exports = root;
