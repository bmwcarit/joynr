/**
 *
 */
package itest.io.joynr.jeeintegration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.io.File;

import javax.inject.Inject;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.spi.TestResult;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.ServiceProviderDiscovery;

/**
 * Integration tests for the JEE integration bean.
 */
@RunWith(Arquillian.class)
public class JeeIntegrationBeanTest {

	@Deployment
	public static JavaArchive createTestArchive() {
		// @formatter:off
		return ShrinkWrap.create(JavaArchive.class)
				.addClasses(ServiceProviderDiscovery.class,
						DefaultJoynrRuntimeFactory.class,
						JeeIntegrationJoynrTestConfigurationProvider.class,
						JoynrIntegrationBean.class,
						TestResult.class
						)
				.addAsManifestResource(new File("src/main/resources/META-INF/beans.xml"));
		// @formatter:on
	}

	@Inject
	private JoynrIntegrationBean joynrIntegrationBean;

	@Test
	public void testJoynrRuntimeAvailable() {
		Assert.assertNotNull(joynrIntegrationBean.getRuntime());
	}

}
