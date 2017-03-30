package io.joynr.accesscontrol.global;

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

import static org.mockito.Matchers.eq;
import io.joynr.accesscontrol.DomainAccessControlStore;

import java.util.ArrayList;
import java.util.List;

import javax.servlet.ServletContextEvent;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

import org.mockito.Mockito;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;
import com.google.inject.servlet.GuiceServletContextListener;
import com.sun.jersey.api.core.PackagesResourceConfig;
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 *  ServletContextListener for AclServletTest. Creates test data and a mock-up for the acl store and a guice
 *  injector with needed bindings.
 */
public class TestContextListener extends GuiceServletContextListener {

    public static final String user1 = "user1";
    public static final String domain1 = "domain1";
    public static final String interface1 = "interface1";
    public static final String operation1 = "operation1";
    public static final String user2 = "user2";
    public static final String domain2 = "domain2";
    public static final String interface2 = "interface2";
    public static final String operation2 = "operation2";
    public static final OwnerAccessControlEntry ownerAccessControlEntry = new OwnerAccessControlEntry(user2,
                                                                                                      domain2,
                                                                                                      interface2,
                                                                                                      TrustLevel.LOW,
                                                                                                      TrustLevel.HIGH,
                                                                                                      operation2,
                                                                                                      Permission.ASK);
    public static GlobalDomainAccessStoreAdmin mockedStore = Mockito.mock(GlobalDomainAccessStoreAdmin.class);

    public static final String OWNER1 = "owner1";
    public static final DomainRoleEntry domainRoleEntry_1 = new DomainRoleEntry(OWNER1, new String[]{ domain1, domain2,
            "domain3" }, Role.MASTER);
    public static final String OWNER2 = "owner2";
    public static final DomainRoleEntry domainRoleEntry_2 = new DomainRoleEntry(OWNER2, new String[]{ domain1, domain2,
            "domain3" }, Role.OWNER);
    public static final MasterAccessControlEntry masterAccessControlEntry = new MasterAccessControlEntry(user1,
                                                                                                         domain1,
                                                                                                         interface1,
                                                                                                         TrustLevel.LOW,
                                                                                                         new TrustLevel[]{ TrustLevel.MID },
                                                                                                         TrustLevel.HIGH,
                                                                                                         new TrustLevel[]{ TrustLevel.LOW },
                                                                                                         operation1,
                                                                                                         Permission.ASK,
                                                                                                         new Permission[]{ Permission.ASK });

    // Create a dummy domain role entry to allow the servlet access to add/update/delete
    private static final String DUMMY_USER_ID = "dummyUserId";
    private static final DomainRoleEntry DUMMY_DOMAIN_ROLE = new DomainRoleEntry(DUMMY_USER_ID, new String[]{ domain1,
            domain2 }, Role.MASTER);

    @Override
    public void contextInitialized(ServletContextEvent servletContextEvent) {

        Mockito.when(mockedStore.getAllDomainRoleEntries()).thenReturn(createDomainRoleEntryList());
        Mockito.when(mockedStore.getAllMasterAclEntries()).thenReturn(createMasterACLEntryList());
        Mockito.when(mockedStore.getAllOwnerAclEntries()).thenReturn(createOwnerACLEntryList());
        Mockito.when(mockedStore.getDomainRole(eq(DUMMY_USER_ID), eq(Role.MASTER))).thenReturn(DUMMY_DOMAIN_ROLE);
        servletContextEvent.getServletContext().setAttribute("DomainAccessStore", mockedStore);
        super.contextInitialized(servletContextEvent);
    }

    // Create domain role entry list
    public static List<DomainRoleEntry> createDomainRoleEntryList() {
        List<DomainRoleEntry> ret = new ArrayList<DomainRoleEntry>();
        ret.add(domainRoleEntry_1);
        ret.add(domainRoleEntry_2);

        return ret;
    }

    @Override
    public void contextDestroyed(ServletContextEvent servletContextEvent) {

    }

    /**
     * Creates guice injector with servlet's jersey resource class and mocked acl store
     * @return
     */
    @Override
    protected Injector getInjector() {
        return Guice.createInjector(new JerseyServletModule() {
            @Override
            protected void configureServlets() {
                bind(GuiceContainer.class);
                // bind Jersey resources
                PackagesResourceConfig resourceConfig = new PackagesResourceConfig("io.joynr.accesscontrol.global");
                for (Class<?> resource : resourceConfig.getClasses()) {
                    bind(resource);
                }

                bindConstant().annotatedWith(Names.named("joynr.messaging.domainaccesscontrollerchannelid"))
                              .to("gdac_channelid");
                bindConstant().annotatedWith(Names.named("joynr.messaging.discoverydirectoriesdomain")).to("io.joynr");

                bind(DomainAccessControlStore.class).toInstance(mockedStore);
                bind(GlobalDomainAccessStoreAdmin.class).toInstance(mockedStore);
                serve("/*").with(GuiceContainer.class);
                serve("/accesscontrol/*").with(AccessControlEditorServlet.class);

            }
        });
    }

    public static List<MasterAccessControlEntry> createMasterACLEntryList() {
        List<MasterAccessControlEntry> masterAcl = new ArrayList<MasterAccessControlEntry>();
        masterAcl.add(masterAccessControlEntry);
        masterAcl.add(new MasterAccessControlEntry(user2,
                                                   domain2,
                                                   interface2,
                                                   TrustLevel.LOW,
                                                   new TrustLevel[]{ TrustLevel.MID },
                                                   TrustLevel.HIGH,
                                                   new TrustLevel[]{ TrustLevel.LOW },
                                                   operation2,
                                                   Permission.ASK,
                                                   new Permission[]{ Permission.ASK }));
        return masterAcl;
    }

    public static List<OwnerAccessControlEntry> createOwnerACLEntryList() {
        List<OwnerAccessControlEntry> ownerACL = new ArrayList<OwnerAccessControlEntry>();
        ownerACL.add(ownerAccessControlEntry);
        return ownerACL;
    }
}
