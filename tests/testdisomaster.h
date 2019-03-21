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
    explicit TestSignalReceiver(TestDISOMaster *parent = nullptr);
public Q_SLOTS:
    void updateJobStatus(JobStatus status, int progress);
private:
    TestDISOMaster* p;
};

class TestDISOMaster : public QObject
{
    Q_OBJECT
public:
    explicit TestDISOMaster(QObject *parent = nullptr);
    JobStatus st;
    int p;

private Q_SLOTS:
    void test_getDevices();
    void test_writeFiles();
    void test_erase();

};

#endif // TESTDISOMASTER_H
