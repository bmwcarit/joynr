package io.joynr.util;

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

import io.joynr.dispatching.DestinationAddress;
import io.joynr.dispatching.InterfaceAddress;

import java.util.Random;

public class MessagingTestsUtil {
    private static Random random = new Random();

    public static DestinationAddress randomDestionationAddress(InterfaceAddress interfaceAddress) {
        String channelId = "channel_" + interfaceAddress.getDomain() + random.nextInt();
        DestinationAddress destinationAddress = new DestinationAddress(interfaceAddress, channelId);
        return destinationAddress;
    }

    public static InterfaceAddress randomInterface(String domainPrefix, String interfacePrefix) {
        String randomSuffix = Integer.toString(random.nextInt());
        InterfaceAddress interfaceAddress = new InterfaceAddress(domainPrefix + randomSuffix, interfacePrefix
                + randomSuffix);
        return interfaceAddress;
    }

    public static InterfaceAddress randomInterface() {
        return randomInterface("testDomain", "testInterface");
    }
}
