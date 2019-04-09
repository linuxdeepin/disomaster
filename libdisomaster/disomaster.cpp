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
#include "disomaster.h"
#include "xorriso.h"
#include <QRegularExpression>

#define PCHAR(s) (char*)(s)

#define XORRISO_OPT(opt, x, ...) \
    Xorriso_set_problem_status(x, PCHAR(""), 0); \
    r = Xorriso_option_##opt(x, __VA_ARGS__); \
    r = Xorriso_eval_problem_status(x, r, 0);

#define JOBFAILED_IF(r, x) \
    if (r <= 0) { \
        Xorriso_option_end(x, 1); \
        Q_EMIT jobStatusChanged(JobStatus::Failed, -1); \
        return; \
    }

class DISOMasterPrivate
{
private:
    DISOMasterPrivate(DISOMaster *q): q_ptr(q) {}
    XorrisO *xorriso;
    QHash<QUrl, QUrl> files;
    QList<DeviceProperty> dev;
    DiskBurner curdev = ~0ULL;
    DISOMaster *q_ptr;
    Q_DECLARE_PUBLIC(DISOMaster)

    void getCurrentDeviceProperty();

public:
    void messageReceived(int type, char *text);
};

int XorrisoResultHandler(void *handle, char *text);
int XorrisoInfoHandler(void *handle, char *text);

DISOMaster::DISOMaster(QObject *parent):
    QObject(parent),
    d_ptr(new DISOMasterPrivate(this))
{
    Q_D(DISOMaster);
    int r = Xorriso_new(&d->xorriso, PCHAR("xorriso"), 0);
    if (r <= 0) {
        d->xorriso = nullptr;
        return;
    }

    r = Xorriso_startup_libraries(d->xorriso, 0);
    if (r <= 0) {
        Xorriso_destroy(&d->xorriso, 0);
        return;
    }

    Xorriso_sieve_big(d->xorriso, 0);
    Xorriso_start_msg_watcher(d->xorriso, XorrisoResultHandler, d, XorrisoInfoHandler, d, 0);
}

DISOMaster::~DISOMaster()
{
    Q_D(DISOMaster);

    if (d->xorriso) {
        Xorriso_stop_msg_watcher(d->xorriso, 0);
        Xorriso_destroy(&d->xorriso, 0);
    }
}

QList<DiskBurner> DISOMaster::getDevices()
{
    Q_D(DISOMaster);
    Xorriso_set_problem_status(d->xorriso, PCHAR(""), 0);
    int r = Xorriso_option_devices(d->xorriso, 0);
    r = Xorriso_eval_problem_status(d->xorriso, r, 0);
    if (r <= 0) {
        return {};
    }

    int ac, avail, id = 0;
    char **av;
    QList<DiskBurner> ret;
    d->dev.clear();

    do {
        r = Xorriso_sieve_get_result(d->xorriso, PCHAR("?  -dev"), &ac, &av, &avail, 0);
        if (r > 0) {
            ret.push_back(id++);
            DeviceProperty p;
            p.name = QString(av[2]) + QString(av[3]);
            p.devid = QString(av[0]);
            d->dev.push_back(p);
        }
    } while (avail > 0);

    Xorriso__dispose_words(&ac, &av);
    return ret;
}

bool DISOMaster::acquireDevice(DiskBurner dev)
{
    Q_D(DISOMaster);

    if (dev < (quint64)d->dev.size()) {
        d->files.clear();
        d->curdev = dev;

        Xorriso_set_problem_status(d->xorriso, PCHAR(""), 0);
        int r = Xorriso_option_dev(d->xorriso, d->dev[d->curdev].devid.toUtf8().data(), 3);
        r = Xorriso_eval_problem_status(d->xorriso, r, 0);
        if (r <= 0) {
            d->curdev = -1;
            return false;
        }
        return true;
    }

    return false;
}

void DISOMaster::releaseDevice()
{
    Q_D(DISOMaster);
    d->curdev = ~0ULL;
    d->files.clear();
    Xorriso_option_end(d->xorriso, 0);
}

DeviceProperty DISOMaster::getDeviceProperty()
{
    Q_D(DISOMaster);
    d->getCurrentDeviceProperty();
    return d->dev[d->curdev];
}

void DISOMaster::stageFiles(const QHash<QUrl, QUrl> filelist)
{
    Q_D(DISOMaster);
    d->files.unite(filelist);
}

const QHash<QUrl, QUrl> &DISOMaster::stagingFiles() const
{
    Q_D(const DISOMaster);
    return d->files;
}

void DISOMaster::removeStagingFiles(const QList<QUrl> filelist)
{
    Q_D(DISOMaster);
    for (auto &i : filelist) {
        auto it = d->files.find(i);
        if (it != d->files.end()) {
            d->files.erase(it);
        }
    }
}

void DISOMaster::commit(int speed, bool closeSession)
{
    Q_D(DISOMaster);
    Q_EMIT jobStatusChanged(JobStatus::Stalled, 0);

    QString spd = QString::number(speed) + "k";
    if (speed == 0) {
        spd = "0";
    }

    int r;

    XORRISO_OPT(speed, d->xorriso, spd.toUtf8().data(), 0);
    JOBFAILED_IF(r, d->xorriso);

    for (auto it = d->files.keyValueBegin(); it != d->files.keyValueEnd(); ++it) {
        XORRISO_OPT(
            map, d->xorriso,
            (*it).first.toString().toUtf8().data(),
            (*it).second.toString().toUtf8().data(),
            0
        );
        JOBFAILED_IF(r, d->xorriso);
    }

    XORRISO_OPT(close, d->xorriso, PCHAR(closeSession ? "on" : "off"), 0);
    JOBFAILED_IF(r, d->xorriso);

    XORRISO_OPT(commit, d->xorriso, 0);
    JOBFAILED_IF(r, d->xorriso);
}

void DISOMaster::erase()
{
    Q_D(DISOMaster);
    Q_EMIT jobStatusChanged(JobStatus::Running, 0);

    int r;
    XORRISO_OPT(blank, d->xorriso, PCHAR("as_needed"), 0);
    JOBFAILED_IF(r, d->xorriso);
}

void DISOMaster::dumpISO(const QUrl isopath)
{
    Q_D(DISOMaster);
    //use osirrox
    //unimplemented
}

void DISOMaster::writeISO(const QUrl isopath, int speed)
{
    Q_D(DISOMaster);
    Q_EMIT jobStatusChanged(JobStatus::Stalled, 0);
    QString spd = QString::number(speed) + "k";
    if (speed == 0) {
        spd = "0";
    }

    int r;

    char **av = new char *[6];
    int dummy;
    av[0] = strdup("cdrecord");
    av[1] = strdup("-v");
    av[2] = strdup((QString("dev=") + d->dev[d->curdev].devid).toUtf8().data());
    av[3] = strdup("blank=as_needed");
    av[4] = strdup((QString("speed=") + spd).toUtf8().data());
    av[5] = strdup(isopath.toString().toUtf8().data());
    XORRISO_OPT(as, d->xorriso, 6, av, &dummy, 1);
    JOBFAILED_IF(r, d->xorriso);
    for (int i = 0; i < 6; ++i) {
        free(av[i]);
    }
    delete []av;
}

void DISOMasterPrivate::getCurrentDeviceProperty()
{
    if (!~curdev) {
        return;
    }

    Xorriso_set_problem_status(xorriso, PCHAR(""), 0);
    int r = Xorriso_option_list_speeds(xorriso, 0);
    r = Xorriso_eval_problem_status(xorriso, r, 0);
    if (r <= 0) {
        return;
    }

    int ac, avail;
    char **av;
    Xorriso_sieve_get_result(xorriso, PCHAR("Media current:"), &ac, &av, &avail, 0);
    if (ac < 1) {
	    Xorriso__dispose_words(&ac, &av);
        return;
    }
    QString mt = av[0];
    const static QHash<QString, MediaType> typemap = {
        {"CD-ROM",   MediaType::CD_ROM},
        {"CD-R",     MediaType::CD_R},
        {"CD-RW",    MediaType::CD_RW},
        {"DVD-ROM",  MediaType::DVD_ROM},
        {"DVD-R",    MediaType::DVD_R},
        {"DVD-RW",   MediaType::DVD_RW},
        {"DVD+R",    MediaType::DVD_PLUS_R},
        {"DVD+R/DL", MediaType::DVD_PLUS_R_DL},
        {"DVD-RAM",  MediaType::DVD_RAM},
        {"DVD+RW",   MediaType::DVD_PLUS_RW},
        {"BD-ROM",   MediaType::BD_ROM},
        {"BD-R",     MediaType::BD_R},
        {"BD-RE",    MediaType::BD_RE}
    };
    mt = mt.left(mt.indexOf(' ')-1);
    if (typemap.find(mt) != typemap.end()) {
        dev[curdev].media = typemap[mt];
    } else {
        dev[curdev].media = MediaType::NoMedia;
    }
    Xorriso__dispose_words(&ac, &av);
    
    Xorriso_sieve_get_result(xorriso, PCHAR("Media summary:"), &ac, &av, &avail, 0);
    if (ac == 4)
    {
        const QString units="kmg";
        dev[curdev].data = atof(av[2]) * (1 << ((units.indexOf(QString(av[2]).back())+1) * 10));
        dev[curdev].avail = atof(av[3]) * (1 << ((units.indexOf(QString(av[3]).back())+1) * 10));
    }
    Xorriso__dispose_words(&ac, &av);

    dev[curdev].writespeed.clear();
    do {
        Xorriso_sieve_get_result(xorriso, PCHAR("Write speed  :"), &ac, &av, &avail, 0);
        if (ac == 2) {
            dev[curdev].writespeed.push_back(QString(av[0]) + '\t' + QString(av[1]));
        }
        Xorriso__dispose_words(&ac, &av);
    } while (avail > 0);
}

void DISOMasterPrivate::messageReceived(int type, char *text)
{
    Q_Q(DISOMaster);
    Q_UNUSED(type);
    QString msg(text);
    msg = msg.trimmed();
    //fprintf(stderr,"msg from xorriso (%s) : %s\n", type ? " info " : "result", msg.toStdString().c_str());
    if (msg.indexOf("UPDATE : Closing track/session.") != -1) {
        Q_EMIT q->jobStatusChanged(JobStatus::Stalled, 1);
        return;
    }

    if (msg.indexOf("UPDATE : Thank you for being patient.") != -1) {
        Q_EMIT q->jobStatusChanged(JobStatus::Stalled, 0);
        return;
    }

    QRegularExpression r("([0-9.]*)%");
    QRegularExpressionMatch m = r.match(msg);
    if (m.hasMatch()) {
        double percentage = m.captured(1).toDouble();
        Q_EMIT q->jobStatusChanged(JobStatus::Running, percentage);
    }

    if (msg.indexOf("Blanking done") != -1 ||
            msg.indexOf(QRegularExpression("Writing to .* completed successfully.")) != -1) {
        Q_EMIT q->jobStatusChanged(JobStatus::Finished, 0);
    }
}

int XorrisoResultHandler(void *handle, char *text)
{
    ((DISOMasterPrivate *)handle)->messageReceived(0, text);
    return 1;
}
int XorrisoInfoHandler(void *handle, char *text)
{
    //working around xorriso passing wrong handle to the callback
    if (strstr(text, "DEBUG : Concurrent message watcher")) {
        return 1;
    }
    ((DISOMasterPrivate *)handle)->messageReceived(1, text);
    return 1;
}
/*
 * xorriso -devices
 * xorriso -dev * -toc
 * xorriso -dev * -list_speeds
 * xorriso -dev * -check_media
 * xorriso -dev * -blank as_needed
 * xorriso -dev * -speed x -map /home/tfosirhc/Pictures/ /Pictures [-close on|off] -commit
 * other todos:
 * -volid
 */
