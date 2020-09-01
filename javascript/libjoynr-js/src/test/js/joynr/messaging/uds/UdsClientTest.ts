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

import UdsClient from "../../../../../main/js/joynr/messaging/uds/UdsClient";
import FakeUdsServer from "./FakeUdsServer";
import MagicCookieUtil from "../../../../../main/js/joynr/messaging/uds/MagicCookieUtil";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";
import * as MessageSerializer from "../../../../../main/js/joynr/messaging/MessageSerializer";
import * as UtilInternal from "../../../../../main/js/joynr/util/UtilInternal";
import UdsClientAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/UdsClientAddress";
import JoynrRuntimeException from "joynr/joynr/exceptions/JoynrRuntimeException";
import fs = require("fs");

const UDS_PATH = "/tmp/joynr.ts.uds.udsclienttest.sock";

let expectedJoynrMessage: JoynrMessage;
let joynrMessage: JoynrMessage;
let serializedJoynrMessage: Buffer;
let expectedSerializedInitMessage: Buffer;
let expectedInitMessageLength: number;

interface UdsLibJoynrProvisioning {
    socketPath: string;
    clientId: string;
    connectSleepTimeMs: number;
    onMessageCallback: Function;
    onFatalRuntimeError: (error: JoynrRuntimeException) => void;
}

const sendSpy = jest.fn();
const shutdownSpy = jest.fn();
const onMessageCallbackSpy = jest.fn();
const onFatalRuntimeErrorSpy = jest.fn();

const parameters: UdsLibJoynrProvisioning = {
    socketPath: UDS_PATH,
    clientId: "testClientId",
    connectSleepTimeMs: 500,
    onMessageCallback: onMessageCallbackSpy,
    onFatalRuntimeError: onFatalRuntimeErrorSpy
};

class SpyUdsClient extends UdsClient {
    private readonly sendSpy: any;
    private readonly shutdownSpy: any;
    public constructor(parameters: UdsLibJoynrProvisioning, shutdownSpy: any, sendSpy: any) {
        super(parameters);
        this.sendSpy = sendSpy;
        this.shutdownSpy = shutdownSpy;
    }

    public async send(joynrMessage: JoynrMessage): Promise<void> {
        super.send(joynrMessage);
        this.sendSpy(joynrMessage);
    }

    public shutdown(callback?: Function): void {
        super.shutdown(callback);
        this.shutdownSpy();
    }
}

let testUdsServer: FakeUdsServer;
let testServerSpy: any;

let testUdsClient: UdsClient;

const sleep = (ms: number): Promise<any> => {
    return new Promise(resolve => setTimeout(resolve, ms));
};

const waitFor = (condition: () => boolean, timeout: number): Promise<any> => {
    const checkIntervalMs = 100;
    const start = Date.now();
    const check = (resolve: any, reject: any) => {
        if (condition()) {
            resolve();
        } else if (Date.now() > start + timeout) {
            reject(`${condition.name} failed, even after getting ${timeout} ms to try`);
        } else {
            setTimeout(_ => check(resolve, reject), checkIntervalMs);
        }
    };
    return new Promise(check);
};

async function waitForConnection() {
    const onConnectionCondition = (): boolean => {
        return testServerSpy.onConnection.mock.calls.length > 0;
    };
    await waitFor(onConnectionCondition, 1000);
    expect(testServerSpy.onConnection).toHaveBeenCalledTimes(1);
}

async function checkInitConnectionOfUdsClient() {
    await waitForConnection();

    const onMessageReceivedCondition = (): boolean => {
        return testServerSpy.onMessageReceived.mock.calls.length === 1;
    };
    await waitFor(onMessageReceivedCondition, 1000);
    expect(testServerSpy.onMessageReceived).toHaveBeenCalledTimes(1);

    const actualReceivedInitMessageBuff = testServerSpy.onMessageReceived.mock.calls[0][1];
    const receivedInitMagicCookie: string = MagicCookieUtil.getMagicCookieBuff(
        actualReceivedInitMessageBuff
    ).toString();
    expect(receivedInitMagicCookie).toEqual(MagicCookieUtil.INIT_COOKIE);

    const receivedLengthOfInitMsg: number = MagicCookieUtil.getPayloadLength(actualReceivedInitMessageBuff);
    expect(receivedLengthOfInitMsg).toEqual(expectedSerializedInitMessage.length);

    const receivedPayloadOfInitMsgBuff = MagicCookieUtil.getPayloadBuff(actualReceivedInitMessageBuff);
    expect(receivedPayloadOfInitMsgBuff).toEqual(expectedSerializedInitMessage);
}

function checkReceivedJoynrMessage(actualJoynrMessage: any): boolean {
    if (actualJoynrMessage) {
        const deserializedJoynrMessage = MessageSerializer.parse(serializedJoynrMessage);
        expect(actualJoynrMessage).toEqual(deserializedJoynrMessage);
        return true;
    }
    return false;
}

async function sendBuffer(socket: any, data: Buffer, onMessageCallbackCondition: () => boolean) {
    socket.write(data);
    await waitFor(onMessageCallbackCondition, 1000);
}

describe(`libjoynr-js.joynr.messaging.uds.UdsClient`, () => {
    beforeEach(done => {
        expectedJoynrMessage = new JoynrMessage({
            payload: "test message from uds client",
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        expectedJoynrMessage.from = "client";
        expectedJoynrMessage.to = "server";
        expectedJoynrMessage.expiryDate = 1;
        expectedJoynrMessage.compress = false;

        expectedSerializedInitMessage = Buffer.from(
            JSON.stringify(
                new UdsClientAddress({
                    id: parameters.clientId
                })
            )
        );

        expectedInitMessageLength =
            expectedSerializedInitMessage.length + MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH;

        joynrMessage = UtilInternal.extendDeep(
            new JoynrMessage({ payload: "test message from uds client", type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST }),
            expectedJoynrMessage
        );
        serializedJoynrMessage = MessageSerializer.stringify(joynrMessage);

        testUdsServer = new FakeUdsServer(UDS_PATH);
        testServerSpy = testUdsServer.getServerSpy();
        testUdsServer.start(done);
    });

    it(`connects to the server`, async () => {
        testUdsClient = new UdsClient(parameters);
        await waitForConnection();
        testUdsClient.shutdown();
    });

    it(`tries to reconnect until the server is available`, async () => {
        testUdsServer.stop();
        testUdsClient = new UdsClient(parameters);
        const onConnectionCondition = (): boolean => {
            return testServerSpy.onConnection.mock.calls.length > 0;
        };

        await sleep(3000);
        expect(testServerSpy.onConnection).toHaveBeenCalledTimes(0);

        testUdsServer.start();
        await waitFor(onConnectionCondition, 1000);
        expect(testServerSpy.onConnection).toHaveBeenCalledTimes(1);
        testUdsClient.shutdown();
    });

    it(`calls onFatalRuntimeError and shutdown when R/W permissions to the socket are not granted`, async () => {
        fs.chmodSync(UDS_PATH, 0);
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await sleep(1000);
        expect(testServerSpy.onConnection).toHaveBeenCalledTimes(0);
        const errorMessage = `Fatal runtime error, stopping all communication permanently: Error: EACCES: permission denied, access '${UDS_PATH}'`;
        expect(onFatalRuntimeErrorSpy).toHaveBeenCalledWith(new JoynrRuntimeException({ detailMessage: errorMessage }));
        expect(shutdownSpy).toHaveBeenCalledTimes(1);
    });

    it(`sends init message on connect`, async () => {
        testUdsClient = new UdsClient(parameters);
        await checkInitConnectionOfUdsClient();
        testUdsClient.shutdown();
    });

    it(`sends a complete message to the server after init message`, async () => {
        testUdsClient = new UdsClient(parameters);
        await checkInitConnectionOfUdsClient();

        jest.clearAllMocks();

        await testUdsClient.send(joynrMessage);

        const onMessageReceivedCondition = (): boolean => {
            return testServerSpy.onMessageReceived.mock.calls.length === 1;
        };
        await waitFor(onMessageReceivedCondition, 1000);
        expect(testServerSpy.onMessageReceived).toHaveBeenCalledTimes(1);

        // actual received buffer by the server
        const actualReceivedBuff = testServerSpy.onMessageReceived.mock.calls[0][1];

        const actualCookie: string = MagicCookieUtil.getMagicCookieBuff(actualReceivedBuff).toString();
        expect(actualCookie).toEqual(MagicCookieUtil.MESSAGE_COOKIE);

        const actualLength: number = MagicCookieUtil.getPayloadLength(actualReceivedBuff);
        expect(actualLength).toEqual(serializedJoynrMessage.length);

        const actualSerializedJoynrMessage = MagicCookieUtil.getPayloadBuff(actualReceivedBuff);
        expect(actualSerializedJoynrMessage.length).toEqual(serializedJoynrMessage.length);
        expect(actualSerializedJoynrMessage).toEqual(serializedJoynrMessage);

        // deserialize received serialized joynr message and check it
        const actualJoynrMessage = MessageSerializer.parse(actualSerializedJoynrMessage);
        expect(checkReceivedJoynrMessage(actualJoynrMessage)).toEqual(true);
        testUdsClient.shutdown();
    });

    it(`disconnects when the server stops/crashes and calls onFatalRuntimeError`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        testUdsServer.stop();

        const onShutdownCondition = (): boolean => {
            return shutdownSpy.mock.calls.length === 1;
        };
        await waitFor(onShutdownCondition, 1000);
        expect(shutdownSpy).toHaveBeenCalledTimes(1);
        expect(onFatalRuntimeErrorSpy).toHaveBeenCalledWith(
            new JoynrRuntimeException({
                detailMessage:
                    "Fatal runtime error, stopping all communication permanently: The server closed the connection."
            })
        );
    });

    it(`calls onMessageCallback when a complete message arrives from the server`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        // when a complete message is arrived as a single or assembled from different chunk,
        const udsSerializedJoynrMessage = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const socket = testServerSpy.onConnection.mock.calls[0][0];
        socket.write(udsSerializedJoynrMessage);

        const onMessageCallbackCondition = (): boolean => {
            return onMessageCallbackSpy.mock.calls.length > 0;
        };

        await waitFor(onMessageCallbackCondition, 1000);

        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(1);

        // check received args of onMessageCallback, which is the joynrMessage
        const actualJoynrMessage = onMessageCallbackSpy.mock.calls[0][0];
        expect(checkReceivedJoynrMessage(actualJoynrMessage)).toEqual(true);

        testUdsClient.shutdown();
    });

    it(`receives a complete message from the server through several chunks`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const socket = testServerSpy.onConnection.mock.calls[0][0];

        // check received joynrMessage
        const udsSerializedJoynrMessage = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const onMessageCallbackCondition = (): boolean => {
            return onMessageCallbackSpy.mock.calls.length > 0;
        };

        // partial magic cookie
        const incompleteMsgPart1 = Buffer.from(udsSerializedJoynrMessage.subarray(0, 3));
        await sendBuffer(socket, incompleteMsgPart1, () => {
            return true;
        });
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);

        // partial payload length header
        const incompleteMsgPart2 = Buffer.from(udsSerializedJoynrMessage.subarray(3, 8));
        await sendBuffer(socket, incompleteMsgPart2, () => {
            return true;
        });
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);

        // partial payload
        const incompleteMsgPart3 = Buffer.from(
            udsSerializedJoynrMessage.subarray(8, udsSerializedJoynrMessage.length - 10)
        );
        await sendBuffer(socket, incompleteMsgPart3, () => {
            return true;
        });
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);

        const incompleteMsgPart4 = Buffer.from(
            udsSerializedJoynrMessage.subarray(udsSerializedJoynrMessage.length - 10, udsSerializedJoynrMessage.length)
        );
        await sendBuffer(socket, incompleteMsgPart4, onMessageCallbackCondition);
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(1);

        // check received joynr message
        const actualJoynrMessage = onMessageCallbackSpy.mock.calls[0][0];
        expect(checkReceivedJoynrMessage(actualJoynrMessage)).toEqual(true);

        testUdsClient.shutdown();
    });

    it(`receives two complete messages in a chunk and calls onmessage for both`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const socket = testServerSpy.onConnection.mock.calls[0][0];

        const udsSerializedJoynrMessage1 = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const udsSerializedJoynrMessage2 = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const onMessageCallbackCondition = (): boolean => {
            return onMessageCallbackSpy.mock.calls.length > 1;
        };

        const twoMessagesArray = [udsSerializedJoynrMessage1, udsSerializedJoynrMessage2];

        // we send a buffer which contains two messages
        const bufferContainsTwoUdsMsgs = Buffer.concat(twoMessagesArray);
        await sendBuffer(socket, bufferContainsTwoUdsMsgs, onMessageCallbackCondition);
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(2);

        // check received joynr messages
        const actualJoynrMessage1 = onMessageCallbackSpy.mock.calls[0][0];
        expect(checkReceivedJoynrMessage(actualJoynrMessage1)).toEqual(true);

        const actualJoynrMessage2 = onMessageCallbackSpy.mock.calls[1][0];
        expect(checkReceivedJoynrMessage(actualJoynrMessage2)).toEqual(true);

        testUdsClient.shutdown();
    });

    it("queues messages when the uds server is still unavailable", async () => {
        testUdsServer.stop();
        testUdsClient = new UdsClient(parameters);
        await testUdsClient.send(joynrMessage);
        await testUdsClient.send(joynrMessage);
        await testUdsClient.send(joynrMessage);
        expect(testUdsClient.numberOfQueuedMessages).toBe(3);
        testUdsClient.shutdown();
    });

    it("delivers queued messages when the uds server becomes available", async () => {
        testUdsServer.stop();
        testUdsClient = new UdsClient(parameters);

        const numberOfMessages = 10;
        const expectedReceivedBytes =
            expectedInitMessageLength +
            (MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedJoynrMessage.length) * numberOfMessages;

        const onCompleteReceivedBytesCondition = (): boolean => {
            const callCount = testServerSpy.onMessageReceived.mock.calls.length;
            if (callCount === 0) {
                return false;
            }
            let accumulatedReceivedBytes = 0;
            for (let i = 0; i < callCount; i++) {
                accumulatedReceivedBytes += testServerSpy.onMessageReceived.mock.calls[i][1].length;
            }
            return accumulatedReceivedBytes === expectedReceivedBytes;
        };

        const promises = [];
        for (let i = 0; i < numberOfMessages; i++) {
            promises.push(testUdsClient.send(joynrMessage));
        }
        await Promise.all(promises);
        expect(testUdsClient.numberOfQueuedMessages).toBe(numberOfMessages);

        testUdsServer.start();
        await waitForConnection();
        expect(testServerSpy.onConnection).toHaveBeenCalledTimes(1);
        expect(testUdsClient.numberOfQueuedMessages).toBe(0);

        await waitFor(onCompleteReceivedBytesCondition, 1000);

        testUdsClient.shutdown();
    });

    it(`closes the connection against the server when receiving data which does not contain magic cookie`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const socket = testServerSpy.onConnection.mock.calls[0][0];

        const garbageData: Buffer = Buffer.from(`garbage data sent by the server, client must close the connection`);
        await sendBuffer(socket, garbageData, () => {
            return true;
        });
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);

        const onShutdownCondition = (): boolean => {
            return shutdownSpy.mock.calls.length > 0;
        };
        await waitFor(onShutdownCondition, 1000);

        expect(shutdownSpy).toHaveBeenCalledTimes(1);
        expect(onFatalRuntimeErrorSpy).toHaveBeenCalledWith(
            new JoynrRuntimeException({
                detailMessage:
                    "Fatal runtime error, stopping all communication permanently: Received invalid cookies garb. Close the connection."
            })
        );
    });

    it(`receives a complete message via multiple chunks from the server`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const socket = testServerSpy.onConnection.mock.calls[0][0];

        const onMessageCallbackCondition = (): boolean => {
            return onMessageCallbackSpy.mock.calls.length > 0;
        };

        const udsSerializedJoynrMessage = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        // server sends chunk1/2
        const chunk1 = Buffer.from(udsSerializedJoynrMessage.subarray(0, udsSerializedJoynrMessage.length - 25));
        await sendBuffer(socket, chunk1, () => {
            return true;
        });
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);

        await sleep(100);

        // server sends chunk2/2
        const chunk2 = Buffer.from(
            udsSerializedJoynrMessage.subarray(udsSerializedJoynrMessage.length - 25, udsSerializedJoynrMessage.length)
        );
        await sendBuffer(socket, chunk2, onMessageCallbackCondition);
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(1);

        testUdsClient.shutdown();
    });

    it(`receives a few messages via multiple chunks from the server`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const socket = testServerSpy.onConnection.mock.calls[0][0];

        const udsSerializedJoynrMessage1 = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const udsSerializedJoynrMessage2 = MagicCookieUtil.writeMagicCookies(
            serializedJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const onMessageCallbackCondition1 = (): boolean => {
            return onMessageCallbackSpy.mock.calls.length > 0;
        };

        const partOfMessage2 = Buffer.from(
            udsSerializedJoynrMessage2.subarray(0, udsSerializedJoynrMessage2.length - 25)
        );

        const arrayOfData = [udsSerializedJoynrMessage1, partOfMessage2];

        // server sends chunk1/2, message1 + part of message2
        const chunk1 = Buffer.concat(arrayOfData);
        await sendBuffer(socket, chunk1, onMessageCallbackCondition1);
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(1);

        await sleep(100);

        // server sends chunk2/2, the remaining from message2
        const chunk2 = Buffer.from(
            udsSerializedJoynrMessage2.subarray(
                udsSerializedJoynrMessage2.length - 25,
                udsSerializedJoynrMessage2.length
            )
        );
        const onMessageCallbackCondition2 = (): boolean => {
            return onMessageCallbackSpy.mock.calls.length === 2;
        };

        await sendBuffer(socket, chunk2, onMessageCallbackCondition2);

        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(2);

        testUdsClient.shutdown();
    });

    it(`receives a message with no payload from the server`, async () => {
        onMessageCallbackSpy.mockClear();
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const zeroPayloadMessage = Buffer.alloc(0, 0);
        const udsZeroPayloadMessage = MagicCookieUtil.writeMagicCookies(
            zeroPayloadMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const socket = testServerSpy.onConnection.mock.calls[0][0];
        await sendBuffer(socket, udsZeroPayloadMessage, () => {
            return true;
        });
        // wait a bit to be sure that we received a message.
        await sleep(1000);
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);

        testUdsClient.shutdown();
    });

    it(`shutdown mode delays resolving UdsClient.send Promise till the data is written out`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await checkInitConnectionOfUdsClient();

        testUdsClient.enableShutdownMode();

        jest.clearAllMocks();

        await testUdsClient.send(joynrMessage);

        testUdsClient.shutdown();

        expect(sendSpy).toHaveBeenCalledWith(joynrMessage);

        //  server receives the message
        const onMessageReceivedCondition = (): boolean => {
            return testServerSpy.onMessageReceived.mock.calls.length > 0;
        };
        await waitFor(onMessageReceivedCondition, 1000);
        expect(testServerSpy.onMessageReceived).toHaveBeenCalledTimes(1);
        const actualReceivedBuff = testServerSpy.onMessageReceived.mock.calls[0][1];
        const actualSerializedJoynrMessage = MagicCookieUtil.getPayloadBuff(actualReceivedBuff);
        expect(actualSerializedJoynrMessage).toEqual(serializedJoynrMessage);
        const actualJoynrMessage = MessageSerializer.parse(actualSerializedJoynrMessage);
        expect(checkReceivedJoynrMessage(actualJoynrMessage)).toEqual(true);
    });

    it(`calls onFatalRuntimeError when deserialization received joynr message fails`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy, sendSpy);
        await waitForConnection();

        const serializedSpoiledJoynrMessage = Buffer.from(serializedJoynrMessage);
        serializedSpoiledJoynrMessage[0] = 56; // spoil the first byte. Default is 1
        const udsSerializedJoynrMessage = MagicCookieUtil.writeMagicCookies(
            serializedSpoiledJoynrMessage,
            MagicCookieUtil.MESSAGE_COOKIE
        );

        const socket = testServerSpy.onConnection.mock.calls[0][0];
        await sendBuffer(socket, udsSerializedJoynrMessage, () => {
            return true;
        });

        const onFatalRuntimeErrorCondition = (): boolean => {
            return onFatalRuntimeErrorSpy.mock.calls.length === 1;
        };
        await waitFor(onFatalRuntimeErrorCondition, 1000);
        expect(onMessageCallbackSpy).toHaveBeenCalledTimes(0);
        expect(onFatalRuntimeErrorSpy).toHaveBeenCalledWith(
            new JoynrRuntimeException({
                detailMessage:
                    "Fatal runtime error, stopping all communication permanently: Error: smrf.deserialize: got exception Error: unsupported version: 56, dropping the message!"
            })
        );
        testUdsClient.shutdown();
    });

    it(`sends huge volume of big messages to check robustness of the internal buffer`, async () => {
        testUdsClient = new UdsClient(parameters);
        await waitForConnection();

        const numberOfMessages = 1024;
        const bigJoynrMessage = new JoynrMessage({
            payload: Buffer.alloc(1024, 65).toString(),
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        bigJoynrMessage.from = "client";
        bigJoynrMessage.to = "server";
        bigJoynrMessage.expiryDate = 1;
        bigJoynrMessage.compress = false;

        const serializedBigJoynrMessage = MessageSerializer.stringify(bigJoynrMessage);
        const expectedReceivedBytes =
            expectedInitMessageLength +
            (MagicCookieUtil.MAGIC_COOKIE_AND_PAYLOAD_LENGTH + serializedBigJoynrMessage.length) * numberOfMessages;

        const onCompleteReceivedBytesCondition = (): boolean => {
            const callCount = testServerSpy.onMessageReceived.mock.calls.length;
            if (callCount === 0) {
                return false;
            }
            let accumulatedReceivedBytes = 0;
            for (let i = 0; i < callCount; i++) {
                accumulatedReceivedBytes += testServerSpy.onMessageReceived.mock.calls[i][1].length;
            }
            return accumulatedReceivedBytes === expectedReceivedBytes;
        };

        const arrayOfPromises = [];
        for (let i = 0; i < numberOfMessages; i++) {
            arrayOfPromises.push(testUdsClient.send(bigJoynrMessage));
        }

        await Promise.all(arrayOfPromises);

        await waitFor(onCompleteReceivedBytesCondition, 1000);

        testUdsClient.shutdown();
    });

    afterEach(done => {
        testUdsServer.stop(() => done());
        jest.clearAllMocks();
    });
});
