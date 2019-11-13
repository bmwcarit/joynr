package io.joynr.androidhelloworldbinderconsumer

import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.click
import androidx.test.espresso.assertion.ViewAssertions.matches
import androidx.test.espresso.matcher.ViewMatchers.withId
import androidx.test.espresso.matcher.ViewMatchers.withText
import androidx.test.rule.ActivityTestRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class MainActivityTest {

    @Rule
    @JvmField
    var activityRule = ActivityTestRule<MainActivity>(MainActivity::class.java)

    @Test
    fun onClickToTalkButtonClicked_textIsDisplayed() {
        // when
        onView(withId(R.id.talk_button)).perform(click())

        val str = "Hello World!"
        activityRule.activity.consumerViewModel.providedStr.postValue(str)

        // then
        onView(withId(R.id.text_box)).check(matches(withText(str)))
    }

}