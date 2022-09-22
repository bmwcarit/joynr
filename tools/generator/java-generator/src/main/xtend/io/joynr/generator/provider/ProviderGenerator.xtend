package io.joynr.generator.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 *
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
 */

import com.google.inject.Inject
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTemplateFactory
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface

class ProviderGenerator {

	@Inject
	extension JoynrJavaGeneratorExtensions

	@Inject
	extension NamingUtil
	@Inject JavaTemplateFactory templateFactory

	def doGenerate(FInterface fInterface, IFileSystemAccess fsa, boolean generateVersion) {
		val path = getPackagePathWithJoynrPrefix(fInterface, File::separator, generateVersion) + File::separator

		var serviceName =  fInterface.joynrName

		var interfaceProviderTemplate = templateFactory.createInterfaceProviderTemplate(fInterface)
		generateFile(
			fsa,
			path + serviceName + "Provider.java",
			interfaceProviderTemplate,
			generateVersion
		);

		var defaultInterfaceProviderTemplate = templateFactory.createDefaultInterfaceProviderTemplate(fInterface)
		generateFile(
			fsa,
			path + "Default" + serviceName + "Provider.java",
			defaultInterfaceProviderTemplate,
			generateVersion
		);

		var interfaceAbstractProviderTemplate = templateFactory.createInterfaceAbstractProviderTemplate(fInterface)
		generateFile(
			fsa,
			path + serviceName + "AbstractProvider.java",
			interfaceAbstractProviderTemplate,
			generateVersion
		);

		var subscriptionPublisherTemplate = templateFactory.createSubscriptionPublisherTemplate(fInterface)
		generateFile(
			fsa,
			path + serviceName + "SubscriptionPublisher.java",
			subscriptionPublisherTemplate,
			generateVersion
		);

		var subscriptionPublisherImplTemplate = templateFactory.createSubscriptionPublisherImplTemplate(fInterface)
		generateFile(
			fsa,
			path + serviceName + "SubscriptionPublisherImpl.java",
			subscriptionPublisherImplTemplate,
			generateVersion
		);
	}
}
