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
#ifndef DISOMASTER_H
#define DISOMASTER_H

#include <QObject>
#include <QHash>
#include <QList>
#include <QUrl>

enum MediaType
{
	NoMedia = 0,
	CD_ROM,
	CD_R,
	CD_RW,
	DVD_ROM,
	DVD_R,
	DVD_RW,
	DVD_PLUS_R,
	DVD_PLUS_R_DL,
	DVD_RAM,
	DVD_PLUS_RW,
    BD_ROM,
	BD_R,
	BD_RE
};

enum JobStatus
{
	Failed = -1,
	Idle,
	Running,
    Stalled,
    Finished
};

typedef quint64 DiskBurner;

struct DeviceProperty
{
	bool writable;
	bool formatted;
	bool blankable;
    int capacity;
    int avail;
    QList<QString> writespeed;
	QString devid;
	QString name;
};

class DISOMasterPrivate;
/*
 * All method calls below are synchronous: they do not
 * return until the operation has completed. Note the
 * signal is emitted from a separate thread (while the
 * job is actually running).
 */
class DISOMaster : public QObject
{
	Q_OBJECT
public:
	explicit DISOMaster(QObject *parent=nullptr);
    ~DISOMaster();

    /*
     * Returns a list of devices that can be acquired.
     */
    QList<DiskBurner> getDevices();
    /*
     * Acquire a certain device. All methods below require
     * a device acquired.
     */
	bool acquireDevice(DiskBurner dev);
    /*
     * Release a device.
     */
	void releaseDevice();

    /*
     * Get the media type currently in the acquired device.
     */
	MediaType getMediaType();
    /*
     * Get the property of the acquired device.
     */
	DeviceProperty getDeviceProperty();

    /*
     * Stage files for writing to the disk.
     */
	void stageFiles(const QHash<QUrl, QUrl> filelist);
    const QHash<QUrl, QUrl> &stagingFiles() const;
	void removeStagingFiles(const QList<QUrl> filelist);
    void commit(int speed = 0, bool closeSession = false);
	void erase();
    //void verify();

	void dumpISO(const QUrl isopath);
	void writeISO(const QUrl isopath, int speed = 0);

Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress);

private:
    DISOMasterPrivate* d_ptr;
    Q_DECLARE_PRIVATE(DISOMaster)
};

#endif
// vim: set tabstop=4 shiftwidth=4 softtabstop expandtab
