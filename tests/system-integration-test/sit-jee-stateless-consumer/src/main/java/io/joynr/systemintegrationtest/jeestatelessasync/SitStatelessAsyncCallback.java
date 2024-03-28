/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.systemintegrationtest.jeestatelessasync;

import javax.ejb.ConcurrencyManagement;
import javax.ejb.ConcurrencyManagementType;
import javax.ejb.Stateless;
import com.google.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.CallbackHandler;
import io.joynr.proxy.ReplyContext;
import io.joynr.systemintegrationtest.jee.Util;
import joynr.test.SystemIntegrationTestStatelessAsyncCallback;

@Stateless
@CallbackHandler
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class SitStatelessAsyncCallback implements SystemIntegrationTestStatelessAsyncCallback {

    private static final Logger logger = LoggerFactory.getLogger(SitStatelessAsyncCallback.class);
    static final String USE_CASE = "sit-jee-stateless-async";
    private StatelessResultQueue resultQueue;

    @Inject
    public SitStatelessAsyncCallback(StatelessResultQueue resultQueue) {
        this.resultQueue = resultQueue;
    }

    @Override
    public String getUseCase() {
        return USE_CASE;
    }

    @Override
    public void addSuccess(Integer additionResult, ReplyContext replyContext) {
        logger.info("addSuccess called for messageId {}, result: {}", replyContext.getMessageId(), additionResult);
        resultQueue.addResult("SIT RESULT success: stateless async JEE consumer -> stateless request "
                + replyContext.getMessageId() + ", result: " + additionResult);
    }

    @Override
    public void addFailed(JoynrRuntimeException runtimeException, ReplyContext replyContext) {
        logger.info("addFailed called for messageId {}, error: {}", replyContext.getMessageId(), runtimeException);
        StringBuffer result = new StringBuffer("SIT RESULT error: stateless async JEE consumer -> stateless request ").append(replyContext.getMessageId())
                                                                                                                      .append(" returned exception: ")
                                                                                                                      .append(runtimeException.toString());
        Util.addStacktraceToResultString(runtimeException, result);
        resultQueue.addResult(result.toString());
    }

}
