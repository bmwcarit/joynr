/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
/* istanbul ignore file */

/**
 * @exports DiagnosticTags
 */
import * as UtilInternal from "../util/UtilInternal";
import { LogLevel } from "./JoynrLogger";

import loggingManager from "./LoggingManager";
import JoynrMessage from "../messaging/JoynrMessage";

/**
 * @param joynrMessage
 * @returns object to be logged
 */
export function forJoynrMessage(joynrMessage: JoynrMessage): any {
    return {
        diagnosticTag: "JoynrMessage",
        from: joynrMessage.from,
        to: joynrMessage.to,
        type: joynrMessage.type
    };
}

/**
 * @param channelInfo.channelUrl
 * @param channelInfo.channelId
 * @param channelInfo.status
 * @param channelInfo.responseText
 */
export function forChannel(channelInfo: {
    channelUrl: string;
    channelId: string;
    status: string;
    responseText: string;
}): any {
    return {
        diagnosticTag: "ChannelInfo",
        channelUrl: channelInfo.channelUrl,
        channelId: channelInfo.channelId,
        status: channelInfo.status,
        responseText: channelInfo.responseText
    };
}

let forRequestHelper = (tagsForRequest: any, requestInfo: any): void => {
    if (requestInfo.request.params) {
        tagsForRequest.params = requestInfo.request.params;
    }
};

/**
 * @param requestInfo
 */
export function forRequest(requestInfo: Record<string, any>): any {
    const tagsForRequest = {
        diagnosticTag: "Request",
        requestReplyId: requestInfo.request.requestReplyId,
        to: requestInfo.to,
        from: requestInfo.from
    };
    forRequestHelper(tagsForRequest, requestInfo);
    return tagsForRequest;
}

let forOneWayRequestHelper = (tagsForOneWayRequest: any, requestInfo: any): void => {
    if (requestInfo.request.params) {
        tagsForOneWayRequest.params = requestInfo.request.params;
    }
};

/**
 * @param requestInfo
 */
export function forOneWayRequest(requestInfo: Record<string, any>): any {
    const tagsForOneWayRequest = {
        diagnosticTag: "OneWayRequest",
        to: requestInfo.to,
        from: requestInfo.from
    };
    forOneWayRequestHelper(tagsForOneWayRequest, requestInfo);
    return tagsForOneWayRequest;
}

let forReplyHelper = (tagsForReply: any, replyInfo: any): void => {
    tagsForReply.response = replyInfo.reply.response;
};

/**
 * @param replyInfo
 */
export function forReply(replyInfo: Record<string, any>): Record<string, any> {
    const tagsForReply: Record<string, any> = {
        diagnosticTag: "Reply",
        requestReplyId: replyInfo.reply.requestReplyId,
        to: replyInfo.to,
        from: replyInfo.from
    };
    if (replyInfo.reply.error) {
        tagsForReply.error = replyInfo.reply.error;
    }
    forReplyHelper(tagsForReply, replyInfo);
    return tagsForReply;
}

/**
 * @param subscriptionReplyInfo
 */
export function forSubscriptionReply(subscriptionReplyInfo: Record<string, any>): any {
    const subscriptionReplyTag: Record<string, any> = {
        diagnosticTag: "SubscriptionReply",
        subscriptionId: subscriptionReplyInfo.subscriptionReply.subscriptionId,
        to: subscriptionReplyInfo.to,
        from: subscriptionReplyInfo.from
    };
    if (subscriptionReplyInfo.subscriptionReply.error) {
        subscriptionReplyTag.error = subscriptionReplyInfo.subscriptionReply.error;
    }

    return subscriptionReplyTag;
}

/**
 * @param subscriptionRequestInfo
 */
export function forMulticastSubscriptionRequest(subscriptionRequestInfo: Record<string, any>): any {
    return {
        diagnosticTag: "MulticastSubscriptionRequest",
        eventName: subscriptionRequestInfo.subscriptionRequest.subscribedToName,
        subscriptionId: subscriptionRequestInfo.subscriptionRequest.subscriptionId,
        multicastId: subscriptionRequestInfo.subscriptionRequest.multicastId,
        to: subscriptionRequestInfo.to,
        from: subscriptionRequestInfo.from
    };
}

/**
 * @param subscriptionRequestInfo
 */
export function forBroadcastSubscriptionRequest(subscriptionRequestInfo: Record<string, any>): any {
    return {
        diagnosticTag: "BroadcastSubscriptionRequest",
        eventName: subscriptionRequestInfo.subscriptionRequest.subscribedToName,
        subscriptionId: subscriptionRequestInfo.subscriptionRequest.subscriptionId,
        to: subscriptionRequestInfo.to,
        from: subscriptionRequestInfo.from
    };
}

/**
 * @param subscriptionRequestInfo
 */
export function forSubscriptionRequest(subscriptionRequestInfo: Record<string, any>): any {
    return {
        diagnosticTag: "SubscriptionRequest",
        attributeName: subscriptionRequestInfo.subscriptionRequest.subscribedToName,
        subscriptionId: subscriptionRequestInfo.subscriptionRequest.subscriptionId,
        to: subscriptionRequestInfo.to,
        from: subscriptionRequestInfo.from
    };
}

/**
 * @param subscriptionStopInfo
 */
export function forSubscriptionStop(subscriptionStopInfo: Record<string, any>): any {
    return {
        diagnosticTag: "SubscriptionStop",
        subscriptionId: subscriptionStopInfo.subscriptionId,
        to: subscriptionStopInfo.to,
        from: subscriptionStopInfo.from
    };
}

let forPublicationHelper = (tagsForPublication: any, publicationInfo: any): void => {
    tagsForPublication.response = publicationInfo.publication.response;
};

/**
 * @param publicationInfo
 */
export function forPublication(publicationInfo: Record<string, any>): Record<string, any> {
    const tagsForPublication: Record<string, any> = {
        diagnosticTag: "Publication",
        subscriptionId: publicationInfo.publication.subscriptionId,
        to: publicationInfo.to,
        from: publicationInfo.from
    };
    forPublicationHelper(tagsForPublication, publicationInfo);
    if (publicationInfo.error) {
        tagsForPublication.error = publicationInfo.error;
    }
    return tagsForPublication;
}

let forMulticastPublicationHelper = (tagsForMulticastPublication: any, publicationInfo: any): void => {
    tagsForMulticastPublication.response = publicationInfo.publication.response;
};

/**
 * @param publicationInfo - multicast publication info
 */
export function forMulticastPublication(publicationInfo: Record<string, any>): Record<string, any> {
    const tagsForMulticastPublication: Record<string, any> = {
        diagnosticTag: "MulticastPublication",
        multicastId: publicationInfo.publication.multicastId,
        from: publicationInfo.from
    };
    forMulticastPublicationHelper(tagsForMulticastPublication, publicationInfo);
    if (publicationInfo.error) {
        tagsForMulticastPublication.error = publicationInfo.error;
    }
    return tagsForMulticastPublication;
}

function logLevelChangedCb(level: LogLevel): void {
    if (level !== loggingManager.LogLevel.DEBUG && level !== loggingManager.LogLevel.TRACE) {
        forRequestHelper = UtilInternal.emptyFunction;
        forOneWayRequestHelper = UtilInternal.emptyFunction;
        forReplyHelper = UtilInternal.emptyFunction;
        forPublicationHelper = UtilInternal.emptyFunction;
        forMulticastPublicationHelper = UtilInternal.emptyFunction;
    }
}

loggingManager.registerForLogLevelChanged(logLevelChangedCb);
