package io.joynr.accesscontrol;

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

import java.util.HashMap;
import java.util.Map;

import joynr.infrastructure.DacTypes.TrustLevel;

public class TrustLevelComparator {

    private static Map<TrustLevel, Integer> trustLevelOrdinalMap = new HashMap<TrustLevel, Integer>();

    static {
        trustLevelOrdinalMap.put(TrustLevel.NONE, 0);
        trustLevelOrdinalMap.put(TrustLevel.LOW, 1);
        trustLevelOrdinalMap.put(TrustLevel.MID, 2);
        trustLevelOrdinalMap.put(TrustLevel.HIGH, 3);
    }

    public static int compare(TrustLevel trustLevelA, TrustLevel trustLevelB) {

        int ordinalA = trustLevelOrdinalMap.get(trustLevelA);
        int ordinalB = trustLevelOrdinalMap.get(trustLevelB);
        int compare = -1;
        if (ordinalA == ordinalB) {
            compare = 0;
        } else if (ordinalA > ordinalB) {
            compare = 1;
        }

        return compare;
    }

}
