/*jslint es5: true, nomen: true */

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

define("joynr/messaging/channel/ChannelMessagingSkeleton", [
    "joynr/util/Typing",
    "joynr/system/LoggerFactory",
    "joynr/system/DiagnosticTags",
    "joynr/exceptions/JoynrException",
    "joynr/messaging/JoynrMessage"
], function(Typing, LoggerFactory, DiagnosticTags, JoynrException, JoynrMessage) {

    var log = LoggerFactory.getLogger("joynr/messaging/channel/ChannelMessagingSkeleton");

    /**
     * @name ChannelMessagingSkeleton
     * @constructor
     *
     * @param {Function}
     *            receiveFunction
     */
    function ChannelMessagingSkeleton(settings) {

        Typing.checkProperty(settings, "Object", "settings");
        if (settings.messageRouter === undefined) {
            throw new Error("messageRouter is undefined");
        }

        this._messageRouter = settings.messageRouter;

    }

    /**
     * Lets all listeners receive a message
     *
     * @name ChannelMessagingSkeleton#receiveMessage
     * @function
     *
     * @param {JoynrMessage} joynrMessage
     */
    ChannelMessagingSkeleton.prototype.receiveMessage =
            function receiveMessage(joynrMessage) {
                joynrMessage = new JoynrMessage(joynrMessage);
                joynrMessage.setReceivedFromGlobal(true);
                try {
                    this._messageRouter.route(joynrMessage);
                } catch (e) {
                    log.error("unable to process message: "
                        + e
                        + (e instanceof JoynrException ? " " + e.detailMessage : "")
                        + " \nmessage: "
                        + DiagnosticTags.forJoynrMessage(joynrMessage));
                }
            };

    return ChannelMessagingSkeleton;

});