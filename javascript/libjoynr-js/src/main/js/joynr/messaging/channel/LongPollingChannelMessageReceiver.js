/*eslint no-unused-vars: "error"*/
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
const uuid = require("../../../lib/uuid-annotated");
const JoynrMessage = require("../JoynrMessage");
const JsonParser = require("../../../lib/JsonParser");
const MessagingQos = require("../MessagingQos");
const DiagnosticTags = require("../../system/DiagnosticTags");
const LoggingManager = require("../../system/LoggingManager");
const Util = require("../../util/UtilInternal");
const LongTimer = require("../../util/LongTimer");

const log = LoggingManager.getLogger("joynr.messaging.LongPollingChannelMessageReceiver");
const storagePrefix = "joynr.channels";

/**
 * LongPollingChannelMessageReceiver handles the long poll to the bounce proxy.
 *
 * @name LongPollingChannelMessageReceiver
 * @constructor
 *
 * @param {Object}
 *            settings an object containing required input for LongPollingChannelMessageReceiver
 * @param {String}
 *            settings.bounceProxyUrl an object describing the bounce proxy URL
 * @param {CommunicationModule}
 *            settings.communicationModule the API required by the LongPollingChannelMessageReceiver to perform XMLHTTPRequests
 *            socket communication
 * @param {Object}
 *            settings.channelQos parameters specifying the qos requirements for the channel (creation)
 * @param {Number}
 *            settings.channelQos.creationTimeout_ms max time to create a channel
 * @param {Number}
 *            settings.channelQos.creationRetryDelay_ms retry interval after a failing channel creation
 *
 * @returns the LongPollingChannelMessageReceiver
 */
function LongPollingChannelMessageReceiver(settings) {
    this._persistency = settings.persistency;
    this._channelOpMessagingQos = new MessagingQos();
    this._communicationModule = settings.communicationModule;

    this._bounceProxyChannelBaseUrl = settings.bounceProxyUrl + "channels/";
    this._channelCreationTimeout_ms =
        settings.channelQos && settings.channelQos.creationTimeout_ms
            ? settings.channelQos.creationTimeout_ms
            : 1000 * 60 * 60 * 24; // default: 1 day
    this._channelCreationRetryDelay_ms =
        settings.channelQos && settings.channelQos.creationRetryDelay_ms
            ? settings.channelQos.creationRetryDelay_ms
            : 1000 * 30; // default: 30s
}

/**
 * Retrieve the receiverId for the given channelId
 *
 * @name LongPollingChannelMessageReceiver#getReceiverId
 * @function
 * @private
 */
LongPollingChannelMessageReceiver.prototype._getReceiverId = function(channelId) {
    let receiverId = this._persistency.getItem(storagePrefix + "." + channelId + ".receiverId");
    if (receiverId === undefined || receiverId === null) {
        receiverId = "tid-" + uuid();
        this._persistency.setItem(storagePrefix + "." + channelId + ".receiverId", receiverId);
    }
    return receiverId;
};

LongPollingChannelMessageReceiver.prototype._callCreate = function callCreate() {
    const that = this;
    const createChannelUrl = this._bounceProxyChannelBaseUrl + "?ccid=" + encodeURIComponent(this._channelId);
    const receiverId = this._getReceiverId(this._channelId);

    function callCreateOnSuccess(xhr) {
        that._channelUrl = xhr.getResponseHeader("Location");

        if (!that._channelUrl) {
            that._channelUrl = xhr.responseText;
        }

        log.debug("created channel with id " + that._channelId + " and url " + that._channelUrl);

        return that._channelUrl;
    }

    return this._communicationModule
        .createXMLHTTPRequest({
            // TODO: check why headers ist not an array
            type: "POST",
            url: createChannelUrl,
            headers: {
                "X-Atmosphere-tracking-id": receiverId
            },
            timeout: this._channelOpMessagingQos.ttl
        })
        .then(callCreateOnSuccess.bind(this));
};

LongPollingChannelMessageReceiver.prototype._logChannelCreationError = function(xhr) {
    log.error("error while creating channel on bounce proxy", {
        channelId: this._channelId,
        channelUrl: this._channelUrl,
        status: xhr.status,
        responseText: xhr.responseText
    });
};

LongPollingChannelMessageReceiver.prototype._createInternal = function createInternal(resolve, reject) {
    const that = this;

    function createInternalOnError(xhr) {
        that._logChannelCreationError(xhr);

        if (that._createChannelTimestamp + that._channelCreationTimeout_ms <= Date.now()) {
            reject(new Error("Error creating channel"));
        } else {
            LongTimer.setTimeout(that._createInternal.bind(that), that._channelCreationRetryDelay_ms, resolve, reject);
        }
    }

    this._callCreate()
        .then(resolve)
        .catch(createInternalOnError.bind(that));
};

/**
 * Stop long polling
 *
 * @name LongPollingChannelMessageReceiver#stop
 * @function
 */
LongPollingChannelMessageReceiver.prototype.stop = function stop() {
    this._communicationModule.atmosphere.unsubscribeUrl(this._channelUrl);
};

/**
 * Removes the channel with a given channel id
 *
 * @name LongPollingChannelMessageReceiver#clear
 * @function
 *
 * @returns {Object} a promise object for async event handling
 */
LongPollingChannelMessageReceiver.prototype.clear = function clear() {
    const that = this;

    function createXMLHTTPRequestOnError(xhr, errorType) {
        const errorString =
            "error while deleting channel on bounce proxy: " +
            errorType +
            (xhr.statusText ? ", " + xhr.statusText : "") +
            (xhr.status === 0 ? "" : ", HTTP" + xhr.status) +
            (xhr.responseText ? ", " + xhr.responseText : "");
        log.error(
            "error while deleting channel on bounce proxy",
            JSON.stringify(
                DiagnosticTags.forChannel({
                    channelId: that._channelId,
                    channelUrl: that._channelUrl,
                    status: xhr.status,
                    responseText: xhr.responseText
                })
            )
        );
        throw new Error(errorString);
    }

    function returnUndefined() {}

    log.debug("clearing channel with id " + that._channelId + " and url " + that._channelUrl);
    return that._communicationModule
        .createXMLHTTPRequest({
            type: "DELETE",
            url: that._channelUrl,
            timeout: that._channelOpMessagingQos.ttl
        })
        .then(returnUndefined) // required for some tests no idea why
        .catch(createXMLHTTPRequestOnError);
};

/**
 * Start long polling. Side effect: first makes sure that the channel exists on the bounceproxy
 *
 * @name LongPollingChannelMessageReceiver#start
 * @function
 *
 * @param {Function}
 *            onMessageCallback callback used to pass the received messages up the processing chain
 * @param {Function}
 *            onError callback used to inform about errors occurred during message recieve
 */
LongPollingChannelMessageReceiver.prototype.start = function start(onMessageCallback, onError) {
    let pollRequest;
    const receiverId = this._getReceiverId(this._channelId);

    this._communicationModule.atmosphere.unsubscribeUrl(this._channelUrl);

    pollRequest = {
        url: this._channelUrl,
        transport: "long-polling",
        contentType: "application/json",
        timeout: 30000,
        enableXDR: false,
        enableProtocol: false,
        readResponseHeaders: false,
        attachHeadersAsQueryString: false,
        dropAtmosphereHeaders: true,
        dropHeaders: false,
        logLevel: "debug",
        headers: {
            "X-Atmosphere-tracking-id": receiverId
        }
    };

    // Called whenever one or more new
    // messages are received
    // from the bounceproxy.
    pollRequest.onMessage = function onMessage(response) {
        let data, jsonParser, joynrMessage;

        try {
            if (response.status === 200) {
                data = response.responseBody;

                if (data.length > 0) {
                    jsonParser = new JsonParser(data);
                    while (jsonParser.hasNext) {
                        // pass the message on
                        joynrMessage = JoynrMessage.parseMessage(jsonParser.next);

                        log.info(
                            "received message with id " + joynrMessage.msgId + ": ",
                            JSON.stringify(DiagnosticTags.forJoynrMessage(joynrMessage))
                        );
                        onMessageCallback(joynrMessage);
                    }
                }
            }
        } catch (e) {
            log.debug("Exception while processing message: " + e.message);
            if (onError) {
                onError(e);
            }
        }
    };

    pollRequest.onError = function(response) {
        log.debug("Encountered atmosphere error :" + response.error);
        if (onError) {
            onError(response.error);
        }
    };

    this._communicationModule.atmosphere.subscribe(pollRequest);

    log.debug("starting to listen on channel with id " + this._channelId + " and url " + this._channelUrl);
};

/**
 * Create the given Channel on the bounceproxy
 *
 * @name LongPollingChannelMessageReceiver#create
 * @function
 *
 * @param {String}
 *            channelId the channelId that will be created, will be URI-Component-encoded
 * @returns {Object} a promise object for async event handling
 */
LongPollingChannelMessageReceiver.prototype.create = function(theChannelId) {
    const that = this;
    if (this._channelId !== undefined) {
        throw new Error(
            "LongPollingChannelMessageReceiver.create has already been called for channelId: " + theChannelId
        );
    }
    this._channelId = theChannelId;
    this._createChannelTimestamp = Date.now();

    function _callCreateOnError(xhr) {
        that._logChannelCreationError(xhr);
        if (that._createChannelTimestamp + that._channelCreationTimeout_ms <= Date.now()) {
            throw new Error("Error creating channel");
        } else {
            const deferred = Util.createDeferred();
            LongTimer.setTimeout(
                that._createInternal.bind(that),
                that._channelCreationRetryDelay_ms,
                deferred.resolve,
                deferred.reject
            );
            return deferred.promise;
        }
    }

    return this._callCreate().catch(_callCreateOnError);
};
module.exports = LongPollingChannelMessageReceiver;
