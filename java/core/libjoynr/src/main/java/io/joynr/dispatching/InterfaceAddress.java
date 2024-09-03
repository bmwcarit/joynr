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
package io.joynr.dispatching;

/**
 * A data structure that contains the domain and interface.
 */
public class InterfaceAddress {

    private String anInterface;
    private String domain;

    public InterfaceAddress(String domain, String anInterface) {
        this.domain = domain;
        this.anInterface = anInterface;
    }

    public InterfaceAddress() {
    }

    /**
     * Copy constructor
     *
     * @param other reference to the object to be copied
     */
    public InterfaceAddress(InterfaceAddress other) {
        this.domain = other.domain;
        this.anInterface = other.anInterface;
    }

    public String getInterface() {
        return anInterface;
    }

    public void setInterface(String aInterface) {
        this.anInterface = aInterface;
    }

    public String getDomain() {
        return domain;
    }

    public void setDomain(String domain) {
        this.domain = domain;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((anInterface == null) ? 0 : anInterface.hashCode());
        result = prime * result + ((domain == null) ? 0 : domain.hashCode());
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
        InterfaceAddress other = (InterfaceAddress) obj;
        if (anInterface == null) {
            if (other.anInterface != null)
                return false;
        } else if (anInterface.compareTo(other.anInterface) != 0)
            return false;
        if (domain == null) {
            if (other.domain != null)
                return false;
        } else if (domain.compareTo(other.domain) != 0)
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "InterfaceAddress [anInterface=" + anInterface + ", domain=" + domain + "]";
    }

}
