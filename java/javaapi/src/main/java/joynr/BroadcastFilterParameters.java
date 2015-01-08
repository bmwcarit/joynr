package joynr;

import io.joynr.subtypes.JoynrType;

import java.util.HashMap;
import java.util.Map;

import javax.annotation.Nonnull;

/*
 * #%L
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
public class BroadcastFilterParameters implements JoynrType {

    Map<String, Object> filterParameters = new HashMap<String, Object>();

    public Map<String, Object> getFilterParameters() {
        return filterParameters;
    }

    public void setFilterParameters(Map<String, Object> filterParameters) {
        this.filterParameters = filterParameters;
    }

    protected void addFilterParameter(@Nonnull String key, Object value) {
        filterParameters.put(key, value);
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((filterParameters == null) ? 0 : filterParameters.hashCode());
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
        BroadcastFilterParameters other = (BroadcastFilterParameters) obj;
        if (filterParameters == null) {
            if (other.filterParameters != null)
                return false;
        } else if (!filterParameters.equals(other.filterParameters))
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "BroadcastFilterParameters [filterParameters=" + filterParameters + "]";
    }

}
