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

import joynr from "joynr";
import { EchoProviderImplementation } from "../generated-javascript/joynr/tests/performance/EchoProvider";

export const implementation: EchoProviderImplementation = {
    simpleAttribute: {
        set() {},

        get() {
            return "hi";
        }
    },

    echoString(opArgs) {
        return { responseData: opArgs.data };
    },

    echoByteArray(opArgs) {
        if (opArgs.data === undefined) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "echoByteArray: invalid argument data"
            });
        } else {
            return { responseData: opArgs.data };
        }
    },

    echoComplexStruct(opArgs) {
        if (opArgs.data === undefined) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "echoComplexStruct: invalid argument data"
            });
        } else {
            return { responseData: opArgs.data };
        }
    }
};
