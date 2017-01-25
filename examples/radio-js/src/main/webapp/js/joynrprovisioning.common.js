/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
provisioning.brokerUri = "tcp://localhost:9001";
var domain = "javascriptproviderdomain";
var queryDomain = window.location.href.match(/domain=([a-zA-Z0-9\-]+)/);
if(queryDomain !== null) {
    domain = queryDomain[1];
}
var queryChannelId = window.location.href.match(/channelId=([a-zA-Z0-9\-]+)/);
if(queryChannelId !== null) {
    provisioning.channelId = queryChannelId[1];
}
