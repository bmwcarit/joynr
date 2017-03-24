package io.joynr.accesscontrol.primarykey;

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

import java.io.Serializable;

/**
 * A key consisting of uid, domain, interface, operation
 */
public class UserDomainInterfaceOperationKey implements Serializable {
    private static final long serialVersionUID = 91769583869440378L;

    // these constants have to match field names so EhCache ReflectionAttributeExtractor may do it's magic
    public static final String USER_ID = "uid";
    public static final String DOMAIN = "domain";
    public static final String INTERFACE = "interfaceName";
    public static final String OPERATION = "operation";

    private final String uid;
    private final String domain;
    private final String interfaceName;
    private final String operation;

    public UserDomainInterfaceOperationKey(String uid, String domain, String interfaceName, String operation) {
        this.uid = uid;
        this.domain = domain;
        this.interfaceName = interfaceName;
        this.operation = operation;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int hashcode = 1;
        hashcode = prime * hashcode + ((domain == null) ? 0 : domain.hashCode());
        hashcode = prime * hashcode + ((interfaceName == null) ? 0 : interfaceName.hashCode());
        hashcode = prime * hashcode + ((operation == null) ? 0 : operation.hashCode());
        hashcode = prime * hashcode + ((uid == null) ? 0 : uid.hashCode());
        return hashcode;
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
        UserDomainInterfaceOperationKey other = (UserDomainInterfaceOperationKey) obj;
        if (domain == null) {
            if (other.domain != null) {
                return false;
            }
        } else if (!domain.equals(other.domain)) {
            return false;
        }
        if (interfaceName == null) {
            if (other.interfaceName != null) {
                return false;
            }
        } else if (!interfaceName.equals(other.interfaceName)) {
            return false;
        }
        if (operation == null) {
            if (other.operation != null) {
                return false;
            }
        } else if (!operation.equals(other.operation)) {
            return false;
        }
        if (uid == null) {
            if (other.uid != null) {
                return false;
            }
        } else if (!uid.equals(other.uid)) {
            return false;
        }
        return true;
    }
}
