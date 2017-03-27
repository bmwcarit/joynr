package io.joynr.messaging;

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

import java.util.Timer;
import java.util.TimerTask;

/**
 * Delayed task execution used in HttpCommunicationManager to repeat a channel creation.
 * 
 */

public abstract class TimedTask {

    Timer timer;

    public TimedTask(long milliSecondsBeforePerformingTask) {
        timer = new Timer();
        timer.schedule(new Task(), milliSecondsBeforePerformingTask);
    }

    public abstract void performTask();

    public class Task extends TimerTask {

        @Override
        public void run() {
            performTask();
            timer.cancel();
        }

    }
}
