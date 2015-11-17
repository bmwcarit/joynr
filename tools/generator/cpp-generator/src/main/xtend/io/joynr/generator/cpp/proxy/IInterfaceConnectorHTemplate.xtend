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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.DatatypeSystemTransformation
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class IInterfaceConnectorHTemplate implements InterfaceTemplate{
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject private CppStdTypeUtil cppStdTypeUtil;
	@Inject private extension QtTypeUtil qtTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension BroadcastUtil
	@Inject private extension InterfaceUtil

	@Inject extension InterfaceSubscriptionUtil
	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_I"+interfaceName+"Connector_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
«FOR parameterType: cppStdTypeUtil.getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»
«FOR parameterType: qtTypeUtil.getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/IConnector.h"
#include "joynr/TypeUtil.h"
#include <memory>

namespace joynr {
	template <class ... Ts> class ISubscriptionListener;
	class ISubscriptionCallback;
	class SubscriptionQos;
	class OnChangeSubscriptionQos;
}

«getNamespaceStarter(serviceInterface)»
class «getDllExportMacro()» I«interfaceName»Subscription{
	/**
	  * in  - subscriptionListener      std::shared_ptr to a SubscriptionListener which will receive the updates.
	  * in  - subscriptionQos           SubscriptionQos parameters like interval and end date.
	  * out - assignedSubscriptionId    Buffer for the assigned subscriptionId.
	  */
public:
	virtual ~I«interfaceName»Subscription() {}

	«produceSubscribeUnsubscribeMethods(serviceInterface, true)»
};

class «getDllExportMacro()» I«interfaceName»Connector: virtual public I«interfaceName», public joynr::IConnector, virtual public I«interfaceName»Subscription{

public:
	virtual ~I«interfaceName»Connector() {}
protected:
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.notifiable]»
		«val returnType = cppStdTypeUtil.getTypeName(attribute)»
		«val returnTypeQT = qtTypeUtil.getTypeName(attribute)»
		«IF needsDatatypeConversion(attribute)»
			class «attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper : public ISubscriptionListener<«returnTypeQT»> {
				public:
					«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper(
							std::shared_ptr<ISubscriptionListener<«returnType»>> wrappedListener
					) :
							wrappedListener(wrappedListener)
					{
					}
					void onReceive(const «returnTypeQT»& receivedValue) {
						wrappedListener->onReceive(«fromQTTypeToStdType(attribute, "receivedValue")»);
					}
					void onError(const exceptions::JoynrRuntimeException& error) {
						wrappedListener->onError(error);
					}

				private:
					std::shared_ptr<ISubscriptionListener<«returnType»>> wrappedListener;
			};

			class «attribute.joynrName.toFirstUpper»AttributeSubscriptionCallbackWrapper : public SubscriptionCallback<«returnTypeQT»> {
			public:
				«attribute.joynrName.toFirstUpper»AttributeSubscriptionCallbackWrapper(
						std::shared_ptr<ISubscriptionListener<«returnType»>> wrappedListener
				) :
						SubscriptionCallback(
								std::shared_ptr<ISubscriptionListener<«returnTypeQT»>>(
										new «attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper(wrappedListener))
								)
				{
				}
				virtual void onSuccess(const «returnTypeQT»& receivedValue) {
					std::shared_ptr<«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper> wrapper =
						std::dynamic_pointer_cast<
								«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper>(listener);
					wrapper->onReceive(receivedValue);
				}
				virtual void onError(const exceptions::JoynrRuntimeException& error) {
					std::shared_ptr<«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper> wrapper =
						std::dynamic_pointer_cast<
								«attribute.joynrName.toFirstUpper»AttributeSubscriptionListenerWrapper>(listener);
					wrapper->onError(error);
				}
			};
		«ENDIF»
	«ENDFOR»
	«FOR broadcast: serviceInterface.broadcasts»
		«IF needsDatatypeConversion(broadcast)»
			«val returnTypesQT = qtTypeUtil.getCommaSeparatedOutputParameterTypes(broadcast)»
			«val returnTypesStd = cppStdTypeUtil.getCommaSeparatedOutputParameterTypes(broadcast)»
			«val outputParametersSignature = qtTypeUtil.getCommaSeperatedTypedConstOutputParameterList(broadcast)»
			class «broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper : public ISubscriptionListener<«returnTypesQT»> {
				public:
					«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper(
							std::shared_ptr<ISubscriptionListener<«returnTypesStd»>> wrappedListener
					) :
							wrappedListener(wrappedListener)
					{
					}
					void onReceive(
							«outputParametersSignature»
					) {
						wrappedListener->onReceive(«qtTypeUtil.getCommaSeperatedUntypedOutputParameterList(broadcast, DatatypeSystemTransformation.FROM_QT_TO_STANDARD)»);
					}
					void onError(const exceptions::JoynrRuntimeException& error) {
						wrappedListener->onError(error);
					}

				private:
					std::shared_ptr<ISubscriptionListener<«returnTypesStd»>> wrappedListener;
			};

			class «broadcast.joynrName.toFirstUpper»BroadcastSubscriptionCallbackWrapper
					: public SubscriptionCallback<«returnTypesQT»>
			{
			public:
				«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionCallbackWrapper(
						std::shared_ptr<ISubscriptionListener<
								«returnTypesStd»
						>> wrappedListener
				) :
						SubscriptionCallback(
								std::shared_ptr<ISubscriptionListener<
										«returnTypesQT»
								>>(new «broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper(wrappedListener)))
				{
				}
				virtual void onSuccess(
						«outputParametersSignature.substring(1)»
				) {
					std::shared_ptr<«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper> wrapper =
						std::dynamic_pointer_cast<
									«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper>(listener);
					wrapper->onReceive(«cppStdTypeUtil.getCommaSeperatedUntypedParameterList(broadcast.outputParameters)»
					);
				}
				virtual void onError(const exceptions::JoynrRuntimeException& error)
				{
					std::shared_ptr<«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper> wrapper =
						std::dynamic_pointer_cast<
									«broadcast.joynrName.toFirstUpper»BroadcastSubscriptionListenerWrapper>(listener);
					wrapper->onError(error);
				}
			};

		«ENDIF»
	«ENDFOR»
};

«getNamespaceEnder(serviceInterface)»
#endif // «headerGuard»
'''
}
