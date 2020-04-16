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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.stream.Collectors;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.runtime.PropertyLoader;
import joynr.types.GlobalDiscoveryEntry;

public class GcdUtilities {
    private static final Logger logger = LoggerFactory.getLogger(GcdUtilities.class);

    public static String[] loadDefaultGbidsFromDefaultMessagingProperties() {
        Properties joynrDefaultProperties = PropertyLoader.loadProperties(DEFAULT_MESSAGING_PROPERTIES_FILE);
        if (!joynrDefaultProperties.containsKey(PROPERTY_GBIDS)) {
            logger.error("No GBIDs found in default properties: {}", joynrDefaultProperties);
            throw new JoynrIllegalStateException("No GBIDs found in default properties.");
        }

        return Arrays.stream(joynrDefaultProperties.getProperty(PROPERTY_GBIDS).split(","))
                     .map(a -> a.trim())
                     .toArray(String[]::new);
    }

    public static Set<String> convertArrayStringToSet(String arrayString, String defaultElement) {

        Set<String> validGbidsSet = new HashSet<>();
        if (arrayString == null || arrayString.isEmpty()) {
            validGbidsSet.add(defaultElement);
        } else {
            String[] validGbidsArray = Arrays.stream(arrayString.split(",")).map(a -> a.trim()).toArray(String[]::new);
            validGbidsSet.addAll(Arrays.asList(validGbidsArray));
        }
        return validGbidsSet;
    }

    public static enum ValidateGBIDsEnum {
        OK, INVALID, UNKNOWN
    };

    /**
     * @param gbids an array of gbids
     * @param gcdGbid the name of gbid of the global capability directory
     */
    public static ValidateGBIDsEnum validateGbids(String[] gbids, String gcdGbid, Set<String> validGbids) {
        if (gcdGbid == null || gcdGbid.isEmpty()) {
            logger.error("Cannot validate against empty gcdGbid: {}", gcdGbid);
            throw new IllegalStateException("Cannot validate against empty gcdGbid: " + gcdGbid);
        }
        if (gbids == null || gbids.length == 0) {
            logger.error("INVALID_GBID: provided list of GBIDs is null or empty.");
            return ValidateGBIDsEnum.INVALID;
        }

        HashSet<String> gbidSet = new HashSet<String>();
        for (String gbid : gbids) {
            if (gbid == null || gbidSet.contains(gbid)) {
                logger.error("INVALID_GBID: provided GBID is null or duplicate: {}.", gbid);
                return ValidateGBIDsEnum.INVALID;
            }
            gbidSet.add(gbid);

            if (!validGbids.contains(gbid)) {
                if (gbid.isEmpty()) {
                    logger.warn("Provided GBID is empty.");
                } else {
                    logger.error("UNKNOWN_GBID: provided GBID is unknown: {}.", gbid);
                    return ValidateGBIDsEnum.UNKNOWN;
                }
            }
        }

        return ValidateGBIDsEnum.OK;
    }

    public static GlobalDiscoveryEntry[] chooseOneGlobalDiscoveryEntryPerParticipantId(Collection<GlobalDiscoveryEntryPersisted> queryResult,
                                                                                       String preferredGbid) {
        Map<String, List<GlobalDiscoveryEntryPersisted>> gdepGrouped = queryResult.stream()
                                                                                  .collect(Collectors.groupingBy(GlobalDiscoveryEntryPersisted::getParticipantId));

        List<GlobalDiscoveryEntry> globalDiscoveryEntries = new ArrayList<GlobalDiscoveryEntry>();
        for (List<GlobalDiscoveryEntryPersisted> gdepListForSingleParticipantId : gdepGrouped.values()) {
            globalDiscoveryEntries.add(GcdUtilities.chooseOneGlobalDiscoveryEntry(gdepListForSingleParticipantId,
                                                                                  preferredGbid));
        }

        GlobalDiscoveryEntry[] globalDiscoveryEntriesArray = new GlobalDiscoveryEntry[globalDiscoveryEntries.size()];
        globalDiscoveryEntriesArray = globalDiscoveryEntries.toArray(globalDiscoveryEntriesArray);
        return globalDiscoveryEntriesArray;
    }

    public static GlobalDiscoveryEntry chooseOneGlobalDiscoveryEntry(Collection<GlobalDiscoveryEntryPersisted> gdepListPerParticipantId,
                                                                     String preferredGbid) {
        logger.debug("Number of entries stored in the list of GlobalDiscoveryEntryPersistedPerParticipandId {}",
                     gdepListPerParticipantId.size());
        if (gdepListPerParticipantId.isEmpty()) {
            logger.error("FATAL: gdepListPerParticipantId is empty."); // should never happen
            throw new JoynrIllegalStateException("FATAL: gdepListPerParticipantId is empty.");
        }

        GlobalDiscoveryEntryPersisted gdep = gdepListPerParticipantId.stream()
                                                                     .filter(globalDiscoveryEntryPersisted -> preferredGbid.equals(globalDiscoveryEntryPersisted.getGbid()))
                                                                     .findAny()
                                                                     .orElse(gdepListPerParticipantId.iterator()
                                                                                                     .next());

        GlobalDiscoveryEntry selectedGlobalDiscoveryEntry = new GlobalDiscoveryEntry(gdep);

        logger.debug("Selecting persisted GlobalDiscoveryEntry {}", selectedGlobalDiscoveryEntry);

        return selectedGlobalDiscoveryEntry;
    }
}
