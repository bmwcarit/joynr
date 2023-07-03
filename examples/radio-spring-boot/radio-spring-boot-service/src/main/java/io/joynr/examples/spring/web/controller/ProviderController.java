/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.examples.spring.web.controller;

import io.joynr.examples.spring.service.ProviderService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;

@Controller
@RequestMapping("provider")
public class ProviderController {
    @Autowired
    private ProviderService service;

    @RequestMapping(value = "/shuffle-stations", method = RequestMethod.POST)
    public @ResponseBody void shuffleRadioStationsFromProvider() {
        service.getProvider().shuffleStations();
    }

    @RequestMapping(value = "/metrics", method = RequestMethod.GET)
    public @ResponseBody String getMetrics() {
        return String.format(
                "Provider statistics: \n Get current radio station invocation count: %d \n Shuffle radio stations invocation count: %d\n",
                service.getProvider().getCurrentRadioStationInvocationCount(),
                service.getProvider().getShuffleRadioStationsInvocationCount());
    }
}
