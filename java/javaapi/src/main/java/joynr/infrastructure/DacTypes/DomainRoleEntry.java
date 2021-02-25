/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

package joynr.infrastructure.DacTypes;

import java.io.Serializable;

import io.joynr.subtypes.JoynrType;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonIgnore;

/**
 * The Domain Role Entry (DRE) stores domains for users/role combination. User may become specified role when accessing given domains ACEs/RCEs in ACL/RCL. DREs are stored in the Domain Roles Table (DRT) using the pair (uid, role) as combined primary key.
 */
@SuppressWarnings("serial")
public class DomainRoleEntry implements Serializable, JoynrType {
    public static final int MAJOR_VERSION = 0;
    public static final int MINOR_VERSION = 0;
    @JsonProperty("uid")
    private String uid;
    @JsonProperty("domains")
    private String[] domains = {};
    @JsonProperty("role")
    private Role role;

    /**
     * Default Constructor
     */
    public DomainRoleEntry() {
        this.uid = "";
        this.role = Role.MASTER;
    }

    /**
     * Copy constructor
     *
     * @param domainRoleEntryObj reference to the object to be copied
     */
    public DomainRoleEntry(DomainRoleEntry domainRoleEntryObj) {
        this.uid = domainRoleEntryObj.uid;
        this.domains = domainRoleEntryObj.domains;
        this.role = domainRoleEntryObj.role;
    }

    /**
     * Parameterized constructor
     *
     * @param uid The unique user ID (UID) this entry applies to.
     * @param domains A list of domains this entry applies to. A domain might also contain the wildcard character (asterisk sign) to refer to all (sub-) domains.
     * @param role The role that is assigned to the specified user for the specified domains.
     */
    public DomainRoleEntry(String uid, String[] domains, Role role) {
        this.uid = uid;
        if (domains != null) {
            this.domains = domains.clone();
        }
        this.role = role;
    }

    /**
     * Gets Uid
     *
     * @return The unique user ID (UID) this entry applies to.
     */
    @JsonIgnore
    public String getUid() {
        return uid;
    }

    /**
     * Sets Uid
     *
     * @param uid The unique user ID (UID) this entry applies to.
     */
    @JsonIgnore
    public void setUid(String uid) {
        if (uid == null) {
            throw new IllegalArgumentException("setting uid to null is not allowed");
        }
        this.uid = uid;
    }

    /**
     * Gets Domains
     *
     * @return A list of domains this entry applies to. A domain might also contain the wildcard character (asterisk sign) to refer to all (sub-) domains.
     */
    @JsonIgnore
    public String[] getDomains() {
        if (domains != null) {
            return domains.clone();
        } else {
            return null;
        }
    }

    /**
     * Sets Domains
     *
     * @param domains A list of domains this entry applies to. A domain might also contain the wildcard character (asterisk sign) to refer to all (sub-) domains.
     */
    @JsonIgnore
    public void setDomains(String[] domains) {
        if (domains == null) {
            throw new IllegalArgumentException("setting domains to null is not allowed");
        }
        this.domains = domains.clone();
    }

    /**
     * Gets Role
     *
     * @return The role that is assigned to the specified user for the specified domains.
     */
    @JsonIgnore
    public Role getRole() {
        return role;
    }

    /**
     * Sets Role
     *
     * @param role The role that is assigned to the specified user for the specified domains.
     */
    @JsonIgnore
    public void setRole(Role role) {
        if (role == null) {
            throw new IllegalArgumentException("setting role to null is not allowed");
        }
        this.role = role;
    }

    /**
     * Stringifies the class
     *
     * @return stringified class content
     */
    @Override
    public String toString() {
        return "DomainRoleEntry [" + "uid=" + this.uid + ", " + "domains=" + java.util.Arrays.toString(this.domains)
                + ", " + "role=" + this.role + "]";
    }

    /**
     * Check for equality
     *
     * @param obj Reference to the object to compare to
     * @return true, if objects are equal, false otherwise
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        DomainRoleEntry other = (DomainRoleEntry) obj;
        if (this.uid == null) {
            if (other.uid != null) {
                return false;
            }
        } else if (!this.uid.equals(other.uid)) {
            return false;
        }
        if (this.domains == null) {
            if (other.domains != null) {
                return false;
            }
        } else if (!java.util.Arrays.deepEquals(this.domains, other.domains)) {
            return false;
        }
        if (this.role == null) {
            if (other.role != null) {
                return false;
            }
        } else if (!this.role.equals(other.role)) {
            return false;
        }
        return true;
    }

    /**
     * Calculate code for hashing based on member contents
     *
     * @return The calculated hash code
     */
    @Override
    public int hashCode() {
        int result = 1;
        final int prime = 31;
        result = prime * result + ((this.uid == null) ? 0 : this.uid.hashCode());
        result = prime * result + ((this.domains == null) ? 0 : java.util.Arrays.hashCode(this.domains));
        result = prime * result + ((this.role == null) ? 0 : this.role.hashCode());
        return result;
    }
}
