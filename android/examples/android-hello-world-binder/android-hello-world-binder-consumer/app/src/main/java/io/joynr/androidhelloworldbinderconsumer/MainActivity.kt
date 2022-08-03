package io.joynr.androidhelloworldbinderconsumer

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    lateinit var consumerViewModel: ConsumerViewModel

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        consumerViewModel = ViewModelProvider(this).get(ConsumerViewModel::class.java)
        consumerViewModel.registerProxy(application)


        talk_button.setOnClickListener {
            consumerViewModel.getString()
        }

        consumerViewModel.providedStr.observe(this, Observer { str ->
            text_box.text = str
        })

    }

}
