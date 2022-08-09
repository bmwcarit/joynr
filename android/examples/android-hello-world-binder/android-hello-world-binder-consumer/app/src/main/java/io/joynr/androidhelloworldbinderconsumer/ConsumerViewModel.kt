package io.joynr.androidhelloworldbinderconsumer

import android.app.Application
import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import io.joynr.arbitration.ArbitrationStrategy
import io.joynr.arbitration.DiscoveryQos
import io.joynr.arbitration.DiscoveryScope
import io.joynr.exceptions.JoynrRuntimeException
import io.joynr.messaging.MessagingQos
import io.joynr.proxy.Callback
import joynr.helloworld.HelloWorldProxy

class ConsumerViewModel : ViewModel() {
    private lateinit var helloWorldProxy: HelloWorldProxy
    private val _providedStr = MutableLiveData<String>()
    val providedStr: LiveData<String> by lazy {
        _providedStr
    }

    fun registerProxy(application: Application) {
        val discoveryQos = DiscoveryQos()
        discoveryQos.discoveryTimeoutMs = 10000
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY
        discoveryQos.cacheMaxAgeMs = java.lang.Long.MAX_VALUE
        discoveryQos.arbitrationStrategy = ArbitrationStrategy.HighestPriority

        val app = application as HelloWorldApplication
        val runtime = app.getJoynRuntime()
        helloWorldProxy = runtime.getProxyBuilder("domain", HelloWorldProxy::class.java)
            .setMessagingQos(MessagingQos()).setDiscoveryQos(discoveryQos).build()
    }

    fun getString() {
        helloWorldProxy.getHello(
            object : Callback<String>() {
                override fun onSuccess(s1: String?) {
                    Log.d("Received Message", s1!!)
                    // Do some logic
                    _providedStr.postValue(s1)
                }

                override fun onFailure(e: JoynrRuntimeException) {
                    Log.d("Failed Message", "Failed to receive message with the error: $e")
                    _providedStr.postValue(e.message)
                }
            }
        )
    }
}
