/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.arbitration;

import io.joynr.messaging.routing.TimedDelayed;

public class DelayableArbitration extends TimedDelayed {
    private Arbitrator arbitration;

    public DelayableArbitration(Arbitrator arbitration, long delayForMs) {
        super(delayForMs);
        this.arbitration = arbitration;
    }

    public Arbitrator getArbitration() {
        return arbitration;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((arbitration == null) ? 0 : arbitration.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        DelayableArbitration other = (DelayableArbitration) obj;
        if (arbitration == null) {
            if (other.arbitration != null) {
                return false;
            }
        } else if (!arbitration.equals(other.arbitration)) {
            return false;
        }
        return true;
    }
}
