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
#ifndef ISUBSCRIPTIONLISTENER_H
#define ISUBSCRIPTIONLISTENER_H

namespace joynr {


 /*
  *    Inherit this Interface to create a SubscriptionListener for a datatype.
  */


template<typename T>class ISubscriptionListener
{
public:
    ISubscriptionListener() {}
    virtual ~ISubscriptionListener(){}
    /*
      *     receive will be called on every received publication.
      */
    virtual void receive(T value) = 0;
    /*
      *     publicationMissed will be called when a publication is not received within the time specified in Subscription QoS.
      *     publicationMissed may not block, call any slow methods (like synchronous requests), wait for user actions, or do larger computation.
      */
    virtual void publicationMissed() = 0;
};


} // namespace joynr
#endif // SUBSCRIPTIONLISTENER_H
