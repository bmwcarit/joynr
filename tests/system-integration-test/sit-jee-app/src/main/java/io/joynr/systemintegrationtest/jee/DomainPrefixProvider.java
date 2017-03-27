package io.joynr.systemintegrationtest.jee;

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

import java.util.HashSet;
import java.util.Set;

import javax.annotation.PostConstruct;
import javax.ejb.Singleton;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Sets;

@Singleton
public class DomainPrefixProvider {

    private static final Logger logger = LoggerFactory.getLogger(DomainPrefixProvider.class);

    private static final String SIT_JEE_APP_DOMAIN_PREFIXES_KEY = "SIT_JEE_APP_DOMAIN_PREFIXES";

    private final static String DEFAULT_PREFIX = "io.joynr.systemintegrationtest";

    private Set<String> domainPrefixes = new HashSet<>();

    @PostConstruct
    public void initialise() {
        String valueFromEnvironment = System.getenv(SIT_JEE_APP_DOMAIN_PREFIXES_KEY);
        if (valueFromEnvironment == null) {
            domainPrefixes.add(DEFAULT_PREFIX);
        } else {
            domainPrefixes.addAll(Sets.newHashSet(valueFromEnvironment.split(",")));
        }
        logger.debug("Initialised with domain prefixes: " + domainPrefixes);
    }

    public Set<String> getDomainPrefixes() {
        return domainPrefixes;
    }

}
