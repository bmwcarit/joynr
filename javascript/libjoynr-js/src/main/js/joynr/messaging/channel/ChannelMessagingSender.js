// problem with functions postMessage and resend calling each other => it is not possible to bring them in an order without having the jslint "was not defined" error
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
var Promise = require("../../../global/Promise");
var Util = require("../../util/UtilInternal");
var Typing = require("../../util/Typing");
var JSONSerializer = require("../../util/JSONSerializer");
var JoynrMessage = require("../JoynrMessage");
var LongTimer = require("../../util/LongTimer");
var DiagnosticTags = require("../../system/DiagnosticTags");
var LoggingManager = require("../../system/LoggingManager");

/**
 * ChannelMessagingSender sends JoynrMessages to their destinations via Http
 *
 * @constructor
 * @name ChannelMessagingSender
 * @param {Object}
 *            settings the settings object holding the dependencies
 */
function ChannelMessagingSender(settings) {
    var log = LoggingManager.getLogger("joynr.messaging.ChannelMessagingSender");
    var messageQueue = []; // use push to add at the back, and shift to take from the front
    var messageProcessors =
        settings.channelQos && settings.channelQos.messageProcessors ? settings.channelQos.messageProcessors : 4;
    var resendDelay_ms =
        settings.channelQos && settings.channelQos.resendDelay_ms ? settings.channelQos.resendDelay_ms : 1000;
    var communicationModule = settings.communicationModule;
    var started = false;
    var terminated = false;

    var getRelativeExpiryDate = function getRelativeExpiryDate(joynrMessage) {
        return parseInt(joynrMessage.expiryDate, 10) - Date.now();
    };

    var checkIfExpired = function(queuedMessage) {
        if (queuedMessage.pending === false || queuedMessage.expiryTimer === undefined) {
            return true;
        }

        var isExpired = getRelativeExpiryDate(queuedMessage.message) <= 0;
        if (isExpired) {
            queuedMessage.pending = false;
            LongTimer.clearTimeout(queuedMessage.expiryTimer);
            delete queuedMessage.expiryTimer;
            var errMsg = "DISCARDED " + JSONSerializer.stringify(queuedMessage.message) + ": ttl expired";
            log.warn(errMsg);
            queuedMessage.reject(new Error(errMsg));
        }

        return isExpired;
    };

    this.start = function() {
        started = true;
        notify();
    };

    /**
     * Sends the message to the given url
     *
     * @name ChannelMessagingSender#postMessage
     * @function
     * @private
     * @param {JoynrMessage}
     *            queuedMessage to be sent
     */
    function postMessage(queuedMessage) {
        if (terminated) {
            queuedMessage.reject(new Error("ChannelMessagingSender is already shut down"));
            return;
        }
        var timeout = getRelativeExpiryDate(queuedMessage.message);

        // XMLHttpRequest uses a timeout of 0 to mean that there is no timeout.
        // ttl = 0 means the opposite (is already expired)
        if (timeout <= 0) {
            timeout = -1;
        }

        messageProcessors--;
        log.info(
            "sending message. timeout in (ms): " + timeout,
            JSONSerializer.stringify(DiagnosticTags.forJoynrMessage(queuedMessage.message))
        );
        log.trace(
            "sending message. timeout in (ms): " +
                timeout +
                JSONSerializer.stringify(queuedMessage.message, undefined, 4)
        );

        function createXMLHTTPRequestOnSuccess(xhr) {
            try {
                log.debug("sending msgId: " + queuedMessage.message.msgId + " completed successfully");
                queuedMessage.pending = false;
                LongTimer.clearTimeout(queuedMessage.expiryTimer);
                delete queuedMessage.expiryTimer;
                queuedMessage.resolve();
            } catch (error) {
                log.error("sending msgId: " + queuedMessage.message.msgId + " deferred caused an exception");
            }
            messageProcessors++;
            notify();
            return xhr;
        }

        function createXMLHTTPRequestOnError(xhr, errorType) {
            try {
                log.debug("sending msgId: " + queuedMessage.message.msgId + " failed");
                resend(queuedMessage);
            } catch (error) {
                log.error("sending msgId: " + queuedMessage.message.msgId + " deferred.reject caused an exception");
            }
            messageProcessors++;
            notify();
            return xhr;
        }

        communicationModule
            .createXMLHTTPRequest({
                type: "POST",
                url: queuedMessage.to,
                data: JSONSerializer.stringify(queuedMessage.message),
                timeout: timeout
            })
            .then(createXMLHTTPRequestOnSuccess)
            .catch(createXMLHTTPRequestOnError);
    }

    /**
     * If a processor is available to send the message, use it, otherwise will wait until one is
     * ready
     *
     * @name ChannelMessagingSender#notify
     * @function
     * @private
     */
    function notify() {
        if (messageProcessors > 0 && started) {
            var nextMessage = messageQueue.shift();
            if (nextMessage === undefined) {
                return;
            }
            postMessage(nextMessage);
        }
    }

    /**
     * This is a function introduced to ensure the expiry timer is correctly set. In sporadic cases,
     * it might happen that the expiryTimer is triggered to early, thus preventing the joynr message
     * from being expired at that point in time. Thus, this helper method ensures that for this
     * sporadic case, yet another expiryTimer is created, which ensures the queued message is really
     * expired.
     *
     * @name ChannelMessagingSender#createExpiryTimer
     * @function
     * @private
     */
    function createExpiryTimer(queuedMessage) {
        if (queuedMessage.expiryTimer !== undefined) {
            LongTimer.clearTimeout(queuedMessage.expiryTimer);
            queuedMessage.expiryTimer = undefined;
        }

        function createExpiryTimerTimeout() {
            if (!checkIfExpired(queuedMessage)) {
                createExpiryTimer(queuedMessage);
            }
        }
        queuedMessage.expiryTimer = LongTimer.setTimeout(
            createExpiryTimerTimeout,
            Math.max(0, getRelativeExpiryDate(queuedMessage.message))
        );
    }

    /**
     * Sends a JoynrMessage to the given channel Id.
     *
     * @name ChannelMessagingSender#send
     * @function
     * @param {JoynrMessage}
     *            joynrMessage - message to be sent. Must be an instanceof JoynrMessage.
     * @param {String}
     *            toChannelAddress - channel address of recipient.
     * @returns {Object} a promise object for async event handling
     */
    this.send = function send(joynrMessage, toChannelAddress) {
        if (!(joynrMessage instanceof JoynrMessage)) {
            return Promise.reject(
                new Error("CANNOT SEND: invalid joynrMessage which is of type " + Typing.getObjectType(joynrMessage))
            );
        }

        if (terminated) {
            return Promise.reject(new Error("ChannelMessagingSender is already shut down"));
        }
        var deferred = Util.createDeferred();
        var queuedMessage = {
            message: joynrMessage,
            to: toChannelAddress.messagingEndpointUrl + "messageWithoutContentType/",
            resolve: deferred.resolve,
            reject: deferred.reject,
            pending: true,
            expiryTimer: undefined
        };
        createExpiryTimer(queuedMessage);
        messageQueue.push(queuedMessage);
        LongTimer.setTimeout(notify, 0);

        return deferred.promise;
    };

    /**
     * Resend in the event of an error, and call the error callback if the ttl is expired.
     *
     * @name ChannelMessagingSender#resend
     * @function
     * @private
     *
     * @param {JoynrMessage}
     * queuedMessage
     */
    function resend(queuedMessage) {
        if (terminated) {
            queuedMessage.reject(new Error("ChannelMessagingSender is already shut down"));
            return;
        }

        if (checkIfExpired(queuedMessage)) {
            return;
        }

        function resendTimeout() {
            delete queuedMessage.resendTimer;
            postMessage(queuedMessage);
        }

        // resend the message
        queuedMessage.resendTimer = LongTimer.setTimeout(resendTimeout, resendDelay_ms);
    }

    /**
     * Shutdown the channel messaging sender
     *
     * @function
     * @name ChannelMessagingSender#shutdown
     */
    this.shutdown = function shutdown() {
        function handleQueuedMessage(queuedMessage) {
            if (queuedMessage.expiryTimer !== undefined) {
                LongTimer.clearTimeout(queuedMessage.expiryTimer);
            }
            if (queuedMessage.resendTimer !== undefined) {
                LongTimer.clearTimeout(queuedMessage.resendTimer);
            }
            queuedMessage.reject(new Error("ChannelMessagingSender is already shut down"));
        }

        messageQueue.forEach(handleQueuedMessage);
        messageQueue = [];
        terminated = true;
    };
}

module.exports = ChannelMessagingSender;
