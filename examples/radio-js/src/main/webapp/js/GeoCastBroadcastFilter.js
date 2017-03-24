/*jslint devel: true es5: true */
/*global $: true, exports: true, module: true, requirejs: true */

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

(function(undefined) {
	/**
	 * @name GeocastBroadcastFilter
	 * @constructor
	 *
	 * @classdesc
	 */
	var GeocastBroadcastFilter = function GeocastBroadcastFilter() {
		if (!(this instanceof GeocastBroadcastFilter)) {
			return new GeocastBroadcastFilter();
		}

		Object.defineProperty(this, 'filter', {
			enumerable: false,
			value: function filter(outputParameters, filterParameters) {
				var radiusOfInterestArea;
				var positionOfInterest;

				if (filterParameters.positionOfInterest === null ||
					filterParameters.positionOfInterest === undefined) {
					// filter parameter not set, so we do no filtering
					return true;
				}
				try {
					positionOfInterest = JSON.parse(filterParameters.positionOfInterest);
				} catch(e) {
					// cannot parse filterparameters
					return true;
				}

				radiusOfInterestArea = parseInt(filterParameters.radiusOfInterestArea, 10);
				if (radiusOfInterestArea === "NaN") {
					// cannot parse filterparameters
					return true;
				}

				// calculate distance between two geo positions using the haversine formula
				// (cf. http://en.wikipedia.org/wiki/Haversine_formula)
				var earthRadius = 6371000; // in meters

				if (Math.toRad === undefined) {
					Math.toRad = function(x) {
						return x * Math.PI / 180;
					};
				}

				var geoPosition = outputParameters.getGeoPosition();

				var lat1 = Math.toRad(geoPosition.latitude);
				var lat2 = Math.toRad(positionOfInterest.latitude);
				var long1 = Math.toRad(geoPosition.longitude);
				var long2 = Math.toRad(positionOfInterest.longitude);

				var latSinePow = Math.pow(Math.sin((lat2 - lat1) / 2), 2.0);
				var longSinePow = Math.pow(Math.sin((long2 - long1) / 2), 2.0);
				var help = Math.sqrt(latSinePow + Math.cos(lat1) * Math.cos(lat2) * longSinePow);
				if (help > 1.0) {
					help = 1.0;
				}
				var distance = 2 * earthRadius * Math.asin(help);
				console.log("distance = " + distance);
				return distance < radiusOfInterestArea;
			}
		});
	};

	// the following variable is just needed to bypass JSLint check
	// 'typeof' is the only way working without exceptions
	var exportsCheck = typeof exports;
	// AMD support
	if (typeof define === 'function' && define.amd) {
		define(["joynr"], function (joynr) {
			GeocastBroadcastFilter.prototype = new joynr.JoynrObject();
			GeocastBroadcastFilter.prototype.constructor = GeocastBroadcastFilter;
			joynr.addType("joynr.vehicle.GeocastBroadcastFilter", GeocastBroadcastFilter);
			return GeocastBroadcastFilter;
		});
	} else if (exportsCheck !== 'undefined') {
		if ((module !== undefined) && module.exports) {
			exports = module.exports = GeocastBroadcastFilter;
		} else {
			// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
			exports.GeocastBroadcastFilter = GeocastBroadcastFilter;
		}
		var joynr = requirejs("joynr");
		GeocastBroadcastFilter.prototype = new joynr.JoynrObject();
		GeocastBroadcastFilter.prototype.constructor = GeocastBroadcastFilter;

		joynr.addType("joynr.vehicle.GeocastBroadcastFilter", GeocastBroadcastFilter);
	} else {
		GeocastBroadcastFilter.prototype = new window.joynr.JoynrObject();
		GeocastBroadcastFilter.prototype.constructor = GeocastBroadcastFilter;
		window.joynr.addType("joynr.vehicle.GeocastBroadcastFilter", GeocastBroadcastFilter);
		window.GeocastBroadcastFilter = GeocastBroadcastFilter;
	}
}());
