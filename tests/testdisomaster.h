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
#ifndef TESTDISOMASTER_H
#define TESTDISOMASTER_H

#include <QObject>
#include <QtTest/QtTest>
#include "../libdisomaster/disomaster.h"

class TestDISOMaster;

class TestSignalReceiver: public QObject
{
    Q_OBJECT
public:
    explicit TestSignalReceiver(DISOMasterNS::DISOMaster *_d, TestDISOMaster *parent = nullptr);
public Q_SLOTS:
    void updateJobStatus(DISOMasterNS::DISOMaster::JobStatus status, int progress);
private:
    TestDISOMaster *p;
    DISOMasterNS::DISOMaster *d;
};

class TestDISOMaster : public QObject
{
    Q_OBJECT
public:
    explicit TestDISOMaster(QObject *parent = nullptr);
    DISOMasterNS::DISOMaster::JobStatus st;
    int p;

private Q_SLOTS:
    void test_getDevice();
    void test_writeFiles();
    void test_erase();
    void test_isoWrite();
    void test_checkMedia();

};

#endif // TESTDISOMASTER_H
