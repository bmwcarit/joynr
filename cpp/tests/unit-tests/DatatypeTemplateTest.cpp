/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "gtest/gtest.h"
using namespace ::testing;
#include "joynr/types/Trip.h"
#include "joynr/types/Location.h"
#include "joynr/types/Word.h"
#include "joynr/types/Vowel.h"

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

TEST(DatatypeTemplateTest, enterAndRetrieveListOfLocationsFromTrip) {

    types::Location loc1 = types::Location(types::GpsFixEnum::ModeNoFix, 1.1, 2.2, 3.3, 0.0, 0.0, 0.0, 0, 0, 1982, "jambit", "germany", "Munich", "Erika-Mann-Str", "63", "80636", "jambit");
    types::Location loc2 = types::Location(types::GpsFixEnum::Mode2D, 4.4, 5.5, 6.6, 0.0, 0.0, 0.0, 0, 0, 1982, "Carit", "germany", "Munich", "Petuelring", "116", "80809", "carit");
    QList<types::GpsLocation> inputList = QList<types::GpsLocation>();
    QList<types::GpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::Trip trip = types::Trip(inputList, "myfirstTryp");
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}


TEST(DatatypeTemplateTest, enterListOfLocationsUsingSetter) {

    types::Location loc1 = types::Location(types::GpsFixEnum::ModeNoFix, 1.1, 2.2, 3.3, 0.0, 0.0, 0.0, 0, 0, 1982, "jambit", "germany", "Munich", "Erika-Mann-Str", "63", "80636", "jambit");
    types::Location loc2 = types::Location(types::GpsFixEnum::Mode2D, 4.4, 5.5, 6.6, 0.0, 0.0, 0.0, 0, 0, 1982, "Carit", "germany", "Munich", "Petuelring", "116", "80809", "carit");
    QList<types::GpsLocation> inputList = QList<types::GpsLocation>();
    QList<types::GpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::Trip trip = types::Trip();
    trip.setLocations(inputList);
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumList) {

    QList<types::Vowel::Enum> inputList = QList<types::Vowel::Enum>();
    inputList.append(types::Vowel::a);
    inputList.append(types::Vowel::e);
    inputList.append(types::Vowel::e);
    inputList.append(types::Vowel::u);
    types::Word myword = types::Word(inputList);
    EXPECT_EQ( myword.getVowels().value(1), types::Vowel::e);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumListviaInternalSetter) {

    QList<QVariant> inputIntList = QList<QVariant>();
    inputIntList.append(QVariant::fromValue(0)); // a == 0
    inputIntList.append(QVariant::fromValue(1)); // e == 1
    inputIntList.append(QVariant::fromValue(1)); // e == 1
    inputIntList.append(QVariant::fromValue(4)); // u == 4
    types::Word myIntWord = types::Word();
    myIntWord.setVowelsInternal(inputIntList);
    EXPECT_EQ( myIntWord.getVowels().value(1), types::Vowel::e); // e == 1

    QList<types::Vowel::Enum> inputList = QList<types::Vowel::Enum>();
    inputList.append(types::Vowel::a);
    inputList.append(types::Vowel::e);
    inputList.append(types::Vowel::e);
    inputList.append(types::Vowel::u);
    types::Word myVowelWord =types::Word(inputList);

    EXPECT_EQ( myVowelWord, myIntWord);
}
