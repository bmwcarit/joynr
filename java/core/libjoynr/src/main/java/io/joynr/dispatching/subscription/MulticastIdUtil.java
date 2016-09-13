package io.joynr.dispatching.subscription;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

public final class MulticastIdUtil {

    private MulticastIdUtil() {
    }

    public static String createMulticastId(String providerParticipantId, String multicastName, String... partitions) {
        StringBuilder builder = new StringBuilder(providerParticipantId);
        builder.append("/").append(multicastName);
        if (partitions != null && partitions.length > 0) {
            for (int index = 0; index < partitions.length; index++) {
                builder.append("/").append(partitions[index]);
            }
        }
        return builder.toString();
    }

}
