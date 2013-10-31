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

import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import joynr.types.CapabilityInformation;

public class CapabilitiesEntryModel {
    private final String title;
    private final String heading;
    private final ConcurrentHashMap<String, List<CapabilityInformation>> entries;

    public CapabilitiesEntryModel(String title,
                                  String heading,
                                  ConcurrentHashMap<String, List<CapabilityInformation>> entries) {
        this.title = title;
        this.heading = heading;
        this.entries = entries;
    }

    public String getTitle() {
        return title;
    }

    public String getHeading() {
        return heading;
    }

    public Map<String, List<CapabilityInformation>> getEntries() {
        return entries;
    }
}
