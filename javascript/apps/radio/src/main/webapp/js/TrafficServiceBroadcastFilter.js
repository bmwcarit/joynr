/*jslint devel: true es5: true */
/*global $: true, exports: true, module: true, requirejs: true */

(function(undefined) {
	/**
	 * @name TrafficServiceBroadcastFilter
	 * @constructor
	 *
	 * @classdesc
	 */
	var TrafficServiceBroadcastFilter = function TrafficServiceBroadcastFilter() {
		if (!(this instanceof TrafficServiceBroadcastFilter)) {
			return new TrafficServiceBroadcastFilter();
		}

		Object.defineProperty(this, 'filter', {
			enumerable: false,
			value: function filter(outputParameters, filterParameters) {
				var hasTrafficService;

				if (filterParameters.hasTrafficService === null ||
					filterParameters.hasTrafficService === undefined) {
					// filter parameter not set, so we do no filtering
					return true;
				}

				/*
				 * BEWARE: in filterParameters the "hasTrafficService" is delivered
				 * as string, not as boolean
				 */
				hasTrafficService =
					(filterParameters.hasTrafficService === 'true');

				return (outputParameters.getDiscoveredStation().trafficService === hasTrafficService);
			}
		});
	};

	// the following variable is just needed to bypass JSLint check
	// 'typeof' is the only way working without exceptions
	var exportsCheck = typeof exports;
	// AMD support
	if (typeof define === 'function' && define.amd) {
		define(["joynr"], function (joynr) {
			TrafficServiceBroadcastFilter.prototype = new joynr.JoynrObject();
			TrafficServiceBroadcastFilter.prototype.constructor = TrafficServiceBroadcastFilter;
			joynr.addType("joynr.vehicle.TrafficServiceBroadcastFilter", TrafficServiceBroadcastFilter);
			return TrafficServiceBroadcastFilter;
		});
	} else if (exportsCheck !== 'undefined') {
		if ((module !== undefined) && module.exports) {
			exports = module.exports = TrafficServiceBroadcastFilter;
		} else {
			// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
			exports.TrafficServiceBroadcastFilter = TrafficServiceBroadcastFilter;
		}
		var joynr = requirejs("joynr");
		TrafficServiceBroadcastFilter.prototype = new joynr.JoynrObject();
		TrafficServiceBroadcastFilter.prototype.constructor = TrafficServiceBroadcastFilter;

		joynr.addType("joynr.vehicle.TrafficServiceBroadcastFilter", TrafficServiceBroadcastFilter);
	} else {
		TrafficServiceBroadcastFilter.prototype = new window.joynr.JoynrObject();
		TrafficServiceBroadcastFilter.prototype.constructor = TrafficServiceBroadcastFilter;
		window.joynr.addType("joynr.vehicle.TrafficServiceBroadcastFilter", TrafficServiceBroadcastFilter);
		window.TrafficServiceBroadcastFilter = TrafficServiceBroadcastFilter;
	}
}());
