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

function makeSetterFunction(
    obj: BroadcastFilterParameters,
    parameterName: string
): (arg: any) => BroadcastFilterParameters {
    return function(arg: any): BroadcastFilterParameters {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        obj.filterParameters![parameterName] = arg;
        return obj;
    };
}

interface BroadcastFilterParameters {
    /** setter functions **/
    [key: string]: ((arg: any) => BroadcastFilterParameters) | any;
}

class BroadcastFilterParameters {
    public _typeName = "joynr.BroadcastFilterParameters";
    public filterParameters: Record<string, any> | null = {};

    /**
     * Constructor of BroadcastFilterParameters object used for subscriptions in generated proxy objects
     *
     * @param [filterParameterProperties] the filterParameters object for the constructor call
     *
     * @returns {BroadcastFilterParameters} a BroadcastFilterParameters Object for subscriptions on broadcasts
     */
    public constructor(filterParameterProperties?: Record<string, any>) {
        Typing.checkPropertyIfDefined(filterParameterProperties, "Object", "filterParameters");

        if (filterParameterProperties === undefined) {
            this.filterParameters = null;
        } else {
            for (const parameterName in filterParameterProperties) {
                if (Object.prototype.hasOwnProperty.call(filterParameterProperties, parameterName)) {
                    const funcName = `set${parameterName.charAt(0).toUpperCase()}${parameterName.substring(1)}`;
                    //filter[funcName] = makeSetterFunction(filter, parameterName);
                    Object.defineProperty(this, funcName, {
                        configurable: false,
                        writable: false,
                        enumerable: false,
                        value: makeSetterFunction(this, parameterName)
                    });
                }
            }
        }
    }
}

export = BroadcastFilterParameters;
