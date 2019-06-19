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
import ArbitrationStrategyCollection from "../../joynr/types/ArbitrationStrategyCollection";

import DiscoveryScope from "../../generated/joynr/types/DiscoveryScope";
import * as UtilInternal from "../util/UtilInternal";

let defaultSettings = {
    discoveryTimeoutMs: 10 * 60 * 1000, // 10 minutes
    discoveryRetryDelayMs: 10 * 1000, // 10 seconds
    arbitrationStrategy: ArbitrationStrategyCollection.LastSeen,
    cacheMaxAgeMs: 0,
    discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
    providerMustSupportOnChange: false,
    additionalParameters: {}
};

namespace DiscoveryQos {
    export interface Settings {
        discoveryTimeoutMs: number;
        discoveryRetryDelayMs: number;
        arbitrationStrategy: Function;
        cacheMaxAgeMs: number;
        discoveryScope: DiscoveryScope;
        providerMustSupportOnChange: boolean;
        additionalParameters: Record<string, any>;
    }
}

class DiscoveryQos {
    public additionalParameters: Record<string, any>;
    public providerMustSupportOnChange: boolean;
    public discoveryScope: DiscoveryScope;
    public cacheMaxAgeMs: number;
    public arbitrationStrategy: Function;
    public discoveryRetryDelayMs: number;
    public discoveryTimeoutMs: number;

    /**
     * Constructor of DiscoveryQos object that is used in the generation of proxy objects
     *
     * @param [settings] the settings object for the constructor call
     * @param [settings.discoveryTimeoutMs] for rpc calls to wait for arbitration to finish.
     * @param [settings.discoveryRetryDelayMs] the minimum delay between two arbitration retries
     * @param [settings.arbitrationStrategy] Strategy for choosing the appropriate provider from the list returned by the capabilities directory with the function signature "function(CapabilityInfos[])" that returns an array of CapabilityInfos
     * @param [settings.cacheMaxAgeMs] Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     * @param [settings.discoveryScope] default  is LOCAL_AND_GLOBAL
     * @param [settings.providerMustSupportOnChange] default false
     * @param [settings.additionalParameters] a map holding additional parameters in the form of key value pairs in the javascript object, e.g.: {"myKey": "myValue", "myKey2": 5} @returns {DiscoveryQos} a discovery Qos Object
     */
    public constructor(settings?: Partial<DiscoveryQos.Settings>) {
        const augmentedSettings: DiscoveryQos.Settings = UtilInternal.extend({}, defaultSettings, settings);

        this.discoveryTimeoutMs = augmentedSettings.discoveryTimeoutMs;
        this.discoveryRetryDelayMs = augmentedSettings.discoveryRetryDelayMs;
        this.arbitrationStrategy = augmentedSettings.arbitrationStrategy;
        this.cacheMaxAgeMs = augmentedSettings.cacheMaxAgeMs;
        this.discoveryScope = augmentedSettings.discoveryScope;
        this.providerMustSupportOnChange = augmentedSettings.providerMustSupportOnChange;
        this.additionalParameters = augmentedSettings.additionalParameters;
    }

    public static setDefaultSettings(settings: Partial<DiscoveryQos.Settings>): void {
        defaultSettings = UtilInternal.extend({}, defaultSettings, settings);
    }
}

export = DiscoveryQos;
