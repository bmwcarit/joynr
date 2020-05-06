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
package io.joynr.test.interlanguage;

import java.io.IOException;
import java.util.Arrays;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface;
import joynr.interlanguagetest.TestInterfaceBroadcastWithFilteringBroadcastFilter;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

public class IltStringBroadcastFilter extends TestInterfaceBroadcastWithFilteringBroadcastFilter {
    private static final Logger logger = LoggerFactory.getLogger(IltStringBroadcastFilter.class);
    private ObjectMapper jsonSerializer;

    public IltStringBroadcastFilter(ObjectMapper jsonSerializer) {
        this.jsonSerializer = jsonSerializer;
    }

    @Override
    public boolean filter(String stringOut,
                          String[] stringArrayOut,
                          ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut,
                          StructWithStringArray structWithStringArrayOut,
                          StructWithStringArray[] structWithStringArrayArrayOut,
                          TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastFilterParameters filterParameters) {
        logger.info("IltStringBroadcastFilter: invoked");
        String[] stringArray = { "Hello", "World" };

        // check output parameter contents
        if (!Arrays.equals(stringArray, stringArrayOut)) {
            logger.info("IltStringBroadcastFilter: invalid stringArrayOut value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            logger.info("IltStringBroadcastFilter: invalid enumerationOut value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (!IltUtil.checkStructWithStringArray(structWithStringArrayOut)) {
            logger.info("IltStringBroadcastFilter: invalid structWithStringArrayOut value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
            logger.info("IltStringBroadcastFilter: invalid structWithStringArrayArrayOut value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }

        // check filter parameter contents

        // variables to store deserialized values
        String[] stringArrayOfInterest;
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOfInterest;
        StructWithStringArray structWithStringArrayOfInterest;
        StructWithStringArray[] structWithStringArrayArrayOfInterest;

        try {
            stringArrayOfInterest = jsonSerializer.readValue(filterParameters.getStringArrayOfInterest(),
                                                             String[].class);
        } catch (IOException e) {
            logger.info("IltStringBroadcastFilter: got exception while deserializing stringArrayOfInterest");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (!Arrays.equals(stringArray, stringArrayOfInterest)) {
            logger.info("IltStringBroadcastFilter: invalid stringArrayOfInterest filter parameter value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }

        try {
            enumerationOfInterest = jsonSerializer.readValue(filterParameters.getEnumerationOfInterest(),
                                                             ExtendedTypeCollectionEnumerationInTypeCollection.class);
        } catch (IOException e) {
            logger.info("IltStringBroadcastFilter: got exception while deserializing enumerationOfInterest");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (enumerationOfInterest != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            logger.info("IltStringBroadcastFilter: invalid enumerationOfInterest filter parameter value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }

        try {
            structWithStringArrayOfInterest = jsonSerializer.readValue(filterParameters.getStructWithStringArrayOfInterest(),
                                                                       StructWithStringArray.class);
        } catch (IOException e) {
            logger.info("IltStringBroadcastFilter: got exception while deserializing structWithStringArrayOfInterest");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (!IltUtil.checkStructWithStringArray(structWithStringArrayOfInterest)) {
            logger.info("IltStringBroadcastFilter: invalid structWithStringArrayOfInterest filter parameter value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }

        try {
            structWithStringArrayArrayOfInterest = jsonSerializer.readValue(filterParameters.getStructWithStringArrayArrayOfInterest(),
                                                                            StructWithStringArray[].class);
        } catch (IOException e) {
            logger.info("IltStringBroadcastFilter: got exception while deserializing structWithStringArrayOfInterest");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }
        if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOfInterest)) {
            logger.info("IltStringBroadcastFilter: invalid structWithStringArrayArrayOfInterest filter parameter value");
            logger.info("IltStringBroadcastFilter: FAILED");
            return false;
        }

        // decision for publication is made based on stringOfInterest
        if (stringOut.equals(filterParameters.getStringOfInterest())) {
            logger.info("IltStringBroadcastFilter: OK - publication should be sent");
            return true;
        } else {
            logger.info("IltStringBroadcastFilter: OK - publication should NOT Be sent");
            return false;
        }
    }
}
