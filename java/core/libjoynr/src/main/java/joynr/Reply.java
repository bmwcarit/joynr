package joynr;

/*
 * #%L
 * joynr::java::messaging::messagingcommon
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

/**
 * Value class for the response of a JSON-RPC function call.
 */
public class Reply implements JoynrMessageType {
    private Object response;
    private String requestReplyId;

    public Reply() {
    }

    public Reply(String requestReplyId, Object response) {
        this.requestReplyId = requestReplyId;
        this.response = response;
    }

    public Object getResponse() {
        return response;
    }

    public String getRequestReplyId() {
        return requestReplyId;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;

        Reply other = (Reply) obj;

        // always non null
        if (!requestReplyId.equals(other.requestReplyId)) {
            return false;
        }

        // always non-null
        if (!response.equals(other.response)) {
            return false;
        }

        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((requestReplyId == null) ? 0 : requestReplyId.hashCode());
        result = prime * result + ((response == null) ? 0 : response.hashCode());
        return result;
    }
}
