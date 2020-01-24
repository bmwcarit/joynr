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

import java.io.Serializable;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonIgnore;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.util.ReflectionUtils;

/**
 * Represents a request which isn't answered by a {@link Reply}.
 */
public class OneWayRequest implements JoynrMessageType {

    private static final long serialVersionUID = 1L;

    private String methodName;
    private String[] paramDatatypes;
    private Object[] params;
    private String creatorUserId;
    private Map<String, Serializable> context;

    public OneWayRequest() {
    }

    public OneWayRequest(String methodName, Object[] params, String[] parameterDatatypes) {
        if (methodName == null || methodName.trim().isEmpty()) {
            throw new JoynrIllegalStateException("Cannot create a request with a null or empty method name.");
        }
        this.methodName = methodName;
        if (parameterDatatypes != null) {
            this.paramDatatypes = parameterDatatypes.clone();
        } else {
            paramDatatypes = null;
        }
        if (params != null) {
            this.params = params.clone();
        } else {
            params = null;
        }
    }

    public OneWayRequest(String methodName, Object[] parameters, Class<?>[] parameterDatatypes) {
        this(methodName, parameters, ReflectionUtils.toDatatypeNames(parameterDatatypes));
    }

    public String getMethodName() {
        return methodName;
    }

    @JsonIgnore
    public String getCreatorUserId() {
        return creatorUserId;
    }

    public void setCreatorUserId(String creator) {
        this.creatorUserId = creator;
    }

    public String[] getParamDatatypes() {
        return paramDatatypes == null ? null : paramDatatypes.clone();
    }

    @JsonIgnore
    public boolean hasParamDatatypes() {
        return paramDatatypes != null && paramDatatypes.length > 0;
    }

    public Object[] getParams() {
        return params == null ? null : params.clone();
    }

    @JsonIgnore
    public boolean hasParams() {
        return params != null && params.length > 0;
    }

    @JsonIgnore
    public Map<String, Serializable> getContext() {
        return context;
    }

    public void setContext(Map<String, Serializable> context) {
        this.context = context;
    }

    @JsonIgnore
    public List<String> getFullyQualifiedParamDatatypes() {
        String[] names = getParamDatatypes();
        if (names == null) {
            return null;
        }
        String[] fullyQualifiedNames = new String[names.length];
        for (int i = 0; i < names.length; i++) {
            String typeName = names[i];
            String type = fullyQualifiedNameFor(typeName);
            fullyQualifiedNames[i] = type;
        }
        return Arrays.asList(fullyQualifiedNames);
    }

    private String fullyQualifiedNameFor(String typeName) {
        if (typeName == null) {
            return null;
        }
        String fullyQualifiedName = null;
        if (typeName.equals("Boolean")) {
            fullyQualifiedName = Boolean.class.getCanonicalName();
        } else if (typeName.equals("Byte")) {
            fullyQualifiedName = Byte.class.getCanonicalName();
        } else if (typeName.equals("Short")) {
            fullyQualifiedName = Short.class.getCanonicalName();
        } else if (typeName.equals("Integer")) {
            fullyQualifiedName = Integer.class.getCanonicalName();
        } else if (typeName.equals("Long")) {
            fullyQualifiedName = Long.class.getCanonicalName();
        } else if (typeName.equals("Float")) {
            fullyQualifiedName = Float.class.getCanonicalName();
        } else if (typeName.equals("Double")) {
            fullyQualifiedName = Double.class.getCanonicalName();
        } else if (typeName.equals("String")) {
            fullyQualifiedName = String.class.getCanonicalName();
        } else if (typeName.equals("List")) {
            fullyQualifiedName = List.class.getCanonicalName();
        } else {
            fullyQualifiedName = typeName;
        }
        return fullyQualifiedName;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((methodName == null) ? 0 : methodName.hashCode());
        result = prime * result + Arrays.hashCode(paramDatatypes);
        result = prime * result + Arrays.hashCode(params);
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        OneWayRequest other = (OneWayRequest) obj;
        if (methodName == null) {
            if (other.methodName != null)
                return false;
        } else if (!methodName.equals(other.methodName))
            return false;
        if (!Arrays.deepEquals(paramDatatypes, other.paramDatatypes))
            return false;
        if (!Arrays.deepEquals(params, other.params))
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "OneWayRequest [methodName=" + methodName + ", paramDatatypes=" + Arrays.toString(paramDatatypes)
                + ", params=" + Arrays.toString(params) + "]";
    }

}
