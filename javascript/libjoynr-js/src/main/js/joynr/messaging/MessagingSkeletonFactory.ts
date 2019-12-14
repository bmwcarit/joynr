/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import * as UtilInternal from "../util/UtilInternal";
import LoggingManager from "../system/LoggingManager";

const log = LoggingManager.getLogger("joynr/messaging/MessagingSkeletonFactory");
type Address = any;
type MessagingSkeleton = any;

class MessagingSkeletonFactory {
    private messagingSkeletons!: Record<string, MessagingSkeleton>;
    /**
     * @constructor
     */
    public constructor() {}

    public setSkeletons(newMessagingSkeletons: Record<string, MessagingSkeleton>): void {
        this.messagingSkeletons = newMessagingSkeletons;
    }

    /**
     * @returnr the skeleton matching the address
     */
    public getSkeleton(address: Address): MessagingSkeleton {
        const className = address._typeName;
        const skeleton = this.messagingSkeletons[className];

        if (UtilInternal.checkNullUndefined(skeleton)) {
            const errorMsg = `Could not find a messaging skeleton for "${className}" within messagingSkeletons [${Object.keys(
                this.messagingSkeletons
            ).join(",")}]`;
            log.debug(errorMsg);
        }
        return skeleton;
    }
}

export = MessagingSkeletonFactory;
