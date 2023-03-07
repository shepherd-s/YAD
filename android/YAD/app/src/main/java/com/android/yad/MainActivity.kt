/**
 * GPL-2.0
 *
 * Copyright (C) 2023 Shepherd <shepherdsoft@outlook.com>.
 */

package com.android.yad

import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Surface
import androidx.compose.material.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Alignment.Companion.BottomCenter
import androidx.compose.ui.Alignment.Companion.Center
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.boundsInWindow
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import com.android.yad.ui.theme.YADTheme
import kotlin.math.*

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val yadIp = "192.168.0.1"
        val yadPort = 60000
        val yadClient = YadClient(yadIp, yadPort)
        val windowInsetsController =
            WindowCompat.getInsetsController(window, window.decorView)

        // Configure the behavior of the hidden system bars.
        if (windowInsetsController != null) {
            window.addFlags (WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

            windowInsetsController.systemBarsBehavior =
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            windowInsetsController.hide(WindowInsetsCompat.Type.systemBars())
        }

        yadClient.start()

        setContent {
            YADTheme {
                Surface(
                    color = MaterialTheme.colors.background
                ) {
                    val space = .07f
                    Row(modifier = Modifier
                        .fillMaxSize()
                    ) {
                        Spacer(modifier = Modifier
                            .fillMaxHeight()
                            .fillMaxWidth(space)
                        )

                        ThrottlePadColumn(space, yadClient)
                        JoystickColumn(space, yadClient)
                    }
                }
            }
        }
    }
}

fun encodeMovementData(direction: String, chill: Int, percentX: Int, percentY: Int): String {
    val absX = abs(percentX)
    val absY = abs(percentY)
    var command: String

    when (direction) {
        "V" -> {
            command = if (chill == 1) "V1" else "V0"
            command += when (absY) {
                in 0..9 -> "00$absY"
                in 10..99 -> "0$absY"
                100 -> "$absY"
                else -> {
                    return ""
                }
            }
        }
        "H" -> {
            command = "H"
            command += if (percentX < 0) "-" else "+"
            command += when (absX) {
                in 0..9 -> "00$absX"
                in 10..99 -> "0$absX"
                100 -> "$absX"
                else -> {
                    return ""
                }
            }
            command += if (percentY < 0) "-" else "+"
            command += when (absY) {
                in 0..9 -> "00$absY"
                in 10..99 -> "0$absY"
                100 -> "$absY"
                else -> {
                    return ""
                }
            }
        }
        "Y" -> {
            command = "Y"
            command += if (percentX < 0) "-" else "+"

            command += when (absX) {
                in 0..9 -> "00$absX"
                in 10..99 -> "0$absX"
                100 -> "$absX"
                else -> {
                    return ""
                }
            }
        }
        else -> {
            return ""
        }
    }

    return command
}

@Composable
fun ThrottlePadColumn(space: Float, yadClient: YadClient) {
    val border = 4f
    var offsetY by remember { mutableStateOf(0f) }
    var offsetX by remember { mutableStateOf(0f) }
    var sliderButtonSize: Float
    var percentY: Int
    var percentX: Int
    var command: String
    var sliderHeight = 0f
    var limitY = 0f
    var limitX = 0f

    Column(
        verticalArrangement = Arrangement.Center,
        modifier = Modifier
            .fillMaxHeight()
            .fillMaxWidth(.5f - space)
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth(.25f)
                .fillMaxHeight(.8f)
                .border(border.dp, Color.Gray, CircleShape)
                .onGloballyPositioned {
                    sliderHeight = it.boundsInWindow().size.height
                }
        ) {
            Box(
                modifier = Modifier
                    .offset { IntOffset(offsetX.roundToInt(), offsetY.roundToInt()) }
                    .fillMaxWidth()
                    .align(BottomCenter)
                    .clip(CircleShape)
                    .aspectRatio(1f)
                    .onGloballyPositioned {
                        sliderButtonSize = it.boundsInWindow().size.height
                        limitY = sliderHeight - sliderButtonSize - 2 * border
                        limitX = sliderButtonSize / 2
                    }
                    .background(Color.Gray)
                    .pointerInput(key1 = 0) {
                        detectDragGestures(
                            onDragEnd = {
                                offsetX = 0f

                                command = encodeMovementData("Y", 0, 0, 0)
                                yadClient.setYawData(command)
                            }
                        ) { change, dragAmount ->
                            change.consume()

                            offsetY = (offsetY + dragAmount.y).coerceIn(-limitY, 0f)
                            percentY = (offsetY * 100 / limitY).roundToInt()

                            command = encodeMovementData("V",0, 0, percentY)

                            yadClient.setVerticalData(command)

                            offsetX = (offsetX + dragAmount.x).coerceIn(-limitX, limitX)
                            percentX = (offsetX * 100 / limitX).roundToInt()

                            command = encodeMovementData("Y",0, percentX, 0)

                            yadClient.setYawData(command)
                        }
                    }
            )

        }
    }
}

@Composable
fun JoystickColumn(space: Float, yadClient: YadClient) {
    val box1Percent = .3f
    val spacerPercent = 0.05f
    val box1Width = .6f
    val box1Height = box1Percent / (1f - spacerPercent)
    val spacer2Height = 2f * spacerPercent / (1f - box1Percent - spacerPercent)
    val box2Width =.6f
    val box2Height = .50f / (1f - box1Percent - spacerPercent - spacerPercent)
    val border = 4f
    val button = Modifier
        .clip(CircleShape)
        .fillMaxHeight(.25f)
        .aspectRatio(1f)
        .background(Color.Gray)
    var offsetX by remember { mutableStateOf(0f) }
    var offsetY by remember { mutableStateOf(0f) }
    var buttonRadius: Float
    var tempX: Float
    var tempY: Float
    var angle: Float
    var percentX: Int
    var percentY: Int
    var command: String
    var limit = 0f
    var padRadius = 0f

    Column(horizontalAlignment = Alignment.End,
            modifier = Modifier
            .fillMaxHeight()
            .fillMaxWidth(1f - 2f * space)
    ) {
        Spacer(modifier = Modifier
            .fillMaxWidth()
            .fillMaxHeight(spacerPercent)
        )

        Box(modifier = Modifier
            .fillMaxWidth(box1Width)
            .fillMaxHeight(box1Height)
        ) {
            Box(modifier = button
                .align(Alignment.BottomStart)
                .background(Color.Red)
                .clickable {
                    yadClient.setCalData("Cw")
                }
            )

            Box(modifier = Modifier
                .fillMaxHeight()
                .aspectRatio(1f)
                .align(Center)
            ) {
                Box(modifier = button
                    .align(Alignment.CenterStart)
                    .clickable {
                        yadClient.setCalData("Cl")
                    }
                )
                Box(modifier = button
                    .align(Alignment.CenterEnd)
                    .clickable {
                        yadClient.setCalData("Cr")
                    }
                )
                Text("Cal", modifier = Modifier.align(Center))
                Box(modifier = button
                    .align(Alignment.TopCenter)
                    .clickable {
                        yadClient.setCalData("Cf")
                    }
                )
                Box(modifier = button
                    .align(BottomCenter)
                    .clickable {
                        yadClient.setCalData("Cb")
                    }
                )
            }

            Box(modifier = button
                .align(Alignment.BottomEnd)
                .background(Color.Red)
                .clickable {
                    yadClient.setCalData("CW")
                }
            )

            Box(modifier = button
                .align(Alignment.TopEnd)
                .background(Color.Yellow)
                .clickable {
                    yadClient.setCalData("Cz")
                }
            )
        }

        Spacer(modifier = Modifier
            .fillMaxWidth()
            .fillMaxHeight(spacer2Height)
        )

        Box(modifier = Modifier
            .fillMaxHeight(box2Height)
            .fillMaxWidth(box2Width)
        ) {
            Box(modifier = Modifier
                .align(Center)
                .fillMaxHeight()
                .aspectRatio(1f)
                .border(border.dp, Color.Gray, CircleShape)
                .onGloballyPositioned {
                    padRadius = it.boundsInWindow().size.height / 2
                }
            ) {//draggable joystick button
                Box(modifier = Modifier
                    .offset { IntOffset(offsetX.roundToInt(), offsetY.roundToInt()) }
                    .align(Center)
                    .clip(CircleShape)
                    .fillMaxWidth(.5f)
                    .aspectRatio(1f)
                    .background(Color.Gray)
                    .onGloballyPositioned {
                        buttonRadius = it.boundsInWindow().size.height / 2
                        limit = padRadius - buttonRadius / 2
                    }

                    .pointerInput(key1 = 1) {
                        detectDragGestures(
                            onDragEnd = {
                                offsetX = 0f
                                offsetY = 0f

                                command = encodeMovementData("H", 0, 0, 0)
                                yadClient.setHorizontalData(command)
                            },

                        ) { change, dragAmount ->
                            change.consume()

                            tempX = (offsetX + dragAmount.x).coerceIn(-limit, limit)
                            tempY = (offsetY + dragAmount.y).coerceIn(-limit, limit)

                            angle = atan(tempY / tempX)
                            
                            offsetX = (offsetX + dragAmount.x)
                                .coerceIn(-abs(cos(angle)) * limit, abs(cos(angle)) * limit)
                            offsetY = (offsetY + dragAmount.y)
                                .coerceIn(-abs(sin(angle)) * limit, abs(sin(angle)) * limit)

                            percentX = (offsetX * 100 / limit).roundToInt()
                            percentY = (offsetY * 100 / limit).roundToInt()

                            command = encodeMovementData("H", 0, percentX, percentY)

                            yadClient.setHorizontalData(command)
                        }
                    }
                )
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun DefaultPreview() {

}
