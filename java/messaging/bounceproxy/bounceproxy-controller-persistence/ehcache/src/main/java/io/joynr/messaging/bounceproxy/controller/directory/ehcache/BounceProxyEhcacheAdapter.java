package io.joynr.messaging.bounceproxy.controller.directory.ehcache;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import net.sf.ehcache.Cache;
import net.sf.ehcache.CacheManager;
import net.sf.ehcache.Element;
import net.sf.ehcache.distribution.CacheManagerPeerProvider;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

/**
 * {@link BounceProxyDirectory} implementation which uses Ehcache to store
 * bounce proxy information.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyEhcacheAdapter implements BounceProxyDirectory {

    private static final Logger log = LoggerFactory.getLogger(BounceProxyEhcacheAdapter.class);

    public final static String PROPERTY_BP_CACHE_NAME = "joynr.bounceproxy.controller.bp_cache_name";
    public final static String PROPERTY_BP_CACHE_CONFIGURATION = "joynr.bounceproxy.controller.bp_cache_config_file";

    private final CacheManager manager;
    private final String cacheName;

    @Inject
    public BounceProxyEhcacheAdapter(@Named(PROPERTY_BP_CACHE_NAME) String cacheName, CacheManager cacheManager) {
        this.cacheName = cacheName;
        this.manager = cacheManager;
    }

    @Override
    public List<BounceProxyRecord> getAssignableBounceProxies() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void updateChannelAssignment(String ccid, BounceProxyInformation bpInfo, long timestamp)
                                                                                                   throws IllegalArgumentException {
        // TODO Auto-generated method stub

    }

    @Override
    public BounceProxyRecord getBounceProxy(String bpId) throws IllegalArgumentException {

        if (log.isTraceEnabled()) {
            log.trace("Retrieving bounce proxy {} from cache {}", bpId, cacheName);
            tracePeers();
        }
        Cache cache = manager.getCache(cacheName);
        Element element = cache.get(bpId);

        if (element == null) {
            throw new IllegalArgumentException("No bounce proxy with ID '" + bpId + "' exists");
        }
        return (BounceProxyRecord) element.getObjectValue();
    }

    @Override
    public boolean containsBounceProxy(String bpId) {

        if (log.isTraceEnabled()) {
            log.trace("containsBounceProxy {} in cache {}", bpId, cacheName);
            tracePeers();
        }
        Cache cache = manager.getCache(cacheName);
        return cache.get(bpId) != null;
    }

    @Override
    public void addBounceProxy(ControlledBounceProxyInformation bpInfo, long timestamp) throws IllegalArgumentException {

        if (log.isTraceEnabled()) {
            log.trace("addBounceProxy {} to cache {}", bpInfo.getId(), cacheName);
            tracePeers();
        }

        Cache cache = manager.getCache(cacheName);
        Element element = new Element(bpInfo.getId(), new BounceProxyRecord(bpInfo));
        cache.put(element);
    }

    @Override
    public void updateBounceProxy(BounceProxyRecord bpRecord, long timestamp) throws IllegalArgumentException {

        if (log.isTraceEnabled()) {
            log.trace("updateBounceProxy {} in cache {}", bpRecord.getBounceProxyId(), cacheName);
            tracePeers();
        }

        Cache cache = manager.getCache(cacheName);
        Element element = new Element(bpRecord.getBounceProxyId(), bpRecord);
        cache.put(element);
    }

    @Override
    public List<BounceProxyStatusInformation> getBounceProxyStatusInformation() {

        if (log.isTraceEnabled()) {
            log.trace("getBounceProxyStatusInformation from cache {}", cacheName);
            tracePeers();
        }

        List<BounceProxyStatusInformation> result = new LinkedList<BounceProxyStatusInformation>();

        Cache cache = manager.getCache(cacheName);
        @SuppressWarnings("unchecked")
        List keys = cache.getKeys();
        Map<Object, Element> elements = cache.getAll(keys);

        for (Element element : elements.values()) {
            result.add((BounceProxyStatusInformation) element.getObjectValue());
        }

        return result;
    }

    private void tracePeers() {
        CacheManagerPeerProvider peerProvider = manager.getCacheManagerPeerProvider("RMI");
        int peers = peerProvider.listRemoteCachePeers(manager.getEhcache(cacheName)).size();
        log.trace("Found {} remote cache peer(s)", peers);
    }

}
