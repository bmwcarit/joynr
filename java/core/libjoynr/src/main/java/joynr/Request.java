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

import static io.joynr.util.JoynrUtil.createUuidString;

import java.util.Arrays;

import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;
import io.joynr.util.ReflectionUtils;

/**
 * This is a value class that represents a JoynRPC function call as JSON. The class also offers a function to
 * deserialize the parameters as an Object[] using meta information.
 */
public class Request extends OneWayRequest implements JoynrMessageType {

    private static final long serialVersionUID = 1L;
    private String requestReplyId;
    @SuppressFBWarnings("SE_TRANSIENT_FIELD_NOT_RESTORED")
    private transient String statelessAsyncCallbackMethodId;

    public Request() {
    }

    @SuppressFBWarnings("EI_EXPOSE_REP2")
    public Request(String methodName,
                   Object[] params,
                   String[] paramDatatypes,
                   String requestReplyId,
                   String statelessAsyncCallbackMethodId) {
        super(methodName, params, paramDatatypes);
        if (requestReplyId == null) {
            this.requestReplyId = createUuidString();
        } else {
            this.requestReplyId = requestReplyId;
        }
        this.statelessAsyncCallbackMethodId = statelessAsyncCallbackMethodId;
    }

    @SuppressFBWarnings("EI_EXPOSE_REP2")
    public Request(String methodName, Object[] params, String[] paramDatatypes, String requestReplyId) {
        this(methodName, params, paramDatatypes, requestReplyId, null);
    }

    public Request(String name, Object[] params, Class<?>[] parameterTypes) {
        this(name, params, ReflectionUtils.toDatatypeNames(parameterTypes), null);
    }

    public Request(String name,
                   Object[] params,
                   Class<?>[] parameterTypes,
                   String requestReplyId,
                   String statelessAsyncCallbackMethodId) {
        this(name,
             params,
             ReflectionUtils.toDatatypeNames(parameterTypes),
             requestReplyId,
             statelessAsyncCallbackMethodId);
    }

    public String getRequestReplyId() {
        return requestReplyId;
    }

    public String getStatelessAsyncCallbackMethodId() {
        return statelessAsyncCallbackMethodId;
    }

    @Override
    public String toString() {
        return "Request: " + getMethodName() + ", requestReplyId: " + requestReplyId + ", params: "
                + Arrays.toString(getParams());
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((requestReplyId == null) ? 0 : requestReplyId.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (!super.equals(obj))
            return false;
        if (getClass() != obj.getClass())
            return false;
        Request other = (Request) obj;
        if (requestReplyId == null) {
            if (other.requestReplyId != null)
                return false;
        } else if (!requestReplyId.equals(other.requestReplyId))
            return false;
        return true;
    }
}
