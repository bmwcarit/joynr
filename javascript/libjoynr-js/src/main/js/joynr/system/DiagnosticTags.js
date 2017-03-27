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

define("joynr/system/DiagnosticTags", [], function() {

    /**
     * @exports DiagnosticTags
     */
    var DiagnosticTags = {};

    /**
     * @param {JoynrMessage}
     *            joynrMessage
     */
    DiagnosticTags.forJoynrMessage = function forJoynrMessage(joynrMessage) {
        return {
            diagnosticTag : "JoynrMessage",
            from : joynrMessage.from,
            to : joynrMessage.to,
            type : joynrMessage.type
        };
    };

    /**
     * @param {String}
     *            channelInfo.channelUrl
     * @param {String}
     *            channelInfo.channelId
     * @param {String}
     *            channelInfo.status
     * @param {String}
     *            channelInfo.responseText
     */
    DiagnosticTags.forChannel = function forChannel(channelInfo) {
        return {
            diagnosticTag : "ChannelInfo",
            channelUrl : channelInfo.channelUrl,
            channelId : channelInfo.channelId,
            status : channelInfo.status,
            responseText : channelInfo.responseText
        };
    };

    /**
     * @param {Object} requestInfo
     */
    DiagnosticTags.forRequest = function forRequest(requestInfo) {
        return {
            diagnosticTag : "Request",
            requestReplyId : requestInfo.request.requestReplyId,
            params : requestInfo.request.params,
            to : requestInfo.to,
            from : requestInfo.from
        };
    };

    /**
     * @param {Object} requestInfo
     */
    DiagnosticTags.forOneWayRequest = function forRequest(requestInfo) {
        return {
            diagnosticTag : "OneWayRequest",
            params : requestInfo.request.params,
            to : requestInfo.to,
            from : requestInfo.from
        };
    };

    /**
     * @param {Object} replyInfo
     */
    DiagnosticTags.forReply = function forReply(replyInfo) {
        return {
            diagnosticTag : "Reply",
            requestReplyId : replyInfo.reply.requestReplyId,
            to : replyInfo.to,
            from : replyInfo.from
        };
    };

    /**
     * @param {Object} subscriptionReplyInfo
     */
    DiagnosticTags.forSubscriptionReply = function forSubscriptionReply(subscriptionReplyInfo) {
        return {
            diagnosticTag : "SubscriptionReply",
            subscriptionId : subscriptionReplyInfo.subscriptionReply.subscriptionId,
            to : subscriptionReplyInfo.to,
            from : subscriptionReplyInfo.from
        };
    };

    /**
     * @param {Object} subscriptionRequestInfo
     */
    DiagnosticTags.forMulticastSubscriptionRequest =
            function forMulticastSubscriptionRequest(subscriptionRequestInfo) {
                return {
                    diagnosticTag : "MulticastSubscriptionRequest",
                    eventName : subscriptionRequestInfo.subscriptionRequest.subscribedToName,
                    subscriptionId : subscriptionRequestInfo.subscriptionRequest.subscriptionId,
                    multicastId : subscriptionRequestInfo.subscriptionRequest.multicastId,
                    to : subscriptionRequestInfo.to,
                    from : subscriptionRequestInfo.from
                };
            };

    /**
     * @param {Object} subscriptionRequestInfo
     */
    DiagnosticTags.forBroadcastSubscriptionRequest =
            function forBroadcastSubscriptionRequest(subscriptionRequestInfo) {
                return {
                    diagnosticTag : "BroadcastSubscriptionRequest",
                    eventName : subscriptionRequestInfo.subscriptionRequest.subscribedToName,
                    subscriptionId : subscriptionRequestInfo.subscriptionRequest.subscriptionId,
                    to : subscriptionRequestInfo.to,
                    from : subscriptionRequestInfo.from
                };
            };

    /**
     * @param {Object} subscriptionRequestInfo
     */
    DiagnosticTags.forSubscriptionRequest =
            function forSubscriptionRequest(subscriptionRequestInfo) {
                return {
                    diagnosticTag : "SubscriptionRequest",
                    attributeName : subscriptionRequestInfo.subscriptionRequest.subscribedToName,
                    subscriptionId : subscriptionRequestInfo.subscriptionRequest.subscriptionId,
                    to : subscriptionRequestInfo.to,
                    from : subscriptionRequestInfo.from
                };
            };

    /**
     * @param {Object} subscriptionStopInfo
     */
    DiagnosticTags.forSubscriptionStop = function forSubscriptionStop(subscriptionStopInfo) {
        return {
            diagnosticTag : "SubscriptionStop",
            subscriptionId : subscriptionStopInfo.subscriptionId,
            to : subscriptionStopInfo.to,
            from : subscriptionStopInfo.from
        };
    };

    /**
     * @param {Object} publicationInfo
     */
    DiagnosticTags.forPublication = function forPublication(publicationInfo) {
        return {
            diagnosticTag : "Publication",
            subscriptionId : publicationInfo.publication.subscriptionId,
            to : publicationInfo.to,
            from : publicationInfo.from
        };
    };

    /**
     * @param {Object} publicationInfo - multicast publication info
     */
    DiagnosticTags.forMulticastPublication = function forMulticastPublication(publicationInfo) {
        return {
            diagnosticTag : "MulticastPublication",
            multicastId : publicationInfo.publication.multicastId,
            from : publicationInfo.from
        };
    };

    return DiagnosticTags;
});