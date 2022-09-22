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
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase

class InterfaceAbstractProviderTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil
	@Inject extension AttributeUtil
	@Inject extension TemplateBase

	override generate(boolean generateVersion) {
		val interfaceName =  francaIntf.joynrName
		val className = interfaceName + "AbstractProvider"
		val providerInterfaceName = interfaceName + "Provider"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".", generateVersion)

		'''
«warning()»
package «packagePath»;

import io.joynr.provider.AbstractJoynrProvider;
«IF !francaIntf.broadcasts.empty»
import java.util.Set;
import java.util.HashSet;
import io.joynr.pubsub.publication.BroadcastFilterImpl;
«ENDIF»

«FOR datatype : getRequiredIncludesFor(francaIntf, false, false, false, true, true, false, generateVersion)»
	import «datatype»;
«ENDFOR»

public abstract class «className» extends AbstractJoynrProvider implements «providerInterfaceName» {

	public «className»() {
		super();
	}

	«IF francaIntf.hasNotifiableAttribute || !francaIntf.broadcasts.empty»
		«IF !francaIntf.broadcasts.empty»
			private Set<BroadcastFilterImpl> queuedBroadcastFilters = new HashSet<>();
		«ENDIF»

		protected «interfaceName»SubscriptionPublisher «interfaceName.toFirstLower»SubscriptionPublisher;

		@Override
		public void setSubscriptionPublisher(«interfaceName»SubscriptionPublisher «interfaceName.toFirstLower»SubscriptionPublisher) {
			this.«interfaceName.toFirstLower»SubscriptionPublisher = «interfaceName.toFirstLower»SubscriptionPublisher;
			«IF !francaIntf.broadcasts.empty»
				for (BroadcastFilterImpl filter: queuedBroadcastFilters) {
					this.«interfaceName.toFirstLower»SubscriptionPublisher.addBroadcastFilter(filter);
				}
				queuedBroadcastFilters.clear();
			«ENDIF»
		}

		«IF !francaIntf.broadcasts.empty»
			public void addBroadcastFilter(BroadcastFilterImpl filter) {
				if (this.«interfaceName.toFirstLower»SubscriptionPublisher != null) {
					this.«interfaceName.toFirstLower»SubscriptionPublisher.addBroadcastFilter(filter);
				} else {
					queuedBroadcastFilters.add(filter);
				}
			}
			public void addBroadcastFilter(BroadcastFilterImpl... filters) {
				if (this.«interfaceName.toFirstLower»SubscriptionPublisher != null) {
					this.«interfaceName.toFirstLower»SubscriptionPublisher.addBroadcastFilter(filters);
				} else {
					for (BroadcastFilterImpl filter: filters) {
						queuedBroadcastFilters.add(filter);
					}
				}
			}
		«ENDIF»
	«ENDIF»

	«FOR attribute : getAttributes(francaIntf).filter(a | a.isNotifiable)»
		«val attributeName = attribute.joynrName»
		«val attributeType = attribute.typeName»
		public void «attributeName»Changed(«attributeType» «attributeName») {
			if («interfaceName.toFirstLower»SubscriptionPublisher != null) {
				«interfaceName.toFirstLower»SubscriptionPublisher.«attributeName»Changed(«attributeName»);
			}
		}
	«ENDFOR»

	«FOR broadcast : francaIntf.broadcasts.filter[selective]»
		«var broadcastName = broadcast.joynrName»
		public void fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedTypedOutputParameterList») {
			if («interfaceName.toFirstLower»SubscriptionPublisher != null) {
				«interfaceName.toFirstLower»SubscriptionPublisher.fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedUntypedOutputParameterList»);
			}
		}

	«ENDFOR»
	«FOR broadcast : francaIntf.broadcasts.filter[!selective]»
		«var broadcastName = broadcast.joynrName»
		public void fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedTypedOutputParameterList»«IF broadcast.outputParameters.length > 0», «ENDIF»String... partitions) {
			if («interfaceName.toFirstLower»SubscriptionPublisher != null) {
				«interfaceName.toFirstLower»SubscriptionPublisher.fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedUntypedOutputParameterList»«IF broadcast.outputParameters.length > 0», «ENDIF»partitions);
			}
		}

	«ENDFOR»
}
		'''
	}
}
