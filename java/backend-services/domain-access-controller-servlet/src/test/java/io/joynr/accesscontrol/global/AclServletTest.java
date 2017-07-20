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

import static org.junit.Assert.assertEquals;

import java.util.logging.Level;
import java.util.logging.Logger;

import javax.ws.rs.core.MediaType;

import joynr.infrastructure.DacTypes.Role;

import org.junit.Test;
import org.mockito.Mockito;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.test.framework.AppDescriptor;
import com.sun.jersey.test.framework.JerseyTest;
import com.sun.jersey.test.framework.WebAppDescriptor;

/**
 * JerseyTest for the ACL editor's rest interface.Tests  the CRUD methods.
 */
public class AclServletTest extends JerseyTest {
    private Gson gson;
    private Logger logger = Logger.getLogger(AclServletTest.class.getName());

    @Override
    protected AppDescriptor configure() {
        gson = new GsonBuilder().create();
        return new WebAppDescriptor.Builder().contextListenerClass(TestContextListener.class)
                                             .initParam("com.sun.jersey.config.property.packages",
                                                        "io.joynr.accesscontrol.global")
                                             .filterClass(com.google.inject.servlet.GuiceFilter.class)
                                             .contextPath("")
                                             .servletPath("/")
                                             .build();
    }

    @Test
    public void testEditorGetsAllDomainRoles() {
        WebResource webResource = resource();
        String responseMsg = webResource.path(AccessControlEditorServlet.SERVLET_PATH
                + AccessControlEditorServlet.DOMAIN_ROLE_PATH)
                                        .type(MediaType.APPLICATION_JSON_TYPE)
                                        .get(String.class);
        assertEquals(gson.toJson(TestContextListener.createDomainRoleEntryList()), responseMsg);
    }

    @Test
    public void testEditorGetsAllMasterAccessControlEntrys() {
        WebResource webResource = resource();
        String responseMsg = webResource.path(AccessControlEditorServlet.SERVLET_PATH
                + AccessControlEditorServlet.MASTER_ACL_PATH)
                                        .type(MediaType.APPLICATION_JSON_TYPE)
                                        .get(String.class);
        assertEquals(gson.toJson(TestContextListener.createMasterACLEntryList()), responseMsg);
    }

    @Test
    public void testEditorGetsAllOwnerAccessControlEntrys() {
        WebResource webResource = resource();
        String responseMsg = webResource.path(AccessControlEditorServlet.SERVLET_PATH
                + AccessControlEditorServlet.OWNER_ACL_PATH)
                                        .type(MediaType.APPLICATION_JSON_TYPE)
                                        .get(String.class);
        assertEquals(gson.toJson(TestContextListener.createOwnerACLEntryList()), responseMsg);

    }

    @Test
    public void testEditorCanRemoveDRT() {
        WebResource webResource = resource();
        String userToDelete = TestContextListener.user1;

        webResource.path(AccessControlEditorServlet.SERVLET_PATH + AccessControlEditorServlet.DOMAIN_ROLE_PATH + "/"
                + userToDelete + "/" + Role.MASTER)
                   .header("Authorization", "testApiKey")
                   .delete();
        Mockito.verify(TestContextListener.mockedStore, Mockito.times(1)).removeDomainRole(userToDelete, Role.MASTER);

    }

    @Test
    public void testEditorCanRemoveMasterACl() {
        WebResource webResource = resource();

        //{userId}/{domain}/{interfaceName}/{operation}
        webResource.path(AccessControlEditorServlet.SERVLET_PATH + AccessControlEditorServlet.MASTER_ACL_PATH + "/"
                + TestContextListener.user1 + "/" + TestContextListener.domain1 + "/" + TestContextListener.interface1
                + "/" + TestContextListener.operation1)
                   .header("Authorization", "testApiKey")
                   .delete();
        Mockito.verify(TestContextListener.mockedStore, Mockito.times(1))
               .removeMasterAccessControlEntry(TestContextListener.user1,
                                               TestContextListener.domain1,
                                               TestContextListener.interface1,
                                               TestContextListener.operation1);

    }

    @Test
    public void testEditorCanRemoveOwnerAcl() {
        WebResource webResource = resource();

        webResource.path(AccessControlEditorServlet.SERVLET_PATH + AccessControlEditorServlet.OWNER_ACL_PATH + "/"
                + TestContextListener.user1 + "/" + TestContextListener.domain1 + "/" + TestContextListener.interface1
                + "/" + TestContextListener.operation1)
                   .header("Authorization", "testApiKey")
                   .delete();
        Mockito.verify(TestContextListener.mockedStore, Mockito.times(1))
               .removeOwnerAccessControlEntry(TestContextListener.user1,
                                              TestContextListener.domain1,
                                              TestContextListener.interface1,
                                              TestContextListener.operation1);

    }

    @Test
    public void testEditorCanUpdateDRT() {
        WebResource webResource = resource();

        webResource.path(AccessControlEditorServlet.SERVLET_PATH + AccessControlEditorServlet.DOMAIN_ROLE_PATH + "/")
                   .type(MediaType.APPLICATION_JSON)
                   .entity(gson.toJson(TestContextListener.domainRoleEntry_1))
                   .header("Authorization", "testApiKey")
                   .post();
        logger.log(Level.INFO, webResource.toString());
        Mockito.verify(TestContextListener.mockedStore, Mockito.times(1))
               .updateDomainRole(TestContextListener.domainRoleEntry_1);
    }

    @Test
    public void testEditorCanUpdateMasterAcl() {
        WebResource webResource = resource();

        webResource.path(AccessControlEditorServlet.SERVLET_PATH + AccessControlEditorServlet.MASTER_ACL_PATH + "/")
                   .type(MediaType.APPLICATION_JSON)
                   .entity(gson.toJson(TestContextListener.masterAccessControlEntry))
                   .header("Authorization", "testApiKey")
                   .post();
        logger.log(Level.INFO, webResource.toString());
        Mockito.verify(TestContextListener.mockedStore, Mockito.times(1))
               .updateMasterAccessControlEntry(TestContextListener.masterAccessControlEntry);
    }

    @Test
    public void testEditorCanUpdateOwnerAcl() {
        WebResource webResource = resource();

        webResource.path(AccessControlEditorServlet.SERVLET_PATH + AccessControlEditorServlet.OWNER_ACL_PATH + "/")
                   .type(MediaType.APPLICATION_JSON)
                   .entity(gson.toJson(TestContextListener.ownerAccessControlEntry))
                   .header("Authorization", "testApiKey")
                   .post();
        logger.log(Level.INFO, webResource.toString());
        Mockito.verify(TestContextListener.mockedStore, Mockito.times(1))
               .updateOwnerAccessControlEntry(TestContextListener.ownerAccessControlEntry);
    }

}
