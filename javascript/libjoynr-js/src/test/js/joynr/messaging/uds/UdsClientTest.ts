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

const expectedJoynrMessage = new JoynrMessage({
    payload: "test message from uds client",
    type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
});
expectedJoynrMessage.from = "client";
expectedJoynrMessage.to = "server";
expectedJoynrMessage.expiryDate = 1;
expectedJoynrMessage.compress = false;

const joynrMessage = UtilInternal.extendDeep(expectedJoynrMessage);
const serializedJoynrMessage = MessageSerializer.stringify(joynrMessage);

const UDS_PATH = "/tmp/joynr.ts.uds.udsclienttest.sock";

interface UdsLibJoynrProvisioning {
    socketPath: string;
    clientId: string;
    connectSleepTimeMs: number;
    onMessageCallback: Function;
}

const shutdownSpy = jest.fn();
const onMessageCallbackSpy = jest.fn();

const parameters: UdsLibJoynrProvisioning = {
    socketPath: UDS_PATH,
    clientId: "testClientId",
    connectSleepTimeMs: 500,
    onMessageCallback: onMessageCallbackSpy
};

class SpyUdsClient extends UdsClient {
    private readonly shutdownSpy: any;
    public constructor(parameters: UdsLibJoynrProvisioning, shutdownSpy: any) {
        super(parameters);
        this.shutdownSpy = shutdownSpy;
    }

    public shutdown(callback?: Function): void {
        super.shutdown(callback);
        this.shutdownSpy();
    }
}

let testUdsServer: FakeUdsServer;
let testServerSpy: any;

let testUdsClient: UdsClient;

const waitFor = (condition: () => boolean, timeout: number): Promise<any> => {
    const checkIntervalMs = 100;
    const start = Date.now();
    const check = (resolve: any, reject: any) => {
        if (condition()) {
            resolve();
        } else if (Date.now() > start + timeout) {
            reject();
        } else {
            setTimeout(_ => check(resolve, reject), checkIntervalMs);
        }
    };
    return new Promise(check);
};

const sleep = (ms: number): Promise<any> => {
    return new Promise(resolve => setTimeout(resolve, ms));
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
    const expectedInitMessage = Buffer.from(
        JSON.stringify(
            new UdsClientAddress({
                id: parameters.clientId
            })
        )
    );
    expect(receivedLengthOfInitMsg).toEqual(expectedInitMessage.length);

    const receivedPayloadOfInitMsgBuff = MagicCookieUtil.getPayloadBuff(actualReceivedInitMessageBuff);
    expect(receivedPayloadOfInitMsgBuff).toEqual(expectedInitMessage);
}

function checkJoynrMessage(actualJoynrMessage: any): boolean {
    if (actualJoynrMessage) {
        expect(actualJoynrMessage.payload).toEqual(expectedJoynrMessage.payload);
        expect(actualJoynrMessage.type).toEqual(expectedJoynrMessage.type);
        expect(actualJoynrMessage.from).toEqual(expectedJoynrMessage.from);
        expect(actualJoynrMessage.to).toEqual(expectedJoynrMessage.to);
        expect(actualJoynrMessage.expiryDate).toEqual(expectedJoynrMessage.expiryDate);
        expect(actualJoynrMessage.compress).toEqual(expectedJoynrMessage.compress);
        return true;
    }
    return false;
}

async function sendBuffer(socket: any, incompleteMsgPart1: Buffer, onMessageCallbackCondition: () => boolean) {
    socket.write(incompleteMsgPart1);
    await waitFor(onMessageCallbackCondition, 1000);
}

describe(`libjoynr-js.joynr.messaging.uds.UdsClient`, () => {
    beforeEach(done => {
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

    it(`sends init message on connect`, async () => {
        testUdsClient = new UdsClient(parameters);
        await checkInitConnectionOfUdsClient();
        testUdsClient.shutdown();
    });

    it(`sends a complete message to the server after init message`, async () => {
        testUdsClient = new UdsClient(parameters);
        await checkInitConnectionOfUdsClient();

        await testUdsClient.send(joynrMessage);

        const onMessageReceivedCondition = (): boolean => {
            return testServerSpy.onMessageReceived.mock.calls.length === 2;
        };
        await waitFor(onMessageReceivedCondition, 1000);
        expect(testServerSpy.onMessageReceived).toHaveBeenCalledTimes(2);

        // actual received buffer by the server
        const actualReceivedBuff = testServerSpy.onMessageReceived.mock.calls[1][1];

        const actualCookie: string = MagicCookieUtil.getMagicCookieBuff(actualReceivedBuff).toString();
        expect(actualCookie).toEqual(MagicCookieUtil.MESSAGE_COOKIE);

        const actualLength: number = MagicCookieUtil.getPayloadLength(actualReceivedBuff);
        expect(actualLength).toEqual(serializedJoynrMessage.length);

        const actualSerializedJoynrMessage = MagicCookieUtil.getPayloadBuff(actualReceivedBuff);
        expect(actualSerializedJoynrMessage.length).toEqual(serializedJoynrMessage.length);
        expect(actualSerializedJoynrMessage).toEqual(serializedJoynrMessage);

        // deserialize received serialized joynr message and check it
        const actualJoynrMessage = MessageSerializer.parse(actualSerializedJoynrMessage);
        expect(checkJoynrMessage(actualJoynrMessage)).toEqual(true);
        testUdsClient.shutdown();
    });

    it(`disconnects when the server stops/crashes`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy);
        await waitForConnection();

        testUdsServer.stop();

        const onShutdownCondition = (): boolean => {
            return shutdownSpy.mock.calls.length > 0;
        };
        await waitFor(onShutdownCondition, 1000);
        expect(shutdownSpy).toHaveBeenCalledTimes(1);
    });

    it(`calls onMessageCallback when a complete message arrives from the server`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy);
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
        expect(checkJoynrMessage(actualJoynrMessage)).toEqual(true);

        testUdsClient.shutdown();
    });

    it(`receives a complete message from the server through several chunks`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy);
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
        expect(checkJoynrMessage(actualJoynrMessage)).toEqual(true);

        testUdsClient.shutdown();
    });

    it(`receives two complete messages in a chunk and calls onmessage for both`, async () => {
        testUdsClient = new SpyUdsClient(parameters, shutdownSpy);
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
        expect(checkJoynrMessage(actualJoynrMessage1)).toEqual(true);

        const actualJoynrMessage2 = onMessageCallbackSpy.mock.calls[1][0];
        expect(checkJoynrMessage(actualJoynrMessage2)).toEqual(true);

        testUdsClient.shutdown();
    });

    afterEach(done => {
        testUdsServer.stop(() => done());
        jest.clearAllMocks();
    });
});
