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
    "joynr/types/TypeRegistrySingleton",
    "joynr/system/LoggerFactory",
    "joynr/system/RoutingTypes/ChannelAddress"
], function(Typing, TypeRegistrySingleton, LoggerFactory, ChannelAddress) {

    /**
     * @name ChannelMessagingSkeleton
     * @constructor
     *
     * @param {Function}
     *            receiveFunction
     */
    function ChannelMessagingSkeleton(settings) {
        var log = LoggerFactory.getLogger("joynr/messaging/channel/ChannelMessagingSkeleton");

        Typing.checkProperty(settings, "Object", "settings");
        if (settings.messageRouter === undefined) {
            throw new Error("messageRouter is undefined");
        }

        var messageRouter = settings.messageRouter;
        var typeRegistry = TypeRegistrySingleton.getInstance();
        // participants from ChannelMessagingSkeleton are always globally visible
        var isGloballyVisible = true;

        /**
         * Lets all listeners receive a message
         *
         * @name ChannelMessagingSkeleton#receiveMessage
         * @function
         *
         * @param {JoynrMessage} joynrMessage
         */
        this.receiveMessage =
                function receiveMessage(joynrMessage) {
                    var replyToAddress;
                    if (joynrMessage.replyChannelId !== undefined) {
                        try {
                            replyToAddress =
                                    Typing.augmentTypes(
                                            JSON.parse(joynrMessage.replyChannelId),
                                            typeRegistry);
                            messageRouter.addNextHop(
                                    joynrMessage.from,
                                    replyToAddress,
                                    isGloballyVisible);
                        } catch (e) {
                            // message dropped if unknown replyTo address type
                            log.error("unable to process message: replyTo address type unknown: "
                                + joynrMessage.replyChannelId);
                            return;
                        }
                    }
                    messageRouter.route(joynrMessage);
                };

    }

    return ChannelMessagingSkeleton;

});