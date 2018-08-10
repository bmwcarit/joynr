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

import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.util.AnnotationUtil;
import joynr.Reply;
import joynr.exceptions.ApplicationException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.regex.Pattern;

public class StatelessAsyncReplyCaller implements ReplyCaller {

    private static final Logger logger = LoggerFactory.getLogger(StatelessAsyncReplyCaller.class);

    private static final Pattern CALLBACK_ID_REGEX = Pattern.compile("^.*:#:(.*)$");

    private final String statelessAsyncCallbackId;
    private final StatelessAsyncCallback statelessAsyncCallback;

    public StatelessAsyncReplyCaller(String statelessAsyncCallbackId, StatelessAsyncCallback statelessAsyncCallback) {
        this.statelessAsyncCallbackId = statelessAsyncCallbackId;
        this.statelessAsyncCallback = statelessAsyncCallback;
    }

    @Override
    public void messageCallBack(Reply payload) {
        String methodCorrelationId = payload.getStatelessCallbackMethodId();
        boolean success = payload.getError() == null;
        boolean withApplicationError = payload.getError() != null && payload.getError() instanceof ApplicationException;
        boolean withException = payload.getError() != null && !(payload.getError() instanceof ApplicationException);
        // Filter chain:
        // - all methods where the correlation ID matches the one we're looking for
        // - if success or withException, then method name must end in Success, otherwise end in Failed
        // - if we're looking for a failure callback and the error payload is an application error, the first
        //   parameter must be an enum
        Method callbackMethod = Arrays.stream(statelessAsyncCallback.getClass().getMethods()).filter(method -> {
            StatelessCallbackCorrelation callbackCorrelation = AnnotationUtil.getAnnotation(method,
                                                                                            StatelessCallbackCorrelation.class);
            return callbackCorrelation != null && callbackCorrelation.value().equals(methodCorrelationId);
        })
                                      .filter(method -> method.getName()
                                                              .endsWith((success || withException) ? "Success"
                                                                      : "Failed"))
                                      .filter(method -> success || withException
                                              || (withApplicationError && method.getParameterTypes()[0].isEnum()))
                                      .findFirst()
                                      .orElseThrow(() -> new JoynrRuntimeException("No suitable callback method found for callback ID "
                                              + payload.getStatelessCallback() + " on " + statelessAsyncCallback));
        if (withException) {
            String methodName = callbackMethod.getName().replaceFirst("Success$", "Failed");
            try {
                callbackMethod = statelessAsyncCallback.getClass().getMethod(methodName,
                                                                             new Class[]{ JoynrRuntimeException.class,
                                                                                     ReplyContext.class });
            } catch (NoSuchMethodException e) {
                throw new JoynrRuntimeException("No suitable failure callback method named " + methodName
                        + " found for callback ID " + payload.getStatelessCallback() + " on " + statelessAsyncCallback);
            }
        }
        try {
            if (success) {
                callbackMethod.invoke(statelessAsyncCallback,
                                      addReplyContext(payload.getResponse(), payload.getRequestReplyId()));
            } else {
                callbackMethod.invoke(statelessAsyncCallback,
                                      addReplyContext(extractErrorEnum(payload), payload.getRequestReplyId()));
            }
        } catch (IllegalAccessException | InvocationTargetException e) {
            logger.error("Error calling callback method {} with reply {}", callbackMethod, payload, e);
        }
    }

    private Object[] extractErrorEnum(Reply payload) {
        JoynrException exception = payload.getError();
        if (exception instanceof ApplicationException) {
            return new Object[]{ ((ApplicationException) exception).getError() };
        } else if (exception instanceof JoynrRuntimeException) {
            return new Object[]{ payload.getError() };
        }
        return new Object[]{
                new JoynrRuntimeException("Unexpected error payload, neither JoynrRuntimeException nor ApplicationException: "
                        + exception) };
    }

    private Object[] addReplyContext(Object[] parameters, String requestReplyId) {
        Object[] parametersWithMessageId = Arrays.copyOf(parameters, parameters.length + 1);
        parametersWithMessageId[parametersWithMessageId.length - 1] = new ReplyContext(requestReplyId);
        return parametersWithMessageId;
    }

    @Override
    public void error(Throwable error) {
        logger.error("Error occurred while handling stateless async reply.", error);
    }

    @Override
    public String getRequestReplyId() {
        return statelessAsyncCallbackId;
    }
}
