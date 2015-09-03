/*jslint es5: true, undef: true */// problem with functions postMessage and resend calling each other => it is not possible to bring them in an order without having the jslint "was not defined" error 

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

define("joynr/messaging/channel/ChannelMessagingSender", [
    "global/Promise",
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "joynr/messaging/JoynrMessage",
    "joynr/util/LongTimer",
    "joynr/system/DiagnosticTags",
    "joynr/system/LoggerFactory"
], function(Promise, Util, Typing, JoynrMessage, LongTimer, DiagnosticTags, LoggerFactory) {

    /**
     * ChannelMessagingSender sends JoynrMessages to their destinations via Http
     *
     * @constructor
     * @name ChannelMessagingSender
     * @param {Object}
     *            settings the settings object holding the dependencies
     * @param {LocalChannelUrlDirectory}
     *            settings.channelUrlDirectory
     */
    function ChannelMessagingSender(settings) {
        var log = LoggerFactory.getLogger("joynr.messaging.ChannelMessagingSender");
        var messageQueue = []; // use push to add at the back, and shift to take from the front
        var messageProcessors = settings.channelQos && settings.channelQos.messageProcessors ? settings.channelQos.messageProcessors : 4;
        var resendDelay_ms = settings.channelQos && settings.channelQos.resendDelay_ms ? settings.channelQos.resendDelay_ms : 1000;
        var channelUrlDirectory = settings.channelUrlDirectory;
        var communicationModule = settings.communicationModule;
        var started = false;

        var checkIfExpired = function(queuedMessage){
            if (queuedMessage.pending === false || queuedMessage.expiryTimer === undefined) {
                return true;
            }

            var joynrMessage = queuedMessage.message;
            var isExpired = joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] <= Date.now();
            if (isExpired) {
                queuedMessage.pending = false;
                LongTimer.clearTimeout(queuedMessage.expiryTimer);
                delete queuedMessage.expiryTimer;
                var errMsg = "DISCARDED " + JSON.stringify(queuedMessage) + ": ttl expired";
                log.warn(errMsg);
                queuedMessage.reject(new Error(errMsg));
            }

            return isExpired;
        };

        this.start = function(){
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
            var timeout = queuedMessage.expiryDate - Date.now();

            // XMLHttpRequest uses a timeout of 0 to mean that there is no timeout.
            // ttl = 0 means the opposite (is already expired)
            if (timeout <= 0) {
                timeout = -1;
            }

            messageProcessors--;
            log.info("sending message. timeout in (ms): " + timeout, JSON.stringify(DiagnosticTags
                    .forJoynrMessage(queuedMessage.message)));
            log.trace("sending message. timeout in (ms): "
                + timeout
                + JSON.stringify(queuedMessage.message, undefined, 4));

            communicationModule.createXMLHTTPRequest({
                type : "POST",
                url : queuedMessage.to,
                data : JSON.stringify(queuedMessage.message),
                timeout : timeout
            }).then(
                    function(xhr) {
                        try {
                            log.debug("sending msgId: "
                                + queuedMessage.message.msgId
                                + " completed successfully");
                            queuedMessage.pending = false;
                            LongTimer.clearTimeout(queuedMessage.expiryTimer);
                            delete queuedMessage.expiryTimer;
                            queuedMessage.resolve();
                        } catch (error) {
                            log.error("sending msgId: "
                                + queuedMessage.message.msgId
                                + " deferred caused an exception");
                        }
                        messageProcessors++;
                        notify();
                    }).catch(function(xhr, errorType) {
                        try {
                            log.debug("sending msgId: " + queuedMessage.message.msgId + " failed");
                            resend(queuedMessage);
                        } catch (error) {
                            log.error("sending msgId: "
                                + queuedMessage.message.msgId
                                + " deferred.reject caused an exception");
                        }
                        messageProcessors++;
                        notify();
                    });
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
         * Sends a JoynrMessage to the given channel Id.
         *
         * @name ChannelMessagingSender#send
         * @function
         * @param {JoynrMessage}
         *            joynrMessage - message to be sent. Must be an instanceof JoynrMessage.
         * @param {String}
         *            toChannel - channel Id of recipient.
         * @returns {Object} a promise object for async event handling
         */
        this.send =
                function send(joynrMessage, toChannel) {
                    if (!joynrMessage instanceof JoynrMessage) {
                        return Promise.reject(new Error(
                                "CANNOT SEND: invalid joynrMessage which is of type "
                                    + Typing.getObjectType(joynrMessage)));
                    }

                    return channelUrlDirectory.getUrlsForChannel({
                            channelId : toChannel
                        }).then(function(channelUrlInformation) {
                            return new Promise(function(resolve, reject) {
                                var queuedMessage = {
                                    message : joynrMessage,
                                    to : channelUrlInformation.urls[0]
                                        + "messageWithoutContentType/",
                                    resolve : resolve,
                                    reject : reject,
                                    pending : true,
                                    expiryTimer : LongTimer.setTimeout(function(){
                                        checkIfExpired(queuedMessage);
                                    }, joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] - Date.now())
                                };
                                messageQueue.push(queuedMessage);
                                LongTimer.setTimeout(notify, 0);
                            });
                        }).catch(function(error) {
                            throw new Error("Could not get URL for channel '"
                                + toChannel + "': " + error);
                        });
                };

         /**
         * Resend in the event of an error, and call the error callback if the ttl is
         expired.
         *
         * @name ChannelMessagingSender#resend
         * @function
         * @private
         *
         * @param {JoynrMessage}
         * queuedMessage
         */
         function resend(queuedMessage) {
             if (checkIfExpired(queuedMessage)) {
                 return;
             }

             // resend the message
             LongTimer.setTimeout(function() {
                 postMessage(queuedMessage);
             }, resendDelay_ms);
         }

    }

    return ChannelMessagingSender;

});
