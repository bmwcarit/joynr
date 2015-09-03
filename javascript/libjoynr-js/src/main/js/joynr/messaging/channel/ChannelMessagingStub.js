/*jslint es5: true */

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

define("joynr/messaging/channel/ChannelMessagingStub", [
    "global/Promise",
    "joynr/util/UtilInternal"
], function(Promise, Util) {

    /**
     * @name ChannelMessagingStub
     * @constructor
     *
     * @param {Object} settings
     * @param {ChannelMessagingSender|Object} settings.channelMessagingSender the channel message sender to send the messages with
     * @param {String} settings.channelId the destination channelId
     */
    function ChannelMessagingStub(settings) {
        Util.checkProperty(settings.channelMessagingSender, [
            "Object",
            "ChannelMessagingSender"
        ], "settings.channelMessagingSender");
        Util.checkProperty(settings.destChannelId, "String", "settings.destChannelId");
        Util.checkProperty(settings.myChannelId, "String", "settings.myChannelId");

        /**
         * @name ChannelMessagingStub#transmit
         * @function
         *
         * @param {JoynrMessage} message the message to transmit
         */
        this.transmit =
                function transmit(joynrMessage) {
                    if (settings.destChannelId === settings.myChannelId) {
                        var errorMsg =
                                "Discarding message "
                                    + joynrMessage.msgId
                                    + ": message marked as outgoing, but channelId "
                                    + settings.destChannelId
                                    + " is the local channelId.";
                        return Promise.reject(errorMsg);
                    }
                    // if outgoing request => set my own channelId as replyChannelId
                    joynrMessage.replyChannelId = settings.myChannelId;
                    return settings.channelMessagingSender.send(
                            joynrMessage,
                            settings.destChannelId);
                };
    }

    return ChannelMessagingStub;
});