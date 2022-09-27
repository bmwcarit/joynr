/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_UTILS_MYTESTPROVIDER_H
#define TESTS_UTILS_MYTESTPROVIDER_H

#include "joynr/tests/DefaulttestProvider.h"

// this class exposes the protected members of testAbstractProvider
class MyTestProvider : public joynr::tests::DefaulttestProvider
{
public:
    using testAbstractProvider::fireBroadcastWithByteBufferParameter;
    using testAbstractProvider::fireBroadcastWithEnumOutput;
    using testAbstractProvider::fireBroadcastWithFiltering;
    using testAbstractProvider::fireEmptyBroadcast;
    using testAbstractProvider::fireLocationUpdate;
    using testAbstractProvider::fireLocationUpdateSelective;
    using testAbstractProvider::fireLocationUpdateWithSpeed;
    using testAbstractProvider::locationChanged;
};

#endif // TESTS_UTILS_MYTESTPROVIDER_H
