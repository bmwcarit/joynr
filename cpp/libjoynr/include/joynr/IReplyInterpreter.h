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
#ifndef IREPLYINTERPRETER_H
#define IREPLYINTERPRETER_H

#include "joynr/IReplyCaller.h"
#include <QVariant>

#include <memory>

namespace joynr
{

class Reply;

class IReplyInterpreter
{
public:
    virtual ~IReplyInterpreter()
    {
    }
    // The ReplyCaller is the instance that calls the actual receiver of the reply.
    virtual void execute(std::shared_ptr<IReplyCaller> replyCaller, const Reply& reply) = 0;
};

} // namespace joynr
#endif // IREPLYINTERPRETER_H
