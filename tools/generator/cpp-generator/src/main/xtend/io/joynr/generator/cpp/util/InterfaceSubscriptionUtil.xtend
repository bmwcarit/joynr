package io.joynr.generator.cpp.util
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
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceSubscriptionUtil {
	@Inject	extension InterfaceUtil
	@Inject	extension AttributeUtil
	@Inject	extension BroadcastUtil
	@Inject	extension NamingUtil
	@Inject	extension CppStdTypeUtil

	def produceSubscribeUnsubscribeMethods(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.notifiable]»
		«val returnType = attribute.typeName»
		/**
		 * @brief creates a new subscription to attribute «attribute.joynrName.toFirstUpper»
		 * @param subscriptionListener The listener callback providing methods to call on publication and failure
		 * @param subscriptionQos The subscription quality of service settings
		 * @return the subscription id as string
		 */
		«IF pure»virtual «ENDIF»std::string subscribeTo«attribute.joynrName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos) «IF pure»= 0«ELSE»override«ENDIF»;

		/**
		 * @brief updates an existing subscription to attribute «attribute.joynrName.toFirstUpper»
		 * @param subscriptionListener The listener callback providing methods to call on publication and failure
		 * @param subscriptionQos The subscription quality of service settings
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 * @return the subscription id as string
		 */
		«IF pure»virtual «ENDIF»std::string subscribeTo«attribute.joynrName.toFirstUpper»(
					std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
					const joynr::SubscriptionQos& subscriptionQos,
					std::string& subscriptionId) «IF pure»= 0«ELSE»override«ENDIF»;

		/**
		 * @brief unsubscribes from attribute «attribute.joynrName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		«IF pure»virtual «ENDIF»void unsubscribeFrom«attribute.joynrName.toFirstUpper»(std::string& subscriptionId) «IF pure»= 0«ELSE»override«ENDIF»;

	«ENDFOR»
	«FOR broadcast: serviceInterface.broadcasts»
		«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
		«IF isSelective(broadcast)»
			/**
			 * @brief subscribes to selective broadcast «broadcast.joynrName.toFirstUpper» with filter parameters
			 * @param filterParameters The filter parameters for selection of suitable broadcasts
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			«IF pure»virtual «ENDIF»std::string subscribeTo«broadcast.joynrName.toFirstUpper»Broadcast(
						const «serviceInterface.name.toFirstUpper»«broadcast.joynrName.toFirstUpper»BroadcastFilterParameters& filterParameters,
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos) «IF pure»= 0«ELSE»override«ENDIF»;

			/**
			 * @brief updates an existing subscription to selective broadcast «broadcast.joynrName.toFirstUpper» with filter parameters
			 * @param filterParameters The filter parameters for selection of suitable broadcasts
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @param subscriptionId The subscription id returned earlier on creation of the subscription
			 * @return the subscription id as string
			 */
			«IF pure»virtual «ENDIF»std::string subscribeTo«broadcast.joynrName.toFirstUpper»Broadcast(
						const «serviceInterface.name.toFirstUpper»«broadcast.joynrName.toFirstUpper»BroadcastFilterParameters& filterParameters,
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos,
						std::string& subscriptionId) «IF pure»= 0«ELSE»override«ENDIF»;
		«ELSE»
			/**
			 * @brief subscribes to broadcast «broadcast.joynrName.toFirstUpper»
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @return the subscription id as string
			 */
			«IF pure»virtual «ENDIF»std::string subscribeTo«broadcast.joynrName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos) «IF pure»= 0«ELSE»override«ENDIF»;

			/**
			 * @brief updates an existing subscription to broadcast «broadcast.joynrName.toFirstUpper»
			 * @param filterParameters The filter parameters for selection of suitable broadcasts
			 * @param subscriptionListener The listener callback providing methods to call on publication and failure
			 * @param subscriptionQos The subscription quality of service settings
			 * @param subscriptionId The subscription id returned earlier on creation of the subscription
			 * @return the subscription id as string
			 */
			«IF pure»virtual «ENDIF»std::string subscribeTo«broadcast.joynrName.toFirstUpper»Broadcast(
						std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»> > subscriptionListener,
						const joynr::OnChangeSubscriptionQos& subscriptionQos,
						std::string& subscriptionId) «IF pure»= 0«ELSE»override«ENDIF»;
		«ENDIF»

		/**
		 * @brief unsubscribes from broadcast «broadcast.joynrName.toFirstUpper»
		 * @param subscriptionId The subscription id returned earlier on creation of the subscription
		 */
		«IF pure»virtual «ENDIF»void unsubscribeFrom«broadcast.joynrName.toFirstUpper»Broadcast(std::string& subscriptionId) «IF pure»= 0«ELSE»override«ENDIF»;

	«ENDFOR»
'''
}
