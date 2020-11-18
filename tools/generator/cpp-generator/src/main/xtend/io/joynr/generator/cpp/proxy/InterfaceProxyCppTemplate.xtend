package io.joynr.generator.cpp.proxy
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.InterfaceUtil

class InterfaceProxyCppTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil

	override generate(boolean generateVersion)
'''
«val interfaceName =  francaIntf.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
«val syncClassName = interfaceName + "SyncProxy"»
«val fireAndForgetClassName = interfaceName + "FireAndForgetProxy"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«className».h"

«getNamespaceStarter(francaIntf, generateVersion)»
«className»::«className»(
		std::weak_ptr<joynr::JoynrRuntimeImpl> runtime,
		std::shared_ptr<joynr::JoynrMessagingConnectorFactory> connectorFactory,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings
) :
		joynr::ProxyBase(runtime, connectorFactory, domain, qosSettings),
		«className»Base(runtime, connectorFactory, domain, qosSettings),
		«IF hasFireAndForgetMethods(francaIntf)»«fireAndForgetClassName»(runtime, connectorFactory, domain, qosSettings),«ENDIF»
		«syncClassName»(runtime, connectorFactory, domain, qosSettings),
		«asyncClassName»(runtime, connectorFactory, domain, qosSettings)
{
}

«getNamespaceEnder(francaIntf, generateVersion)»
'''
}
