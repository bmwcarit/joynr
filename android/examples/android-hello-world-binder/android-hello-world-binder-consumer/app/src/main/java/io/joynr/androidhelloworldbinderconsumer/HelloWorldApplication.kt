package io.joynr.androidhelloworldbinderconsumer

import android.app.Application
import io.joynr.android.AndroidBinderRuntime
import io.joynr.runtime.JoynrRuntime
import org.slf4j.LoggerFactory
import org.slf4j.impl.AndroidLogger
import org.slf4j.impl.StaticLoggerBinder

class HelloWorldApplication : Application() {

    // Must be included for logs to work
    private val logger = LoggerFactory.getLogger(HelloWorldApplication::class.java)

    lateinit var joynrRuntime: JoynrRuntime private set

    override fun onCreate() {
        super.onCreate()

        // This is where you can define the debug level
        StaticLoggerBinder.setLogLevel(AndroidLogger.LogLevel.DEBUG)

        logger.debug("Starting binder runtime...")

        // Initializing Android Binder Runtime with Application Context
        // This needs to be in the Application onCreate in order for the JoynrRuntime to be
        // ready once the application is started.
        joynrRuntime = AndroidBinderRuntime.init(this)
    }

    fun getJoynRuntime(): JoynrRuntime {
        return this.joynrRuntime
    }
}
