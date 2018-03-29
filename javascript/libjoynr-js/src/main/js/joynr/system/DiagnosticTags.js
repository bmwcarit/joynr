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

/**
 * @exports DiagnosticTags
 */
var Util = require("../util/UtilInternal");
var loggingManager = require("./LoggingManager");
var DiagnosticTags = {};

/**
 * @param {JoynrMessage}
 *            joynrMessage
 */
DiagnosticTags.forJoynrMessage = function forJoynrMessage(joynrMessage) {
    return {
        diagnosticTag: "JoynrMessage",
        from: joynrMessage.from,
        to: joynrMessage.to,
        type: joynrMessage.type
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
        diagnosticTag: "ChannelInfo",
        channelUrl: channelInfo.channelUrl,
        channelId: channelInfo.channelId,
        status: channelInfo.status,
        responseText: channelInfo.responseText
    };
};

var forRequestHelper = function forRequestHelper(tagsForRequest, requestInfo) {
    if (requestInfo.request.params) {
        tagsForRequest.params = JSON.stringify(requestInfo.request.params);
    }
};

/**
 * @param {Object} requestInfo
 */
DiagnosticTags.forRequest = function forRequest(requestInfo) {
    var tagsForRequest = {
        diagnosticTag: "Request",
        requestReplyId: requestInfo.request.requestReplyId,
        to: requestInfo.to,
        from: requestInfo.from
    };
    forRequestHelper(tagsForRequest, requestInfo);
    return tagsForRequest;
};

var forOneWayRequestHelper = function(tagsForOneWayRequest, requestInfo) {
    if (requestInfo.request.params) {
        tagsForOneWayRequest.params = JSON.stringify(requestInfo.request.params);
    }
};

/**
 * @param {Object} requestInfo
 */
DiagnosticTags.forOneWayRequest = function forOneWayRequest(requestInfo) {
    var tagsForOneWayRequest = {
        diagnosticTag: "OneWayRequest",
        to: requestInfo.to,
        from: requestInfo.from
    };
    forOneWayRequestHelper(tagsForOneWayRequest, requestInfo);
    return tagsForOneWayRequest;
};

var forReplyHelper = function(tagsForReply, replyInfo) {
    if (replyInfo.reply.error) {
        tagsForReply.error = JSON.stringify(replyInfo.reply.error);
    } else {
        tagsForReply.response = JSON.stringify(replyInfo.reply.response);
    }
};

/**
 * @param {Object} replyInfo
 */
DiagnosticTags.forReply = function forReply(replyInfo) {
    var tagsForReply = {
        diagnosticTag: "Reply",
        requestReplyId: replyInfo.reply.requestReplyId,
        to: replyInfo.to,
        from: replyInfo.from
    };
    forReplyHelper(tagsForReply, replyInfo);
    return tagsForReply;
};

/**
 * @param {Object} subscriptionReplyInfo
 */
DiagnosticTags.forSubscriptionReply = function forSubscriptionReply(subscriptionReplyInfo) {
    return {
        diagnosticTag: "SubscriptionReply",
        subscriptionId: subscriptionReplyInfo.subscriptionReply.subscriptionId,
        to: subscriptionReplyInfo.to,
        from: subscriptionReplyInfo.from
    };
};

/**
 * @param {Object} subscriptionRequestInfo
 */
DiagnosticTags.forMulticastSubscriptionRequest = function forMulticastSubscriptionRequest(subscriptionRequestInfo) {
    return {
        diagnosticTag: "MulticastSubscriptionRequest",
        eventName: subscriptionRequestInfo.subscriptionRequest.subscribedToName,
        subscriptionId: subscriptionRequestInfo.subscriptionRequest.subscriptionId,
        multicastId: subscriptionRequestInfo.subscriptionRequest.multicastId,
        to: subscriptionRequestInfo.to,
        from: subscriptionRequestInfo.from
    };
};

/**
 * @param {Object} subscriptionRequestInfo
 */
DiagnosticTags.forBroadcastSubscriptionRequest = function forBroadcastSubscriptionRequest(subscriptionRequestInfo) {
    return {
        diagnosticTag: "BroadcastSubscriptionRequest",
        eventName: subscriptionRequestInfo.subscriptionRequest.subscribedToName,
        subscriptionId: subscriptionRequestInfo.subscriptionRequest.subscriptionId,
        to: subscriptionRequestInfo.to,
        from: subscriptionRequestInfo.from
    };
};

/**
 * @param {Object} subscriptionRequestInfo
 */
DiagnosticTags.forSubscriptionRequest = function forSubscriptionRequest(subscriptionRequestInfo) {
    return {
        diagnosticTag: "SubscriptionRequest",
        attributeName: subscriptionRequestInfo.subscriptionRequest.subscribedToName,
        subscriptionId: subscriptionRequestInfo.subscriptionRequest.subscriptionId,
        to: subscriptionRequestInfo.to,
        from: subscriptionRequestInfo.from
    };
};

/**
 * @param {Object} subscriptionStopInfo
 */
DiagnosticTags.forSubscriptionStop = function forSubscriptionStop(subscriptionStopInfo) {
    return {
        diagnosticTag: "SubscriptionStop",
        subscriptionId: subscriptionStopInfo.subscriptionId,
        to: subscriptionStopInfo.to,
        from: subscriptionStopInfo.from
    };
};

var forPublicationHelper = function(tagsForPublication, publicationInfo) {
    tagsForPublication.response = JSON.stringify(publicationInfo.publication.response);
};

/**
 * @param {Object} publicationInfo
 */
DiagnosticTags.forPublication = function forPublication(publicationInfo) {
    var tagsForPublication = {
        diagnosticTag: "Publication",
        subscriptionId: publicationInfo.publication.subscriptionId,
        to: publicationInfo.to,
        from: publicationInfo.from
    };
    forPublicationHelper(tagsForPublication, publicationInfo);
    return tagsForPublication;
};

var forMulticastPublicationHelper = function(tagsForMulticastPublication, publicationInfo) {
    tagsForMulticastPublication.response = JSON.stringify(publicationInfo.publication.response);
};

/**
 * @param {Object} publicationInfo - multicast publication info
 */
DiagnosticTags.forMulticastPublication = function forMulticastPublication(publicationInfo) {
    var tagsForMulticastPublication = {
        diagnosticTag: "MulticastPublication",
        multicastId: publicationInfo.publication.multicastId,
        from: publicationInfo.from
    };
    forMulticastPublicationHelper(tagsForMulticastPublication, publicationInfo);
    return tagsForMulticastPublication;
};

function logLevelChangedCb(level) {
    if (level !== loggingManager.LogLevel.DEBUG && level !== loggingManager.LogLevel.TRACE) {
        forRequestHelper = Util.emptyFunction;
        forOneWayRequestHelper = Util.emptyFunction;
        forReplyHelper = Util.emptyFunction;
        forPublicationHelper = Util.emptyFunction;
        forMulticastPublicationHelper = Util.emptyFunction;
    }
}

loggingManager.registerForLogLevelChanged(logLevelChangedCb);

module.exports = DiagnosticTags;
