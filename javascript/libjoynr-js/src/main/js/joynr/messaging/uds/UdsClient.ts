/* eslint-disable @typescript-eslint/no-non-null-assertion */
/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import UdsClientAddress from "../../../generated/joynr/system/RoutingTypes/UdsClientAddress";
import * as MessageSerializer from "../MessageSerializer";

import LongTimer from "../../util/LongTimer";
import LoggingManager from "../../system/LoggingManager";
const log = LoggingManager.getLogger("joynr.messaging.socket.UdsClient");

import net = require("net");
import MagicCookieUtil from "./MagicCookieUtil";
import util from "util";
import JoynrMessage from "../JoynrMessage";
import * as DiagnosticTags from "../../system/DiagnosticTags";

interface UdsLibJoynrProvisioning {
    socketPath: string;
    clientId: string;
    connectSleepTimeMs: number;
    onMessageCallback: Function;
}

class UdsClient {
    /**
     util.promisify creates the callback which will be called by ws once the message was sent.
     When the callback is called with an error it will reject the promise, otherwise resolve it.
     The arrow function is there to assure that socket.send is called with socket as its this context.
     */
    private readonly udsSendAsync: (marshalledMessage: Buffer) => Promise<void>;

    private encodingConf = { binary: true };
    private reconnectTimer: any;
    private connected: boolean = false;
    private closed: boolean = false;
    private firstMagicCookieFound: boolean = false;
    private queuedMessages: any = [];
    private readonly onMessageCallback: Function;
    private readonly connectSleepTimeMs: number;
    private socket: any;
    private readonly socketPath: string;
    private readonly clientId?: string;
    private internBuff: Buffer;
    private readonly serializedUdsClientAddress: Buffer;

    /**
     * @constructor
     * @param parameters
     * @param parameters.socketPath: path of uds socket to communicate to the server through it
     * @param parameters.clientId: client id
     * @param parameters.connectSleepTimeMs: interval to try to connect to the server when it is unavailable
     * @param parameters.onMessageCallback: callback to be called when a complete message arrives
     */
    public constructor(parameters: UdsLibJoynrProvisioning) {
        this.socketPath = parameters.socketPath;
        this.clientId = parameters.clientId;
        this.connectSleepTimeMs = parameters.connectSleepTimeMs;
        this.onMessageCallback = parameters.onMessageCallback;
        this.internBuff = Buffer.alloc(0, 0);

        this.serializedUdsClientAddress = Buffer.from(
            JSON.stringify(
                new UdsClientAddress({
                    id: this.clientId
                })
            )
        );

        if (this.serializedUdsClientAddress === undefined || this.serializedUdsClientAddress === null) {
            const errorMsg: string = `Error in serializing uds client address ${this.serializedUdsClientAddress}`;
            log.debug(errorMsg);
            throw new Error(errorMsg);
        }

        log.debug(`Client trying to connect to ${this.socketPath} with clientId=${this.clientId} ...`);
        this.udsSendAsync = util.promisify((marshaledMessage: Buffer, cb: any) =>
            this.socket!.write(marshaledMessage, cb, this.encodingConf)
        );

        this.onClose = this.onClose.bind(this);
        this.onConnect = this.onConnect.bind(this);
        this.onReceivedMessage = this.onReceivedMessage.bind(this);
        this.onEnd = this.onEnd.bind(this);
        this.onError = this.onError.bind(this);
        this.establishConnection = this.establishConnection.bind(this);

        this.establishConnection();
    }

    private readonly establishConnection = (): void => {
        this.reconnectTimer = undefined;
        if (this.closed) {
            log.debug(`Connection to the server is closed`);
            return;
        }
        this.socket = net.createConnection({ path: this.socketPath });
        this.socket.on(`connect`, this.onConnect);
        this.socket.on(`close`, this.onClose);
        this.socket.on(`end`, this.onEnd);
        this.socket.on(`error`, this.onError);
        this.socket.on(`data`, this.onReceivedMessage);
    };

    private readonly onConnect = (): void => {
        this.connected = true;
        log.debug(`Connected to ${this.socketPath} with clientId ${this.clientId}`);
        const initMsgBuff: Buffer = MagicCookieUtil.writeMagicCookies(
            this.serializedUdsClientAddress,
            MagicCookieUtil.INIT_COOKIE
        );
        log.debug(
            `Send Init Message: ${MagicCookieUtil.getMagicCookieBuff(
                initMsgBuff
            ).toString()}${MagicCookieUtil.getPayloadLength(initMsgBuff).toString()}${MagicCookieUtil.getPayloadBuff(
                initMsgBuff
            ).toString()}`
        );
        this.socket.write(initMsgBuff, this.encodingConf);

        try {
            this.sendQueuedMessages();
        } catch (e) {
            log.error(`Sending queued messages failed. It fails with ${e}`);
            this.shutdown();
        }
    };

    private readonly onClose = (event: { hadError: boolean }) => {
        if (this.closed) {
            return;
        }

        if (!this.connected) {
            log.debug(`Server is not yet available. Try to connect in ${this.connectSleepTimeMs} ms`);
            this.reconnectTimer = LongTimer.setTimeout(this.establishConnection, this.connectSleepTimeMs);
            return;
        }
        let msg = `Server terminates the connection gracefully. Connection to the socket is closed.`;
        if (event.hadError) {
            msg = `The socket had a transmission error. Connection to the socket is closed.`;
        }
        log.info(msg);
        this.shutdown();
    };

    private readonly onEnd = () => {
        log.debug(`end callback is called`);
        this.shutdown();
    };

    private readonly onError = () => {
        // TODO: Ticket to handle errors
        log.debug(`on error callback is called`);
    };

    private readonly onReceivedMessage = (data: Buffer): void => {
        if (Buffer.byteLength(data) <= 0) {
            // nothing to process.
            return;
        }

        // append what we received to the former received data
        this.internBuff = Buffer.concat([this.internBuff, data]);

        if (!MagicCookieUtil.validateCookie(this.internBuff)) {
            // received little amount of bytes, not enough to process them. continue receiving
            return;
        }
        // we received at least magic cookie and the length of serialized joynr message.
        // we might have received a huge buffer which contains several messages
        // on first reception, scan the buffer to get the magic cookie. If not found, purge the internBuff.
        if (!this.firstMagicCookieFound && !(this.internBuff.indexOf(MagicCookieUtil.MESSAGE_COOKIE) > -1)) {
            this.internBuff = Buffer.alloc(0, 0);
            return;
        }
        if (!this.firstMagicCookieFound && this.internBuff.indexOf(MagicCookieUtil.MESSAGE_COOKIE) > -1) {
            // extract data from magic cookie upwards. everything before is useless
            this.internBuff = this.internBuff.slice(this.internBuff.indexOf(MagicCookieUtil.MESSAGE_COOKIE));
            this.firstMagicCookieFound = true;
        }

        // start processing the received buffer
        while (this.internBuff.length >= MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH) {
            this.processReceivedBuffer();
        }
    };

    private processReceivedBuffer(): void {
        try {
            const serializedJoynrMessageLength: number = MagicCookieUtil.getPayloadLength(this.internBuff);
            if (serializedJoynrMessageLength === 0) {
                // no payload received, adjust internal buffer and continue receiving
                this.internBuff = this.internBuff.slice(
                    MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessageLength
                );
                return;
            }

            if (
                this.internBuff.length <
                MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessageLength
            ) {
                // message is incomplete. Already buffered, continue receiving
                return;
            }

            const receivedPayloadBuff: Buffer = MagicCookieUtil.getPayloadBuff(this.internBuff);

            if (receivedPayloadBuff.length === serializedJoynrMessageLength) {
                // message is complete. no extra bytes received
                this.internBuff = Buffer.alloc(0, 0);
                this.firstMagicCookieFound = false;
                this.processExtractedMessage(receivedPayloadBuff);
                return;
            }

            if (receivedPayloadBuff.length > serializedJoynrMessageLength) {
                // message is complete. intern buffer still has data from the next message.
                this.internBuff = this.internBuff.slice(
                    MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessageLength
                );
                const buff = receivedPayloadBuff.slice(0, serializedJoynrMessageLength);
                this.processExtractedMessage(buff);
                return;
            }
        } catch (e) {
            log.error(`Could not unmarshal joynrMessage: ${e}`);
        }
    }

    private processExtractedMessage(serializedJoynrMessage: Buffer): void {
        log.debug(`Received joynr serialized message: ${serializedJoynrMessage.toString()}`);
        const joynrMessage = MessageSerializer.parse(serializedJoynrMessage);
        if (joynrMessage && this.onMessageCallback !== null) {
            this.onMessageCallback(joynrMessage);
        }
    }

    public get numberOfQueuedMessages(): number {
        return this.queuedMessages.length;
    }

    private sendQueuedMessages(): void {
        log.debug(`Sending queued messages if any: ${this.queuedMessages.length} queued.`);
        while (this.queuedMessages.length) {
            const queuedMessage = this.queuedMessages.shift();
            const queuedMessageBuff: Buffer = Buffer.from(queuedMessage);
            this.socket.write(
                MagicCookieUtil.writeMagicCookies(queuedMessageBuff, MagicCookieUtil.MESSAGE_COOKIE),
                this.encodingConf
            );
        }
    }

    /**
     * @param joynrMessage the joynr message to transmit
     */
    public send(joynrMessage: JoynrMessage): Promise<void> {
        log.info(`>>> OUTGOING >>> message: ${joynrMessage}`);
        return this.sendMessage(joynrMessage);
    }

    private async sendMessage(joynrMessage: JoynrMessage): Promise<void> {
        let marshaledMessage;
        try {
            marshaledMessage = MessageSerializer.stringify(joynrMessage);
        } catch (e) {
            log.error(
                `Could not marshal joynrMessage: ${DiagnosticTags.forJoynrMessage(joynrMessage)}. It failed with ${e}`
            );
            return Promise.resolve();
        }

        if (this.socket !== null && this.connected === true && this.socket!.readyState === "open") {
            try {
                await this.sendInternal(marshaledMessage);
            } catch (e) {
                log.error(`Could not send joynrMessage: ${DiagnosticTags.forJoynrMessage(joynrMessage)}. Error: ${e}.`);
                return Promise.resolve();
            }
        } else if (!this.closed) {
            // push new messages on to the back of the queue
            log.debug(`Push new messages to the back of the queue`);
            this.queuedMessages.push(marshaledMessage);
        } else {
            log.error(
                `Dropping message because connection is already closed: ${DiagnosticTags.forJoynrMessage(joynrMessage)}`
            );
        }
    }

    private async sendInternal(marshaledMessage: Buffer): Promise<void> {
        try {
            this.socket!.write(
                MagicCookieUtil.writeMagicCookies(marshaledMessage, MagicCookieUtil.MESSAGE_COOKIE),
                this.encodingConf
            );
        } catch (e) {
            log.error(`Catching an exception in sendInternal ${e}`);
            return Promise.resolve();
        }
    }

    /**
     * Normally the UdsClient.write api automatically resolves the Promise
     * when it's called. But this doesn't mean that the data was actually
     * written out. This method is a helper for a graceful shutdown which delays
     * the resolving of the UdsClient.send Promise till the data is
     * written out, to make sure that unsubscribe messages are successfully sent.
     */
    public enableShutdownMode(): void {
        this.sendInternal = this.udsSendAsync;
    }

    /**
     * Ends connection to the socket
     */
    public shutdown(callback?: Function): void {
        log.info(`shutdown of uds client is invoked`);
        this.closed = true;
        if (this.reconnectTimer !== undefined) {
            LongTimer.clearTimeout(this.reconnectTimer);
            this.reconnectTimer = undefined;
        }
        if (this.socket) {
            this.socket.end(callback);
            delete this.socket;
            this.socket = null;
        }
    }
}

export = UdsClient;
