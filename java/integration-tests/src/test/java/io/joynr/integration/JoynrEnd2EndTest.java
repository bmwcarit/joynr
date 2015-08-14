package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import joynr.infrastructure.ChannelUrlDirectoryProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import joynr.infrastructure.dactypes.MasterAccessControlEntry;
import joynr.infrastructure.dactypes.Permission;
import joynr.infrastructure.dactypes.TrustLevel;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;

public class JoynrEnd2EndTest {

    protected static void provisionPermissiveAccessControlEntry(String domain, String interfaceName) throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        List<MasterAccessControlEntry> provisionedAccessControlEntries = new ArrayList<MasterAccessControlEntry>();
        String existingAccessControlEntriesJson = System.getProperty(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES);
        if (existingAccessControlEntriesJson != null) {
            provisionedAccessControlEntries.addAll(Arrays.asList(objectMapper.readValue(existingAccessControlEntriesJson,
                                                                                        MasterAccessControlEntry[].class)));
        }

        MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry("*",
                                                                                            domain,
                                                                                            interfaceName,
                                                                                            TrustLevel.LOW,
                                                                                            Arrays.asList(TrustLevel.LOW),
                                                                                            TrustLevel.LOW,
                                                                                            Arrays.asList(TrustLevel.LOW),
                                                                                            "*",
                                                                                            Permission.YES,
                                                                                            Arrays.asList(Permission.YES));

        provisionedAccessControlEntries.add(newMasterAccessControlEntry);
        String provisionedAccessControlEntriesAsJson = objectMapper.writeValueAsString(provisionedAccessControlEntries.toArray());
        System.setProperty(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES,
                           provisionedAccessControlEntriesAsJson);
    }

    protected static void provisionDiscoveryDirectoryAccessControlEntries() throws Exception {
        provisionPermissiveAccessControlEntry("io.joynr", GlobalCapabilitiesDirectoryProvider.INTERFACE_NAME);
        provisionPermissiveAccessControlEntry("io.joynr", ChannelUrlDirectoryProvider.INTERFACE_NAME);
    }
}
