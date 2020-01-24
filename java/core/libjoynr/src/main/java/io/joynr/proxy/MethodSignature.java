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
package io.joynr.proxy;

import java.util.Arrays;
import java.util.List;

import io.joynr.dispatching.RequestCaller;

public class MethodSignature {
    private RequestCaller requestCaller;
    private String methodName;
    private String[] parameterTypeNames;

    public MethodSignature(RequestCaller requestCaller, String methodName, String[] parameterTypeNames) {
        this.requestCaller = requestCaller;
        this.methodName = methodName;
        this.parameterTypeNames = parameterTypeNames == null ? null : parameterTypeNames.clone();
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((parameterTypeNames == null) ? 0 : Arrays.hashCode(parameterTypeNames));
        result = prime * result + ((methodName == null) ? 0 : methodName.hashCode());
        result = prime * result + ((requestCaller == null) ? 0 : requestCaller.getProxy().getClass().hashCode());
        return result;
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
        MethodSignature other = (MethodSignature) obj;
        if (parameterTypeNames == null) {
            if (other.parameterTypeNames != null) {
                return false;
            }
        } else if (!Arrays.equals(parameterTypeNames, other.parameterTypeNames)) {
            return false;
        }
        if (methodName == null) {
            if (other.methodName != null) {
                return false;
            }
        } else if (!methodName.equals(other.methodName)) {
            return false;
        }
        if (requestCaller == null) {
            if (other.requestCaller != null) {
                return false;
            }
        } else if (!requestCaller.getProxy().getClass().equals(other.requestCaller.getProxy().getClass())) {
            return false;
        }
        return true;
    }

    public RequestCaller getRequestCaller() {
        return requestCaller;
    }

    public String getMethodName() {
        return methodName;
    }

    public List<String> getParameterTypeNames() {
        return Arrays.asList(parameterTypeNames);
    }

}
