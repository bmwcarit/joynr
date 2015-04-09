/*
 * #%L
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
#include "joynr/types/GpsLocation.h"
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

TEST(DatatypeTemplateTest, enterAndRetrieveListOfGpsLocationsFromTrip) {

    types::GpsLocation loc1 = types::GpsLocation(1.1, 1.2, 1.3, types::GpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::GpsLocation loc2 = types::GpsLocation(2.1, 2.2, 2.3, types::GpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::GpsLocation> inputList = QList<types::GpsLocation>();
    QList<types::GpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::Trip trip = types::Trip(inputList, "myfirstTryp");
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}


TEST(DatatypeTemplateTest, enterListOfGpsLocationsUsingSetter) {

    types::GpsLocation loc1 = types::GpsLocation(1.1, 1.2, 1.3, types::GpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::GpsLocation loc2 = types::GpsLocation(2.1, 2.2, 2.3, types::GpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
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
    inputList.append(types::Vowel::A);
    inputList.append(types::Vowel::E);
    inputList.append(types::Vowel::E);
    inputList.append(types::Vowel::U);
    types::Word myword = types::Word(inputList);
    EXPECT_EQ( myword.getVowels().value(1), types::Vowel::E);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumListviaInternalSetter) {

    QList<QVariant> inputSerializedList = QList<QVariant>();
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("A")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("U")));
    types::Word myWord = types::Word();
    myWord.setVowelsInternal(inputSerializedList);
    EXPECT_EQ( myWord.getVowels().value(1), types::Vowel::E);

    QList<types::Vowel::Enum> inputList = QList<types::Vowel::Enum>();
    inputList.append(types::Vowel::A);
    inputList.append(types::Vowel::E);
    inputList.append(types::Vowel::E);
    inputList.append(types::Vowel::U);
    types::Word myVowelWord =types::Word(inputList);

    EXPECT_EQ( myVowelWord, myWord);
}
