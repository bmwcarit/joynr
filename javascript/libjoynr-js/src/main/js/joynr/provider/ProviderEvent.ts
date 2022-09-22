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
import BroadcastOutputParameters from "./BroadcastOutputParameters";

import * as UtilInternal from "../util/UtilInternal";
import * as SubscriptionUtil from "../dispatching/subscription/util/SubscriptionUtil";

interface ProviderEventSettings {
    eventName: string;
    selective?: boolean;
    outputParameterProperties: Record<string, any>;
    filterSettings: Record<string, any>;
}

class ProviderEvent {
    public selective?: boolean;
    private settings: ProviderEventSettings;
    private filters: { filter: Function }[];
    private callbacks: Function[];
    /**
     * Constructor of ProviderEvent object that is used in the generation of provider objects
     *
     * @constructor
     *
     * @param settings the settings for this provider event
     * @param settings.eventName the name of the event
     * @param settings.selective true if the broadcast is selective
     * @param settings.outputParameterProperties the output parameter names and types
     * @param settings.filterSettings the filter settings
     */
    public constructor(settings: ProviderEventSettings) {
        this.callbacks = [];
        this.filters = [];
        this.settings = settings;
        this.selective = settings.selective;
    }

    /**
     * @param filterParametersInput an object containing a map filterParameters
     */
    public checkFilterParameters(filterParametersInput: Record<string, any>): { caughtErrors: string[] } {
        const filterParameters = filterParametersInput || {};
        return SubscriptionUtil.checkFilterParameters(
            this.settings.filterSettings,
            filterParameters.filterParameters,
            this.settings.eventName
        );
    }

    public createBroadcastOutputParameters(): BroadcastOutputParameters {
        return new BroadcastOutputParameters(this.settings.outputParameterProperties);
    }

    /**
     * if this event is fired the applications should call this function with the new
     * output parameters which causes the publication containing the values to be
     * sent to all subscribers.
     *
     * @param broadcastOutputParameters the broadcast output parameters
     * @param [partitions] - the partitions to be used for multicasts
     * @throws {Error} if partitions contains invalid characters
     */
    public fire(broadcastOutputParameters: BroadcastOutputParameters, partitions?: string[]): void {
        SubscriptionUtil.validatePartitions(partitions);
        // the UtilInternal.fire method accepts exactly one argument for the callback
        const data = {
            broadcastOutputParameters,
            filters: this.filters,
            partitions: partitions || []
        };
        UtilInternal.fire(this.callbacks, data);
    }

    /**
     * Registers an Observer for value changes
     *
     * @param observer the callback function with the signature "function(value){..}"
     * @see ProviderEvent#unregisterObserver
     */
    public registerObserver(observer: Function): void {
        this.callbacks.push(observer);
    }

    /**
     * Unregisters an Observer for value changes
     *
     * @param observer the callback function with the signature "function(value){..}"
     * @see ProviderEvent#registerObserver
     */
    public unregisterObserver(observer: Function): void {
        UtilInternal.removeElementFromArray(this.callbacks, observer);
    }

    /**
     * Registers a filter
     *
     * @param filter the callback object with a filter function that executes the filtering
     * @see ProviderEvent#deleteBroadcastFilter
     */
    public addBroadcastFilter(filter: { filter: Function }): void {
        this.filters.push(filter);
    }

    /**
     * Unregisters an Observer for value changes
     *
     * @param filter the to be removed callback object with a filter function that executes the filtering
     * @see ProviderEvent#addBroadcastFilter
     */
    public deleteBroadcastFilter(filter: { filter: Function }): void {
        UtilInternal.removeElementFromArray(this.filters, filter);
    }
}

export = ProviderEvent;
