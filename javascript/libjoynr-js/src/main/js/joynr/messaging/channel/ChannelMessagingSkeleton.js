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

define("joynr/messaging/channel/ChannelMessagingSkeleton", [
    "joynr/util/Util",
    "joynr/system/RoutingTypes/ChannelAddress"
], function(Util, ChannelAddress) {

    /**
     * @name ChannelMessagingSkeleton
     * @constructor
     *
     * @param {Function}
     *            receiveFunction
     */
    function ChannelMessagingSkeleton(settings) {

        Util.checkProperty(settings, "Object", "settings");
        if (settings.messageRouter === undefined) {
            throw new Error("messageRouter is undefined");
        }

        var messageRouter = settings.messageRouter;

        /**
         * Lets all listeners receive a message
         *
         * @name ChannelMessagingSkeleton#receiveMessage
         * @function
         *
         * @param {JoynrMessage} joynrMessage
         */
        this.receiveMessage = function receiveMessage(joynrMessage) {
            if (joynrMessage.replyChannelId !== undefined) {
                messageRouter.addNextHop(joynrMessage.from, new ChannelAddress({
                    channelId : joynrMessage.replyChannelId
                }));
            }
            messageRouter.route(joynrMessage);
        };

    }

    return ChannelMessagingSkeleton;

});