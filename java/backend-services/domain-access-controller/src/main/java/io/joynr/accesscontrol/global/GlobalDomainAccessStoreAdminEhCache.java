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

import io.joynr.accesscontrol.DomainAccessControlProvisioning;
import io.joynr.accesscontrol.DomainAccessControlStoreEhCache;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import net.sf.ehcache.Cache;
import net.sf.ehcache.CacheManager;
import net.sf.ehcache.Element;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * Uses EhCache to implement a GlobalDomainAccessStore.
 * Add/Remove operations can be expensive. Get operations should be fast.
 */
//TODO this will be redundant when we use the provider directly
@Singleton
public class GlobalDomainAccessStoreAdminEhCache extends DomainAccessControlStoreEhCache implements
        GlobalDomainAccessStoreAdmin {

    @Inject
    public GlobalDomainAccessStoreAdminEhCache(CacheManager ehCacheManager,
                                               DomainAccessControlProvisioning domainAccessControlProvisioning) {
        super(ehCacheManager, domainAccessControlProvisioning);
    }

    @Override
    public List<MasterAccessControlEntry> getAllMasterAclEntries() {
        List<MasterAccessControlEntry> result = new ArrayList<MasterAccessControlEntry>();
        Cache aclCache = getCache(CacheId.MASTER_ACL);
        Map<Object, Element> aclMap = aclCache.getAll(aclCache.getKeys());
        Iterator<Map.Entry<Object, Element>> iterator = aclMap.entrySet().iterator();
        while (iterator.hasNext()) {
            Map.Entry<Object, Element> thisMapEntry = iterator.next();
            Element thisElement = thisMapEntry.getValue();
            MasterAccessControlEntry objectValue = getElementValue(thisElement);
            result.add(objectValue);
        }
        return result;
    }

    @Override
    public List<OwnerAccessControlEntry> getAllOwnerAclEntries() {
        List<OwnerAccessControlEntry> result = new ArrayList<OwnerAccessControlEntry>();
        Cache aclCache = getCache(CacheId.OWNER_ACL);
        Map<Object, Element> aclMap = aclCache.getAll(aclCache.getKeys());
        Iterator<Map.Entry<Object, Element>> iterator = aclMap.entrySet().iterator();
        while (iterator.hasNext()) {
            Map.Entry<Object, Element> thisMapEntry = iterator.next();
            Element thisElement = thisMapEntry.getValue();
            OwnerAccessControlEntry objectValue = getElementValue(thisElement);
            result.add(objectValue);
        }
        return result;
    }

    @Override
    public List<DomainRoleEntry> getAllDomainRoleEntries() {
        List<DomainRoleEntry> result = Lists.newArrayList();
        Cache roleCache = getCache(CacheId.DOMAIN_ROLES);
        Map<Object, Element> roleMap = roleCache.getAll(roleCache.getKeys());
        Iterator<Map.Entry<Object, Element>> iterator = roleMap.entrySet().iterator();
        while (iterator.hasNext()) {
            Element thisElement = iterator.next().getValue();
            DomainRoleEntry objectValue = getElementValue(thisElement);
            result.add(objectValue);
        }
        return result;
    }
}
