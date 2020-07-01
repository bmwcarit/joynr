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

/* eslint no-console: "off" */
import FakeUdsServer from "./FakeUdsServer";
import net = require("net");

class TestUdsClient {
    /**
     * Client
     */
    private client: any;

    /**
     * Path of unix domain socket to listen
     */
    private readonly udsPath: string;

    /**
     * @constructor
     * @param path - path of the unix domain socket
     */
    public constructor(path: string) {
        this.udsPath = path;
        this.client = net
            .createConnection(this.udsPath)
            .on("connect", this.onConnect)
            .on("data", this.onMessageReceived)
            .on("error", this.onError);
    }

    public disconnect = () => {
        console.log("Disconnecting from server...");
        this.client.end();
    };

    public sendMessage = (message: string) => {
        this.client.write(message);
    };

    private onConnect = () => {
        console.log("Client connected with server!");
    };

    private onMessageReceived = (msg: any) => {
        msg = msg.toString();
        console.info("Server:", msg);
    };

    private onError = (error: any) => {
        console.error("Server not active. Error:", error.toString());
    };
}

const UDS_PATH = "/tmp/joynr.ts.uds.fakeservertest.sock";

let server: FakeUdsServer;
let serverSpy: any;
let client: TestUdsClient;

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

describe(`libjoynr-js.joynr.messaging.uds.FakeUDSTest`, () => {
    beforeEach(done => {
        server = new FakeUdsServer(UDS_PATH);
        serverSpy = server.getServerSpy();
        server.start(done);
    });

    it(`server connected to the client`, async () => {
        client = new TestUdsClient(UDS_PATH);
        const onConnectionCondition = (): boolean => {
            return serverSpy.onConnection.mock.calls.length > 0;
        };
        await waitFor(onConnectionCondition, 1000);

        expect(serverSpy.onConnection).toHaveBeenCalledTimes(1);

        client.disconnect();
        const onClientDisconnectedCondition = () => {
            return serverSpy.onClientDisconnected.mock.calls.length > 0;
        };
        await waitFor(onClientDisconnectedCondition, 1000);
    });

    it(`server receives expected message from the client and sends a message back`, async () => {
        const expectedMessage = "Message from the client";

        client = new TestUdsClient(UDS_PATH);
        const onConnectionCondition = (): boolean => {
            return serverSpy.onConnection.mock.calls.length > 0;
        };
        await waitFor(onConnectionCondition, 1000);

        client.sendMessage(expectedMessage);
        const onMessageReceivedCondition = (): boolean => {
            return serverSpy.onMessageReceived.mock.calls.length > 0;
        };
        await waitFor(onMessageReceivedCondition, 1000);

        expect(serverSpy.onConnection).toHaveBeenCalled();
        expect(serverSpy.onMessageReceived).toHaveBeenCalled();
        const actualMessage = serverSpy.onMessageReceived.mock.calls[0][1].toString();
        expect(actualMessage).toEqual(expectedMessage);
        // get socket for further message sending to the client
        const socket = serverSpy.onMessageReceived.mock.calls[0][0];
        // send message to the client
        socket.write("Msg from server.");

        client.disconnect();
        const onClientDisconnectedCondition = () => {
            return serverSpy.onClientDisconnected.mock.calls.length > 0;
        };
        await waitFor(onClientDisconnectedCondition, 1000);
    });

    afterEach(done => {
        server.stop(done);
    });
});
