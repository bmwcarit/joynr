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
#ifndef MOCKTASKSEQUENCER_H
#define MOCKTASKSEQUENCER_H

#include "tests/utils/Gmock.h"

#include "joynr/TaskSequencer.h"

template<typename T>
class MockTaskSequencer : public joynr::TaskSequencer<T>
{
public:
    using MockTaskWithExpiryDate = typename TaskSequencer<T>::TaskWithExpiryDate;

    MockTaskSequencer(std::chrono::milliseconds defaultTimeToWait) : TaskSequencer<T>(defaultTimeToWait)
    {
        // Do nothing
    }

    MOCK_METHOD0(cancel, void());
    MOCK_METHOD1_T(add, void(const MockTaskWithExpiryDate& taskWithExpiryDate));
};


#endif // MOCKTASKSEQUENCER_H
