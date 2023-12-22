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

import java.util.ArrayList;
import java.util.List;

import io.joynr.exceptions.JoynrRuntimeException;

public abstract class AbstractPublication implements JoynrMessageType {

    private static final long serialVersionUID = 1L;

    private List<? extends Object> response;
    private JoynrRuntimeException error;

    public AbstractPublication() {
    }

    public AbstractPublication(List<? extends Object> response) {
        this.response = (response != null) ? new ArrayList<>(response) : null;
    }

    public AbstractPublication(JoynrRuntimeException error) {
        this.error = error;
    }

    public Object getResponse() {
        return (response != null) ? new ArrayList<>(response) : null;
    }

    public JoynrRuntimeException getError() {
        return error;
    }

    @Override
    public String toString() {
        return "AbstractPublication [" + (response != null ? "response=" + response + ", " : "")
                + (error != null ? "error=" + error : "") + "]";
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
        AbstractPublication other = (AbstractPublication) obj;
        if (error == null) {
            if (other.error != null) {
                return false;
            }
        } else if (!error.equals(other.error)) {
            return false;
        }
        if (response == null) {
            if (other.response != null) {
                return false;
            }
        } else if (!response.equals(other.response)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((error == null) ? 0 : error.hashCode());
        result = prime * result + ((response == null) ? 0 : response.hashCode());
        return result;
    }
}
