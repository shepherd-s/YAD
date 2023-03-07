/**
 * GPL-2.0
 *
 * Copyright (C) 2023 Shepherd <shepherdsoft@outlook.com>.
 */

package com.android.yad

import android.util.Log
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress

class YadClient(ipStr: String, portNr: Int) : Thread() {
    private val port = portNr
    private var socket: DatagramSocket? = null
    private var ip = InetAddress.getByName(ipStr)
    private var verticalData = "V0000"
    private var horizontalData = "H+000+000"
    private var yawData = "Y+000"
    private var calData = "CF"
    private var calibration = false

    @Override
    override fun run() {
        try {
            socket = DatagramSocket(port)
            Log.i("Socket", ip.toString())
        }
        catch (e: Exception) {
            //Log.e("Socket", e.toString())
        }

        while (true) {
            send()
            sleep(2)
        }
    }

    private fun send() {
        try {
            val message = "$verticalData$horizontalData$yawData"
            val packet = DatagramPacket(message.toByteArray(), message.length, ip, port)

            if (calibration) {
                val calPacket = DatagramPacket(calData.toByteArray(), calData.length, ip, port)
                socket?.send(calPacket)
                calibration = false
            }

            socket?.send(packet)
        }
        catch (e: Exception) {
            //Log.e("Socket", e.toString())
        }
    }

    @Synchronized
    fun setVerticalData(data: String) {
        this.verticalData = data
    }

    @Synchronized
    fun setHorizontalData(data: String) {
        this.horizontalData = data
    }

    @Synchronized
    fun setYawData(data: String) {
        this.yawData = data
    }

    @Synchronized
    fun setCalData(data: String) {
        calibration = true
        calData = data
    }
}