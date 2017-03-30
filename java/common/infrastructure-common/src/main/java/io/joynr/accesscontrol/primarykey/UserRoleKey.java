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

import joynr.infrastructure.DacTypes.Role;

/**
 * A key consisting of uid, role
 */
public class UserRoleKey implements Serializable {
    private static final long serialVersionUID = -5375570298012028755L;

    // these constants have to match field names so EhCache ReflectionAttributeExtractor may do it's magic
    public static final String USER_ID = "uid";
    public static final String ROLE = "role";
    private final String uid;
    private final Role role;

    public UserRoleKey(String uid, Role role) {
        this.uid = uid;
        this.role = role;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int hashcode = 1;
        hashcode = prime * hashcode + ((role == null) ? 0 : role.hashCode());
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
        UserRoleKey other = (UserRoleKey) obj;
        if (role == null) {
            if (other.role != null) {
                return false;
            }
        } else if (!role.equals(other.role)) {
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
