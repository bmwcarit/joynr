package io.joynr.capabilities.directory;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
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
 * #L%
 */

import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.proxy.Future;

import java.util.List;

import joynr.types.CapabilityInformation;
import joynr.types.ProviderQosRequirements;

public class CapabilitiesClientDummy implements GlobalCapabilitiesDirectoryClient {

    @Override
    public void registerCapabilities(Callback<Void> callback, List<CapabilityInformation> capabilities) {
        assert (false);

    }

    @Override
    public void registerCapability(Callback<Void> callback, CapabilityInformation capability) {
        assert (false);

    }

    @Override
    public Future<List<CapabilityInformation>> lookupCapabilities(Callback<List<CapabilityInformation>> callback,
                                                                  String domain,
                                                                  String interfaceName,
                                                                  ProviderQosRequirements qos) {
        assert (false);
        return null;
    }

    @Override
    public Future<List<CapabilityInformation>> getCapabilitiesForChannelId(Callback<List<CapabilityInformation>> callback,
                                                                           String channelId) {
        assert (false);
        return null;
    }

    @Override
    public Future<List<CapabilityInformation>> getCapabilitiesForParticipantId(Callback<List<CapabilityInformation>> callback,
                                                                               String participantId) {
        assert (false);
        return null;
    }

    @Override
    public void unregisterCapabilities(Callback<Void> callback, List<CapabilityInformation> capabilities) {
        assert (false);

    }

    @Override
    public void unregisterCapability(Callback<Void> callback, CapabilityInformation capability) {
        assert (false);

    }

    @Override
    public void registerCapabilities(List<CapabilityInformation> capabilities) {
        assert (false);

    }

    @Override
    public void registerCapability(CapabilityInformation capability) {
        assert (false);

    }

    @Override
    public List<CapabilityInformation> lookupCapabilities(String domain,
                                                          String interfaceName,
                                                          ProviderQosRequirements qos) {
        assert (false);
        return null;
    }

    @Override
    public List<CapabilityInformation> getCapabilitiesForChannelId(String channelId) {
        assert (false);
        return null;
    }

    @Override
    public List<CapabilityInformation> getCapabilitiesForParticipantId(String participantId) {
        assert (false);
        return null;
    }

    @Override
    public void unregisterCapabilities(List<CapabilityInformation> capabilities) {
        assert (false);

    }

    @Override
    public void unregisterCapability(CapabilityInformation capability) {
        assert (false);

    }

}
