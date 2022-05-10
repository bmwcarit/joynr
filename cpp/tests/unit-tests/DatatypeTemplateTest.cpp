/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "tests/utils/Gtest.h"
using namespace ::testing;
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/TestTypes/Word.h"
#include "joynr/types/TestTypes/Vowel.h"

using namespace joynr;

//    Those test-enums and structs must be part of IDL for this test to compile:

//    struct Trip {
//      tripId : string
//      locations : Location[]
//      tripTitle : string
//    }
//    enum Vowel {
//         a, e, i, o, u
//    }
//    struct Word {
//        vowels : Vowel[]
//    }

TEST(DatatypeTemplateTest, enterAndRetrieveListOfGpsLocationsFromTrip)
{

    types::Localisation::GpsLocation loc1 =
            types::Localisation::GpsLocation(1.1,
                                             1.2,
                                             1.3,
                                             types::Localisation::GpsFixEnum::MODE2D,
                                             1.4,
                                             1.5,
                                             1.6,
                                             1.7,
                                             18,
                                             19,
                                             110);
    types::Localisation::GpsLocation loc2 =
            types::Localisation::GpsLocation(2.1,
                                             2.2,
                                             2.3,
                                             types::Localisation::GpsFixEnum::MODE2D,
                                             2.4,
                                             2.5,
                                             2.6,
                                             2.7,
                                             28,
                                             29,
                                             210);
    std::vector<types::Localisation::GpsLocation> inputList =
            std::vector<types::Localisation::GpsLocation>();
    std::vector<types::Localisation::GpsLocation> returnList;
    inputList.push_back(loc1);
    inputList.push_back(loc2);
    types::Localisation::Trip trip = types::Localisation::Trip(inputList, "myfirstTryp");
    returnList = trip.getLocations();
    EXPECT_EQ(returnList, inputList);
    EXPECT_EQ(returnList.at(1), loc2);
}

TEST(DatatypeTemplateTest, enterListOfGpsLocationsUsingSetter)
{

    types::Localisation::GpsLocation loc1 =
            types::Localisation::GpsLocation(1.1,
                                             1.2,
                                             1.3,
                                             types::Localisation::GpsFixEnum::MODE2D,
                                             1.4,
                                             1.5,
                                             1.6,
                                             1.7,
                                             18,
                                             19,
                                             110);
    types::Localisation::GpsLocation loc2 =
            types::Localisation::GpsLocation(2.1,
                                             2.2,
                                             2.3,
                                             types::Localisation::GpsFixEnum::MODE2D,
                                             2.4,
                                             2.5,
                                             2.6,
                                             2.7,
                                             28,
                                             29,
                                             210);
    std::vector<types::Localisation::GpsLocation> inputList =
            std::vector<types::Localisation::GpsLocation>();
    std::vector<types::Localisation::GpsLocation> returnList;
    inputList.push_back(loc1);
    inputList.push_back(loc2);
    types::Localisation::Trip trip = types::Localisation::Trip();
    trip.setLocations(inputList);
    returnList = trip.getLocations();
    EXPECT_EQ(returnList, inputList);
    EXPECT_EQ(returnList.at(1), loc2);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumList)
{

    std::vector<types::TestTypes::Vowel::Enum> inputList =
            std::vector<types::TestTypes::Vowel::Enum>();
    inputList.push_back(types::TestTypes::Vowel::A);
    inputList.push_back(types::TestTypes::Vowel::E);
    inputList.push_back(types::TestTypes::Vowel::E);
    inputList.push_back(types::TestTypes::Vowel::U);
    types::TestTypes::Word myword = types::TestTypes::Word(inputList);
    EXPECT_EQ(myword.getVowels().at(1), types::TestTypes::Vowel::E);
}
