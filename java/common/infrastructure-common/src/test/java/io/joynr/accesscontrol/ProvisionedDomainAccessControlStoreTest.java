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
package io.joynr.accesscontrol;

import static org.junit.Assert.assertEquals;

import java.util.Properties;

import org.junit.Before;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Provides;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

import net.sf.ehcache.CacheManager;
import net.sf.ehcache.config.Configuration;

public class ProvisionedDomainAccessControlStoreTest {

    private static final String UID1 = "uid1";
    private static final String DOMAIN1 = "domain1";
    private static final String INTERFACE1 = "interface1";
    private static final String OPERATION1 = "operation1";

    private MasterAccessControlEntry expectedMasterAccessControlEntry;
    private DomainRoleEntry expectedUserDomainRoleEntry;
    private String domainRoleEntryString;
    private String masterAccessControlEntryString;

    @Before
    public void setup() throws Exception {

        ObjectMapper objectMapper = new ObjectMapper();
        // instantiate some template objects
        expectedUserDomainRoleEntry = new DomainRoleEntry(UID1, new String[0], Role.OWNER);
        DomainRoleEntry[] provisionedDomainRoles = { expectedUserDomainRoleEntry };
        domainRoleEntryString = objectMapper.writeValueAsString(provisionedDomainRoles);

        expectedMasterAccessControlEntry = new MasterAccessControlEntry(UID1,
                                                                        DOMAIN1,
                                                                        INTERFACE1,
                                                                        TrustLevel.LOW,
                                                                        new TrustLevel[]{ TrustLevel.MID,
                                                                                TrustLevel.LOW },
                                                                        TrustLevel.LOW,
                                                                        new TrustLevel[]{ TrustLevel.MID,
                                                                                TrustLevel.LOW },
                                                                        OPERATION1,
                                                                        Permission.NO,
                                                                        new Permission[]{ Permission.ASK,
                                                                                Permission.NO });
        MasterAccessControlEntry[] provisionedMasterAccessControlEntries = { expectedMasterAccessControlEntry };
        masterAccessControlEntryString = objectMapper.writeValueAsString(provisionedMasterAccessControlEntries);
    }

    private Injector getInjector(Properties customProperties) {
        Injector injector = Guice.createInjector(new JoynrPropertiesModule(customProperties), new AbstractModule() {

            @Override
            protected void configure() {
                bind(DomainAccessControlProvisioning.class).to(StaticDomainAccessControlProvisioning.class);
                bind(DomainAccessControlStore.class).to(DomainAccessControlStoreEhCache.class);
            }

            @Provides
            CacheManager provideCacheManager() {
                Configuration configuration = new Configuration();
                configuration.setName("GDACEhCacheManager");
                return CacheManager.create(configuration);
            }
        });
        return injector;
    }

    @Test
    public void testGetDomainRoles() throws Exception {
        Properties customProperties = new Properties();
        customProperties.put(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_DOMAIN_ROLES,
                             domainRoleEntryString);

        Injector injector = getInjector(customProperties);
        DomainAccessControlStore store = injector.getInstance(DomainAccessControlStore.class);

        assertEquals("DRE for UID1 should be the same as expectedOwnerAccessControlEntry",
                     expectedUserDomainRoleEntry,
                     store.getDomainRoles(UID1).get(0));
        assertEquals("DRE for UID1 and Role.OWNER should be the same as expectedOwnerAccessControlEntry",
                     expectedUserDomainRoleEntry,
                     store.getDomainRole(UID1, Role.OWNER));

        CacheManager cacheManager = injector.getInstance(CacheManager.class);
        cacheManager.removeAllCaches();
    }

    @Test
    public void testGetMasterAce() throws Exception {
        Properties customProperties = new Properties();
        customProperties.put(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES,
                             masterAccessControlEntryString);

        Injector injector = getInjector(customProperties);

        DomainAccessControlStore store = injector.getInstance(DomainAccessControlStore.class);

        assertEquals("Master ACE associated to UID1 from Master ACL should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntries(UID1).get(0));
        assertEquals("Master ACE associated to DOMAIN1 and INTERFACE1 should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntries(DOMAIN1, INTERFACE1).get(0));
        assertEquals("Master ACE associated to UID1, DOMAIN1 and INTERFACE1 should be the same as expectedMasterAccessControlEntry",
                     expectedMasterAccessControlEntry,
                     store.getMasterAccessControlEntries(UID1, DOMAIN1, INTERFACE1).get(0));

        CacheManager cacheManager = injector.getInstance(CacheManager.class);
        cacheManager.removeAllCaches();
    }
}
