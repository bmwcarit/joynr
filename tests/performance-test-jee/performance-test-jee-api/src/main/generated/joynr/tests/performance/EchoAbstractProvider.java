/*
 *
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

// #####################################################
//#######################################################
//###                                                 ###
//##    WARNING: This file is generated. DO NOT EDIT   ##
//##             All changes will be lost!             ##
//###                                                 ###
//#######################################################
// #####################################################
package joynr.tests.performance;

import io.joynr.provider.AbstractJoynrProvider;
import java.util.Set;
import java.util.HashSet;
import io.joynr.pubsub.publication.BroadcastFilterImpl;


public abstract class EchoAbstractProvider extends AbstractJoynrProvider implements EchoProvider {

	public EchoAbstractProvider() {
		super();
	}

	private Set<BroadcastFilterImpl> queuedBroadcastFilters = new HashSet<>();

	protected EchoSubscriptionPublisher echoSubscriptionPublisher;

	@Override
	public void setSubscriptionPublisher(EchoSubscriptionPublisher echoSubscriptionPublisher) {
		this.echoSubscriptionPublisher = echoSubscriptionPublisher;
		for (BroadcastFilterImpl filter: queuedBroadcastFilters) {
			this.echoSubscriptionPublisher.addBroadcastFilter(filter);
		}
		queuedBroadcastFilters.clear();
	}

	public void addBroadcastFilter(BroadcastFilterImpl filter) {
		if (this.echoSubscriptionPublisher != null) {
			this.echoSubscriptionPublisher.addBroadcastFilter(filter);
		} else {
			queuedBroadcastFilters.add(filter);
		}
	}
	public void addBroadcastFilter(BroadcastFilterImpl... filters){
		if (this.echoSubscriptionPublisher != null) {
			this.echoSubscriptionPublisher.addBroadcastFilter(filters);
		} else {
			for (BroadcastFilterImpl filter: filters) {
				queuedBroadcastFilters.add(filter);
			}
		}
	}

	public void simpleAttributeChanged(String simpleAttribute) {
		if (echoSubscriptionPublisher != null) {
			echoSubscriptionPublisher.simpleAttributeChanged(simpleAttribute);
		}
	}

	public void fireBroadcastWithSinglePrimitiveParameter(String stringOut, String... partitions) {
		if (echoSubscriptionPublisher != null) {
			echoSubscriptionPublisher.fireBroadcastWithSinglePrimitiveParameter(stringOut, partitions);
		}
	}

}
