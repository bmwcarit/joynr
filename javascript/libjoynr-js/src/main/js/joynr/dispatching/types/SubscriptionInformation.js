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

define("joynr/dispatching/types/SubscriptionInformation", [
    "joynr/util/Typing",
    "joynr/dispatching/types/SubscriptionRequest"
], function(Typing, SubscriptionRequest) {

    /**
     * @name SubscriptionInformation
     * @constructor
     * @param {String}
     *            proxyParticipantId Id of the proxy
     * @param {String}
     *            providerParticipantId Id of the provider
     * @param {Object|SubscriptionRequest}
     *            [request] the subscription request
     */
    function SubscriptionInfo(proxyParticipantId, providerParticipantId, request) {

        this.proxyParticipantId = proxyParticipantId;
        this.providerParticipantId = providerParticipantId;
        this.subscriptionId = request.subscriptionId;
        this.subscribedToName = request.subscribedToName;
        this.qos = request.qos;
        this.lastPublication = 0;

        /**
         * The joynr type name
         *
         * @name SubscriptionInformation#_typeName
         * @type String
         * @field
         */
        Typing.augmentTypeName(this, "joynr");
    }

    return SubscriptionInfo;

});