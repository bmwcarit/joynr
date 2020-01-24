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
import java.util.Optional;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.EntityTransaction;
import javax.persistence.Query;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.jdbc.entities.BounceProxyInformationEntity;
import io.joynr.messaging.bounceproxy.controller.directory.jdbc.entities.ChannelEntity;
import io.joynr.messaging.info.Channel;

/**
 * Access layer that stores and retrieves channels to and from a relational
 * database via JDBC.
 * 
 * @author christina.strobel
 * 
 */
public class ChannelDatabase implements ChannelDirectory {

    private static final Logger logger = LoggerFactory.getLogger(ChannelDatabase.class);

    private final EntityManagerFactory emf;

    @Inject
    public ChannelDatabase(EntityManagerFactory emf) {
        this.emf = emf;
    }

    @Override
    public List<Channel> getChannels() {

        logger.trace("getChannels()");

        List<Channel> channels = new LinkedList<Channel>();

        EntityManager em = emf.createEntityManager();

        String sqlQueryString = String.format("SELECT x FROM %s x", ChannelEntity.class.getSimpleName());
        Query query = em.createQuery(sqlQueryString);
        @SuppressWarnings("unchecked")
        List<ChannelEntity> resultList = query.getResultList();

        for (ChannelEntity result : resultList) {
            channels.add(result.convertToChannel());
        }

        logger.debug("retrieved {} channels", channels.size());
        return channels;
    }

    @Override
    public Channel getChannel(Optional<String> ccid) {

        String channelId;
        if (ccid.isPresent()) {
            channelId = ccid.get();
        } else {
            logger.debug("no channel found for ID NULL");
            return null;
        }
        logger.trace("getChannel({})", channelId);

        EntityManager em = emf.createEntityManager();
        ChannelEntity channelEntity = em.find(ChannelEntity.class, channelId);
        if (channelEntity == null) {
            logger.debug("no channel found for ID {}", channelId);
            return null;
        }

        return channelEntity.convertToChannel();
    }

    @Override
    public void addChannel(Channel channel) {

        logger.trace("add channel {}", channel);

        EntityManager em = emf.createEntityManager();
        EntityTransaction tx = em.getTransaction();

        try {
            tx.begin();

            BounceProxyInformationEntity bpInfoEntity = em.find(BounceProxyInformationEntity.class,
                                                                channel.getBounceProxy().getId());
            if (bpInfoEntity == null) {
                tx.rollback();
                logger.error("No bounce proxy with ID {} registered for channel {}",
                             channel.getBounceProxy().getId(),
                             channel.getChannelId());
                throw new IllegalArgumentException("No bounce proxy with ID '" + channel.getBounceProxy().getId()
                        + "' registered for channel '" + channel.getChannelId() + "'");
            }

            ChannelEntity entity = new ChannelEntity(channel.getChannelId(),
                                                     bpInfoEntity,
                                                     channel.getLocation().toString());
            em.persist(entity);

            tx.commit();
        } catch (RuntimeException ex) {
            logger.error("Error persisting channel with ID {}: error: {}", channel.getChannelId(), ex.getMessage());
            if (tx != null && tx.isActive())
                tx.rollback();
            throw ex;
        }
    }
}
