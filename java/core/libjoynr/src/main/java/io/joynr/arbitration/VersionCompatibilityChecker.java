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
package io.joynr.arbitration;

import static java.lang.String.format;

import joynr.types.Version;

/**
 * Utility class to check whether two {@link Version versions} are compatible
 * with each other or not. Use the {@link #check(Version, Version)} method
 * to perform the version compatibility check.
 */
public class VersionCompatibilityChecker {

    /**
     * Compatibility of two versions is defined as the caller and provider versions having the same major version, and
     * the provider having the same or a higher minor version as the caller.
     *
     * @param caller
     *            the version of the participant performing the call. Must not be <code>null</code>. If either major or
     *            minor is <code>null</code>, this will be interpreted as <code>0</code>.
     * @param provider
     *            the version of the provider who is being called. Must not be <code>null</code>. If either major or
     *            minor is <code>null</code>, this will be interpreted as <code>0</code>.
     *
     * @return <code>true</code> if the versions are compatible as described above, or <code>false</code> if not.
     *
     * @throws IllegalArgumentException
     *             if either caller or provider are <code>null</code>.
     */
    public boolean check(Version caller, Version provider) {
        if (caller == null || provider == null) {
            throw new IllegalArgumentException(format("Both caller (%s) and provider (%s) must be non-null.",
                                                      caller,
                                                      provider));
        }
        if (caller.getMajorVersion() == null || caller.getMinorVersion() == null || provider.getMajorVersion() == null
                || provider.getMinorVersion() == null) {
            throw new IllegalArgumentException(format("Neither major nor minor version values can be null in either caller %s or provider %s.",
                                                      caller,
                                                      provider));
        }

        int callerMajor = caller.getMajorVersion();
        int callerMinor = caller.getMinorVersion();
        int providerMajor = provider.getMajorVersion();
        int providerMinor = provider.getMinorVersion();

        return callerMajor == providerMajor && callerMinor <= providerMinor;
    }

}
