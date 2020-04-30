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

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import joynr.types.Version;

/**
 * This exception is thrown when a multi-domain proxy encounters {@link NoCompatibleProviderFoundException} for any of
 * the domains being queried. It contains a map from domain to the specific {@link NoCompatibleProviderFoundException}
 * information for that domain.
 */
public class MultiDomainNoCompatibleProviderFoundException extends JoynrRuntimeException {

    /**
     * For serialization.
     */
    private static final long serialVersionUID = 1L;

    private final Map<String, NoCompatibleProviderFoundException> exceptionsByDomain;

    /**
     * Constructor which takes the map of domains to {@link NoCompatibleProviderFoundException}.
     *
     * @param exceptionsByDomain
     *            the exceptions keyed by domain.
     */
    public MultiDomainNoCompatibleProviderFoundException(Map<String, NoCompatibleProviderFoundException> exceptionsByDomain) {
        this.exceptionsByDomain = exceptionsByDomain;
    }

    /**
     * Gets the map of domains to {@link NoCompatibleProviderFoundException}.
     *
     * @return see method description.
     */
    public Map<String, NoCompatibleProviderFoundException> getExceptionsByDomain() {
        return exceptionsByDomain;
    }

    /**
     * Gets a set of domains for which we have {@Link NoCompatibleProviderFoundException} information available.
     *
     * @return see method description.
     */
    public Set<String> getDomainsWithExceptions() {
        return exceptionsByDomain.keySet();
    }

    /**
     * Checks to see if information is available for a given domain.
     *
     * @param domain
     *            the domain to check for.
     *
     * @return <code>true</code> if there is information available for the domain in the form of a
     *         {@link #getExceptionForDomain(String) no compatible provider found exception} or <code>false</code>
     *         otherwise.
     */
    public boolean hasExceptionForDomain(String domain) {
        return exceptionsByDomain.containsKey(domain);
    }

    /**
     * Returns the {@Link NoCompatibleProviderFoundException} for the given domain if one exists.
     *
     * @param domain
     *            the domain for which to get the {@Link NoCompatibleProviderFoundException}.
     *
     * @return the exception if there is one, or <code>null</code> if not.
     */
    public NoCompatibleProviderFoundException getExceptionForDomain(String domain) {
        return exceptionsByDomain.get(domain);
    }

    /**
     * Returns the set of versions which were discovered for a given domain.
     *
     * @param domain
     *            the domain for which to return the set of versions which were found by that domain.
     *
     * @return the set of discovered versions for the given domain, or an empty set if no discovered versions are
     *         available for that domain. Will never be <code>null</code>.
     */
    public Set<Version> getDiscoveredVersionsForDomain(String domain) {
        Set<Version> result = new HashSet<>();
        if (hasExceptionForDomain(domain)) {
            result.addAll(exceptionsByDomain.get(domain).getDiscoveredVersions());
        }
        return result;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((exceptionsByDomain == null) ? 0 : exceptionsByDomain.hashCode());
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
        MultiDomainNoCompatibleProviderFoundException other = (MultiDomainNoCompatibleProviderFoundException) obj;
        if (exceptionsByDomain == null) {
            if (other.exceptionsByDomain != null)
                return false;
        } else if (!exceptionsByDomain.equals(other.exceptionsByDomain))
            return false;
        return true;
    }
}
