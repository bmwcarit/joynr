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
package io.joynr.test.interlanguage.jee;

import javax.inject.Inject;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;

import org.junit.runner.JUnitCore;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.ilt.testresults.TestResult;
import io.joynr.jeeintegration.api.ServiceLocator;

@Path("/")
@Produces(MediaType.APPLICATION_JSON)
public class IltConsumerRestEndpoint {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerRestEndpoint.class);

    @Inject
    public IltConsumerRestEndpoint(ServiceLocator serviceLocator) {
        IltConsumerHelper.setServiceLocator(serviceLocator);
    }

    @GET
    @Path("/start-tests")
    public TestResult startTests() {
        TestResult testResult;

        logger.info("startTests: entering");
        JUnitCore runner = new JUnitCore();
        IltConsumerJUnitListener listener = new IltConsumerJUnitListener();
        runner.addListener(listener);
        runner.run(IltConsumerTestSuite.class);
        testResult = listener.getTestResult();
        logger.info("startTests: leaving");
        return testResult;
    }
}
