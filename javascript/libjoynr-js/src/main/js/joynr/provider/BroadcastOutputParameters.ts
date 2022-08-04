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
import * as Typing from "../util/Typing";

function makeSetterFunction(obj: BroadcastOutputParameters, pos: number): (arg: any) => BroadcastOutputParameters {
    return function(arg: any) {
        obj.outputParameters[pos] = arg;
        return obj;
    };
}
function makeGetterFunction(obj: BroadcastOutputParameters, pos: number): () => any {
    return function() {
        return obj.outputParameters[pos];
    };
}

interface BroadcastOutputParameters {
    /**
     * setter and getter functions or public members.
     **/
    [key: string]: ((arg: any) => BroadcastOutputParameters) | (() => any) | any;
}

class BroadcastOutputParameters {
    // @ts-ignore
    public outputParameters: any[];
    // @ts-ignore
    public _typeName = "joynr.BroadcastOutputParameters";

    /**
     * Constructor of BroadcastOutputParameters object used for subscriptions in generated provider objects
     *
     * @param [outputParameterProperties] the outputParameters object for the constructor call
     */
    public constructor(outputParameterProperties: Record<string, any>) {
        Typing.checkPropertyIfDefined(outputParameterProperties, "Array", "outputParameters");

        //for (parameterName in outputParameterProperties) {
        for (let i = 0; i < outputParameterProperties.length; i++) {
            if (Object.prototype.hasOwnProperty.call(outputParameterProperties[i], "name")) {
                const parameterName = outputParameterProperties[i].name;
                const setterFuncName = `set${parameterName.charAt(0).toUpperCase()}${parameterName.substring(1)}`;
                //output[funcName] = makeSetterFunction(output, parameterName);
                Object.defineProperty(this, setterFuncName, {
                    configurable: false,
                    writable: false,
                    enumerable: false,
                    value: makeSetterFunction(this, i)
                });
                const getterFuncName = `get${parameterName.charAt(0).toUpperCase()}${parameterName.substring(1)}`;
                Object.defineProperty(this, getterFuncName, {
                    configurable: false,
                    writable: false,
                    enumerable: false,
                    value: makeGetterFunction(this, i)
                });
            }
        }

        this.outputParameters = [];
    }
}

export = BroadcastOutputParameters;
