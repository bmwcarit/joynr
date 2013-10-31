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
/**
  * Util class to let tests sleep for a given time in milliseconds.
  * http://stackoverflow.com/questions/3831439/qthow-to-give-a-delay-in-loop-execution/3831460#3831460
  */
#ifndef QTHREADSLEEP_H
#define QTHREADSLEEP_H
#include <QThread>

class QThreadSleep : public QThread {
public:
    static void msleep(unsigned long msecs){
        QThread::msleep(msecs);
    }
};

#endif //QTHREADSLEEP_H
