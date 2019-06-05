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
package io.joynr.capabilities.directory.util;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;
import static io.joynr.messaging.MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE;

import java.util.Arrays;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.runtime.PropertyLoader;
import joynr.exceptions.ProviderRuntimeException;

public class GcdUtilities {
    private static final Logger logger = LoggerFactory.getLogger(GcdUtilities.class);

    public static String[] loadDefaultGbidsFromDefaultMessagingProperties() {
        Properties joynrDefaultProperties = PropertyLoader.loadProperties(DEFAULT_MESSAGING_PROPERTIES_FILE);
        if (!joynrDefaultProperties.containsKey(PROPERTY_GBIDS)) {
            logger.error("No GBIDs found in default properties: " + joynrDefaultProperties);
            throw new JoynrIllegalStateException("No GBIDs found in default properties.");
        }

        return Arrays.stream(joynrDefaultProperties.getProperty(PROPERTY_GBIDS).split(","))
                     .map(a -> a.trim())
                     .toArray(String[]::new);
    }

    public static enum ValidateGBIDsEnum {
        OK, INVALID, UNKNOWN
    };

    /**
     * @param gbids an array of gbids
     * @param gcdGbid the name of gbid of the global capability directory
     */
    public static ValidateGBIDsEnum validateGbids(String[] gbids, String gcdGbid) {
        if (gcdGbid == null || gcdGbid.isEmpty()) {
            logger.error("Cannot validate against empty gcdGbid: {}", gcdGbid);
            throw new IllegalStateException("Cannot validate against empty gcdGbid: " + gcdGbid);
        }
        if (gbids == null || gbids.length == 0) {
            logger.error("INVALID_GBID: provided list of GBIDs is null or empty.");
            return ValidateGBIDsEnum.INVALID;
        } else if (gbids.length > 1) {
            logger.error("MULTIPLE GBIDs {} ARE NOT PERMITTED FOR THE MOMENT", Arrays.toString(gbids));
            throw new ProviderRuntimeException("MULTIPLE GBIDs ARE NOT PERMITTED FOR THE MOMENT");
        } else if (gbids[0] == null || gbids[0].isEmpty()) {
            logger.error("INVALID_GBID: provided GBID is null or empty: {}.", gbids[0]);
            return ValidateGBIDsEnum.INVALID;
        } else if (!gcdGbid.equals(gbids[0])) {
            logger.error("UNKNOWN_GBID: {}", gbids[0]);
            return ValidateGBIDsEnum.UNKNOWN;
        }
        return ValidateGBIDsEnum.OK;
    }
}
