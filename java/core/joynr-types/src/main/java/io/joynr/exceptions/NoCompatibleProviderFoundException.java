package io.joynr.exceptions;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static java.lang.String.format;

import java.util.Set;

/**
 * Joynr exception to report arbitration failures because of version incompatibility.
 */
public class NoCompatibleProviderFoundException extends DiscoveryException {

    private static final long serialVersionUID = 1L;

    /**
     * A detail class holding information about a version which was discovered
     * for the {@Link NoCompatibleProviderFoundException#getInterfaceName() interface}
     * for which this exception is being thrown.
     */
    public static class VersionInformation {
        private int major;
        private int minor;

        public VersionInformation(int major, int minor) {
            this.major = major;
            this.minor = minor;
        }

        public int getMajor() {
            return major;
        }

        public int getMinor() {
            return minor;
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + major;
            result = prime * result + minor;
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
            VersionInformation other = (VersionInformation) obj;
            if (major != other.major)
                return false;
            if (minor != other.minor)
                return false;
            return true;
        }

        @Override
        public String toString() {
            return "VersionInformation [major=" + major + ", minor=" + minor + "]";
        }

    }

    private String interfaceName;
    private Set<VersionInformation> discoveredVersions;

    /**
     * Constructor for a NoCompatibleProviderFoundException with the name of
     * the interface for which no compatible version was found and a set of
     * versions for which providers were found.
     *
     * @param interfaceName
     *            the name of the interface for which no matching provider was found
     * @param discoveredVersions
     *            the set of versions for which providers were found, but aren't compatible with the version being
     *            looked for.
     */
    public NoCompatibleProviderFoundException(String interfaceName, Set<VersionInformation> discoveredVersions) {
        super(format("Unable to find a provider for %s with a compatible version.%nVersions found: %s",
                     interfaceName,
                     discoveredVersions));
        this.interfaceName = interfaceName;
        this.discoveredVersions = discoveredVersions;
    }

    public String getInterfaceName() {
        return interfaceName;
    }

    public Set<VersionInformation> getDiscoveredVersions() {
        return discoveredVersions;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((discoveredVersions == null) ? 0 : discoveredVersions.hashCode());
        result = prime * result + ((interfaceName == null) ? 0 : interfaceName.hashCode());
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
        if (interfaceName == null) {
            if (other.interfaceName != null)
                return false;
        } else if (!interfaceName.equals(other.interfaceName))
            return false;
        return true;
    }
}
