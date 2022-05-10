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
#ifndef TESTS_MOCK_MOCKLOCATIONUPDATEDSELECTIVEFILTER_H
#define TESTS_MOCK_MOCKLOCATIONUPDATEDSELECTIVEFILTER_H

#include "tests/utils/Gmock.h"

#include "joynr/tests/TestLocationUpdateSelectiveBroadcastFilter.h"

class MockLocationUpdatedSelectiveFilter : public joynr::tests::TestLocationUpdateSelectiveBroadcastFilter {
public:
    MOCK_METHOD2(filter,
                 bool(
                     const joynr::types::Localisation::GpsLocation &location,
                     const joynr::tests::TestLocationUpdateSelectiveBroadcastFilterParameters &filterParameters));
};

#endif // TESTS_MOCK_MOCKLOCATIONUPDATEDSELECTIVEFILTER_H
