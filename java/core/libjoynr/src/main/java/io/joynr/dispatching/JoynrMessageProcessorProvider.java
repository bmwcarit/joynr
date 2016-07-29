package io.joynr.dispatching;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.ArrayList;
import java.util.List;

import com.google.inject.Provider;

public class JoynrMessageProcessorProvider implements Provider<List<JoynrMessageProcessor>> {

    private List<JoynrMessageProcessor> joynrMessageProcessors = new ArrayList<>();

    public void addProcessors(JoynrMessageProcessor ... processors) {
        for (JoynrMessageProcessor processor : processors) {
            if (processor != null) {
                joynrMessageProcessors.add(processor);
            }
        }
    }

    @Override
    public List<JoynrMessageProcessor> get() {
        return joynrMessageProcessors;
    }

}
