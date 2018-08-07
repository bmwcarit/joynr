/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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
package io.joynr.proxy;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import io.joynr.UsedBy;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.util.AnnotationUtil;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Singleton
public class StatelessAsyncCallbackDirectory {

    private static final Logger logger = LoggerFactory.getLogger(StatelessAsyncCallbackDirectory.class);

    private Map<String, StatelessAsyncCallback> directory = new ConcurrentHashMap<>();

    private ReplyCallerDirectory replyCallerDirectory;

    private final StatelessAsyncIdCalculator statelessAsyncIdCalculator;

    @Inject
    public StatelessAsyncCallbackDirectory(ReplyCallerDirectory replyCallerDirectory,
                                           StatelessAsyncIdCalculator statelessAsyncIdCalculator) {
        this.replyCallerDirectory = replyCallerDirectory;
        this.statelessAsyncIdCalculator = statelessAsyncIdCalculator;
    }

    public void register(StatelessAsyncCallback statelessAsyncCallback) {
        String useCase = statelessAsyncCallback.getUseCaseName();
        StatelessAsyncCallback oldValue = directory.putIfAbsent(useCase, statelessAsyncCallback);
        if (oldValue != null) {
            String message = String.format("Multiple registration attempts of stateless callback for use case %s. %s already registered. %s will not be registered.",
                                           useCase,
                                           oldValue,
                                           statelessAsyncCallback);
            logger.error(message);
            throw new JoynrIllegalStateException(message);
        }
        String callbackId = calculateCallbackId(statelessAsyncCallback);
        replyCallerDirectory.addReplyCaller(callbackId,
                                            new StatelessAsyncReplyCaller(callbackId, statelessAsyncCallback),
                                            ExpiryDate.fromAbsolute(Long.MAX_VALUE));
    }

    public StatelessAsyncCallback get(String useCase) {
        return directory.get(useCase);
    }

    private String calculateCallbackId(StatelessAsyncCallback statelessAsyncCallback) {
        UsedBy usedBy = AnnotationUtil.getAnnotation(statelessAsyncCallback.getClass(), UsedBy.class);
        if (usedBy == null) {
            throw new JoynrIllegalStateException("No @UsedBy annotation found on " + statelessAsyncCallback.getClass());
        }
        try {
            String interfaceName = (String) usedBy.value().getField("INTERFACE_NAME").get(null);
            // Trigger calculation of the participant ID, so that the hashed UUID is registered at startup
            statelessAsyncIdCalculator.calculateParticipantId(interfaceName, statelessAsyncCallback);
            return statelessAsyncIdCalculator.calculateStatelessCallbackId(interfaceName, statelessAsyncCallback);
        } catch (IllegalAccessException | NoSuchFieldException e) {
            throw new JoynrIllegalStateException("Unable to get interface name from " + usedBy, e);
        }
    }
}
