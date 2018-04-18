/*eslint no-unused-vars: "off", global-require: "off"*/
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
let libjoynrExports;
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
const Runtime = require("./joynr/Runtime");
const buildSignature = require("./joynr/buildSignature");
const MessagingQos = require("./joynr/messaging/MessagingQos");
const PeriodicSubscriptionQos = require("./joynr/proxy/PeriodicSubscriptionQos");
const OnChangeSubscriptionQos = require("./joynr/proxy/OnChangeSubscriptionQos");
const MulticastSubscriptionQos = require("./joynr/proxy/MulticastSubscriptionQos");
const OnChangeWithKeepAliveSubscriptionQos = require("./joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
const BroadcastFilterParameters = require("./joynr/proxy/BroadcastFilterParameters");
const ArbitrationStrategyCollection = require("./joynr/types/ArbitrationStrategyCollection");
const Util = require("./joynr/util/Util");
const GenerationUtil = require("./joynr/util/GenerationUtil");
const JoynrException = require("./joynr/exceptions/JoynrException");
const JoynrRuntimeException = require("./joynr/exceptions/JoynrRuntimeException");
const ApplicationException = require("./joynr/exceptions/ApplicationException");
const DiscoveryException = require("./joynr/exceptions/DiscoveryException");
const IllegalAccessException = require("./joynr/exceptions/IllegalAccessException");
const MethodInvocationException = require("./joynr/exceptions/MethodInvocationException");
const NoCompatibleProviderFoundException = require("./joynr/exceptions/NoCompatibleProviderFoundException");
const ProviderRuntimeException = require("./joynr/exceptions/ProviderRuntimeException");
const PublicationMissedException = require("./joynr/exceptions/PublicationMissedException");
const SubscriptionException = require("./joynr/exceptions/SubscriptionException");
const ProviderQos = require("./generated/joynr/types/ProviderQos");
const ProviderScope = require("./generated/joynr/types/ProviderScope");
const CustomParameter = require("./generated/joynr/types/CustomParameter");
const DiscoveryQos = require("./joynr/proxy/DiscoveryQos");
const DiscoveryScope = require("./generated/joynr/types/DiscoveryScope");
const BrowserAddress = require("./generated/joynr/system/RoutingTypes/BrowserAddress");
const ChannelAddress = require("./generated/joynr/system/RoutingTypes/ChannelAddress");
const WebSocketAddress = require("./generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketClientAddress = require("./generated/joynr/system/RoutingTypes/WebSocketClientAddress");
const LongTimer = require("./joynr/util/LongTimer");

// load all external modules
let nsContext, nsElem, nsElems, i, value;
const root = {};
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
            enumerable: true,
            configurable: false,
            writable: false,
            value
        });
        nsContext = value;
    }
}
module.exports = root;
