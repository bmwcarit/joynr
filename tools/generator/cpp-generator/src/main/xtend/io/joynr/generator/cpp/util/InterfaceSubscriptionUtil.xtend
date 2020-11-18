package io.joynr.generator.cpp.util
/*
 * !!!
 *
 * Copyright (C) 2017 BMW Car IT GmbH
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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBroadcast

class InterfaceSubscriptionUtil {
	@Inject	extension InterfaceUtil
	@Inject	extension AttributeUtil
	@Inject	extension NamingUtil
	@Inject	extension CppStdTypeUtil

	def produceSubscribeToAttributeComments(FAttribute attribute)
'''
/**
 * @brief creates a new subscription to attribute «attribute.joynrName.toFirstUpper»
 * @param subscriptionListener The listener callback providing methods to call on publication and failure
 * @param subscriptionQos The subscription quality of service settings
 * @return a future representing the result (subscription id) as string. It provides methods to wait for
 * completion, to get the subscription id or the request status object. The subscription id will be available
 * when the subscription is successfully registered at the provider.
 */
'''

	def produceUpdateAttributeSubscriptionComments(FAttribute attribute)
'''
/**
 * @brief updates an existing subscription to attribute «attribute.joynrName.toFirstUpper»
 * @param subscriptionListener The listener callback providing methods to call on publication and failure
 * @param subscriptionQos The subscription quality of service settings
 * @param subscriptionId The subscription id returned earlier on creation of the subscription
 * @return a future representing the result (subscription id) as string. It provides methods to wait for
 * completion, to get the subscription id or the request status object. The subscription id will be available
 * when the subscription is successfully registered at the provider.
 */
'''

	def produceUnsubscribeFromAttributeComments(FAttribute attribute)
'''
/**
 * @brief unsubscribes from attribute «attribute.joynrName.toFirstUpper»
 * @param subscriptionId The subscription id returned earlier on creation of the subscription
 */
'''

	def produceSubscribeToBroadcastComments(FBroadcast broadcast)
'''
/**
 * @brief subscribes to «IF broadcast.selective»selective «ENDIF»broadcast «broadcast.joynrName.toFirstUpper»«IF broadcast.selective» with filter parameters
 * @param filterParameters The filter parameters for selection of suitable broadcasts«ENDIF»
 * @param subscriptionListener The listener callback providing methods to call on publication and failure
 * @param subscriptionQos The subscription quality of service settings
 * @return a future representing the result (subscription id) as string. It provides methods to wait for
 * completion, to get the subscription id or the request status object. The subscription id will be available
 * when the subscription is successfully registered at the provider.
 */
'''

	def produceUpdateBroadcastSubscriptionComments(FBroadcast broadcast)
'''
/**
 * @brief updates an existing subscription to «IF broadcast.selective»selective «ENDIF»broadcast «broadcast.joynrName.toFirstUpper»«IF broadcast.selective» with filter parameters
 * @param filterParameters The filter parameters for selection of suitable broadcasts«ENDIF»
 * @param subscriptionListener The listener callback providing methods to call on publication and failure
 * @param subscriptionQos The subscription quality of service settings
 * @param subscriptionId The subscription id returned earlier on creation of the subscription
 * @return a future representing the result (subscription id) as string. It provides methods to wait for
 * completion, to get the subscription id or the request status object. The subscription id will be available
 * when the subscription is successfully updated at the provider.
 */
'''

	def produceUnsubscribeFromBroadcastComments(FBroadcast broadcast)
'''
/**
 * @brief unsubscribes from broadcast «broadcast.joynrName.toFirstUpper»
 * @param subscriptionId The subscription id returned earlier on creation of the subscription
 */
'''

	def produceSubscribeToAttributeSignature(FAttribute attribute, boolean updateSubscription, String className, boolean generateVersion)
'''
«val returnType = attribute.getTypeName(generateVersion)»
std::shared_ptr<joynr::Future<std::string>> «IF className !== null»«className»::«ENDIF»subscribeTo«attribute.joynrName.toFirstUpper»(
			std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
			std::shared_ptr<joynr::SubscriptionQos> subscriptionQos«IF updateSubscription»,
			const std::string& subscriptionId«ENDIF»)
'''

	def produceSubscribeToAttributeSignature(FAttribute attribute, boolean generateVersion) {
		return produceSubscribeToAttributeSignature(attribute, false, null, generateVersion)
	}

	def produceSubscribeToAttributeSignature(FAttribute attribute, String className, boolean generateVersion) {
		return produceSubscribeToAttributeSignature(attribute, false, className, generateVersion)
	}

	def produceUpdateAttributeSubscriptionSignature(FAttribute attribute, boolean generateVersion) {
		return produceSubscribeToAttributeSignature(attribute, true, null, generateVersion)
	}

	def produceUpdateAttributeSubscriptionSignature(FAttribute attribute, String className, boolean generateVersion) {
		return produceSubscribeToAttributeSignature(attribute, true, className, generateVersion)
	}

	def produceUnsubscribeFromAttributeSignature(FAttribute attribute, String className)
'''
void «IF className !== null»«className»::«ENDIF»unsubscribeFrom«attribute.joynrName.toFirstUpper»(const std::string& subscriptionId)
'''

	def produceUnsubscribeFromAttributeSignature(FAttribute attribute) {
		return produceUnsubscribeFromAttributeSignature(attribute, null)
	}

	def produceSubscribeToBroadcastSignature(FBroadcast broadcast, FInterface serviceInterface, boolean updateSubscription, String className, boolean hTemplate, boolean generateVersion)
'''
«val returnTypes = broadcast.getCommaSeparatedOutputParameterTypes(generateVersion)»
std::shared_ptr<joynr::Future<std::string>> «IF className !== null»«className»::«ENDIF»subscribeTo«broadcast.joynrName.toFirstUpper»Broadcast(
			«IF updateSubscription»
			const std::string& subscriptionId,
			«ENDIF»
			«IF broadcast.selective»
			const «serviceInterface.name.toFirstUpper»«broadcast.joynrName.toFirstUpper»BroadcastFilterParameters& filterParameters,
			std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»>> subscriptionListener,
			std::shared_ptr<joynr::OnChangeSubscriptionQos> subscriptionQos
			«ELSE»
			std::shared_ptr<joynr::ISubscriptionListener<«returnTypes»>> subscriptionListener,
			std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
			const std::vector<std::string>& partitions«IF hTemplate» = std::vector<std::string>()«ENDIF»«
			»«ENDIF»
)
'''

	def produceSubscribeToBroadcastSignature(FBroadcast broadcast, FInterface serviceInterface, boolean generateVersion) {
		return produceSubscribeToBroadcastSignature(broadcast, serviceInterface, false, null, false, generateVersion)
	}

	def produceSubscribeToBroadcastSignature(FBroadcast broadcast, FInterface serviceInterface, boolean hTemplate, boolean generateVersion) {
		return produceSubscribeToBroadcastSignature(broadcast, serviceInterface, false, null, hTemplate, generateVersion)
	}


	def produceSubscribeToBroadcastSignature(FBroadcast broadcast, FInterface serviceInterface, String className, boolean generateVersion) {
		return produceSubscribeToBroadcastSignature(broadcast, serviceInterface, false, className, false, generateVersion)
	}

	def produceUpdateBroadcastSubscriptionSignature(FBroadcast broadcast, FInterface serviceInterface, boolean generateVersion) {
		return produceSubscribeToBroadcastSignature(broadcast, serviceInterface, true, null, false, generateVersion)
	}

	def produceUpdateBroadcastSubscriptionSignature(FBroadcast broadcast, FInterface serviceInterface, boolean hTemplate, boolean generateVersion) {
		return produceSubscribeToBroadcastSignature(broadcast, serviceInterface, true, null, hTemplate, generateVersion)
	}

	def produceUpdateBroadcastSubscriptionSignature(FBroadcast broadcast, FInterface serviceInterface, String className, boolean generateVersion) {
		return produceSubscribeToBroadcastSignature(broadcast, serviceInterface, true, className, false, generateVersion)
	}

	def produceUnsubscribeFromBroadcastSignature(FBroadcast broadcast, String className)
'''
void «IF className !== null»«className»::«ENDIF»unsubscribeFrom«broadcast.joynrName.toFirstUpper»Broadcast(const std::string& subscriptionId)
'''

	def produceUnsubscribeFromBroadcastSignature(FBroadcast broadcast) {
		produceUnsubscribeFromBroadcastSignature(broadcast, null)
	}

	def produceSubscribeUnsubscribeMethodDeclarations(FInterface serviceInterface, boolean pure, boolean generateVersion) {
		produceSubscribeUnsubscribeMethodDeclarations(serviceInterface, pure, false, generateVersion)
	}

	def produceSubscribeUnsubscribeMethodDeclarations(FInterface serviceInterface, boolean pure, boolean hTemplate, boolean generateVersion)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.notifiable]»
		«produceSubscribeToAttributeComments(attribute)»
		«IF pure»virtual «ENDIF»«produceSubscribeToAttributeSignature(attribute, generateVersion)» «IF pure»= 0«ELSE»override«ENDIF»;

		«produceUpdateAttributeSubscriptionComments(attribute)»
		«IF pure»virtual «ENDIF»«produceUpdateAttributeSubscriptionSignature(attribute, generateVersion)» «IF pure»= 0«ELSE»override«ENDIF»;

		«produceUnsubscribeFromAttributeComments(attribute)»
		«IF pure»virtual «ENDIF»«produceUnsubscribeFromAttributeSignature(attribute)» «IF pure»= 0«ELSE»override«ENDIF»;

	«ENDFOR»
	«FOR broadcast: serviceInterface.broadcasts»
		«produceSubscribeToBroadcastComments(broadcast)»
		«IF pure»virtual «ENDIF»«produceSubscribeToBroadcastSignature(broadcast, serviceInterface, hTemplate, generateVersion)» «IF pure»= 0«ELSE»override«ENDIF»;

		«produceUpdateBroadcastSubscriptionComments(broadcast)»
		«IF pure»virtual «ENDIF»«produceUpdateBroadcastSubscriptionSignature(broadcast, serviceInterface, hTemplate, generateVersion)» «IF pure»= 0«ELSE»override«ENDIF»;

		«produceUnsubscribeFromBroadcastComments(broadcast)»
		«IF pure»virtual «ENDIF»«produceUnsubscribeFromBroadcastSignature(broadcast)» «IF pure»= 0«ELSE»override«ENDIF»;

	«ENDFOR»
'''
}
