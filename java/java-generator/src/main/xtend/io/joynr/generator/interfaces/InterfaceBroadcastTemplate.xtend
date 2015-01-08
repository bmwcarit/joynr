package io.joynr.generator.interfaces

/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FInterface
import io.joynr.generator.util.JoynrJavaGeneratorExtensions

class InterfaceBroadcastTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface serviceInterface) {
		val interfaceName = serviceInterface.joynrName
		val broadcastClassName = interfaceName + "BroadcastInterface"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''
«warning()»
package «packagePath»;

import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.dispatcher.rpc.JoynrBroadcastSubscriptionInterface;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import io.joynr.pubsub.SubscriptionQos;
import joynr.BroadcastFilterParameters;

«FOR datatype: getRequiredIncludesFor(serviceInterface, false, false, false, true)»
	import «datatype»;
«ENDFOR»

public interface «broadcastClassName» extends JoynrBroadcastSubscriptionInterface, «interfaceName» {

		«FOR broadcast : serviceInterface.broadcasts»
			«val broadcastName = broadcast.joynrName»
			«val filterParameters = getFilterParameters(broadcast)»
			«val filterParameterType = broadcastName.toFirstUpper + "BroadcastFilterParameters"»
			«val listenerInterface = broadcastName.toFirstUpper + "BroadcastListener"»

			public interface «listenerInterface» extends BroadcastSubscriptionListener {
				public void receive(«getMappedOutputParametersCommaSeparated(broadcast, false)»);
			}

			«IF isSelective(broadcast)»
			public class «filterParameterType» extends BroadcastFilterParameters {
				public «filterParameterType»() {};
				
				«IF filterParameters.size > 0»
				public «filterParameterType»(«getCommaSeperatedTypedFilterParameterList(broadcast)») {
				«FOR filterPrameter : filterParameters»
					super.addFilterParameter("«filterPrameter»", «filterPrameter»);
				«ENDFOR»
				}
				«ENDIF»
				«FOR filterPrameter : filterParameters»
					public void add«filterPrameter.toFirstUpper»FilterParameter(Object «filterPrameter») {
						super.addFilterParameter("«filterPrameter»", «filterPrameter»);
					}
				«ENDFOR»
			}

			@JoynrRpcBroadcast(broadcastName = "«broadcastName»")
			abstract void subscribeTo«broadcastName.toFirstUpper»Broadcast(
			        «listenerInterface» broadcastListener,
			        SubscriptionQos subscriptionQos,
			        «filterParameterType» filterParameters);
			«ELSE»
			@JoynrRpcBroadcast(broadcastName = "«broadcastName»")
			abstract void subscribeTo«broadcastName.toFirstUpper»Broadcast(
			        «listenerInterface» subscriptionListener,
			        SubscriptionQos subscriptionQos);
			«ENDIF»

			abstract void unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(String subscriptionId);
		«ENDFOR»


}
'''
	}

}
