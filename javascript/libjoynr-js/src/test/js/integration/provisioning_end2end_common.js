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
(function() {
    var setupProvisionedData = function(obj) {
        obj.interfaceNameComm = "vehicle/Radio";
        obj.interfaceNameDatatypes = "datatypes/Datatypes";
        obj.proxyChannelId = "End2EndTestProxyChannel";
        obj.providerParticipantIdComm = "End2EndCommTestParticipantId";
        obj.providerParticipantIdDatatypes = "End2EndDatatypesTestParticipantId";
        obj.providerChannelIdComm = "End2EndCommTestProviderChannel";
        obj.providerChannelIdDatatypes = "End2EndDatatypesTestProviderChannel";
        return obj;
    };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("integration/provisioning_end2end_common", [], function() {
            return setupProvisionedData({});
        });
    } else {
        setupProvisionedData(window);
    }
}());