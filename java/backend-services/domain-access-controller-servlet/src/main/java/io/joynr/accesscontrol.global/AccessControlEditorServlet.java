package io.joynr.accesscontrol.global;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import com.google.inject.Inject;
import com.sun.jersey.api.core.InjectParam;
import io.joynr.servlet.JoynrWebServlet;
import joynr.infrastructure.DomainRoleEntry;
import joynr.infrastructure.MasterAccessControlEntry;
import joynr.infrastructure.OwnerAccessControlEntry;
import joynr.infrastructure.Role;

import javax.inject.Singleton;
import javax.servlet.http.HttpServlet;
import javax.ws.rs.GET;
import javax.ws.rs.DELETE;
import javax.ws.rs.POST;
import javax.ws.rs.Consumes;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import java.util.List;
import java.util.logging.Logger;

@Singleton
@JoynrWebServlet(value = "/" + AccessControlEditorServlet.SERVLET_PATH + "/")
@Path(AccessControlEditorServlet.SERVLET_PATH)
public class AccessControlEditorServlet extends HttpServlet {
    private static final long serialVersionUID = -6719217871956034142L;
    private static Logger logger = Logger.getLogger(AccessControlEditorServlet.class.getName());
    public static final String SERVLET_PATH = "/accesscontrol";
    public static final String DOMAIN_ROLE_PATH = "/domainroles";
    public static final String OWNER_ACL_PATH = "/owneracl";
    public static final String MASTER_ACL_PATH = "/masteracl";

    // This servlet cannot be serialized
    private transient GlobalDomainAccessStoreAdmin domainAccessStore;

    @Inject
    public AccessControlEditorServlet(@InjectParam GlobalDomainAccessStoreAdmin domainAccessStore) {
        this.domainAccessStore = domainAccessStore;
    }

    /**
     * Getter for all Domain Role Table entries to list them in editor web app.
     * @return List of domain role entries.
     */
    @GET
    @Path(DOMAIN_ROLE_PATH)
    @Produces(MediaType.APPLICATION_JSON)
    public List<DomainRoleEntry> getDomainRoleList() {
        logger.info("DRT entries requested");
        List<DomainRoleEntry> allDomainRoleEntries = domainAccessStore.getAllDomainRoleEntries();
        return allDomainRoleEntries;
    }

    /**
     * Getter for all Master ACL entries to list them in editor web app.
     * @return list of master acl entries
     */
    @GET
    @Path(MASTER_ACL_PATH)
    @Produces(MediaType.APPLICATION_JSON)
    public List<MasterAccessControlEntry> getMasterACL() {
        logger.info("Master ACL entries requested");
        List<MasterAccessControlEntry> allMasterACLEntries = domainAccessStore.getAllMasterAclEntries();
        return allMasterACLEntries;
    }

    /**
     * Getter for all Owner ACL entries to list them in editor web app.
     * @return
     */
    @GET
    @Path(OWNER_ACL_PATH)
    @Produces(MediaType.APPLICATION_JSON)
    public List<OwnerAccessControlEntry> getOwnerACL() {
        logger.info("Owner ACL entries requested");
        List<OwnerAccessControlEntry> allOwnerACLEntries = domainAccessStore.getAllOwnerAclEntries();
        return allOwnerACLEntries;
    }

    /**
     * Deletes a single DRT entry. Identified by the user id and role.
     * @param userId Id if the user
     * @param role  Role (MASTER/OWNER)
     */
    @DELETE
    @Path(DOMAIN_ROLE_PATH + "/{userId}/{role}")
    public void deleteDRTEntry(@PathParam("userId") String userId, @PathParam("role") Role role) {
        logger.info("Deleting DRT entry for " + userId + "/" + role);
        domainAccessStore.removeDomainRole(userId, role);
    }

    /**
     * Deletes a single master acl entry identified by /{userId}/{domain}/{interfaceName}/{operation}
     * @param userId
     * @param domain
     * @param interfaceName
     * @param operation
     */
    @DELETE
    @Path(MASTER_ACL_PATH + "/{userId}/{domain}/{interfaceName}/{operation}")
    public void deleteMasterACLEntry(@PathParam("userId") String userId,
                                     @PathParam("domain") String domain,
                                     @PathParam("interfaceName") String interfaceName,
                                     @PathParam("operation") String operation) {
        logger.info("Deleting Master ACL entry for " + userId + "/" + domain + "/" + interfaceName + "/" + operation);
        domainAccessStore.removeMasterAccessControlEntry(userId, domain, interfaceName, operation);
    }

    /**
     * Deletes a single owner acl entry identified by /{userId}/{domain}/{interfaceName}/{operation}
     * @param userId
     * @param domain
     * @param interfaceName
     * @param operation
     */
    @DELETE
    @Path(OWNER_ACL_PATH + "/{userId}/{domain}/{interfaceName}/{operation}")
    public void deleteOwnerACLEntry(@PathParam("userId") String userId,
                                    @PathParam("domain") String domain,
                                    @PathParam("interfaceName") String interfaceName,
                                    @PathParam("operation") String operation) {
        logger.info("Deleting Owner ACL entry for " + userId + "/" + domain + "/" + interfaceName + "/" + operation);
        domainAccessStore.removeOwnerAccessControlEntry(userId, domain, interfaceName, operation);
    }

    /**
     * Updates an existing DRT entry. Identifies the entry to update by the userId and role of the updated.
     * @param domainRoleEntry Updated entry. UserId and role may not be changed or the entry to update can not be found
     */
    @POST
    @Path(DOMAIN_ROLE_PATH + "/")
    @Consumes(MediaType.APPLICATION_JSON)
    public void updateDrtEntry(DomainRoleEntry domainRoleEntry) {
        logger.info("Updating DRT entry: " + domainRoleEntry);
        domainAccessStore.updateDomainRole(domainRoleEntry);
    }

    /**
     * Updates an existing Master ACL entry. Identifies the entry to update by the userId, domain, interfaceName and operation of the updated.
     * @param masterAccessControlEntry Updated entry. userId, domain, interfaceName and operation may not be changed or the entry to update can not be found
     */
    @POST
    @Path(MASTER_ACL_PATH + "/")
    @Consumes(MediaType.APPLICATION_JSON)
    public void updateMasterAclEntry(MasterAccessControlEntry masterAccessControlEntry) {
        logger.info("Updating Master ACL entry: " + masterAccessControlEntry);
        domainAccessStore.updateMasterAccessControlEntry(masterAccessControlEntry);
    }

    /**
     * Updates an existing Owner ACL entry. Identifies the entry to update by the userId, domain, interfaceName and operation of the updated.
     * @param ownerAccessControlEntry Updated entry. userId, domain, interfaceName and operation may not be changed or the entry to update can not be found
     */
    @POST
    @Path(OWNER_ACL_PATH + "/")
    @Consumes(MediaType.APPLICATION_JSON)
    public void updateMasterAclEntry(OwnerAccessControlEntry ownerAccessControlEntry) {
        logger.info("Updating Owner ACL entry: " + ownerAccessControlEntry);
        domainAccessStore.updateOwnerAccessControlEntry(ownerAccessControlEntry);
    }

    public void destroy() {
        // do nothing.
    }
}