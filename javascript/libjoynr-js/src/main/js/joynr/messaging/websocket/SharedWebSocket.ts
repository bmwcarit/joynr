/* eslint-disable @typescript-eslint/no-non-null-assertion */
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
/*eslint no-use-before-define: "off"*/
import util from "util";
import * as MessageSerializer from "../MessageSerializer";
import * as WebSocketAddress from "../../../generated/joynr/system/RoutingTypes/WebSocketAddress";
import * as WebSocketClientAddress from "../../../generated/joynr/system/RoutingTypes/WebSocketClientAddress";

import WebSocketNode = require("../../../global/WebSocketNode");
import LongTimer from "../../util/LongTimer";
import LoggingManager from "../../system/LoggingManager";
import JoynrMessage = require("../JoynrMessage");

const log = LoggingManager.getLogger("joynr.messaging.websocket.SharedWebSocket");

/**
 * @param address
 * @param address to which messages are sent on the clustercontroller.
 * @returns a url
 */
function webSocketAddressToUrl(address: WebSocketAddress): string {
    let url = `${address.protocol.name.toLowerCase()}://${address.host}:${address.port}`;
    if (address.path) {
        url += address.path;
    }
    return url;
}

interface SharedWebSocketSettings {
    localAddress: WebSocketClientAddress;
    remoteAddress: WebSocketAddress;
    provisioning: { reconnectSleepTimeMs?: number };
    keychain: any;
}

class SharedWebSocket {
    public static EVENT_CODE_SHUTDOWN = 4000;

    /**
       util.promisify creates the callback which will be called by ws once the message was sent.
       When the callback is called with an error it will reject the promise, otherwise resolve it.
       The arrow function is there to assure that websocket.send is called with websocket as its this context.
     */
    private webSocketSendAsync: (marshalledMessage: Buffer) => Promise<void>;

    private sendConfig = { binary: true };
    private reconnectTimer: any;
    private closed = false;
    private queuedMessages: any = [];
    private onmessageCallback: any = null;
    private remoteUrl: any;
    private localAddress: WebSocketClientAddress;
    private reconnectSleepTimeMs: number; // default value = 1000ms
    private websocket: WebSocketNode | null = null;
    private keychain: any;
    /**
     * @constructor
     * @param settings
     * @param settings.localAddress address used by the websocket server to contact this
     *            client. This address is used in in the init phase on the websocket, and
     *            then registered with the message router on the remote side
     * @param settings.remoteAddress to which messages are sent on the websocket server.
     * @param settings.provisioning
     * @param settings.provisioning.reconnectSleepTimeMs
     * @param settings.keychain
     */
    public constructor(settings: SharedWebSocketSettings) {
        settings.provisioning = settings.provisioning || {};
        this.reconnectSleepTimeMs = settings.provisioning.reconnectSleepTimeMs || 1000;
        this.localAddress = settings.localAddress;
        this.remoteUrl = webSocketAddressToUrl(settings.remoteAddress);
        this.webSocketSendAsync = util.promisify((marshaledMessage: Buffer, cb: any) =>
            this.websocket!.send(marshaledMessage, this.sendConfig, cb)
        );
        this.keychain = settings.keychain;

        this.onOpen = this.onOpen.bind(this);
        this.onClose = this.onClose.bind(this);
        this.onError = this.onError.bind(this);
        this.resetConnection = this.resetConnection.bind(this);

        this.resetConnection();
    }

    public get onmessage(): Function {
        return this.onmessageCallback;
    }
    public set onmessage(newCallback: Function) {
        this.onmessageCallback = (data: any) => {
            try {
                const joynrMessage = MessageSerializer.parse(data.data as Buffer);
                if (joynrMessage) {
                    newCallback(joynrMessage);
                }
            } catch (e) {
                log.error(`could not unmarshal joynrMessage: ${e}`);
            }
        };
        this.websocket!.onmessage = this.onmessageCallback;
    }

    public get numberOfQueuedMessages(): number {
        return this.queuedMessages.length;
    }

    private sendQueuedMessages(): void {
        while (this.queuedMessages.length) {
            const queued = this.queuedMessages.shift();
            try {
                this.websocket!.send(queued, { binary: true });
                // Error is thrown if the socket is no longer open
            } catch (e) {
                // so add the message back to the front of the queue
                this.queuedMessages.unshift(queued);
                throw e;
            }
        }
    }

    private resetConnection(): void {
        this.reconnectTimer = undefined;
        if (this.closed) {
            return;
        }
        this.websocket = new WebSocketNode(this.remoteUrl, this.keychain);
        this.websocket.onopen = this.onOpen;
        this.websocket.onclose = this.onClose;
        this.websocket.onerror = this.onError;
        if (this.onmessageCallback !== null) {
            this.websocket.onmessage = this.onmessageCallback;
        }
    }

    private onError(event: any): void {
        if (this.closed) {
            return;
        }
        log.error(`error in websocket: ${util.inspect(event)}. Resetting connection`);
        if (this.reconnectTimer !== undefined) {
            LongTimer.clearTimeout(this.reconnectTimer);
        }
        if (this.websocket) {
            this.websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
            this.websocket = null;
        }
        this.reconnectTimer = LongTimer.setTimeout(this.resetConnection, this.reconnectSleepTimeMs);
    }

    private onClose(event: { wasClean: boolean; code: number; reason: string; target: any }): void {
        if (this.closed) {
            return;
        }
        if (event.code !== SharedWebSocket.EVENT_CODE_SHUTDOWN) {
            log.info(
                `connection closed unexpectedly. code: ${event.code} reason: ${event.reason}. Trying to reconnect...`
            );
            if (this.reconnectTimer !== undefined) {
                LongTimer.clearTimeout(this.reconnectTimer);
            }
            if (this.websocket) {
                this.websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
                this.websocket = null;
            }
            this.reconnectTimer = LongTimer.setTimeout(this.resetConnection, this.reconnectSleepTimeMs);
        } else {
            log.info(`connection closed. reason: ${event.reason}`);
            if (this.websocket) {
                this.websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
                this.websocket = null;
            }
        }
    }

    /* send all queued messages, requeuing to the front in case of a problem*/
    private onOpen(): void {
        try {
            log.debug("connection opened.");
            this.websocket!.send(Buffer.from(JSON.stringify(this.localAddress)), this.sendConfig);
            this.sendQueuedMessages();
        } catch (e) {
            this.resetConnection();
        }
    }

    public async sendInternal(marshaledMessage: Buffer): Promise<void> {
        this.websocket!.send(marshaledMessage, this.sendConfig);
    }

    public async sendMessage(joynrMessage: JoynrMessage): Promise<void> {
        let marshaledMessage;
        try {
            marshaledMessage = MessageSerializer.stringify(joynrMessage);
        } catch (e) {
            log.error(`could not marshal joynrMessage: ${joynrMessage.msgId} ${e}`);
            return Promise.resolve();
        }
        if (this.websocket !== null && this.websocket!.readyState === WebSocketNode.WSN_OPEN) {
            try {
                await this.sendInternal(marshaledMessage);
                // Error is thrown if the socket is no longer open, so requeue to the front
            } catch (e) {
                // add the message back to the front of the queue
                this.queuedMessages.unshift(marshaledMessage);
                log.error(`could not send joynrMessage: ${joynrMessage.msgId} requeuing message. Error: ${e}`);
            }
        } else {
            // push new messages onto the back of the queue
            this.queuedMessages.push(marshaledMessage);
        }
    }

    /**
     * @param joynrMessage the joynr message to transmit
     */
    public send(joynrMessage: JoynrMessage): Promise<void> {
        log.debug(`>>> OUTGOING >>> message with ID ${joynrMessage.msgId}`);
        return this.sendMessage(joynrMessage);
    }

    /**
     * Normally the SharedWebsocket.send api automatically resolves the Promise
     * when it's called. But this doesn't mean that the data was actually
     * written out. This method is a helper for a graceful shutdown which delays
     * the resolving of the SharedWebSocket.send Promise till the data is
     * written out, to make sure that unsubscribe messages are successfully sent.
     */
    public enableShutdownMode(): void {
        this.sendInternal = this.webSocketSendAsync;
    }

    public close(): void {
        this.closed = true;
        if (this.reconnectTimer !== undefined) {
            LongTimer.clearTimeout(this.reconnectTimer);
            this.reconnectTimer = undefined;
        }
        if (this.websocket !== null) {
            this.websocket.close(SharedWebSocket.EVENT_CODE_SHUTDOWN, "shutdown");
            this.websocket = null;
        }
    }
}

export = SharedWebSocket;
