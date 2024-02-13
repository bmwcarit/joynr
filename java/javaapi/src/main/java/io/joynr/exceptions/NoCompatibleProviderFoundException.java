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
package io.joynr.exceptions;

import static java.lang.String.format;

import java.util.HashSet;
import java.util.Set;

import joynr.types.Version;

/**
 * Joynr exception to report arbitration failures because of version incompatibility.
 */
public class NoCompatibleProviderFoundException extends DiscoveryException {

    private static final long serialVersionUID = 1L;

    private final String interfaceName;
    private final Version interfaceVersion;
    private final Set<Version> discoveredVersions;
    private final String domain;

    /**
     * Constructor for a NoCompatibleProviderFoundException with the name of the interface for which no compatible
     * version was found and a set of versions for which providers were found.
     *
     * @param interfaceName
     *            the name of the interface for which no matching provider was found.
     * @param interfaceVersion
     *            the version of the interface for which a provider was looked for.
     * @param domain
     *            the domain in which a provider for the interface was looked for.
     * @param discoveredVersions
     *            the set of versions for which providers were found, but aren't compatible with the version being
     *            looked for.
     */
    public NoCompatibleProviderFoundException(String interfaceName,
                                              Version interfaceVersion,
                                              String domain,
                                              Set<Version> discoveredVersions) {
        super(format("Unable to find a provider for %s %s in domain %s with a compatible version.%nVersions found: %s",
                     interfaceName,
                     interfaceVersion,
                     domain,
                     discoveredVersions));
        this.interfaceName = interfaceName;
        this.interfaceVersion = (interfaceVersion != null) ? new Version(interfaceVersion) : null;
        this.domain = domain;
        this.discoveredVersions = (discoveredVersions != null) ? new HashSet<>(discoveredVersions) : null;
    }

    public String getInterfaceName() {
        return interfaceName;
    }

    public Version getInterfaceVersion() {
        return (interfaceVersion != null) ? new Version(interfaceVersion) : null;
    }

    public String getDomain() {
        return domain;
    }

    public Set<Version> getDiscoveredVersions() {
        return (discoveredVersions != null) ? new HashSet<>(discoveredVersions) : null;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((discoveredVersions == null) ? 0 : discoveredVersions.hashCode());
        result = prime * result + ((domain == null) ? 0 : domain.hashCode());
        result = prime * result + ((interfaceName == null) ? 0 : interfaceName.hashCode());
        result = prime * result + ((interfaceVersion == null) ? 0 : interfaceVersion.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (!super.equals(obj))
            return false;
        if (getClass() != obj.getClass())
            return false;
        NoCompatibleProviderFoundException other = (NoCompatibleProviderFoundException) obj;
        if (discoveredVersions == null) {
            if (other.discoveredVersions != null)
                return false;
        } else if (!discoveredVersions.equals(other.discoveredVersions))
            return false;
        if (domain == null) {
            if (other.domain != null)
                return false;
        } else if (!domain.equals(other.domain))
            return false;
        if (interfaceName == null) {
            if (other.interfaceName != null)
                return false;
        } else if (!interfaceName.equals(other.interfaceName))
            return false;
        if (interfaceVersion == null) {
            if (other.interfaceVersion != null)
                return false;
        } else if (!interfaceVersion.equals(other.interfaceVersion))
            return false;
        return true;
    }

}
