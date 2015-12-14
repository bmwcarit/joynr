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
#include "joynr/FormatString.h"
#include <exception>

namespace joynr
{

FormatString::FormatString(const String& input)
        : input(input), output(""), args(), positions(), argCount(0), posCount(0)
{
}

template <typename T>
FormatString FormatString::addArgument(T arg)
{
    if (argCount == maxArguments) {
        return *this;
    }

    try {
        args.at(argCount) = std::to_string(arg);
        ++argCount;
    } catch (...) {
        // do nothing
    }

    return *this;
}

template <>
FormatString FormatString::addArgument<double>(double arg)
{
    if (argCount == maxArguments) {
        return *this;
    }

    try {
        char buffer[maxNumericBufferLength];
        snprintf(buffer, maxNumericBufferLength, "%g", arg);
        args.at(argCount) = buffer;
        ++argCount;
    } catch (...) {
        // do nothing
    }

    return *this;
}

FormatString FormatString::arg(const String& param)
{
    if (argCount == maxArguments) {
        return *this;
    }

    try {
        args.at(argCount) = param;
        ++argCount;
    } catch (...) {
        // do nothing
    }

    return *this;
}

FormatString FormatString::arg(long long param)
{
    return addArgument(param);
}

FormatString FormatString::arg(long param)
{
    return addArgument(param);
}

FormatString FormatString::arg(unsigned long param)
{
    return addArgument(param);
}

FormatString FormatString::arg(unsigned long long param)
{
    return addArgument(param);
}

FormatString FormatString::arg(int param)
{
    return addArgument(param);
}

FormatString FormatString::arg(unsigned int param)
{
    return addArgument(param);
}

FormatString FormatString::arg(short param)
{
    return addArgument(param);
}

FormatString FormatString::arg(unsigned short param)
{
    return addArgument(param);
}

FormatString FormatString::arg(char param)
{
    return addArgument(param);
}

FormatString FormatString::arg(double param)
{
    return addArgument<double>(param);
}

FormatString FormatString::arg(float param)
{
    return addArgument<double>(param);
}

String FormatString::str()
{
    try {
        fillPositionsVector();
        reserveOutputStringSize();
        constructOutputString();
    } catch (...) {
        output.append(warningString);
    }

    return output;
}

void FormatString::fillPositionsVector()
{
    posCount = 0;

    size_t pos = input.find("%", 0);
    while (pos != String::npos) {
        if (posCount == maxPositions) {
            return;
        }

        positions.at(posCount) = pos;
        pos = input.find("%", pos + 1);

        ++posCount;
    };
}

void FormatString::reserveOutputStringSize()
{
    unsigned outputSize = input.size();
    for (auto i : args) {
        outputSize += i.size();
    }

    output.reserve(outputSize);
}

template <class T>
unsigned FormatString::numDigits(T number)
{
    unsigned digits = 0;
    if (number < 0)
        digits = 1; // remove this line if '-' counts as a digit
    while (number) {
        number /= decimalDigits;
        digits++;
    }
    return digits;
}

bool FormatString::checkArgument(unsigned pos, unsigned& argNumber, unsigned& offset)
{
    bool result = false;
    offset = 1; // the "%" sign
    argNumber = 0;

    for (unsigned i = 0; i < numDigits(maxArguments); ++i) {
        if ((pos + i + 1) < input.size()) {
            unsigned number =
                    (int)input.at(pos + i + 1) - ASCIINumbersOffset; // offset for "%" sign

            if (number < decimalDigits) {
                argNumber = argNumber * decimalDigits + number;
                result = true;
                ++offset;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (argNumber != 0) {
        --argNumber;
    }

    return result;
}

void FormatString::constructOutputString()
{
    unsigned currentPos = 0;
    unsigned lastPos = 0;
    unsigned offset = 1; // the "%" sign
    unsigned argNumber = 0;

    // iterate throgh positions, not arguments!
    for (unsigned i = 0; i < posCount; ++i) {
        currentPos = positions.at(i);
        output.append(input.substr(lastPos, currentPos - lastPos));

        if (checkArgument(currentPos, argNumber, offset)) {
            if (argNumber < argCount) {
                output.append(args.at(argNumber));
            }
        } else {
            output.append("%"); // if there is no argument after "%" sign
        }

        lastPos = currentPos + offset;
    }

    // add what is left after the last argument
    output.append(input.substr(lastPos));
}
}
