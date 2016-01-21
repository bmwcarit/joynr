/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef FORMATSTRING_H
#define FORMATSTRING_H

#include <string>
#include <array>

namespace joynr
{

typedef std::string String;

/**
 * FormatString is a helper class used to partly emulate QString behavior
 * Usage example:
 * std::string test = FormatString("Param %1, Param %2, Param %3,
*End").arg("One").arg(2).arg(3.0).str()
 * Output: Param One, Param 2, Param 3.0, End
**/
class FormatString
{
    static constexpr const int maxArguments = 10;
    static constexpr const int maxPositions = 100;
    static constexpr const int maxNumericBufferLength = 50;
    static constexpr const char* warningString = " WARNING: FALSE STRING!";
    static constexpr const unsigned ASCIINumbersOffset = 48;
    static constexpr const unsigned decimalDigits = 10;

public:
    // Disable construction & copy of the class, only use with arg() and str() functions
    FormatString() = delete;
    FormatString& operator=(const FormatString&) = delete;

    explicit FormatString(const String& input);

    FormatString arg(long long param);
    FormatString arg(long param);
    FormatString arg(unsigned long param);
    FormatString arg(int param);
    FormatString arg(unsigned int param);
    FormatString arg(unsigned long long param);
    FormatString arg(short param);
    FormatString arg(unsigned short param);
    FormatString arg(char param);
    FormatString arg(double param);
    FormatString arg(float param);
    FormatString arg(const String& param);

    String str();

private:
    String input;
    String output;

    std::array<String, maxArguments> args;
    std::array<unsigned, maxPositions> positions;

    unsigned argCount;
    unsigned posCount;

    void fillPositionsVector();
    void reserveOutputStringSize();
    void constructOutputString();
    bool checkArgument(unsigned pos, unsigned& argNumber, unsigned& offset);

    template <typename T>
    FormatString addArgument(T arg);

    template <class T>
    unsigned numDigits(T number);
};
} // namespace joynr
#endif // FORMATSTRING_H
