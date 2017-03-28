package io.joynr.dispatching.subscription;

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

public class PubSubState {

    boolean interrupted;
    boolean stopped;
    long timeOfLastPublication;

    public PubSubState(boolean interrupted, boolean stopped, long timeOfLastPublication) {
        super();
        this.interrupted = interrupted;
        this.stopped = stopped;
        this.timeOfLastPublication = timeOfLastPublication;
    }

    public PubSubState() {
        this(false, false, 0);
    }

    public boolean isInterrupted() {
        return interrupted;
    }

    public void setInterrupted(boolean interrupted) {
        this.interrupted = interrupted;
    }

    public void interrupt() {
        this.interrupted = true;
    }

    public boolean isStopped() {
        return stopped;
    }

    public void setStopped(boolean stopped) {
        this.stopped = stopped;
    }

    public void stop() {
        this.stopped = true;
    }

    public long getTimeOfLastPublication() {
        return timeOfLastPublication;
    }

    public void updateTimeOfLastPublication() {
        this.timeOfLastPublication = System.currentTimeMillis();
    }

}
