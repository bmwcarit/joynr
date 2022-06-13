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
import * as DiscoveryEntryWithMetaInfo from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as MessagingQos from "../messaging/MessagingQos";
import { OneWayRequest } from "./types/OneWayRequest";
import * as Reply from "./types/Reply";
import * as Request from "./types/Request";

import * as Typing from "../util/Typing";
import * as UtilInternal from "../util/UtilInternal";
import * as JSONSerializer from "../util/JSONSerializer";
import MethodInvocationException from "../exceptions/MethodInvocationException";
import ProviderRuntimeException from "../exceptions/ProviderRuntimeException";
import Version from "../../generated/joynr/types/Version";
import LoggingManager from "../system/LoggingManager";
import util from "util";
import Dispatcher = require("./Dispatcher");
import JoynrException = require("../exceptions/JoynrException");

const CLEANUP_CYCLE_INTERVAL = 1000;
const log = LoggingManager.getLogger("joynr.dispatching.RequestReplyManager");

type ReplyCallback = (replySettings: any, reply: Reply.Reply) => void;
type PromisifyCallback = (error: any, success?: any) => void;
interface ReplyCaller {
    callback: PromisifyCallback;
    callbackSettings: any;
    expiresAt?: number;
}

class RequestReplyManager {
    private dispatcher: Dispatcher;

    /**
     * @name RequestReplyManager#sendRequest
     * @function
     *
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos quality-of-service parameters such as time-to-live
     * @param settings.request the Request to send
     * @param callbackSettings additional settings to handle the reply.
     * @returns the Promise for the Request
     */
    public sendRequest: (
        settings: {
            from: string;
            toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
            messagingQos: MessagingQos;
            request: Request.Request;
        },
        callbackSettings: Record<string, any>
    ) => Promise<{
        response: any[];
        settings: { outputParameter: { name: string; type: string }[] };
    }>;

    private cleanupInterval: NodeJS.Timer;
    private started = true;
    private replyCallers: Map<string, ReplyCaller>;
    private providers: Record<string, any> = {};
    /**
     * The RequestReplyManager is responsible maintaining a list of providers that wish to
     * receive incoming requests, and also a list of requestReplyIds which is used to match
     * an incoming message with an expected reply.
     *
     * @constructor
     *
     * @param dispatcher
     */
    public constructor(dispatcher: Dispatcher) {
        this.replyCallers = new Map();

        this.cleanupInterval = setInterval(() => {
            const currentTime = Date.now();
            for (const [id, caller] of this.replyCallers) {
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                if (caller.expiresAt! <= currentTime) {
                    caller.callback(new Error(`Request with id "${id}" failed: ttl expired`));
                    this.replyCallers.delete(id);
                }
            }
        }, CLEANUP_CYCLE_INTERVAL);

        this.sendRequestInternal = this.sendRequestInternal.bind(this);
        this.sendRequest = util.promisify<any, any, any>(this.sendRequestInternal);

        this.dispatcher = dispatcher;
    }

    private checkIfReady(): void {
        if (!this.started) {
            throw new Error("RequestReplyManager is already shut down");
        }
    }

    private sendRequestInternal(
        settings: {
            from: string;
            toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
            messagingQos: MessagingQos;
            request: Request.Request;
        },
        callbackSettings: Record<string, any>,
        callback: (error: any, success?: any) => void
    ): void {
        try {
            this.checkIfReady();
        } catch (e) {
            callback(e);
            return;
        }

        this.addReplyCaller(
            settings.request.requestReplyId,
            {
                callback,
                callbackSettings
            },
            settings.messagingQos.ttl
        );

        this.dispatcher.sendRequest(settings);
    }

    private createReplyFromError(
        exception: JoynrException,
        requestReplyId: string,
        handleReplyCallback: ReplyCallback,
        replySettings: any
    ): void {
        const reply = Reply.create({
            error: exception,
            requestReplyId
        });
        return handleReplyCallback(replySettings, reply);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos quality-of-service parameters such as time-to-live
     * @param settings.request the Request to send
     * @returns the Promise for the Request
     */
    public sendOneWayRequest(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        messagingQos: MessagingQos;
        request: OneWayRequest;
    }): Promise<any> {
        this.checkIfReady();
        return this.dispatcher.sendOneWayRequest(settings);
    }

    /**
     * The function addRequestCaller is called when a provider wishes to receive
     * incoming requests for the given participantId
     *
     * @param participantId of the provider receiving the incoming requests
     * @param provider
     */
    public addRequestCaller(participantId: string, provider: Record<string, any>): void {
        this.checkIfReady();
        this.providers[participantId] = provider;
    }

    /**
     * The function addReplyCaller is called when a proxy get/set or rpc is called and
     * is waiting for a reply The reply caller is automatically
     * removed when the ttl expires.
     *
     * @param requestReplyId
     * @param replyCaller
     * @param ttlMs relative number of milliseconds to wait for the reply.
     *            The replycaller will be removed in ttl_ms and and Error will be passed
     *            to the replyCaller
     */
    public addReplyCaller(requestReplyId: string, replyCaller: ReplyCaller, ttlMs: number): void {
        this.checkIfReady();
        replyCaller.expiresAt = Date.now() + ttlMs;
        this.replyCallers.set(requestReplyId, replyCaller);
    }

    /**
     * The function removeRequestCaller is called when a provider no longer wishes to
     * receive incoming requests
     *
     * @param participantId
     */
    public removeRequestCaller(participantId: string): void {
        this.checkIfReady();
        try {
            delete this.providers[participantId];
        } catch (error) {
            log.error(`error removing provider with participantId: ${participantId} error: ${error}`);
        }
    }

    /**
     * @param providerParticipantId
     * @param request
     * @param handleReplyCallback callback for handling the reply
     * @param replySettings settings for handleReplyCallback to avoid unnecessary function object creation
     * @returns
     */
    public async handleRequest(
        providerParticipantId: string,
        request: Request.Request,
        handleReplyCallback: ReplyCallback,
        replySettings: Record<string, any>
    ): Promise<void> {
        let exception;

        try {
            this.checkIfReady();
        } catch (error) {
            exception = new MethodInvocationException({
                detailMessage: `error handling request: ${JSONSerializer.stringify(
                    request
                )} for providerParticipantId ${providerParticipantId}. Joynr runtime already shut down.`
            });
            return this.createReplyFromError(exception, request.requestReplyId, handleReplyCallback, replySettings);
        }
        const provider = this.providers[providerParticipantId];
        if (!provider) {
            // TODO error handling request
            // TODO what if no provider is found in the mean time?
            // Do we need to add a task to handleRequest later?
            exception = new MethodInvocationException({
                detailMessage: `error handling request: ${JSONSerializer.stringify(
                    request
                )} for providerParticipantId ${providerParticipantId}`
            });
            return this.createReplyFromError(exception, request.requestReplyId, handleReplyCallback, replySettings);
        }

        // if there's an operation available to call
        let result;
        if (provider[request.methodName] && provider[request.methodName].callOperation) {
            // may throw an immediate exception when callOperation checks the
            // arguments, in this case exception must be caught.
            // If final customer provided method implementation gets called,
            // that one may return either promise (preferred) or direct result
            // and may possibly also throw exception in the latter case.
            try {
                result = await provider[request.methodName].callOperation(request.params, request.paramDatatypes);
            } catch (internalException) {
                exception = internalException;
            }
            // otherwise, check whether request is an attribute get, set or an operation
        } else {
            const match = request.methodName.match(/([gs]et)?(\w+)/) as RegExpMatchArray;
            const getSet = match[1];
            if (getSet) {
                const attributeName = match[2];
                const attributeObject = provider[attributeName] || provider[UtilInternal.firstLower(attributeName)];
                // if the attribute exists in the provider
                if (attributeObject && !attributeObject.callOperation) {
                    try {
                        if (getSet === "get") {
                            result = await attributeObject.get();
                        } else if (getSet === "set") {
                            // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                            result = await attributeObject.set(request.params![0]);
                        }
                    } catch (internalGetterSetterException) {
                        if (internalGetterSetterException instanceof ProviderRuntimeException) {
                            exception = internalGetterSetterException;
                        } else {
                            exception = new ProviderRuntimeException({
                                detailMessage: `getter/setter method of attribute ${attributeName} reported an error ${internalGetterSetterException}`
                            });
                        }
                    }
                } else {
                    // if neither an operation nor an attribute exists in the
                    // provider => deliver MethodInvocationException
                    exception = new MethodInvocationException({
                        detailMessage: `Could not find an operation "${
                            request.methodName
                        }" or an attribute "${attributeName}" in the provider`,
                        providerVersion: new Version({
                            majorVersion: provider.constructor.MAJOR_VERSION,
                            minorVersion: provider.constructor.MINOR_VERSION
                        })
                    });
                }
            } else {
                // if no operation was found and methodName didn't start with "get"
                // or "set" => deliver MethodInvocationException
                exception = new MethodInvocationException({
                    detailMessage: `Could not find an operation "${request.methodName}" in the provider`,
                    providerVersion: new Version({
                        majorVersion: provider.constructor.MAJOR_VERSION,
                        minorVersion: provider.constructor.MINOR_VERSION
                    })
                });
            }
        }

        /*
          both ProviderOperation.callOperation and ProviderAttribute.get/set have
          have asynchronous API. Therefore the result is always a promise and thus
          it's possible to await for its result.
        */

        if (exception) {
            return this.createReplyFromError(exception, request.requestReplyId, handleReplyCallback, replySettings);
        }

        const reply = Reply.create({
            response: result,
            requestReplyId: request.requestReplyId
        });
        return handleReplyCallback(replySettings, reply);
    }

    /**
     * @param providerParticipantId
     * @param request
     */
    public handleOneWayRequest(providerParticipantId: string, request: OneWayRequest): void {
        this.checkIfReady();
        const provider = this.providers[providerParticipantId];
        if (!provider) {
            throw new MethodInvocationException({
                detailMessage: `error handling one-way request: ${JSONSerializer.stringify(
                    request
                )} for providerParticipantId ${providerParticipantId}`
            });
        }

        // if there's an operation available to call
        if (provider[request.methodName] && provider[request.methodName].callOperation) {
            // If final customer provided method implementation gets called,
            // that one may return either promise (preferred) or direct result
            // and may possibly also throw exception in the latter case.
            provider[request.methodName].callOperation(request.params, request.paramDatatypes);
        } else {
            throw new MethodInvocationException({
                detailMessage: `Could not find an operation "${request.methodName}" in the provider`,
                providerVersion: new Version({
                    majorVersion: provider.constructor.MAJOR_VERSION,
                    minorVersion: provider.constructor.MINOR_VERSION
                })
            });
        }
    }

    /**
     * @param reply
     */
    public handleReply(reply: Reply.Reply): void {
        const replyCaller = this.replyCallers.get(reply.requestReplyId);

        if (replyCaller === undefined) {
            log.error(
                `error handling reply resolve, because replyCaller could not be found: ${JSONSerializer.stringify(
                    reply
                )}`
            );
            return;
        }

        try {
            if (reply.error) {
                log.info(`RequestReplyManager::handleReply got reply with exception ${util.inspect(reply.error)}`);
                if (reply.error instanceof Error) {
                    replyCaller.callback(reply.error);
                } else {
                    replyCaller.callback(Typing.augmentTypes(reply.error));
                }
            } else {
                replyCaller.callback(undefined, {
                    response: reply.response,
                    settings: replyCaller.callbackSettings
                });
            }

            this.replyCallers.delete(reply.requestReplyId);
        } catch (e) {
            log.error(`exception thrown during handling reply ${JSONSerializer.stringify(reply)}:\n${e.stack}`);
        }
    }

    /**
     * Shutdown the request reply manager
     */
    public shutdown(): void {
        clearInterval(this.cleanupInterval);

        for (const replyCaller of this.replyCallers.values()) {
            if (replyCaller) {
                replyCaller.callback(new Error("RequestReplyManager is already shut down"));
            }
        }
        this.replyCallers.clear();
        this.started = false;
    }
}

export = RequestReplyManager;
