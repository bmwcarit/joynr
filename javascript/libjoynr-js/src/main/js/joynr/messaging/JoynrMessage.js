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

define("joynr/messaging/JoynrMessage", [
    "joynr/util/UtilInternal",
    "uuid"
], function(Util, uuid) {

    /**
     * @name JoynrMessage
     * @constructor
     *
     * @param {String}
     *            messageType the message type as defined by JoynrMessage.JOYNRMESSAGE_TYPE_*
     */
    function JoynrMessage(messageType) {
        var i, headerProperty;

        /**
         * The joynr type name
         *
         * @name JoynrMessage#_typeName
         * @type String
         * @field
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.JoynrMessage");

        /**
         * The message type as defined by JoynrMessage.JOYNRMESSAGE_TYPE_*
         *
         * @name JoynrMessage#type
         * @type String
         * @field
         */
        Util.objectDefineProperty(this, "type", messageType);

        /**
         * The message header holding additional values
         *
         * @name JoynrMessage#header
         * @type Object
         * @field
         */
        Util.objectDefineProperty(this, "header", {});

        /**
         * The serialized message payload
         *
         * @name JoynrMessage#payload
         * @type String
         * @field
         */
        this.payload = "";

        /**
         * @name JoynrMessage#setHeader
         * @function
         *
         * @param {String}
         *            key is one of the header keys defined in JoynrMessagingDefines
         * @param {Any}
         *            value of the header
         * @returns {JoynrMessage}
         */
        Object.defineProperty(this, "setHeader", {
            enumerable : false,
            configurable : false,
            writable : false,
            value : function(key, value) {
                this.header[key] = value;
                return this;
            }
        });

        var headerProperties = [
            JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID,
            JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID,
            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
            JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
            JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE
        ];

        function constructGetter(header, property) {
            return function() {
                return header[property];
            };
        }

        function constructSetter(header, property) {
            return function(value) {
                header[property] = value;
            };
        }

        /**
         * The user ID of the message creator
         *
         * @name JoynrMessage#creator
         * @type String
         * @field
         */
        /**
         * The participant id the message is from
         *
         * @name JoynrMessage#from
         * @type String
         * @field
         */
        /**
         * The participant id the message is to
         *
         * @name JoynrMessage#to
         * @type String
         * @field
         */
        /**
         * The reply channel Id to return response messages to
         *
         * @name JoynrMessage#replyChannelId
         * @type String
         * @field
         */
        /**
         * The expiry date of the message
         *
         * @name JoynrMessage#expiryDate
         * @type String
         * @field
         */
        for (i = 0; i < headerProperties.length; ++i) {
            headerProperty = headerProperties[i];
            Object.defineProperty(this, headerProperty, {
                set : constructSetter(this.header, headerProperty),
                get : constructGetter(this.header, headerProperty)
            });
        }

        this.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE, "application/json");
        if (this.JOYNRMESSAGE_HEADER_MESSAGE_ID === undefined) {
            this.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID, uuid());
        }
    }

    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY = "oneWay";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST = "request";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_REPLY
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_REPLY = "reply";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST = "broadcastSubscriptionRequest";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP
     */
    JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID = "msgId";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID = "creator";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE = "expiryDate";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID = "replyChannelId";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE = "contentType";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_SUBSCRIPTION_ATTRIBUTE
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_SUBSCRIPTION_ATTRIBUTE = "subscriptionAttribute";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID = "to";
    /**
     * @static
     * @readonly
     * @type String
     * @name JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID
     */
    JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID = "from";

    return JoynrMessage;

});