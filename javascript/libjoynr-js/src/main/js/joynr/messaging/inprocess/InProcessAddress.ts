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
import InProcessMessagingSkeleton = require("./InProcessMessagingSkeleton");

class InProcessAddress {
    private inProcessMessagingSkeleton: InProcessMessagingSkeleton;
    public _typeName = "joynr.system.RoutingTypes.InProcessAddress";
    public static _typeName = "joynr.system.RoutingTypes.InProcessAddress";

    /**
     * @param inProcessMessagingSkeleton the skeleton that should be addressed in process
     */
    public constructor(inProcessMessagingSkeleton: InProcessMessagingSkeleton) {
        this.inProcessMessagingSkeleton = inProcessMessagingSkeleton;
    }

    /**
     * The receive function from the corresponding local messaging receiver
     *
     * @returns the skeleton that should be addressed
     */
    public getSkeleton(): InProcessMessagingSkeleton {
        return this.inProcessMessagingSkeleton;
    }

    public equals(other: any): boolean {
        if (this === other) {
            return true;
        }
        if (other === undefined || other === null) {
            return false;
        }
        if (other._typeName === undefined || this._typeName !== other._typeName) {
            return false;
        }
        if (this.getSkeleton() !== other.getSkeleton()) {
            return false;
        }
        return true;
    }
}

export = InProcessAddress;
