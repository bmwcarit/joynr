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

import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatcher.rpc.JoynrBroadcastSubscriptionInterface;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.MulticastSubscriptionQos;


public interface EchoBroadcastInterface extends JoynrBroadcastSubscriptionInterface, Echo {


public interface BroadcastWithSinglePrimitiveParameterBroadcastListener extends BroadcastSubscriptionListener {
	public void onReceive(String stringOut);
}

public class BroadcastWithSinglePrimitiveParameterBroadcastAdapter implements BroadcastWithSinglePrimitiveParameterBroadcastListener {
	public void onReceive(String stringOut) {
		// empty implementation
	}
	public void onError(SubscriptionException error) {
		// empty implementation
	}
	public void onSubscribed(String subscriptionId) {
		// empty implementation
	}
}

@JoynrMulticast(name = "broadcastWithSinglePrimitiveParameter")
abstract Future<String> subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(
		BroadcastWithSinglePrimitiveParameterBroadcastListener subscriptionListener,
		MulticastSubscriptionQos subscriptionQos,
		String... partitions);

@JoynrMulticast(name = "broadcastWithSinglePrimitiveParameter")
abstract Future<String> subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(
		String subscriptionId,
		BroadcastWithSinglePrimitiveParameterBroadcastListener subscriptionListener,
		MulticastSubscriptionQos subscriptionQos,
		String... partitions);

abstract void unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(String subscriptionId);
}
