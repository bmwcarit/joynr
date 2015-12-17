package io.joynr.examples.android_example;

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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import android.os.AsyncTask;
import joynr.types.Localisation.GpsLocation;
import joynr.vehicle.GpsProxy;

class GetGpsTask extends AsyncTask<GpsProxy, Void, GpsLocation> {
    private static final Logger logger = LoggerFactory.getLogger(GetGpsTask.class);

    private Exception exception;

    @Override
    protected GpsLocation doInBackground(GpsProxy... params) {
        try {
            return params[0].getLocation();
        } catch (Exception e) {
            this.exception = e;
        }
        return null;
    }

    @Override
    protected void onPostExecute(GpsLocation location) {
        if (exception != null) {
            logger.error("unable to retrieve Gps Location: {}", exception.getMessage());
            logger.debug("unable to retrieve Gps Location", exception);
        } else {
            logger.info("location: {}", location);
        }
    }
}
