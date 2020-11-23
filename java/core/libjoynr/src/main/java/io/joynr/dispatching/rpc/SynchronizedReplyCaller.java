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
package io.joynr.dispatching.rpc;

import java.util.List;

import joynr.Reply;
import joynr.Request;

public class SynchronizedReplyCaller implements ReplyCaller {
    private List<Object> responsePayloadContainer;
    final private String fromParticipantId;
    final private String requestReplyId;
    final private Request request;

    public SynchronizedReplyCaller(String fromParticipantId, String requestReplyId, Request request) {
        this.fromParticipantId = fromParticipantId;
        this.requestReplyId = requestReplyId;
        this.request = request;
    }

    @Override
    public void messageCallBack(Reply payload) {
        if (responsePayloadContainer == null) {
            throw new IllegalStateException("SynchronizedReplyCaller: ResponseContainer not set!");
        }
        synchronized (responsePayloadContainer) {
            responsePayloadContainer.add(payload);
            responsePayloadContainer.notify();
        }

    }

    @Override
    public void error(Throwable error) {
        synchronized (responsePayloadContainer) {
            responsePayloadContainer.add(error);
            responsePayloadContainer.notify();
        }

    }

    public void setResponseContainer(List<Object> responsePayloadContainer) {
        this.responsePayloadContainer = responsePayloadContainer;

    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("ReplyCaller: ");
        stringBuilder.append("\r\n");
        stringBuilder.append("sender: ");
        stringBuilder.append(fromParticipantId);
        stringBuilder.append("\r\n");
        stringBuilder.append("requestReplyId: ");
        stringBuilder.append(requestReplyId);
        stringBuilder.append("\r\n");
        stringBuilder.append("request: ");
        stringBuilder.append(request);
        stringBuilder.append("\r\n");
        return stringBuilder.toString();
    }
}
