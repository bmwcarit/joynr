package joynr;

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

import java.util.List;

import io.joynr.exceptions.JoynrRuntimeException;

public class MulticastPublication extends AbstractPublication {

    private static final long serialVersionUID = 1L;

    private String multicastId;

    public MulticastPublication() {
    }

    public MulticastPublication(List<? extends Object> response, String multicastId) {
        super(response);
        this.multicastId = multicastId;
    }

    public MulticastPublication(JoynrRuntimeException error, String multicastId) {
        super(error);
        this.multicastId = multicastId;
    }

    public String getMulticastId() {
        return multicastId;
    }

    public void setMulticastId(String multicastId) {
        this.multicastId = multicastId;
    }

    @Override
    public String toString() {
        return "MulticastPublication [" + "multicastId=" + multicastId + ", "
                + (getResponse() != null ? "response=" + getResponse() + ", " : "")
                + (getError() != null ? "error=" + getError() : "") + "]";
    }

    @Override
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if (o == null || getClass() != o.getClass())
            return false;
        if (!super.equals(o))
            return false;

        MulticastPublication that = (MulticastPublication) o;

        return multicastId.equals(that.multicastId);

    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + multicastId.hashCode();
        return result;
    }
}
