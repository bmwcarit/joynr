/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#ifndef TESTS_MOCKIUDSSENDER_H
#define TESTS_MOCKIUDSSENDER_H

#include "tests/utils/Gmock.h"

#include "joynr/IUdsSender.h"

class MockIUdsSender : public joynr::IUdsSender {
public:
    MOCK_METHOD0(dtorCalled, void());
    ~MockIUdsSender() override
    {
        dtorCalled();
    }

    MOCK_METHOD2(send, void (const smrf::ByteArrayView& message,
                             const SendFailed& onFailure));
};

#endif // TESTS_MOCKIUDSSENDER_H
