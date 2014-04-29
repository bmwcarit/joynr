package io.joynr.capabilities.directory;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

public class CapabilitiesClientDummy implements GlobalCapabilitiesDirectoryClient {

    @Override
    public void add(Callback<Void> callback, List<CapabilityInformation> capabilities) {
        assert (false);

    }

    @Override
    public void add(Callback<Void> callback, CapabilityInformation capability) {
        assert (false);

    }

    @Override
    public Future<List<CapabilityInformation>> lookup(Callback<List<CapabilityInformation>> callback,
                                                      String domain,
                                                      String interfaceName) {
        assert (false);
        return null;
    }

    @Override
    public Future<CapabilityInformation> lookup(Callback<CapabilityInformation> callback, String participantId) {
        assert (false);
        return null;
    }

    @Override
    public void remove(Callback<Void> callback, List<String> capabilities) {
        assert (false);

    }

    @Override
    public void remove(Callback<Void> callback, String capability) {
        assert (false);

    }

    @Override
    public void add(List<CapabilityInformation> capabilities) {
        assert (false);

    }

    @Override
    public void add(CapabilityInformation capability) {
        assert (false);

    }

    @Override
    public List<CapabilityInformation> lookup(String domain, String interfaceName) {
        assert (false);
        return null;
    }

    @Override
    public CapabilityInformation lookup(String participantId) {
        assert (false);
        return null;
    }

    @Override
    public void remove(List<String> capabilities) {
        assert (false);

    }

    @Override
    public void remove(String capability) {
        assert (false);

    }

}
