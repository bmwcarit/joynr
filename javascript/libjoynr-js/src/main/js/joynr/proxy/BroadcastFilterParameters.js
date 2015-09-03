/*global joynr: true */
define("joynr/proxy/BroadcastFilterParameters", [
    "joynr/util/UtilInternal",
    "joynr/system/LoggerFactory"
], function(Util, LoggerFactory) {

    /**
     * Constructor of BroadcastFilterParameters object used for subscriptions in generated proxy objects
     *
     * @constructor
     * @name BroadcastFilterParameters
     *
     * @param {Object}
     *            [filterParameters] the filterParameters object for the constructor call
     *
     * @returns {BroadcastFilterParameters} a BroadcastFilterParameters Object for subscriptions on broadcasts
     */
    function BroadcastFilterParameters(filterParameterProperties) {
        if (!(this instanceof BroadcastFilterParameters)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new BroadcastFilterParameters(filterParameterProperties);
        }

        var log = LoggerFactory.getLogger("joynr.proxy.BroadcastFilterParameters");

        /**
         * @name BroadcastFilterParameters#_typeName
         * @type String
         * @field
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.BroadcastFilterParameters");
        Util.checkPropertyIfDefined(filterParameterProperties, "Object", "filterParameters");

        function makeSetterFunction(obj, parameterName) {
            return function(arg) {
                obj.filterParameters[parameterName] = arg;
                return obj;
            };
        }

        var parameterName;
        var funcName;

        for (parameterName in filterParameterProperties) {
            if (filterParameterProperties.hasOwnProperty(parameterName)) {
                funcName =
                        "set" + parameterName.charAt(0).toUpperCase() + parameterName.substring(1);
                //filter[funcName] = makeSetterFunction(filter, parameterName);
                Object.defineProperty(this, funcName, {
                    configurable : false,
                    writable : false,
                    enumerable : false,
                    value : makeSetterFunction(this, parameterName)
                });
            }
        }

        /**
         * @name BroadcastFilterParameters#filterParameters
         * @type Object
         * @field
         */
        this.filterParameters = {};
    }

    return BroadcastFilterParameters;
});
