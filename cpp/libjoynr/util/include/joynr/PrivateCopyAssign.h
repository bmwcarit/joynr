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
#ifndef PRIVATECOPYASSIGN_H
#define PRIVATECOPYASSIGN_H

//------------------------------------------------------------------------------
// This header file should be included manually in every Joynr source file that
// requires it.
//
// This defines a macro that deletes copy constructor and assignment operator.
//------------------------------------------------------------------------------

#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                         \
    TypeName(const TypeName&) = delete;                                                            \
    TypeName& operator=(const TypeName&) = delete

#define DISALLOW_MOVE_AND_ASSIGN(TypeName)                                                         \
    TypeName(TypeName&&) = delete;                                                                 \
    TypeName& operator=(TypeName&&) = delete

#endif // PRIVATECOPYASSIGN_H
