/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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
package io.joynr.messaging.bounceproxy.controller.directory.jdbc;

import java.util.LinkedList;
import java.util.List;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.EntityTransaction;
import javax.persistence.Query;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.bounceproxy.controller.directory.jdbc.entities.BounceProxyEntity;
import io.joynr.messaging.bounceproxy.controller.directory.jdbc.entities.ChannelEntity;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.system.TimestampProvider;

/**
 * Access layer that stores and retrieves bounce proxies to and from a
 * relational database via JDBC.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyDatabase implements BounceProxyDirectory {

    private static final Logger logger = LoggerFactory.getLogger(BounceProxyDatabase.class);

    private final EntityManagerFactory emf;

    private final TimestampProvider timestampProvider;

    @Inject
    public BounceProxyDatabase(EntityManagerFactory emf, TimestampProvider timestampProvider) {
        this.emf = emf;
        this.timestampProvider = timestampProvider;
    }

    @Override
    public List<BounceProxyRecord> getAssignableBounceProxies() {
        List<BounceProxyRecord> bounceProxyList = new LinkedList<BounceProxyRecord>();

        for (BounceProxyEntity entity : getBounceProxyEntityList(getSqlWhereClauseForAssignableBounceProxies())) {
            bounceProxyList.add(entity.convertToBounceProxyRecord());
        }

        logger.debug("Found {} assignable bounce proxies", bounceProxyList.size());

        return bounceProxyList;
    }

    @Override
    public void updateChannelAssignment(String ccid, BounceProxyInformation bpInfo) throws IllegalArgumentException {

        logger.trace("UpdateChannelAssignment(ccid={}, bpId={})", ccid, bpInfo.getId());

        EntityManager em = emf.createEntityManager();
        EntityTransaction tx = em.getTransaction();

        try {
            tx.begin();
            BounceProxyEntity bpEntity = em.find(BounceProxyEntity.class, bpInfo.getId());
            if (bpEntity == null) {
                logger.error("No bounce proxy with ID {} registered", bpInfo.getId());
                throw new IllegalArgumentException("No bounce proxy with ID '" + bpInfo.getId() + "' registered");
            }

            ChannelEntity channelEntity = em.find(ChannelEntity.class, ccid);
            if (channelEntity == null) {
                logger.error("No channel with ID {} was registered before.", ccid);
                throw new IllegalArgumentException("No channel with ID '" + ccid + "' was registered before.");
            }

            // TODO what to do if channel with the same ID is added, but
            // different other properties?
            bpEntity.addChannel(channelEntity);

            em.merge(bpEntity);

            tx.commit();

        } catch (RuntimeException ex) {
            logger.error("Channel assignment could not be persisted. Error:", ex);
            if (tx != null && tx.isActive())
                tx.rollback();
            throw ex;
        }
    }

    @Override
    public BounceProxyRecord getBounceProxy(String bpId) throws IllegalArgumentException {

        logger.trace("GetBounceProxy(bpid={})", bpId);

        EntityManager em = emf.createEntityManager();
        BounceProxyEntity bounceProxyEntity = em.find(BounceProxyEntity.class, bpId);
        if (bounceProxyEntity == null) {
            logger.error("No bounce proxy with ID {} in bounce proxy database", bpId);
            throw new IllegalArgumentException("No bounce proxy with ID '" + bpId + "' in bounce proxy database");
        }

        return bounceProxyEntity.convertToBounceProxyRecord();
    }

    @Override
    public boolean containsBounceProxy(String bpId) {

        logger.trace("ContainsBounceProxy(bpid={})", bpId);

        EntityManager em = emf.createEntityManager();
        BounceProxyEntity bounceProxyEntity = em.find(BounceProxyEntity.class, bpId);
        return bounceProxyEntity != null;
    }

    @Override
    public void addBounceProxy(ControlledBounceProxyInformation bpInfo) throws IllegalArgumentException {

        logger.trace("AddBounceProxy(bpid={})", bpInfo.getId());

        BounceProxyRecord record = new BounceProxyRecord(bpInfo);
        record.setFreshness(timestampProvider.getCurrentTime());

        BounceProxyEntity bpEntity = new BounceProxyEntity(record);

        EntityManager em = emf.createEntityManager();
        EntityTransaction tx = em.getTransaction();
        try {
            tx.begin();
            em.persist(bpEntity);
            tx.commit();
        } catch (RuntimeException ex) {
            logger.error("Bounce proxy {} could not be persisted. Error:", bpInfo.getId(), ex);
            if (tx != null && tx.isActive())
                tx.rollback();
            throw ex;
        }
    }

    @Override
    public void updateBounceProxy(BounceProxyRecord bpRecord) throws IllegalArgumentException {

        logger.trace("UpdateBounceProxy(bpid={})", bpRecord.getBounceProxyId());

        EntityManager em = emf.createEntityManager();
        EntityTransaction tx = em.getTransaction();

        try {
            tx.begin();

            BounceProxyEntity bpEntity = new BounceProxyEntity(bpRecord);
            bpEntity.setFreshness(timestampProvider.getCurrentTime());
            em.merge(bpEntity);

            tx.commit();
        } catch (RuntimeException ex) {
            logger.error("Bounce proxy {} could not be updated. Error:", bpRecord.getBounceProxyId(), ex);
            if (tx != null && tx.isActive())
                tx.rollback();
            throw ex;
        }
    }

    @Override
    public List<BounceProxyStatusInformation> getBounceProxyStatusInformation() {
        List<BounceProxyStatusInformation> result = new LinkedList<BounceProxyStatusInformation>();

        for (BounceProxyEntity bp : getBounceProxyEntityList()) {
            result.add(bp.convertToBounceProxyRecord());
        }

        logger.debug("Found {} bounce proxies", result.size());
        return result;
    }

    private List<BounceProxyEntity> getBounceProxyEntityList() {
        return getBounceProxyEntityList(null);
    }

    private List<BounceProxyEntity> getBounceProxyEntityList(String sqlWhereClause) {

        String query;
        if (sqlWhereClause == null) {
            query = String.format("SELECT x FROM %s x", BounceProxyEntity.class.getSimpleName());
        } else {
            query = String.format("SELECT x FROM %s x WHERE %s",
                                  BounceProxyEntity.class.getSimpleName(),
                                  sqlWhereClause);
        }

        EntityManager em = emf.createEntityManager();

        Query getBounceProxiesQuery = em.createQuery(query);
        @SuppressWarnings("unchecked")
        List<BounceProxyEntity> queryResults = getBounceProxiesQuery.getResultList();

        return queryResults;
    }

    private String getSqlWhereClauseForAssignableBounceProxies() {
        StringBuffer sqlWhereClause = new StringBuffer();
        for (BounceProxyStatus status : BounceProxyStatus.values()) {
            if (status.isAssignable()) {

                if (sqlWhereClause.length() != 0) {
                    sqlWhereClause.append(" OR ");
                }

                sqlWhereClause.append("x.status = '").append(status.name()).append("'");
            }
        }
        return sqlWhereClause.toString();
    }

}
