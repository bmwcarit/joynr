/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
const InProcessMessagingStub = require("./InProcessMessagingStub");

/**
 * @constructor
 * @name InProcessMessagingStubFactory
 */
class InProcessMessagingStubFactory {
    /**
     * @name InProcessMessagingStubFactory#build
     * @function
     *
     * @param {InProcessAddress} address the address to generate a messaging stub for
     */
    build(address) {
        return new InProcessMessagingStub(address.getSkeleton());
    }
}

module.exports = InProcessMessagingStubFactory;
