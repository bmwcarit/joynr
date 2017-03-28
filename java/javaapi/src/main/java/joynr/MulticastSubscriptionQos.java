package joynr;

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

import io.joynr.pubsub.SubscriptionQos;

/**
 * Class representing the quality of service settings for non-selective broadcasts
 */
public class MulticastSubscriptionQos extends SubscriptionQos {

    private static final long serialVersionUID = 1L;

    /**
     * Default Constructor
     */
    public MulticastSubscriptionQos() {
    }

    @Override
    public MulticastSubscriptionQos setExpiryDateMs(long expiryDateMs) {
        return (MulticastSubscriptionQos) super.setExpiryDateMs(expiryDateMs);
    }

    @Override
    public MulticastSubscriptionQos setValidityMs(long validityMs) {
        return (MulticastSubscriptionQos) super.setValidityMs(validityMs);
    }
}
