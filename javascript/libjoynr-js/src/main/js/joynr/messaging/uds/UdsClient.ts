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
const log = LoggingManager.getLogger("joynr.messaging.uds.UdsClient");

import net = require("net");
import fs = require("fs");
import MagicCookieUtil from "./MagicCookieUtil";
import util from "util";
import JoynrMessage from "../JoynrMessage";
import * as DiagnosticTags from "../../system/DiagnosticTags";
import JoynrRuntimeException from "../../exceptions/JoynrRuntimeException";

interface UdsLibJoynrProvisioning {
    socketPath: string;
    clientId: string;
    connectSleepTimeMs: number;
    onMessageCallback: Function;
    onFatalRuntimeError: (error: JoynrRuntimeException) => void;
}

class UdsClient {
    /**
     util.promisify creates the callback which will be called by socket.write once the message was sent.
     When the callback is called with an error it will reject the promise, otherwise resolve it.
     The arrow function is there to assure that socket.write is called with socket as its this context.
     */
    private readonly promisifyWriteToSocket: (marshalledMessage: Buffer) => Promise<void>;

    private reconnectTimer: any;
    private connected = false;
    private closed = false;
    private queuedMessages: any = [];
    private readonly onMessageCallback: Function;
    private readonly onFatalRuntimeError: (error: JoynrRuntimeException) => void;
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
     * @param parameters.onFatalRuntimeError: callback to be called when a fatal error in UdsClient happens that
     * prevents further communication via UDS in this runtime
     */
    public constructor(parameters: UdsLibJoynrProvisioning) {
        this.socketPath = parameters.socketPath;
        this.clientId = parameters.clientId;
        this.connectSleepTimeMs = parameters.connectSleepTimeMs;
        this.onMessageCallback = parameters.onMessageCallback;
        this.onFatalRuntimeError = parameters.onFatalRuntimeError;
        this.internBuff = Buffer.alloc(0, 0);

        this.serializedUdsClientAddress = Buffer.from(
            JSON.stringify(
                new UdsClientAddress({
                    id: this.clientId
                })
            )
        );

        if (this.serializedUdsClientAddress === undefined || this.serializedUdsClientAddress === null) {
            const errorMsg = `Error in serializing uds client address ${this.serializedUdsClientAddress}`;
            log.debug(errorMsg);
            throw new Error(errorMsg);
        }

        log.info(`Client trying to connect to ${this.socketPath} with clientId=${this.clientId} ...`);
        this.promisifyWriteToSocket = util.promisify((marshaledMessage: Buffer, cb: any) =>
            this.socket!.write(MagicCookieUtil.writeMagicCookies(marshaledMessage, MagicCookieUtil.MESSAGE_COOKIE), cb)
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
        log.info(`Connected to ${this.socketPath} with clientId ${this.clientId}`);
        const initMsgBuff: Buffer = MagicCookieUtil.writeMagicCookies(
            this.serializedUdsClientAddress,
            MagicCookieUtil.INIT_COOKIE
        );
        log.trace(
            `Send Init Message: ${MagicCookieUtil.getMagicCookieBuff(
                initMsgBuff
            ).toString()}${MagicCookieUtil.getPayloadLength(initMsgBuff).toString()}${MagicCookieUtil.getPayloadBuff(
                initMsgBuff
            ).toString()}`
        );
        this.socket.write(initMsgBuff);

        this.sendQueuedMessages();
    };

    private readonly onClose = (event: { hadError: boolean }): void => {
        if (this.closed) {
            log.info(`UdsClient.onClose called.`);
            return;
        }

        if (!this.connected) {
            let isSocketExist = true;
            try {
                fs.statSync(this.socketPath);
            } catch (e) {
                isSocketExist = false;
            }
            if (isSocketExist) {
                try {
                    fs.accessSync(this.socketPath, fs.constants.R_OK && fs.constants.W_OK);
                } catch (e) {
                    log.fatal(e.message);
                    this.onFatalRuntimeError(
                        new JoynrRuntimeException({
                            detailMessage: `Fatal runtime error, stopping all communication permanently: ${e}`
                        })
                    );
                    this.shutdown();
                    return;
                }
            }

            log.info(`Server is not yet available. Try to connect in ${this.connectSleepTimeMs} ms`);
            this.reconnectTimer = LongTimer.setTimeout(this.establishConnection, this.connectSleepTimeMs);
            return;
        }

        let msg = `Fatal runtime error, stopping all communication permanently: `;
        if (event.hadError) {
            msg += `The socket had a transmission error.`;
        } else {
            msg += `The server terminated the connection.`;
        }
        log.fatal(msg);
        this.onFatalRuntimeError(new JoynrRuntimeException({ detailMessage: msg }));
        this.shutdown();
    };

    private readonly onEnd = (): void => {
        if (this.closed) {
            log.info(`UdsClient.onEnd called.`);
        } else {
            const msg =
                "Fatal runtime error, stopping all communication permanently: The server closed the connection.";
            log.fatal(msg);
            this.onFatalRuntimeError(new JoynrRuntimeException({ detailMessage: msg }));
            this.shutdown();
        }
    };

    private readonly onError = (event: { err: Error }): void => {
        if (event.err) {
            const msg = `UdsClient.onError called: ${event.err}`;
            log.fatal(msg);
            this.onFatalRuntimeError(
                new JoynrRuntimeException({
                    detailMessage: `Fatal runtime error, stopping all communication permanently: ${msg}`
                })
            );
            this.shutdown();
        }
    };

    private readonly onReceivedMessage = (data: Buffer): void => {
        // append what we received to the former received data
        this.internBuff = Buffer.concat([this.internBuff, data]);
        while (this.processReceivedBuffer()) {}
    };

    private processReceivedBuffer(): boolean {
        if (Buffer.byteLength(this.internBuff) < MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH) {
            // received little amount of bytes, not enough to process them. continue receiving
            return false;
        }

        // we received at least amount of magic cookie and the length of serialized joynr message.
        // it might be a huge buffer which contains several messages
        if (!MagicCookieUtil.checkCookie(this.internBuff)) {
            this.onFatalRuntimeError(
                new JoynrRuntimeException({
                    detailMessage: `Fatal runtime error, stopping all communication permanently: Received invalid cookies ${MagicCookieUtil.getMagicCookieBuff(
                        this.internBuff
                    ).toString()}. Close the connection.`
                })
            );
            this.shutdown();
            return false;
        }

        const serializedJoynrMessageLength: number = MagicCookieUtil.getPayloadLength(this.internBuff);

        // message is incomplete. Already buffered, continue receiving
        if (this.internBuff.length < MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessageLength) {
            return false;
        }

        // internBuff has at least one complete message
        const extractMsgBuff: Buffer = Buffer.from(
            this.internBuff.subarray(
                MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH,
                MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessageLength
            )
        );

        // adjust internBuff after extracting the message
        this.internBuff = this.internBuff.slice(
            MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessageLength
        );

        this.processExtractedMessage(extractMsgBuff);
        return true;
    }

    private processExtractedMessage(extractMsgBuff: Buffer): void {
        let joynrMessage;
        try {
            joynrMessage = MessageSerializer.parse(extractMsgBuff);
        } catch (e) {
            const msg = `Fatal runtime error, stopping all communication permanently: ${e}, dropping the message!`;
            log.fatal(msg);
            this.onFatalRuntimeError(
                new JoynrRuntimeException({
                    detailMessage: msg
                })
            );
            this.shutdown();
            return;
        }
        if (this.onMessageCallback) {
            try {
                if (joynrMessage) {
                    this.onMessageCallback(joynrMessage);
                }
            } catch (e) {
                log.fatal(`Error from onMessageCallback ${e}`);
                this.onFatalRuntimeError(
                    new JoynrRuntimeException({
                        detailMessage: `Fatal runtime error, stopping all communication permanently: Error from onMessageCallback: ${e}`
                    })
                );
                this.shutdown();
                return;
            }
        }
    }

    public get numberOfQueuedMessages(): number {
        return this.queuedMessages.length;
    }

    private sendQueuedMessages(): void {
        log.debug(`Sending queued messages if any: ${this.queuedMessages.length} queued.`);
        try {
            while (this.queuedMessages.length) {
                const queuedMessageBuff = this.queuedMessages.shift();
                this.writeToSocket(queuedMessageBuff);
            }
        } catch (e) {
            const msg = `Sending queued messages failed. It fails with ${e}`;
            log.fatal(msg);
            this.onFatalRuntimeError(
                new JoynrRuntimeException({
                    detailMessage: `Fatal runtime error, stopping all communication permanently: ${msg}`
                })
            );
            this.shutdown();
        }
        return;
    }

    private serializeJoynrMessage(joynrMessage: JoynrMessage): Buffer | undefined {
        let marshaledMessage;
        try {
            marshaledMessage = MessageSerializer.stringify(joynrMessage);
        } catch (e) {
            log.error(
                `Could not marshal joynrMessage: ${DiagnosticTags.forJoynrMessage(joynrMessage)}. It failed with ${e}.`
            );
        }
        return marshaledMessage;
    }

    /**
     * @param joynrMessage the joynr message to transmit
     */
    public async send(joynrMessage: JoynrMessage): Promise<void> {
        log.info(`>>> OUTGOING >>> message: ${joynrMessage.msgId}`);
        let marshaledMessage;
        let serializingTries = 2;
        while (serializingTries > 0) {
            --serializingTries;
            marshaledMessage = this.serializeJoynrMessage(joynrMessage);
            if (marshaledMessage) {
                break;
            }
        }

        if (!marshaledMessage) {
            log.error(`Discarding the message because of a failure in serializing it.`);
        } else if (this.socket !== null && this.connected && this.socket!.readyState === "open") {
            await this.sendInternal(marshaledMessage);
        } else if (!this.closed) {
            log.debug(`Not connected, push new messages to the back of the queue`);
            this.queuedMessages.push(marshaledMessage);
        } else {
            log.error(
                `Dropping message because connection is already closed. ${DiagnosticTags.forJoynrMessage(joynrMessage)}`
            );
        }
    }

    private async sendInternal(marshaledMessage: Buffer): Promise<void> {
        this.writeToSocket(marshaledMessage);
    }

    private writeToSocket(marshaledMessage: Buffer): void {
        this.socket!.write(MagicCookieUtil.writeMagicCookies(marshaledMessage, MagicCookieUtil.MESSAGE_COOKIE));
    }

    /**
     * Normally the UdsClient.send api automatically resolves the Promise
     * when it's called. But this doesn't mean that the data was actually
     * written out. This method is a helper for a graceful shutdown which delays
     * the resolving of the UdsClient.send Promise till the data is
     * written out, to make sure that unsubscribe messages are successfully sent.
     */
    public enableShutdownMode(): void {
        this.sendInternal = this.promisifyWriteToSocket;
    }

    /**
     * Ends connection to the socket
     */
    public shutdown(callback?: Function): void {
        if (this.closed && this.socket === null) {
            return;
        }
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
