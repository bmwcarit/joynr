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
package io.joynr.accesscontrol;

import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

/**
 * Manages access control lists for local providers.
 */
public interface LocalDomainAccessController {

    /**
     * Check if user uid has role role for domain.
     * Used by an ACL editor app to verify whether the user is allowed to change ACEs or not
     *
     * @param userId        The user accessing the interface
     * @param domain    The trust level of the device accessing the interface
     * @param role        The domain that is being accessed
     * @return Returns true, if user uid has role role for domain domain.
     */
    boolean hasRole(String userId, String domain, Role role);

    /**
     * Get consumer permission to access an interface
     *
     * @param userId        The user accessing the interface
     * @param domain        The domain that is being accessed
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     * @param callback      Will be called when the permission is available. The callback's
     *                      argument is the permission to access the given interface, or NULL if
     *                      there is more than one ACE for given uid, domain, interfaceName.
     *                      If the callback returns NULL, use {@link #getConsumerPermission(String,
     *                      String, String, String, TrustLevel)} to gain Permission on interface operation.
     */
    void getConsumerPermission(String userId,
                               String domain,
                               String interfaceName,
                               TrustLevel trustLevel,
                               GetConsumerPermissionCallback callback);

    /**
     * Get consumer permission to access an interface operation
     *
     * @param userId        The user accessing the interface
     * @param domain        The domain that is being accessed
     * @param interfaceName The interface that is being accessed
     * @param operation    The operation user requests to execute on interface
     * @param trustLevel    The trust level of the device accessing the interface
     * @return Permission to access given interface operation.
     */
    Permission getConsumerPermission(String userId,
                                     String domain,
                                     String interfaceName,
                                     String operation,
                                     TrustLevel trustLevel);

    /**
     * Get provider permission to expose an interface
     *
     * @param uid        The userId of the provider exposing the interface
     * @param domain        The domain where interface belongs
     * @param interfaceName The interface that is being accessed
     * @param trustLevel    The trust level of the device accessing the interface
     * @return provider permission
     */
    Permission getProviderPermission(String uid, String domain, String interfaceName, TrustLevel trustLevel);

}
