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

var provisioning = {};

// lets assume the bounce proxy is on the same server as the app is deployed to
//provisioning.bounceProxyBaseUrl = window.location.origin + "/com/joyn";
provisioning.bounceProxyBaseUrl = window.location.origin;

provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";
var domain = "javascriptproviderdomain";
var queryDomain = window.location.href.match(/domain=([a-zA-Z0-9\-]+)/);
if(queryDomain !== null) {
    domain = queryDomain[1];
}

var discoveryChannel = "discoverydirectory_channelid";

provisioning.channelUrls = {};
provisioning.channelUrls[discoveryChannel] = [ provisioning.bounceProxyBaseUrl + "/discovery/channels/" + discoveryChannel + "/"];

var globalCapDirCapability = {
    domain : "io.joynr",
    interfaceName : "infrastructure/GlobalCapabilitiesDirectory",
    providerQos : {
        qos : [],
        version : 0,
        priority : 1,
        isLocalOnly : false,
        onChangeSubscriptions : true
    },
    channelId : discoveryChannel,
    participantId : "capabilitiesdirectory_participantid"
};

var channelUrlDirCapability = {
        domain : "io.joynr",
        interfaceName : "infrastructure/ChannelUrlDirectory",
        providerQos : {
            qos : [],
            version : 0,
            priority : 1,
            isLocalOnly : false,
            onChangeSubscriptions : true
        },
        channelId : discoveryChannel,
        participantId : "channelurldirectory_participantid"
};

provisioning.capabilities = [ globalCapDirCapability, channelUrlDirCapability ];