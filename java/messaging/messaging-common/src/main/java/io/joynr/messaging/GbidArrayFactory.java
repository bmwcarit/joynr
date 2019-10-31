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
package io.joynr.messaging;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;

import java.util.Arrays;
import java.util.HashSet;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class GbidArrayFactory {

    private static final Logger logger = LoggerFactory.getLogger(GbidArrayFactory.class);
    private String[] gbidArray;

    @Inject
    public GbidArrayFactory(@Named(PROPERTY_GBIDS) String gbids) {
        gbidArray = Arrays.stream(gbids.split(",")).map(a -> a.trim()).toArray(String[]::new);
        if (gbidArray.length == 0) {
            if (gbids.contains(",")) {
                throw new JoynrIllegalStateException("just comma is not allowed in gbids");
            }
            logger.warn("Using empty GBID.");
            gbidArray = new String[]{ "" };
        } else if (gbidArray.length != 1 && Arrays.stream(gbidArray).anyMatch(gbid -> gbid.isEmpty())) {
            // empty string is only allowed for single GBID
            logger.error("GBID must not be empty: {}!", gbids);
            throw new JoynrIllegalStateException("GBID must not be empty: " + gbids + "!");
        }
        HashSet<String> gbidsSet = new HashSet<String>();
        for (String gbid : gbidArray) {
            if (gbidsSet.contains(gbid)) {
                throw new JoynrIllegalStateException("duplicate gbid found");
            }
            gbidsSet.add(gbid);
        }
    }

    public String[] create() {
        return gbidArray.clone();
    }
}
