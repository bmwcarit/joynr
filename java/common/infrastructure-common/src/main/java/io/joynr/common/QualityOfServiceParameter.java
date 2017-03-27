package io.joynr.common;

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

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

public class QualityOfServiceParameter {
    Object parameter;
    Class<?> type;

    @JsonCreator
    public QualityOfServiceParameter(@JsonProperty("parameter") Object parameter) {
        this.parameter = parameter;
        type = parameter.getClass();
    }

    @JsonProperty("parameter")
    public Object getParameter() {
        return parameter;
    }

    public void setParameter(Object parameter) {
        this.parameter = parameter;
    }

    public boolean matches(QualityOfServiceParameter requestedParameter) {
        if (requestedParameter.type != type) {
            return false;
        }
        if (type == String.class) {
            return matchString(requestedParameter);
        } else if (type == int.class) {
            return matchLong(requestedParameter);
        } else if (type == boolean.class) {
            return matchBoolean(requestedParameter);
        } else {
            return false;
        }
    }

    private boolean matchBoolean(QualityOfServiceParameter requestedParameter) {
        return ((Boolean) parameter.equals(requestedParameter.getParameter()));
    }

    private boolean matchLong(QualityOfServiceParameter requestedParameter) {
        if ((Long) parameter >= (Long) requestedParameter.getParameter()) {
            return true;
        } else {
            return false;
        }
    }

    private boolean matchString(QualityOfServiceParameter requestedParameter) {
        if (((String) requestedParameter.getParameter()).compareTo((String) parameter) == 0) {
            return true;
        } else {
            return false;
        }
    }

}
