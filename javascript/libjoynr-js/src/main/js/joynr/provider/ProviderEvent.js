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
const BroadcastOutputParameters = require("./BroadcastOutputParameters");
const UtilInternal = require("../util/UtilInternal");
const SubscriptionUtil = require("../dispatching/subscription/util/SubscriptionUtil");

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

    this._callbacks = [];
    this._filters = [];
    this._settings = settings;
    this.selective = settings.selective;
}

/**
 * @name ProviderEvent#checkFilterParameters
 * @param {BroadcastFilterParameters} filterParameters
 * @param {Object} filterParameters.filterParameters an object containing a map filterParameters
 *
 */
ProviderEvent.prototype.checkFilterParameters = function checkFilterParameters(filterParametersInput) {
    const filterParameters = filterParametersInput || {};
    return SubscriptionUtil.checkFilterParameters(
        this._settings.filterSettings,
        filterParameters.filterParameters,
        this._settings.eventName
    );
};

ProviderEvent.prototype.createBroadcastOutputParameters = function createBroadcastOutputParameters() {
    return new BroadcastOutputParameters(this._settings.outputParameterProperties);
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
ProviderEvent.prototype.fire = function fire(broadcastOutputParameters, partitions) {
    SubscriptionUtil.validatePartitions(partitions);
    // the UtilInternal.fire method accepts exactly one argument for the callback
    const data = {
        broadcastOutputParameters,
        filters: this._filters,
        partitions: partitions || []
    };
    UtilInternal.fire(this._callbacks, data);
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
ProviderEvent.prototype.registerObserver = function registerObserver(observer) {
    this._callbacks.push(observer);
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
ProviderEvent.prototype.unregisterObserver = function unregisterObserver(observer) {
    UtilInternal.removeElementFromArray(this._callbacks, observer);
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
ProviderEvent.prototype.addBroadcastFilter = function addBroadcastFilter(filter) {
    this._filters.push(filter);
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
ProviderEvent.prototype.deleteBroadcastFilter = function deleteBroadcastFilter(filter) {
    UtilInternal.removeElementFromArray(this._filters, filter);
};

module.exports = ProviderEvent;
