package io.joynr.generator.cpp.proxy
/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceProxyCppTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject private extension NamingUtil

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override generate()
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«className».h"

«getNamespaceStarter(francaIntf)»
«className»::«className»(
		std::shared_ptr<joynr::system::RoutingTypes::Address> messagingAddress,
		joynr::ConnectorFactory* connectorFactory,
		joynr::IClientCache *cache,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings,
		bool cached
) :
		joynr::ProxyBase(connectorFactory, cache, domain, qosSettings, cached),
		«className»Base(messagingAddress, connectorFactory, cache, domain, qosSettings, cached),
		«syncClassName»(messagingAddress, connectorFactory, cache, domain, qosSettings, cached),
		«asyncClassName»(messagingAddress, connectorFactory, cache, domain, qosSettings, cached)
{
}

«getNamespaceEnder(francaIntf)»
'''
}