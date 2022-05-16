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

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.accesscontrol.primarykey.UserDomainInterfaceOperationKey;
import io.joynr.accesscontrol.primarykey.UserRoleKey;
import joynr.infrastructure.DacTypes.ControlEntry;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;
import net.sf.ehcache.Cache;
import net.sf.ehcache.CacheException;
import net.sf.ehcache.CacheManager;
import net.sf.ehcache.Element;
import net.sf.ehcache.config.CacheConfiguration;
import net.sf.ehcache.config.SearchAttribute;
import net.sf.ehcache.config.Searchable;
import net.sf.ehcache.search.Attribute;
import net.sf.ehcache.search.Direction;
import net.sf.ehcache.search.Query;
import net.sf.ehcache.search.Result;
import net.sf.ehcache.search.Results;

/**
 * Uses EhCache to implement a DomainAccessControlStore.
 * Add/Remove operations can be expensive. Get operations should be fast.
 */
public class DomainAccessControlStoreEhCache implements DomainAccessControlStore {
    private static final Logger logger = LoggerFactory.getLogger(DomainAccessControlStoreEhCache.class);
    private static final String WILDCARD = "*";
    private final CacheManager cacheManager;

    public enum CacheId {

        MASTER_ACL("io.joynr.MasterACL"), OWNER_ACL("io.joynr.OwnerACL"), MEDIATOR_ACL(
                "io.joynr.MediatorACL"), DOMAIN_ROLES("io.joynr.DomainRoleTable"), MASTER_RCL(
                        "io.joynr.MasterRCL"), OWNER_RCL("io.joynr.OwnerRCL"), MEDIATOR_RCL("io.joynr.MediatorRCL");

        private final String idAsString;

        CacheId(String id) {
            this.idAsString = id;
        }

        public String getIdAsString() {
            return idAsString;
        }

        public boolean isACL() {
            return this.equals(CacheId.MASTER_ACL) || this.equals(CacheId.OWNER_ACL)
                    || this.equals(CacheId.MEDIATOR_ACL);
        }
    }

    @Inject
    public DomainAccessControlStoreEhCache(CacheManager ehCacheManager,
                                           DomainAccessControlProvisioning domainAccessControlProvisioning) {
        this.cacheManager = ehCacheManager;
        Collection<DomainRoleEntry> domainRoleEntries = domainAccessControlProvisioning.getDomainRoleEntries();
        for (DomainRoleEntry provisionedDomainRoleEntry : domainRoleEntries) {
            updateDomainRole(provisionedDomainRoleEntry);
        }

        Collection<MasterAccessControlEntry> masterAccessControlEntries = domainAccessControlProvisioning.getMasterAccessControlEntries();
        for (MasterAccessControlEntry provisionedMasterAccessControlEntry : masterAccessControlEntries) {
            updateMasterAccessControlEntry(provisionedMasterAccessControlEntry);
        }
    }

    @Override
    public List<DomainRoleEntry> getDomainRoles(String uid) {
        Cache cache = getCache(CacheId.DOMAIN_ROLES);
        List<DomainRoleEntry> domainRoles = new ArrayList<DomainRoleEntry>();
        Attribute<String> uidAttribute = cache.getSearchAttribute(UserRoleKey.USER_ID);
        // query is the fastest if you search for keys and if you need value then call Cache.get(key)
        Query queryRequestedUid = cache.createQuery().addCriteria(uidAttribute.eq(uid)).includeKeys().end();
        Results results = queryRequestedUid.execute();
        for (Result result : results.all()) {
            domainRoles.add(DomainAccessControlStoreEhCache.<DomainRoleEntry> getElementValue(cache.get(result.getKey())));
        }

        return domainRoles;
    }

    @Override
    public DomainRoleEntry getDomainRole(String uid, Role role) {
        Cache cache = getCache(CacheId.DOMAIN_ROLES);
        Attribute<String> uidAttribute = cache.getSearchAttribute(UserRoleKey.USER_ID);
        Attribute<Role> roleAttribute = cache.getSearchAttribute(UserRoleKey.ROLE);
        // query is the fastest if you search for keys and if you need value then call Cache.get(key)
        Query queryRequestedUid = cache.createQuery()
                                       .addCriteria(uidAttribute.eq(uid))
                                       .addCriteria(roleAttribute.eq(role))
                                       .includeKeys()
                                       .end();
        Results results = queryRequestedUid.execute();
        DomainRoleEntry domainRole = null;
        if (!results.all().isEmpty()) {
            // Note: since (uid, role) is the primary key in domain role table
            // results is either empty or contains exactly one entry
            assert (results.all().size() == 1);
            domainRole = (DomainAccessControlStoreEhCache.<DomainRoleEntry> getElementValue(cache.get(results.all()
                                                                                                             .get(0)
                                                                                                             .getKey())));
        }

        return domainRole;
    }

    @Override
    public Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        boolean updateSuccess = false;
        Cache cache = getCache(CacheId.DOMAIN_ROLES);
        UserRoleKey dreKey = new UserRoleKey(updatedEntry.getUid(), updatedEntry.getRole());
        try {
            cache.put(new Element(dreKey, updatedEntry));
            updateSuccess = true;
        } catch (IllegalArgumentException | IllegalStateException | CacheException e) {
            logger.error("UpdateDomainRole failed.", e);
        }

        return updateSuccess;
    }

    @Override
    public Boolean removeDomainRole(String uid, Role role) {
        UserRoleKey dreKey = new UserRoleKey(uid, role);
        return removeControlEntry(CacheId.DOMAIN_ROLES, dreKey);
    }

    @Override
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid) {
        return getControlEntries(uid, CacheId.MASTER_ACL);
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMasterAccessControlEntries(String uid) {
        return getEditableAces(uid, CacheId.MASTER_ACL, Role.MASTER);
    }

    @Override
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String domain, String interfaceName) {
        return getControlEntries(domain, interfaceName, CacheId.MASTER_ACL);
    }

    @Override
    public List<MasterAccessControlEntry> getMasterAccessControlEntries(String uid,
                                                                        String domain,
                                                                        String interfaceName) {
        return getControlEntries(CacheId.MASTER_ACL, uid, domain, interfaceName);
    }

    @Override
    public MasterAccessControlEntry getMasterAccessControlEntry(String uid,
                                                                String domain,
                                                                String interfaceName,
                                                                String operation) {
        MasterAccessControlEntry masterAce = getControlEntry(CacheId.MASTER_ACL, uid, domain, interfaceName, operation);
        if (masterAce == null) {
            masterAce = getControlEntry(CacheId.MASTER_ACL, uid, domain, interfaceName, WILDCARD);
        }

        return masterAce;
    }

    @Override
    public Boolean updateMasterAccessControlEntry(MasterAccessControlEntry updatedMasterAce) {
        boolean updateSuccess = false;
        UserDomainInterfaceOperationKey aceKey = new UserDomainInterfaceOperationKey(updatedMasterAce.getUid(),
                                                                                     updatedMasterAce.getDomain(),
                                                                                     updatedMasterAce.getInterfaceName(),
                                                                                     updatedMasterAce.getOperation());
        updateSuccess = updateControlEntry(updatedMasterAce, CacheId.MASTER_ACL, aceKey);

        return updateSuccess;
    }

    @Override
    public Boolean removeMasterAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        UserDomainInterfaceOperationKey aceKey = new UserDomainInterfaceOperationKey(uid,
                                                                                     domain,
                                                                                     interfaceName,
                                                                                     operation);
        return removeControlEntry(CacheId.MASTER_ACL, aceKey);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid) {
        return getControlEntries(uid, CacheId.MEDIATOR_ACL);
    }

    @Override
    public List<MasterAccessControlEntry> getEditableMediatorAccessControlEntries(String uid) {
        return getEditableAces(uid, CacheId.MEDIATOR_ACL, Role.MASTER);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String domain, String interfaceName) {
        return getControlEntries(domain, interfaceName, CacheId.MEDIATOR_ACL);
    }

    @Override
    public List<MasterAccessControlEntry> getMediatorAccessControlEntries(String uid,
                                                                          String domain,
                                                                          String interfaceName) {
        return getControlEntries(CacheId.MEDIATOR_ACL, uid, domain, interfaceName);
    }

    @Override
    public MasterAccessControlEntry getMediatorAccessControlEntry(String uid,
                                                                  String domain,
                                                                  String interfaceName,
                                                                  String operation) {
        MasterAccessControlEntry mediatorAce = getControlEntry(CacheId.MEDIATOR_ACL,
                                                               uid,
                                                               domain,
                                                               interfaceName,
                                                               operation);
        if (mediatorAce == null) {
            mediatorAce = getControlEntry(CacheId.MEDIATOR_ACL, uid, domain, interfaceName, WILDCARD);
        }

        return mediatorAce;
    }

    @Override
    public Boolean updateMediatorAccessControlEntry(MasterAccessControlEntry updatedMediatorAce) {

        boolean updateSuccess = false;
        MasterAccessControlEntry masterAce = getMasterAccessControlEntry(updatedMediatorAce.getUid(),
                                                                         updatedMediatorAce.getDomain(),
                                                                         updatedMediatorAce.getInterfaceName(),
                                                                         updatedMediatorAce.getOperation());

        AceValidator aceValidator = new AceValidator(masterAce, updatedMediatorAce, null);
        if (aceValidator.isMediatorValid()) {
            UserDomainInterfaceOperationKey aceKey = new UserDomainInterfaceOperationKey(updatedMediatorAce.getUid(),
                                                                                         updatedMediatorAce.getDomain(),
                                                                                         updatedMediatorAce.getInterfaceName(),
                                                                                         updatedMediatorAce.getOperation());
            updateSuccess = updateControlEntry(updatedMediatorAce, CacheId.MEDIATOR_ACL, aceKey);
        }

        return updateSuccess;
    }

    @Override
    public Boolean removeMediatorAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        UserDomainInterfaceOperationKey aceKey = new UserDomainInterfaceOperationKey(uid,
                                                                                     domain,
                                                                                     interfaceName,
                                                                                     operation);
        return removeControlEntry(CacheId.MEDIATOR_ACL, aceKey);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid) {
        return getControlEntries(uid, CacheId.OWNER_ACL);
    }

    @Override
    public List<OwnerAccessControlEntry> getEditableOwnerAccessControlEntries(String uid) {
        return getEditableAces(uid, CacheId.OWNER_ACL, Role.OWNER);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String domain, String interfaceName) {
        return getControlEntries(domain, interfaceName, CacheId.OWNER_ACL);
    }

    @Override
    public List<OwnerAccessControlEntry> getOwnerAccessControlEntries(String uid, String domain, String interfaceName) {
        return getControlEntries(CacheId.OWNER_ACL, uid, domain, interfaceName);
    }

    @Override
    public OwnerAccessControlEntry getOwnerAccessControlEntry(String uid,
                                                              String domain,
                                                              String interfaceName,
                                                              String operation) {
        OwnerAccessControlEntry ownerAce = getControlEntry(CacheId.OWNER_ACL, uid, domain, interfaceName, operation);
        if (ownerAce == null) {
            ownerAce = getControlEntry(CacheId.OWNER_ACL, uid, domain, interfaceName, WILDCARD);
        }

        return ownerAce;
    }

    @Override
    public Boolean updateOwnerAccessControlEntry(OwnerAccessControlEntry updatedOwnerAce) {
        boolean updateSuccess = false;
        MasterAccessControlEntry masterAce = getMasterAccessControlEntry(updatedOwnerAce.getUid(),
                                                                         updatedOwnerAce.getDomain(),
                                                                         updatedOwnerAce.getInterfaceName(),
                                                                         updatedOwnerAce.getOperation());

        MasterAccessControlEntry mediatorAce = getMediatorAccessControlEntry(updatedOwnerAce.getUid(),
                                                                             updatedOwnerAce.getDomain(),
                                                                             updatedOwnerAce.getInterfaceName(),
                                                                             updatedOwnerAce.getOperation());

        AceValidator aceValidator = new AceValidator(masterAce, mediatorAce, updatedOwnerAce);
        if (aceValidator.isOwnerValid()) {
            UserDomainInterfaceOperationKey aceKey = new UserDomainInterfaceOperationKey(updatedOwnerAce.getUid(),
                                                                                         updatedOwnerAce.getDomain(),
                                                                                         updatedOwnerAce.getInterfaceName(),
                                                                                         updatedOwnerAce.getOperation());
            updateSuccess = updateControlEntry(updatedOwnerAce, CacheId.OWNER_ACL, aceKey);
        }

        return updateSuccess;
    }

    @Override
    public Boolean removeOwnerAccessControlEntry(String uid, String domain, String interfaceName, String operation) {
        UserDomainInterfaceOperationKey aceKey = new UserDomainInterfaceOperationKey(uid,
                                                                                     domain,
                                                                                     interfaceName,
                                                                                     operation);
        return removeControlEntry(CacheId.OWNER_ACL, aceKey);
    }

    @Override
    public List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String uid) {
        return getControlEntries(uid, CacheId.MASTER_RCL);
    }

    @Override
    public List<MasterRegistrationControlEntry> getEditableMasterRegistrationControlEntries(String uid) {
        return getEditableAces(uid, CacheId.MASTER_RCL, Role.MASTER);
    }

    @Override
    public List<MasterRegistrationControlEntry> getMasterRegistrationControlEntries(String domain,
                                                                                    String interfaceName) {
        return getControlEntries(domain, interfaceName, CacheId.MASTER_RCL);
    }

    @Override
    public MasterRegistrationControlEntry getMasterRegistrationControlEntry(String uid,
                                                                            String domain,
                                                                            String interfaceName) {
        return getControlEntry(CacheId.MASTER_RCL, uid, domain, interfaceName, WILDCARD);
    }

    @Override
    public Boolean updateMasterRegistrationControlEntry(MasterRegistrationControlEntry updatedMasterRce) {
        boolean updateSuccess = false;
        UserDomainInterfaceOperationKey rceKey = new UserDomainInterfaceOperationKey(updatedMasterRce.getUid(),
                                                                                     updatedMasterRce.getDomain(),
                                                                                     updatedMasterRce.getInterfaceName(),
                                                                                     WILDCARD);
        updateSuccess = updateControlEntry(updatedMasterRce, CacheId.MASTER_RCL, rceKey);

        return updateSuccess;
    }

    @Override
    public Boolean removeMasterRegistrationControlEntry(String uid, String domain, String interfaceName) {
        UserDomainInterfaceOperationKey rceKey = new UserDomainInterfaceOperationKey(uid,
                                                                                     domain,
                                                                                     interfaceName,
                                                                                     WILDCARD);
        return removeControlEntry(CacheId.MASTER_RCL, rceKey);
    }

    @Override
    public List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String uid) {
        return getControlEntries(uid, CacheId.MEDIATOR_RCL);
    }

    @Override
    public List<MasterRegistrationControlEntry> getEditableMediatorRegistrationControlEntries(String uid) {
        return getEditableAces(uid, CacheId.MEDIATOR_RCL, Role.MASTER);
    }

    @Override
    public List<MasterRegistrationControlEntry> getMediatorRegistrationControlEntries(String domain,
                                                                                      String interfaceName) {
        return getControlEntries(domain, interfaceName, CacheId.MEDIATOR_RCL);
    }

    @Override
    public MasterRegistrationControlEntry getMediatorRegistrationControlEntry(String uid,
                                                                              String domain,
                                                                              String interfaceName) {
        return getControlEntry(CacheId.MEDIATOR_RCL, uid, domain, interfaceName, WILDCARD);
    }

    @Override
    public Boolean updateMediatorRegistrationControlEntry(MasterRegistrationControlEntry updatedMediatorRce) {
        boolean updateSuccess = false;
        MasterRegistrationControlEntry masterRce = getMasterRegistrationControlEntry(updatedMediatorRce.getUid(),
                                                                                     updatedMediatorRce.getDomain(),
                                                                                     updatedMediatorRce.getInterfaceName());

        RceValidator rceValidator = new RceValidator(masterRce, updatedMediatorRce, null);
        if (rceValidator.isMediatorValid()) {
            UserDomainInterfaceOperationKey rceKey = new UserDomainInterfaceOperationKey(updatedMediatorRce.getUid(),
                                                                                         updatedMediatorRce.getDomain(),
                                                                                         updatedMediatorRce.getInterfaceName(),
                                                                                         WILDCARD);
            updateSuccess = updateControlEntry(updatedMediatorRce, CacheId.MEDIATOR_RCL, rceKey);
        }

        return updateSuccess;
    }

    @Override
    public Boolean removeMediatorRegistrationControlEntry(String uid, String domain, String interfaceName) {
        UserDomainInterfaceOperationKey rceKey = new UserDomainInterfaceOperationKey(uid,
                                                                                     domain,
                                                                                     interfaceName,
                                                                                     WILDCARD);
        return removeControlEntry(CacheId.MEDIATOR_RCL, rceKey);
    }

    @Override
    public List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String uid) {
        return getControlEntries(uid, CacheId.OWNER_RCL);
    }

    @Override
    public List<OwnerRegistrationControlEntry> getEditableOwnerRegistrationControlEntries(String uid) {
        return getEditableAces(uid, CacheId.OWNER_RCL, Role.OWNER);
    }

    @Override
    public List<OwnerRegistrationControlEntry> getOwnerRegistrationControlEntries(String domain, String interfaceName) {
        return getControlEntries(domain, interfaceName, CacheId.OWNER_RCL);
    }

    @Override
    public OwnerRegistrationControlEntry getOwnerRegistrationControlEntry(String uid,
                                                                          String domain,
                                                                          String interfaceName) {
        return getControlEntry(CacheId.OWNER_RCL, uid, domain, interfaceName, WILDCARD);
    }

    @Override
    public Boolean updateOwnerRegistrationControlEntry(OwnerRegistrationControlEntry updatedOwnerRce) {
        boolean updateSuccess = false;
        MasterRegistrationControlEntry masterRce = getMasterRegistrationControlEntry(updatedOwnerRce.getUid(),
                                                                                     updatedOwnerRce.getDomain(),
                                                                                     updatedOwnerRce.getInterfaceName());

        MasterRegistrationControlEntry mediatorRce = getMediatorRegistrationControlEntry(updatedOwnerRce.getUid(),
                                                                                         updatedOwnerRce.getDomain(),
                                                                                         updatedOwnerRce.getInterfaceName());

        RceValidator rceValidator = new RceValidator(masterRce, mediatorRce, updatedOwnerRce);
        if (rceValidator.isOwnerValid()) {
            UserDomainInterfaceOperationKey rceKey = new UserDomainInterfaceOperationKey(updatedOwnerRce.getUid(),
                                                                                         updatedOwnerRce.getDomain(),
                                                                                         updatedOwnerRce.getInterfaceName(),
                                                                                         WILDCARD);
            updateSuccess = updateControlEntry(updatedOwnerRce, CacheId.OWNER_RCL, rceKey);
        }

        return updateSuccess;
    }

    @Override
    public Boolean removeOwnerRegistrationControlEntry(String uid, String domain, String interfaceName) {
        UserDomainInterfaceOperationKey rceKey = new UserDomainInterfaceOperationKey(uid,
                                                                                     domain,
                                                                                     interfaceName,
                                                                                     WILDCARD);
        return removeControlEntry(CacheId.OWNER_RCL, rceKey);
    }

    private <T extends ControlEntry> T getControlEntry(CacheId cacheId,
                                                       String uid,
                                                       String domain,
                                                       String interfaceName,
                                                       String operation) {
        Cache cache = getCache(cacheId);
        Attribute<String> uidAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.USER_ID);
        Attribute<String> domainAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.DOMAIN);
        Attribute<String> interfaceAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.INTERFACE);
        Attribute<String> operationAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.OPERATION);
        Query queryAllOperations = cache.createQuery()
                                        .addCriteria(uidAttribute.eq(uid).or(uidAttribute.eq(WILDCARD)))
                                        .addCriteria(domainAttribute.eq(domain))
                                        .addCriteria(interfaceAttribute.eq(interfaceName));
        if (cacheId.isACL()) {
            queryAllOperations.addCriteria(operationAttribute.eq(operation));
        }
        // have specific user ids appear before wildcards
        queryAllOperations.addOrderBy(uidAttribute, Direction.DESCENDING).includeKeys().end();
        Results results = queryAllOperations.execute();
        T controlEntry = null;
        if (!results.all().isEmpty()) {
            controlEntry = DomainAccessControlStoreEhCache.<T> getElementValue(cache.get(results.all()
                                                                                                .get(0)
                                                                                                .getKey()));
        }

        return controlEntry;
    }

    private <T extends ControlEntry> List<T> getControlEntries(String uid, CacheId cacheId) {
        Cache cache = getCache(cacheId);
        List<T> controlEntries = new ArrayList<T>();
        // here search on uid take place
        Attribute<String> uidAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.USER_ID);
        // query is the fastest if you search for keys and if you need value then call Cache.get(key)
        Query queryRequestedUid = cache.createQuery()
                                       .addCriteria(uidAttribute.eq(uid).or(uidAttribute.eq(WILDCARD)))
                                       // have specific user ids appear before wildcards
                                       .addOrderBy(uidAttribute, Direction.DESCENDING)
                                       .includeKeys()
                                       .end();
        Results results = queryRequestedUid.execute();
        for (Result result : results.all()) {
            controlEntries.add(DomainAccessControlStoreEhCache.<T> getElementValue(cache.get(result.getKey())));
        }

        return controlEntries;
    }

    private <T extends ControlEntry> List<T> getControlEntries(String domain, String interfaceName, CacheId cacheId) {
        Cache cache = getCache(cacheId);
        List<T> controlEntries = new ArrayList<T>();
        // here search on domain and interface take place
        Attribute<String> domainAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.DOMAIN);
        Attribute<String> interfaceAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.INTERFACE);
        // query is the fastest if you search for keys and if you need value then call Cache.get(key)
        Query queryDomainInterface = cache.createQuery()
                                          .addCriteria(domainAttribute.eq(domain)
                                                                      .and(interfaceAttribute.eq(interfaceName)))
                                          .includeKeys()
                                          .end();
        Results results = queryDomainInterface.execute();
        for (Result result : results.all()) {
            T controlEntry = DomainAccessControlStoreEhCache.<T> getElementValue(cache.get(result.getKey()));
            controlEntries.add(controlEntry);
        }

        return controlEntries;
    }

    private <T extends ControlEntry> List<T> getControlEntries(CacheId cacheId,
                                                               String uid,
                                                               String domain,
                                                               String interfaceName) {
        Cache cache = getCache(cacheId);
        Attribute<String> uidAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.USER_ID);
        Attribute<String> domainAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.DOMAIN);
        Attribute<String> interfaceAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.INTERFACE);
        Query queryAllOperations = cache.createQuery()
                                        .addCriteria(uidAttribute.eq(uid).or(uidAttribute.eq(WILDCARD)))
                                        .addCriteria(domainAttribute.eq(domain))
                                        .addCriteria(interfaceAttribute.eq(interfaceName))
                                        // have specific user ids appear before wildcards
                                        .addOrderBy(uidAttribute, Direction.DESCENDING)
                                        .includeKeys()
                                        .end();
        Results results = queryAllOperations.execute();
        List<T> controlEntries = new ArrayList<T>();
        String currentUid = null;
        for (Result result : results.all()) {
            T controlEntry = DomainAccessControlStoreEhCache.<T> getElementValue(cache.get(result.getKey()));

            // Don't add uid wildcards if a specific uid has been added to the results
            if (currentUid == null) {
                currentUid = controlEntry.getUid();
            } else if (!currentUid.equals(controlEntry.getUid())) {
                break;
            }

            controlEntries.add(controlEntry);
        }

        return controlEntries;
    }

    private <T extends ControlEntry> List<T> getEditableAces(String uid, CacheId cacheId, Role role) {
        List<T> controlEntries = new ArrayList<T>();
        // find out first on which domains uid has specified role
        Cache drtCache = getCache(CacheId.DOMAIN_ROLES);
        UserRoleKey dreKey = new UserRoleKey(uid, role);
        String[] uidDomains = null;
        // read domains from DRE
        if (drtCache.isKeyInCache(dreKey)) {
            DomainRoleEntry dre = DomainAccessControlStoreEhCache.<DomainRoleEntry> getElementValue(drtCache.get(dreKey));
            uidDomains = dre.getDomains();
        }
        // if uid has no domains with specified role return empty list
        if (uidDomains == null || uidDomains.length == 0) {
            return controlEntries;
        }

        Cache cache = getCache(cacheId);
        // here should search on uid and domain take place
        Attribute<String> uidAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.USER_ID);
        Attribute<String> domainAttribute = cache.getSearchAttribute(UserDomainInterfaceOperationKey.DOMAIN);
        for (String domain : uidDomains) {
            Query query = cache.createQuery()
                               .addCriteria(uidAttribute.eq(uid).and(domainAttribute.eq(domain)))
                               .includeKeys()
                               .end();
            Results results = query.execute();
            for (Result result : results.all()) {
                controlEntries.add(DomainAccessControlStoreEhCache.<T> getElementValue(cache.get(result.getKey())));
            }

        }

        return controlEntries;
    }

    private <T extends ControlEntry> Boolean updateControlEntry(T accessControlEntry,
                                                                CacheId cacheId,
                                                                Object controlEntryKey) {
        Cache cache = getCache(cacheId);
        boolean updateSuccess = false;
        try {
            cache.put(new Element(controlEntryKey, accessControlEntry));
            updateSuccess = true;
        } catch (IllegalArgumentException | IllegalStateException | CacheException e) {
            logger.error("Update {} failed:", cacheId, e);
        }

        return updateSuccess;
    }

    private boolean removeControlEntry(CacheId cacheId, Object controlEntryKey) {
        Cache cache = getCache(cacheId);
        boolean removeResult = false;
        try {
            removeResult = cache.remove(controlEntryKey);
        } catch (IllegalArgumentException | IllegalStateException | CacheException e) {
            logger.error("Remove {} failed.", cacheId, e);
        }

        return removeResult;
    }

    protected Cache getCache(CacheId cacheId) {
        Cache cache = cacheManager.getCache(cacheId.getIdAsString());
        if (cache == null) {
            switch (cacheId) {
            case MASTER_ACL:
            case MASTER_RCL:
            case MEDIATOR_RCL:
            case OWNER_RCL:
            case MEDIATOR_ACL:
            case OWNER_ACL: {
                cache = createAclCache(cacheId);
                break;
            }
            case DOMAIN_ROLES: {
                cache = createDrtCache();
                break;
            }
            default: {
                break;
            }
            }
        }

        return cache;
    }

    private Cache createAclCache(CacheId cacheId) {
        // configure cache as searchable
        CacheConfiguration cacheConfig = new CacheConfiguration(cacheId.getIdAsString(), 0).eternal(true);
        Searchable searchable = new Searchable();
        cacheConfig.addSearchable(searchable);
        // register searchable attributes
        searchable.addSearchAttribute(new SearchAttribute().name(UserDomainInterfaceOperationKey.USER_ID));
        searchable.addSearchAttribute(new SearchAttribute().name(UserDomainInterfaceOperationKey.DOMAIN));
        searchable.addSearchAttribute(new SearchAttribute().name(UserDomainInterfaceOperationKey.INTERFACE));
        searchable.addSearchAttribute(new SearchAttribute().name(UserDomainInterfaceOperationKey.OPERATION));
        cacheManager.addCache(new Cache(cacheConfig));
        return cacheManager.getCache(cacheId.getIdAsString());
    }

    private Cache createDrtCache() {
        // configure cache as searchable
        CacheConfiguration cacheConfig = new CacheConfiguration(CacheId.DOMAIN_ROLES.getIdAsString(), 0).eternal(true);
        Searchable searchable = new Searchable();
        cacheConfig.addSearchable(searchable);
        // register searchable attributes
        searchable.addSearchAttribute(new SearchAttribute().name(UserRoleKey.USER_ID));
        searchable.addSearchAttribute(new SearchAttribute().name(UserRoleKey.ROLE));
        cacheManager.addCache(new Cache(cacheConfig));
        return cacheManager.getCache(CacheId.DOMAIN_ROLES.getIdAsString());
    }

    @SuppressWarnings("unchecked")
    public static <T> T getElementValue(Element e) {
        return (T) e.getObjectValue();
    }
}
