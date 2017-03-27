package io.joynr.capabilities;

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

import io.joynr.ProvidedBy;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.provider.JoynrInterface;

public final class ParticipantIdKeyUtil {

    public static final String JOYNR_PARTICIPANT_PREFIX = "joynr.participant.";

    private ParticipantIdKeyUtil() {
    }

    public static String getProviderParticipantIdKey(String domain, String interfaceName) {
        String token = JOYNR_PARTICIPANT_PREFIX + domain + "." + interfaceName;
        return token.toLowerCase().replace('/', '.');
    }

    public static String getProviderParticipantIdKey(String domain, Class interfaceClass) {
        Class annotatedInterface;
        if (interfaceClass.getAnnotation(ProvidedBy.class) != null) {
            annotatedInterface = ((ProvidedBy) interfaceClass.getAnnotation(ProvidedBy.class)).value();
        } else {
            annotatedInterface = interfaceClass;
        }
        JoynrInterface joynrInterface = (JoynrInterface) annotatedInterface.getAnnotation(JoynrInterface.class);
        if (joynrInterface == null) {
            throw new JoynrIllegalStateException("Class " + annotatedInterface
                    + " not annotated with @JoynrInterface. Can't get interface name.");
        }
        String interfaceName = joynrInterface.name();
        return getProviderParticipantIdKey(domain, interfaceName);
    }

}
