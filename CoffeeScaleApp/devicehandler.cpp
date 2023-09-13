/***************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "heartrate-global.h"
#include "devicehandler.h"
#include "deviceinfo.h"
#include <QtEndian>
#include <QRandomGenerator>

#include <iostream>
using namespace std;

QString bluscaleuuid("00000eb7-0000-1000-8000-00805f9b34fb");
QString bluscalechar("00000eb8-0000-1000-8000-00805f9b34fb");


DeviceHandler::DeviceHandler(QObject *parent) :
    BluetoothBaseClass(parent),
	m_foundHeartRateService(false),
	maxsamples(240)
{
#ifdef SIMULATOR
    m_demoTimer.setSingleShot(false);
    m_demoTimer.setInterval(2000);
    connect(&m_demoTimer, &QTimer::timeout, this, &DeviceHandler::updateDemoHR);
	m_demoTimer.();
    updateDemoHR();
#endif
}

DeviceHandler::~DeviceHandler()
{
	if (m_service)
		finalize();
}

void DeviceHandler::setAddressType(AddressType type)
{
    switch (type) {
    case DeviceHandler::AddressType::PublicAddress:
        m_addressType = QLowEnergyController::PublicAddress;
        break;
    case DeviceHandler::AddressType::RandomAddress:
        m_addressType = QLowEnergyController::RandomAddress;
        break;
    }
}

DeviceHandler::AddressType DeviceHandler::addressType() const
{
    if (m_addressType == QLowEnergyController::RandomAddress)
        return DeviceHandler::AddressType::RandomAddress;

    return DeviceHandler::AddressType::PublicAddress;
}

void DeviceHandler::setDevice(DeviceInfo *device)
{
    clearMessages();
    m_currentDevice = device;

#ifdef SIMULATOR
    setInfo(tr("Demo device connected."));
    return;
#endif

    // Disconnect and delete old connection
    if (m_control) {
        m_control->disconnectFromDevice();
        delete m_control;
        m_control = nullptr;
    }

    // Create new controller and connect it if device available
    if (m_currentDevice) {

        // Make connections
        //! [Connect-Signals-1]
        m_control = QLowEnergyController::createCentral(m_currentDevice->getDevice(), this);
        //! [Connect-Signals-1]
        m_control->setRemoteAddressType(m_addressType);
        //! [Connect-Signals-2]
        connect(m_control, &QLowEnergyController::serviceDiscovered,
                this, &DeviceHandler::serviceDiscovered);
        connect(m_control, &QLowEnergyController::discoveryFinished,
                this, &DeviceHandler::serviceScanDone);

        connect(m_control, static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
                this, [this](QLowEnergyController::Error error) {
            Q_UNUSED(error);
            setError("Cannot connect to remote device.");
        });
        connect(m_control, &QLowEnergyController::connected, this, [this]() {
            setInfo("Controller connected. Search services...");
            m_control->discoverServices();
        });
        connect(m_control, &QLowEnergyController::disconnected, this, [this]() {
			setError("Device disconnected");
        });

        // Connect
        m_control->connectToDevice();
        //! [Connect-Signals-2]
    }
	emit deviceChanged();
}



//! [Filter HeartRate service 1]
void DeviceHandler::serviceDiscovered(const QBluetoothUuid &gatt)
{
	if (gatt == QBluetoothUuid(bluscaleuuid)) {
		setInfo("scale service discovered. Waiting for service scan to be done...");
        m_foundHeartRateService = true;
    }
}
//! [Filter HeartRate service 1]

void DeviceHandler::serviceScanDone()
{
	//setInfo("Service scan done.");

    // Delete old service if available
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }

//! [Filter HeartRate service 2]
    // If heartRateService found, create new service
    if (m_foundHeartRateService)
		m_service = m_control->createServiceObject(QBluetoothUuid(bluscaleuuid), this);

    if (m_service) {
        connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::serviceStateChanged);
		connect(m_service, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::serialAvailable);
        connect(m_service, &QLowEnergyService::descriptorWritten, this, &DeviceHandler::confirmedDescriptorWrite);
        m_service->discoverDetails();
    } else {
		setError("scale service not found.");
    }
//! [Filter HeartRate service 2]
}

// Service functions
//! [Find HRM characteristic]
void DeviceHandler::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::DiscoveringServices:
        setInfo(tr("Discovering services..."));
        break;
    case QLowEnergyService::ServiceDiscovered:
    {
        setInfo(tr("Service discovered."));
		clearMessages();

		const QLowEnergyCharacteristic bluscaleChar = m_service->characteristic(QBluetoothUuid(bluscalechar));
		if (!bluscaleChar.isValid()) {
			setError("scale Data not found.");
            break;
        }

		m_notificationDesc = bluscaleChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
		if (m_notificationDesc.isValid())
		{
			m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
		}
		//start();
        break;
    }
    default:
        //nothing for now
        break;
    }

    emit aliveChanged();
}
//! [Find HRM characteristic]

//! [Reading value]
void DeviceHandler::serialAvailable(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    // ignore any other characteristic change -> shouldn't really happen though
	if (c.uuid() != QBluetoothUuid(bluscalechar))
	{
		qDebug() << "invalid ble characteristic";
		return;
	}
	bool convertionOK = false;
	auto nsample = value.toFloat(&convertionOK);
	if (convertionOK)
	{
		m_samples.append(nsample);
		if (m_samples.size() > maxsamples)
		{
			m_samples.removeFirst();
		}
		emit newSample();
	}
	else if ( isalpha(value.front()) )
	{
		qDebug() << "isalpha";
	}
	else
	{
		qDebug() << "error";
	}
}
//! [Reading value]

#ifdef SIMULATOR
void DeviceHandler::updateDemoHR()
{
    int randomValue = 0;
    if (m_currentValue < 30) { // Initial value
        randomValue = 55 + QRandomGenerator::global()->bounded(30);
    } else if (!m_measuring) { // Value when relax
        randomValue = qBound(55, m_currentValue - 2 + QRandomGenerator::global()->bounded(5), 75);
    } else { // Measuring
        randomValue = m_currentValue + QRandomGenerator::global()->bounded(10) - 2;
    }

    addMeasurement(randomValue);
}
#endif

void DeviceHandler::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    if (d.isValid() && d == m_notificationDesc && value == QByteArray::fromHex("0000")) {
        //disabled notifications -> assume disconnect intent
        m_control->disconnectFromDevice();
        delete m_service;
        m_service = nullptr;
    }
}

void DeviceHandler::disconnectService()
{
    m_foundHeartRateService = false;

    //disable notifications
    if (m_notificationDesc.isValid() && m_service
            && m_notificationDesc.value() == QByteArray::fromHex("0100")) {
        m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0000"));
    } else {
        if (m_control)
            m_control->disconnectFromDevice();

        delete m_service;
        m_service = nullptr;
	}
}

void DeviceHandler::start()
{
	qDebug() << __func__ ;
	if (!this->alive())
		return;
	auto characteristic = m_service->characteristic(QBluetoothUuid(bluscalechar));
	QByteArray startCmd("s");
	m_service->writeCharacteristic(characteristic,startCmd);
}

void DeviceHandler::finalize()
{
	qDebug() << __func__ ;
	auto characteristic = m_service->characteristic(QBluetoothUuid(bluscalechar));
	QByteArray finCmd("f");
	m_service->writeCharacteristic(characteristic,finCmd);
}

void DeviceHandler::tare()
{
	qDebug() << __func__ ;
	auto characteristic = m_service->characteristic(QBluetoothUuid(bluscalechar));
	QByteArray tareCmd("t");
	m_service->writeCharacteristic(characteristic,tareCmd);
}

void DeviceHandler::calibrate(float c)
{
	qDebug() << __func__ ;
	auto characteristic = m_service->characteristic(QBluetoothUuid(bluscalechar));
	QByteArray calCmd("c ");
	calCmd.append(QString::number(c));
	m_service->writeCharacteristic(characteristic,calCmd);
}

QString DeviceHandler::deviceAddr()
{
	if(m_currentDevice)
	{
		return m_currentDevice->getAddress();
	}
	return QString("");
}

QString DeviceHandler::deviceName()
{
	if(m_currentDevice)
	{
		return m_currentDevice->getName();
	}
	return QString("");
}

bool DeviceHandler::alive() const
{
#ifdef SIMULATOR
    return true;
#endif

    if (m_service)
	{
		return m_service->state() == QLowEnergyService::ServiceDiscovered;
	}

	return false;
}

QVariant DeviceHandler::samples() const
{
	return QVariant::fromValue(m_samples);
}
