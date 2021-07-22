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
package io.joynr.messaging.inprocess;

import java.io.Serializable;

import io.joynr.subtypes.JoynrType;
import joynr.system.RoutingTypes.LocalAddress;

public class InProcessAddress extends LocalAddress implements Serializable, JoynrType {

    private static final long serialVersionUID = -647813790331888374L;

    private transient InProcessMessagingSkeleton skeleton;

    public InProcessAddress() {
        this(null);
    }

    public InProcessAddress(InProcessMessagingSkeleton skeleton) {
        this.skeleton = skeleton;
    }

    public void setSkeleton(InProcessMessagingSkeleton skeleton) {
        this.skeleton = skeleton;
    }

    public InProcessMessagingSkeleton getSkeleton() {
        return skeleton;
    }

    @Override
    public boolean equals(Object obj) {
        return super.equals(obj) && ((InProcessAddress) obj).getSkeleton() == skeleton;
    }

    @Override
    public int hashCode() {
        return super.hashCode();
    }

    @Override
    public String toString() {
        return "InProcessAddress [" + super.toString() + ", messagingSkeleton: " + skeleton + "]";
    }
}
