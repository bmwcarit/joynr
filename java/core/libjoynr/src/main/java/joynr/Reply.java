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
package joynr;

import java.util.Arrays;

import io.joynr.exceptions.JoynrException;

/**
 * Value class for the response of a JSON-RPC function call.
 */
public class Reply implements JoynrMessageType {
    private static final long serialVersionUID = 1L;
    private Object[] response;
    private JoynrException error;
    private String requestReplyId;
    private transient String statelessAsyncCallbackId;
    private transient String statelessAsyncCallbackMethodId;

    public Reply() {
    }

    public Reply(String requestReplyId, Object... response) {
        this.requestReplyId = requestReplyId;
        this.response = response;
        this.error = null;
    }

    public Reply(String requestReplyId, JoynrException error) {
        this.requestReplyId = requestReplyId;
        this.error = error;
        this.response = null;
    }

    //The internal implementation has to be exposed here so that RpcUtils can change it
    // TODO: redesign to avoid internal implementation of being exposed
    public Object[] getResponse() {
        return response;
    }

    public JoynrException getError() {
        return error;
    }

    public String getRequestReplyId() {
        return requestReplyId;
    }

    public String getStatelessAsyncCallbackId() {
        return statelessAsyncCallbackId;
    }

    public void setStatelessAsyncCallbackId(String statelessAsyncCallbackId) {
        this.statelessAsyncCallbackId = statelessAsyncCallbackId;
    }

    public String getStatelessAsyncCallbackMethodId() {
        return statelessAsyncCallbackMethodId;
    }

    public void setStatelessAsyncCallbackMethodId(String statelessAsyncCallbackMethodId) {
        this.statelessAsyncCallbackMethodId = statelessAsyncCallbackMethodId;
    }

    @Override
    public String toString() {
        return "Reply: " + "requestReplyId: " + requestReplyId
                + (response == null ? "" : ", response: " + Arrays.toString(response))
                + (error == null ? "" : ", error: " + error)
                + (statelessAsyncCallbackId == null ? "" : ", statelessAsyncCallbackId: " + statelessAsyncCallbackId)
                + (statelessAsyncCallbackMethodId == null ? ""
                        : ", statelessAsyncCallbackMethodId: " + statelessAsyncCallbackMethodId);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        Reply other = (Reply) obj;
        if (error == null) {
            if (other.error != null) {
                return false;
            }
        } else if (!error.equals(other.error)) {
            return false;
        }
        if (requestReplyId == null) {
            if (other.requestReplyId != null) {
                return false;
            }
        } else if (!requestReplyId.equals(other.requestReplyId)) {
            return false;
        }
        if (response == null) {
            if (other.response != null) {
                return false;
            }
        } else if (!Arrays.deepEquals(response, other.response)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((error == null) ? 0 : error.hashCode());
        result = prime * result + ((requestReplyId == null) ? 0 : requestReplyId.hashCode());
        result = prime * result + ((response == null) ? 0 : Arrays.hashCode(response));
        return result;
    }
}
