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
#include <QThread>
#include <QMetaType>

TestSignalReceiver::TestSignalReceiver(TestDISOMaster *parent) : QObject(parent), p(parent)
{
}
void TestSignalReceiver::updateJobStatus(JobStatus status, int progress)
{
    fprintf(stderr, "status update: %d %d\n", status, progress);
    p->st = status;
    p->p = progress;
}

TestDISOMaster::TestDISOMaster(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<JobStatus>("JobStatus");
}

void TestDISOMaster::test_getDevices()
{
    DISOMaster* x=new DISOMaster;
    QList<DiskBurner> b = x->getDevices();
    x->acquireDevice(0);
    x->getDeviceProperty();
    x->releaseDevice();
    QVERIFY(b.size()>0);
    delete x;
}

void TestDISOMaster::test_writeFiles()
{
    DISOMaster *x=new DISOMaster;
    TestSignalReceiver *r=new TestSignalReceiver(this);
    connect(x, &DISOMaster::jobStatusChanged, r, &TestSignalReceiver::updateJobStatus);
    QThread *th=QThread::create([x]{
        x->acquireDevice(0);
        QHash<QUrl, QUrl> files{
            {QUrl("/home/tfosirhc/Pictures"),  QUrl("/Pictures")},
            {QUrl("/home/tfosirhc/Downloads"), QUrl("/Test")}
        };
        x->stageFiles(files);
        x->commit();
        x->releaseDevice();
    });
    th->start();
    QTRY_VERIFY_WITH_TIMEOUT(st==JobStatus::Finished, 120000);
    th->wait();
    delete th;
    delete r;
    delete x;
}

void TestDISOMaster::test_erase()
{
    DISOMaster *x=new DISOMaster;
    TestSignalReceiver *r=new TestSignalReceiver(this);
    connect(x, &DISOMaster::jobStatusChanged, r, &TestSignalReceiver::updateJobStatus);
    QThread *th=QThread::create([x]{
        x->acquireDevice(0);
        x->erase();
        x->releaseDevice();
    });
    th->start();
    QTRY_VERIFY_WITH_TIMEOUT(st==JobStatus::Finished, 60000);
    th->wait();
    delete th;
    delete r;
    delete x;
}

QTEST_MAIN(TestDISOMaster)
