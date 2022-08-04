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
import net = require("net");
import fs = require("fs");

class FakeUdsServer {
    /**
     * Server
     */
    private server: any;

    /**
     * Socket for communication with the client.
     * NOTE: the FakeUdsServer only supports a single client.
     */
    private clientSocket: any;

    /**
     * Path of unix domain socket to listen
     */
    private readonly udsPath: string;

    private serverSpy: any;

    /**
     * @constructor
     * @param path - path of the unix domain socket
     */
    public constructor(path: string) {
        // Create a server
        this.udsPath = path;
        this.unlinkUdsFile(this.udsPath);
        this.server = net
            .createServer()
            .on("connection", this.handleConnection)
            .on("error", this.onServerError)
            .on("close", this.onServerClose);
        // Create a server spy
        this.serverSpy = {
            // callback when new connection made, pass socket as an argument
            onConnection: jest.fn(),
            // callback when message from client received
            // pass two arguments (socket, message)
            onMessageReceived: jest.fn(),
            // callback when client disconnected, pass no arguments
            onClientDisconnected: jest.fn(),
            // callback when server error occurred, pass single argument (error)
            onServerError: jest.fn(),
            // callback when server closed, pass no arguments
            onServerClose: jest.fn()
        };
    }

    /**
     * Start listening / accepting clients
     */
    public start = (callback?: Function): void => {
        console.log("Starting server...");
        this.server.listen(this.udsPath, callback);
    };

    /**
     * Terminate the server.
     */
    public stop = (callback?: Function): void => {
        console.log("Terminating server...");
        if (this.clientSocket) {
            this.clientSocket.end();
            delete this.clientSocket;
        }
        this.server.close(callback);
    };

    public getServerSpy = (): any => {
        return this.serverSpy;
    };

    /**
     * Unlink existing unix domain socket file, if it exists.
     * @param udsPath - path of the unix domain socket file
     */
    private unlinkUdsFile = (udsPath: string): void => {
        console.log("Checking for leftover unix domain socket:", udsPath);
        try {
            fs.statSync(udsPath);
        } catch (err) {
            console.log("No leftover unix domain socket", udsPath, "found.");
            return;
        }
        console.log("Removing leftover unix domain socket:", udsPath);
        fs.unlinkSync(udsPath);
        return;
    };

    /**
     * Callback function on 'connection' (connect) event of client
     * representing a connection listener of the server.
     * @param socket - connection with the server
     */
    private handleConnection = (socket: any): void => {
        console.log("Client connection acknowledged!");
        this.clientSocket = socket;

        // Call server spy function when new connection is made
        this.serverSpy.onConnection(socket);

        // disconnect the socket
        this.clientSocket.on("end", this.onClientDisconnected);

        // Handle data received from the client
        this.clientSocket.on("data", this.onMessageReceived);
    };

    /**
     * Callback function on 'end' (disconnect) event of client.
     * Close the the established connection with the client.
     * @param socket - connection object representing client
     */
    private onClientDisconnected = (): void => {
        console.log("Client disconnected!");
        // call server spy function when client disconnected
        this.serverSpy.onClientDisconnected();
        delete this.clientSocket;
    };

    /**
     * Callback function on 'data' event of the client is used
     * to process received message and send a response back to the client.
     * @param msg - message received from the client
     */
    private onMessageReceived = (msg: any): void => {
        // call server spy function when message received from client
        this.serverSpy.onMessageReceived(this.clientSocket, msg);
        msg = msg.toString();
        console.log("Message received from client:", msg);
    };

    /**
     * Callback function on 'error' event of the server is used
     * to handle the server if error occurred
     * @param err - error object called by the server
     */
    private onServerError = (err: Error): void => {
        // call server spy function when server error occurred
        this.serverSpy.onServerError(err);
        console.log("Error on the server occurred:", err);
    };

    /**
     * Callback function on 'close' event of the server.
     * It is emitted only when all connections are closed.
     */
    private onServerClose = (): void => {
        // call server spy function when server closed
        this.serverSpy.onServerClose();
        console.log("Server is closed!");
    };
}

export = FakeUdsServer;
