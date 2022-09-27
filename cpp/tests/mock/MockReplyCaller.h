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
#ifndef TESTS_MOCK_MOCKREPLYCALLER_H
#define TESTS_MOCK_MOCKREPLYCALLER_H

#include "tests/utils/Gmock.h"

#include "joynr/ReplyCaller.h"

template <typename T>
class MockReplyCaller : public joynr::ReplyCaller<T>
{
public:
    MockReplyCaller(
            std::function<void(const T& returnValue)> callbackFct,
            std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>& error)>
                    errorFct)
            : joynr::ReplyCaller<T>(callbackFct, errorFct)
    {
    }
    MOCK_METHOD1_T(returnValue, void(const T& payload));
    MOCK_METHOD0_T(timeOut, void());
    MOCK_CONST_METHOD0_T(getType, std::string());
};

#endif // TESTS_MOCK_MOCKREPLYCALLER_H
