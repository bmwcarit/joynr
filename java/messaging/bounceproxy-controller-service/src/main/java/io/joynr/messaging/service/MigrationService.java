package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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

/**
 * Interface for migration service implementation.
 * 
 * @author christina.strobel
 * 
 */
public interface MigrationService {

    /**
     * Migrates a whole cluster of bounce proxy instances to another cluster.
     * For each channel a new decision is taken to which bounce proxy instance
     * it will be migrated.
     * 
     * @param clusterId the identifier of the cluster to migrate
     */
    void migrateCluster(String clusterId);

}
