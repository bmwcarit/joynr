/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
var BroadcastOutputParameters = require("./BroadcastOutputParameters");
var Util = require("../util/UtilInternal");
var SubscriptionUtil = require("../dispatching/subscription/util/SubscriptionUtil");

/**
 * Constructor of ProviderEvent object that is used in the generation of provider objects
 *
 * @name ProviderEvent
 * @constructor
 *
 * @param {Object}
 *            settings the settings for this provider event
 * @param {String}
 *            settings.eventName the name of the event
 * @param {Boolean}
 *            settings.selective true if the broadcast is selective
 * @param {Object}
 *            settings.outputParameterProperties the output parameter names and types
 * @param {Object}
 *            settings.filterSettings the filter settings
 */
function ProviderEvent(settings) {
    if (!(this instanceof ProviderEvent)) {
        // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
        return new ProviderEvent(settings);
    }

    var callbacks = [];
    var filters = [];

    this.selective = settings.selective;
    /**
     * @name ProviderEvent#checkFilterParameters
     * @param {BroadcastFilterParameters} filterParameters
     * @param {Object} filterParameters.filterParameters an object containing a map filterParameters
     *
     */
    this.checkFilterParameters = function checkFilterParameters(filterParametersInput) {
        var filterParameters = filterParametersInput || {};
        return SubscriptionUtil.checkFilterParameters(
            settings.filterSettings,
            filterParameters.filterParameters,
            settings.eventName
        );
    };

    this.createBroadcastOutputParameters = function createBroadcastOutputParameters() {
        return new BroadcastOutputParameters(settings.outputParameterProperties);
    };

    /**
     * if this event is fired the applications should call this function with the new
     * output parameters which causes the publication containing the values to be
     * sent to all subscribers.
     *
     * @name ProviderEvent#fire
     * @function
     *
     * @param {BroadcastOutputParameters}
     *     broadcastOutputParameters the broadcast output parameters
     * @param {String[]}
     *     [partitions] - the partitions to be used for multicasts
     * @throws {Error} if partitions contains invalid characters
     */
    this.fire = function fire(broadcastOutputParameters, partitions) {
        SubscriptionUtil.validatePartitions(partitions);
        // the Util.fire method accepts exactly one argument for the callback
        var data = {
            broadcastOutputParameters: broadcastOutputParameters,
            filters: filters,
            partitions: partitions || []
        };
        Util.fire(callbacks, data);
    };

    /**
     * Registers an Observer for value changes
     *
     * @name ProviderAttribute#registerObserver
     * @function
     *
     * @param {Function}
     *            observer the callback function with the signature "function(value){..}"
     * @see ProviderEvent#unregisterObserver
     */
    this.registerObserver = function registerObserver(observer) {
        callbacks.push(observer);
    };

    /**
     * Unregisters an Observer for value changes
     *
     * @name ProviderAttribute#unregisterObserver
     * @function
     *
     * @param {Function}
     *            observer the callback function with the signature "function(value){..}"
     * @see ProviderEvent#registerObserver
     */
    this.unregisterObserver = function unregisterObserver(observer) {
        Util.removeElementFromArray(callbacks, observer);
    };

    /**
     * Registers a filter
     *
     * @name ProviderEvent#addBroadcastFilter
     * @function
     *
     * @param {Function}
     *            filter the callback object that executes the filtering
     * @see ProviderEvent#deleteBroadcastFilter
     */
    this.addBroadcastFilter = function addBroadcastFilter(filter) {
        filters.push(filter);
    };

    /**
     * Unregisters an Observer for value changes
     *
     * @name ProviderAttribute#deleteBroadcastFilter
     * @function
     *
     * @param {Function}
     *            filter the callback object that executes the filtering
     * @see ProviderEvent#addBroadcastFilter
     */
    this.deleteBroadcastFilter = function deleteBroadcastFilter(filter) {
        Util.removeElementFromArray(filters, filter);
    };

    return Object.freeze(this);
}

module.exports = ProviderEvent;
