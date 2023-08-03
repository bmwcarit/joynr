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

import io.joynr.examples.spring.service.ConsumerService;
import joynr.vehicle.RadioStation;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;

@Controller
@RequestMapping("consumer")
public class ConsumerController {
    @Autowired
    private ConsumerService service;

    @RequestMapping(value = "/current-station", method = RequestMethod.GET)
    public @ResponseBody RadioStation getCurrentStation() {
        return service.getConsumer().getCurrentRadioStation();
    }

    @RequestMapping(value = "/shuffle-stations", method = RequestMethod.POST)
    public @ResponseBody RadioStation shuffleRadioStations() {
        return service.getConsumer().shuffleStations();
    }
}
