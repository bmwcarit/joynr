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
import util from "util";

import mqtt from "mqtt";
import * as MqttAddress from "../../../generated/joynr/system/RoutingTypes/MqttAddress";

import * as MessagingQosEffort from "../MessagingQosEffort";
import * as Typing from "../../util/Typing";
import * as MessageSerializer from "../MessageSerializer";
import * as UtilInternal from "../../util/UtilInternal";
import JoynrMessage = require("../JoynrMessage");
import LoggingManager = require("../../system/LoggingManager");

const log = LoggingManager.getLogger("joynr/messaging/routing/MessageRouter");

interface Queued {
    message: JoynrMessage;
    resolve: Function;
    options: {
        qos: mqtt.QoS;
    };
    topic: string;
}

class SharedMqttClient {
    public static BEST_EFFORT_QOS_LEVEL: mqtt.QoS = 0;
    public static DEFAULT_QOS_LEVEL: mqtt.QoS = 1;
    private queuedUnsubscriptions: string[] = [];
    private queuedSubscriptions: string[] = [];
    private qosLevel: mqtt.QoS;
    private onConnectedDeferred: UtilInternal.Deferred;
    private connected = false;
    private closed = false;
    private queuedMessages: Queued[] = [];
    private onmessageCallback?: Function;
    private address: MqttAddress;
    private client!: mqtt.Client;
    /**
     * @constructor
     * @param settings
     * @param settings.address to be used to connect to mqtt broker
     */
    public constructor(settings: { address: MqttAddress; provisioning: { qosLevel?: mqtt.QoS } }) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.address, "MqttAddress", "settings.address");

        this.address = settings.address;
        this.onConnectedDeferred = UtilInternal.createDeferred();

        this.qosLevel =
            settings.provisioning.qosLevel !== undefined
                ? settings.provisioning.qosLevel
                : SharedMqttClient.DEFAULT_QOS_LEVEL;

        this.onOpen = this.onOpen.bind(this);
        this.onMessage = this.onMessage.bind(this);

        this.resetConnection();
    }

    public set onmessage(newCallback: Function) {
        this.onmessageCallback = newCallback;
        if (typeof newCallback !== "function") {
            throw new Error(`onmessage callback must be a function, but instead was of type ${typeof newCallback}`);
        }
    }

    public get onmessage(): Function {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return this.onmessageCallback!;
    }

    private onMessage(topic: string, payload: Buffer): void {
        if (this.onmessageCallback !== undefined) {
            this.onmessageCallback(topic, MessageSerializer.parse(payload));
        }
    }

    private onError(error: Error): void {
        log.error(`MqttClient emitted error ${error}`);
    }

    private resetConnection(): void {
        if (this.closed) {
            return;
        }
        this.client = mqtt.connect(this.address.brokerUri);
        this.client.on("connect", this.onOpen);
        this.client.on("message", this.onMessage);
        this.client.on("error", this.onError);
    }

    public onConnected(): Promise<any> {
        return this.onConnectedDeferred.promise;
    }

    private sendQueuedUnsubscriptions(): void {
        for (let i = 0; i < this.queuedUnsubscriptions.length; i++) {
            this.client.unsubscribe(this.queuedUnsubscriptions[i]);
        }
        this.queuedUnsubscriptions = [];
    }

    private sendQueuedMessages(): void {
        while (this.queuedMessages.length) {
            const queued = this.queuedMessages.shift() as Queued;
            try {
                this.client.publish(queued.topic, MessageSerializer.stringify(queued.message), queued.options);
                queued.resolve();
                // Error is thrown if the connection is no longer open
            } catch (e) {
                // so add the message back to the front of the queue
                this.queuedMessages.unshift(queued);
                throw e;
            }
        }
    }

    private sendQueuedSubscriptions(): void {
        const subscribeObject: mqtt.ISubscriptionMap = {};
        for (let i = 0; i < this.queuedSubscriptions.length; i++) {
            const topic = this.queuedSubscriptions[i];
            subscribeObject[topic] = { qos: this.qosLevel };
        }
        this.queuedSubscriptions = [];
        this.client.subscribe(subscribeObject, (err: Error, _granted: mqtt.ISubscriptionGrant[]) => {
            if (err) {
                log.error(`Error subscribing to topics ${util.inspect(subscribeObject)}`);
                this.client.reconnect();
            } else {
                log.info("onConnection resolved");
                this.onConnectedDeferred.resolve();
            }
        });
    }

    /* send all queued messages, requeuing to the front in case of a problem*/
    private onOpen(): void {
        try {
            this.connected = true;
            this.sendQueuedMessages();
            this.sendQueuedUnsubscriptions();
            this.sendQueuedSubscriptions();
        } catch (e) {
            log.error("Error opening MqttClient connection");
            this.resetConnection();
        }
    }

    /**
     * @param topic the topic to publish the message
     * @param joynrMessage the joynr message to transmit
     */
    public send(topic: string, joynrMessage: JoynrMessage): Promise<any> {
        let sendQosLevel = this.qosLevel;
        if (MessagingQosEffort.BEST_EFFORT.value === joynrMessage.effort) {
            sendQosLevel = SharedMqttClient.BEST_EFFORT_QOS_LEVEL;
        }

        const deferred = UtilInternal.createDeferred();
        try {
            this.client.publish(topic, MessageSerializer.stringify(joynrMessage), {
                qos: sendQosLevel
            });
            deferred.resolve();
            // Error is thrown if the socket is no longer open, so requeue to the front
        } catch (e) {
            // add the message back to the front of the queue
            this.queuedMessages.unshift({
                message: joynrMessage,
                resolve: deferred.resolve,
                options: {
                    qos: sendQosLevel
                },
                topic
            });
            log.error(`failed to send Message ${util.inspect(joynrMessage)} due to ${e}`);
            this.client.reconnect();
        }

        return deferred.promise;
    }

    public close(): void {
        log.info("SharedMqttClient::onClose");
        this.closed = true;
        if (this.client && this.client.end) {
            this.client.end();
        }
    }

    public subscribe(topic: string): void {
        if (this.connected) {
            this.client.subscribe(topic, { qos: this.qosLevel });
        } else {
            this.queuedSubscriptions.push(topic);
        }
    }

    public unsubscribe(topic: string): void {
        if (this.connected) {
            this.client.unsubscribe(topic);
        } else {
            this.queuedUnsubscriptions.push(topic);
        }
    }
}

export = SharedMqttClient;
