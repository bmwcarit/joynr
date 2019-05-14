/*eslint no-unused-vars: "off"*/
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
const mqtt = require("mqtt");
const MessagingQosEffort = require("../MessagingQosEffort");
const Typing = require("../../util/Typing");
const MessageSerializer = require("../MessageSerializer");
const UtilInternal = require("../../util/UtilInternal");

/**
 * @param {mqtt}
 *            client to use for sending messages
 * @param {Array}
 *            queuedMessages
 */
function sendQueuedMessages(client, queuedMessages) {
    let queued;
    while (queuedMessages.length) {
        queued = queuedMessages.shift();
        try {
            client.publish(queued.topic, MessageSerializer.stringify(queued.message), queued.options);
            queued.resolve();
            // Error is thrown if the connection is no longer open
        } catch (e) {
            // so add the message back to the front of the queue
            queuedMessages.unshift(queued);
            throw e;
        }
    }
}

function sendQueuedUnsubscriptions(client, queuedUnsubscriptions) {
    let i;
    for (i = 0; i < queuedUnsubscriptions.length; i++) {
        client.unsubscribe(queuedUnsubscriptions[i]);
    }
    queuedUnsubscriptions = [];
}

function sendQueuedSubscriptions(client, queuedSubscriptions, qosLevel, sendFinishedCb) {
    const subscribeObject = {};
    for (let i = 0; i < queuedSubscriptions.length; i++) {
        const topic = queuedSubscriptions[i];
        subscribeObject[topic] = qosLevel;
    }
    client.subscribe(subscribeObject, undefined, (err, granted) => {
        //TODO error handling
        queuedSubscriptions = [];
        sendFinishedCb();
    });
}

function sendMessage(client, topic, joynrMessage, sendQosLevel, queuedMessages) {
    const deferred = UtilInternal.createDeferred();
    try {
        client.publish(topic, MessageSerializer.stringify(joynrMessage), {
            qos: sendQosLevel
        });
        deferred.resolve();
        // Error is thrown if the socket is no longer open, so requeue to the front
    } catch (e) {
        // add the message back to the front of the queue
        queuedMessages.unshift({
            message: joynrMessage,
            resolve: deferred.resolve,
            options: {
                qos: sendQosLevel
            },
            topic
        });
        throw e;
    }

    return deferred.promise;
}

/**
 * @name SharedMqttClient
 * @constructor
 * @param {Object}
 *            settings
 * @param {MqttAddress}
 *            settings.address to be used to connect to mqtt broker
 */
const SharedMqttClient = function SharedMqttClient(settings) {
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.address, "MqttAddress", "settings.address");

    this._client = null;
    this._address = settings.address;
    this._onmessageCallback = null;
    this._queuedMessages = [];
    this._closed = false;
    this._connected = false;
    this._onConnectedDeferred = UtilInternal.createDeferred();

    this._qosLevel =
        settings.provisioning.qosLevel !== undefined
            ? settings.provisioning.qosLevel
            : SharedMqttClient.DEFAULT_QOS_LEVEL;

    this._queuedSubscriptions = [];
    this._queuedUnsubscriptions = [];

    // using the defineProperty syntax for onmessage to be able to keep
    // the same API as WebSocket but have a setter function called when
    // the attribute is set.
    Object.defineProperty(this, "onmessage", {
        set(newCallback) {
            this._onmessageCallback = newCallback;
            if (typeof newCallback !== "function") {
                throw new Error(`onmessage callback must be a function, but instead was of type ${typeof newCallback}`);
            }
        },
        get() {
            return this._onmessageCallback;
        },
        enumerable: false,
        configurable: false
    });
    this._resetConnection();
};

SharedMqttClient.prototype._onMessage = function(topic, payload) {
    if (this._onmessageCallback !== undefined) {
        this._onmessageCallback(topic, MessageSerializer.parse(payload));
    }
};

SharedMqttClient.prototype._resetConnection = function resetConnection() {
    if (this._closed) {
        return;
    }
    this._client = new mqtt.connect(this._address.brokerUri);
    this._client.on("connect", this._onOpen.bind(this));
    this._client.on("message", this._onMessage.bind(this));
};

SharedMqttClient.prototype.onConnected = function() {
    return this._onConnectedDeferred.promise;
};

// send all queued messages, requeuing to the front in case of a problem
SharedMqttClient.prototype._onOpen = function onOpen() {
    try {
        this._connected = true;
        sendQueuedMessages(this._client, this._queuedMessages);
        sendQueuedUnsubscriptions(this._client, this._queuedUnsubscriptions);
        sendQueuedSubscriptions(
            this._client,
            this._queuedSubscriptions,
            this._qosLevel,
            this._onConnectedDeferred.resolve
        );
    } catch (e) {
        this._resetConnection();
    }
};

/**
 * @name SharedMqttClient#send
 * @function
 * @param {String}
 *            topic the topic to publish the message
 * @param {JoynrMessage}
 *            joynrMessage the joynr message to transmit
 */
SharedMqttClient.prototype.send = function send(topic, joynrMessage) {
    let sendQosLevel = this._qosLevel;
    if (MessagingQosEffort.BEST_EFFORT === joynrMessage.effort) {
        sendQosLevel = SharedMqttClient.BEST_EFFORT_QOS_LEVEL;
    }

    return sendMessage(this._client, topic, joynrMessage, sendQosLevel, this._queuedMessages).catch(
        this._resetConnection
    );
};

/**
 * @name SharedMqttClient#close
 * @function
 */
SharedMqttClient.prototype.close = function close() {
    this._closed = true;
    if (this._client !== null && this._client.end) {
        this._client.end();
    }
};

SharedMqttClient.prototype.subscribe = function(topic) {
    if (this._connected) {
        this._client.subscribe(topic, { qos: this._qosLevel });
    } else {
        this._queuedSubscriptions.push(topic);
    }
};

SharedMqttClient.prototype.unsubscribe = function(topic) {
    if (this._connected) {
        this._client.unsubscribe(topic);
    } else {
        this._queuedUnsubscriptions.push(topic);
    }
};

SharedMqttClient.DEFAULT_QOS_LEVEL = 1;
SharedMqttClient.BEST_EFFORT_QOS_LEVEL = 0;

module.exports = SharedMqttClient;
