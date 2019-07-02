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

import buildSignature from "./joynr/buildSignature";
import MessagingQos from "./joynr/messaging/MessagingQos";
import PeriodicSubscriptionQos from "./joynr/proxy/PeriodicSubscriptionQos";
import OnChangeSubscriptionQos from "./joynr/proxy/OnChangeSubscriptionQos";
import MulticastSubscriptionQos from "./joynr/proxy/MulticastSubscriptionQos";
import OnChangeWithKeepAliveSubscriptionQos from "./joynr/proxy/OnChangeWithKeepAliveSubscriptionQos";
import BroadcastFilterParameters from "./joynr/proxy/BroadcastFilterParameters";
import ArbitrationStrategyCollection from "./joynr/types/ArbitrationStrategyCollection";
import * as Util from "./joynr/util/Util";
import JoynrException from "./joynr/exceptions/JoynrException";
import JoynrRuntimeException from "./joynr/exceptions/JoynrRuntimeException";
import ApplicationException from "./joynr/exceptions/ApplicationException";
import DiscoveryException from "./joynr/exceptions/DiscoveryException";
import IllegalAccessException from "./joynr/exceptions/IllegalAccessException";
import MethodInvocationException from "./joynr/exceptions/MethodInvocationException";
import NoCompatibleProviderFoundException from "./joynr/exceptions/NoCompatibleProviderFoundException";
import ProviderRuntimeException from "./joynr/exceptions/ProviderRuntimeException";
import PublicationMissedException from "./joynr/exceptions/PublicationMissedException";
import SubscriptionException from "./joynr/exceptions/SubscriptionException";
import ProviderQos from "./generated/joynr/types/ProviderQos";
import ProviderScope from "./generated/joynr/types/ProviderScope";
import CustomParameter from "./generated/joynr/types/CustomParameter";
import DiscoveryQos from "./joynr/proxy/DiscoveryQos";
import DiscoveryScope from "./generated/joynr/types/DiscoveryScope";
import BrowserAddress from "./generated/joynr/system/RoutingTypes/BrowserAddress";
import ChannelAddress from "./generated/joynr/system/RoutingTypes/ChannelAddress";
import WebSocketAddress from "./generated/joynr/system/RoutingTypes/WebSocketAddress";
import WebSocketClientAddress from "./generated/joynr/system/RoutingTypes/WebSocketClientAddress";
import LongTimer from "./joynr/util/LongTimer";

/* istanbul ignore file */

class JoynrApi {
    public constructor() {}
    public buildSignature: string = buildSignature;
    public messaging = {
        MessagingQos
    };
    public proxy = {
        PeriodicSubscriptionQos,
        OnChangeSubscriptionQos,
        MulticastSubscriptionQos,
        OnChangeWithKeepAliveSubscriptionQos,
        BroadcastFilterParameters,
        DiscoveryQos
    };
    public types = {
        ArbitrationStrategyCollection,
        ProviderQos,
        ProviderScope,
        CustomParameter,
        DiscoveryScope
    };
    public util: { LongTimer: LongTimer; Util: typeof Util } = { LongTimer, Util };
    public exceptions = {
        JoynrException,
        JoynrRuntimeException,
        ApplicationException,
        DiscoveryException,
        IllegalAccessException,
        MethodInvocationException,
        NoCompatibleProviderFoundException,
        ProviderRuntimeException,
        PublicationMissedException,
        SubscriptionException
    };
    public system = {
        RoutingTypes: {
            BrowserAddress,
            ChannelAddress,
            WebSocketAddress,
            WebSocketClientAddress
        }
    };
}

export = JoynrApi;
