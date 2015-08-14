/*global joynr: true */
define("joynr/provider/BroadcastOutputParameters", [
    "joynr/util/UtilInternal",
    "joynr/system/LoggerFactory"
], function(Util, LoggerFactory) {

    /**
     * Constructor of BroadcastOutputParameters object used for subscriptions in generated provider objects
     *
     * @constructor
     * @name BroadcastOutputParameters
     *
     * @param {Object}
     *            [outputParameters] the outputParameters object for the constructor call
     *
     * @returns {BroadcastOutputParameters} a BroadcastOutputParameters Object for subscriptions on broadcasts
     */
    function BroadcastOutputParameters(outputParameterProperties) {
        if (!(this instanceof BroadcastOutputParameters)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new BroadcastOutputParameters(outputParameterProperties);
        }

        var log = LoggerFactory.getLogger("joynr.provider.BroadcastOutputParameters");

        /**
         * @name BroadcastOutputParameters#_typeName
         * @type String
         * @field
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.BroadcastOutputParameters");
        Util.checkPropertyIfDefined(outputParameterProperties, "Array", "outputParameters");

        function makeSetterFunction(obj, pos) {
            return function(arg) {
                obj.outputParameters[pos] = arg;
                return obj;
            };
        }
        function makeGetterFunction(obj, pos) {
            return function() {
                return obj.outputParameters[pos];
            };
        }

        var parameterName;
        var setterFuncName;
        var getterFuncName;
        var i;

        //for (parameterName in outputParameterProperties) {
        for (i = 0; i < outputParameterProperties.length; i++) {
            if (outputParameterProperties[i].hasOwnProperty("name")) {
                parameterName = outputParameterProperties[i].name;
                setterFuncName =
                        "set" + parameterName.charAt(0).toUpperCase() + parameterName.substring(1);
                //output[funcName] = makeSetterFunction(output, parameterName);
                Object.defineProperty(this, setterFuncName, {
                    configurable : false,
                    writable : false,
                    enumerable : false,
                    value : makeSetterFunction(this, i)
                });
                getterFuncName =
                        "get" + parameterName.charAt(0).toUpperCase() + parameterName.substring(1);
                Object.defineProperty(this, getterFuncName, {
                    configurable : false,
                    writable : false,
                    enumerable : false,
                    value : makeGetterFunction(this, i)
                });
            }
        }

        /**
         * @name BroadcastOutputParameters#outputParameters
         * @type Array
         * @field
         */
        this.outputParameters = [];
    }

    return BroadcastOutputParameters;
});
