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

import java.util.Arrays;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.exceptions.ProviderRuntimeException;

public class Utilities {
    private static final Logger logger = LoggerFactory.getLogger(Utilities.class);

    public static enum ValidateGBIDsEnum {
        OK, INVALID, UNKNOWN
    };

    /**
     * @param gbids an array of gbids
     * @param gcdGbId the name of gbid of the global capability directory
     */
    public static ValidateGBIDsEnum validateGbids(String[] gbids, String gcdGbId) {
        if (gcdGbId == null || gcdGbId.isEmpty()) {
            logger.error("Cant validate against empty gcdGbId: {}", gcdGbId);
            throw new IllegalStateException();
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
        } else if (!gcdGbId.equals(gbids[0])) {
            logger.error("UNKNOWN_GBID: {}", gbids[0]);
            return ValidateGBIDsEnum.UNKNOWN;
        }
        return ValidateGBIDsEnum.OK;
    }
}
