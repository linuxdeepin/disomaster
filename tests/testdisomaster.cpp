/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Chris Xiong <chirs241097@gmail.com>
 *
 * Maintainer: Chris Xiong <chirs241097@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "testdisomaster.h"
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QMetaType>

using namespace DISOMasterNS;

TestSignalReceiver::TestSignalReceiver(TestDISOMaster *parent) : QObject(parent), p(parent)
{
}
void TestSignalReceiver::updateJobStatus(DISOMaster::JobStatus status, int progress)
{
    fprintf(stderr, "status update: %d %d\n", status, progress);
    p->st = status;
    p->p = progress;
}

TestDISOMaster::TestDISOMaster(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<DISOMaster::JobStatus>(QT_STRINGIFY(DISOMaster::JobStatus));
}

void TestDISOMaster::test_getDevice()
{
    Q_ASSUME(qEnvironmentVariableIsSet("DISOMASTERTEST_DEVICE"));
    const QString dev = qEnvironmentVariable("DISOMASTERTEST_DEVICE");
    DISOMaster* x=new DISOMaster;
    x->acquireDevice(dev);
    x->getDeviceProperty();
    x->releaseDevice();
    DeviceProperty dp = x->getDevicePropertyCached(dev);
    fprintf(stderr, "data: %llu, avail: %llu\n", dp.data, dp.avail);
    QVERIFY(dp.devid != "");
    delete x;
}

void TestDISOMaster::test_writeFiles()
{
    Q_ASSUME(qEnvironmentVariableIsSet("DISOMASTERTEST_DEVICE"));
    Q_ASSUME(qEnvironmentVariableIsSet("DISOMASTERTEST_DATAPATH"));
    const QString dev = qEnvironmentVariable("DISOMASTERTEST_DEVICE");
    const QString path = qEnvironmentVariable("DISOMASTERTEST_DATAPATH");

    st = DISOMaster::JobStatus::Idle;
    DISOMaster *x = new DISOMaster;
    TestSignalReceiver *r = new TestSignalReceiver(this);
    connect(x, &DISOMaster::jobStatusChanged, r, &TestSignalReceiver::updateJobStatus);

    QFuture<void> f = QtConcurrent::run([=] {
        x->acquireDevice(dev);
        QHash<QUrl, QUrl> files{
            {QUrl(path), QUrl("/")}
        };
        x->stageFiles(files);
        x->commit();
        x->releaseDevice();
    });

    QTRY_VERIFY_WITH_TIMEOUT(st == DISOMaster::JobStatus::Finished, 120000);
    f.waitForFinished();
    delete r;
    delete x;
}

void TestDISOMaster::test_erase()
{
    Q_ASSUME(qEnvironmentVariableIsSet("DISOMASTERTEST_DEVICE"));
    const QString dev = qEnvironmentVariable("DISOMASTERTEST_DEVICE");

    st = DISOMaster::JobStatus::Idle;
    DISOMaster *x = new DISOMaster;
    TestSignalReceiver *r = new TestSignalReceiver(this);
    connect(x, &DISOMaster::jobStatusChanged, r, &TestSignalReceiver::updateJobStatus);

    QFuture<void> f = QtConcurrent::run([=] {
        x->acquireDevice(dev);
        x->erase();
        x->releaseDevice();
    });

    QTRY_VERIFY_WITH_TIMEOUT(st == DISOMaster::JobStatus::Finished, 60000);
    f.waitForFinished();
    delete r;
    delete x;
}

void TestDISOMaster::test_isoWrite()
{
    Q_ASSUME(qEnvironmentVariableIsSet("DISOMASTERTEST_ISOFILE"));
    Q_ASSUME(qEnvironmentVariableIsSet("DISOMASTERTEST_DEVICE"));
    const QString dev = qEnvironmentVariable("DISOMASTERTEST_DEVICE");
    const QString iso = qEnvironmentVariable("DISOMASTERTEST_ISOFILE");

    st = DISOMaster::JobStatus::Idle;
    DISOMaster *x = new DISOMaster;
    TestSignalReceiver *r = new TestSignalReceiver(this);
    connect(x, &DISOMaster::jobStatusChanged, r, &TestSignalReceiver::updateJobStatus);

    QFuture<void> f = QtConcurrent::run([=] {
        x->acquireDevice(dev);
        x->writeISO(iso);
        x->releaseDevice();
    });

    QTRY_VERIFY_WITH_TIMEOUT(st == DISOMaster::JobStatus::Finished, 750000);
    f.waitForFinished();
    delete r;
    delete x;
}

QTEST_MAIN(TestDISOMaster)
