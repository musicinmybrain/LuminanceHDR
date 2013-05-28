/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 *
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef HDRCREATIONMANAGER_H
#define HDRCREATIONMANAGER_H

#include <cstddef>
#include <vector>
#include <cmath>

#include <QProcess>
#include <QPair>
#include <QSharedPointer>

#include <Libpfs/frame.h>

#include "Common/LuminanceOptions.h"
#include "arch/math.h"
#include "HdrCreation/createhdr.h"
#include "HdrCreation/createhdr_common.h"
#include "HdrCreation/fusionoperator.h"

// Some other file expect this to be available
extern const config_triple predef_confs[6];

// defines an element that contains all the informations for this particular
// image to be used inside the HdrWizard
class HdrCreationItem
{
public:
    HdrCreationItem(const QString& filename);
    ~HdrCreationItem();

    const QString& filename() const     { return m_filename; }
    const pfs::FramePtr& frame() const  { return m_frame; }
    pfs::FramePtr& frame()              { return m_frame; }
    bool isValid() const                { return m_frame->isValid(); }

    bool hasAverageLuminance() const    { return (m_averageLuminance != -1.f); }
    void setAverageLuminance(float avl) { m_averageLuminance = avl; }
    float getAverageLuminance() const   { return m_averageLuminance; }

    bool hasEV() const                  { return hasAverageLuminance(); }
    void setEV(float ev)                { m_averageLuminance = std::pow(2.f, ev); }
    float getEV() const                 { return log2(m_averageLuminance); }

    void setThumbnail(QImage* thumbnail){ QSharedPointer<QImage> other(thumbnail); m_thumbnail.swap(other); } 
    QImage* qimage()                    { return m_thumbnail.data(); }
    const QImage* qimage() const        { return m_thumbnail.data(); }

private:
    QString                 m_filename;
    float                   m_averageLuminance;
    pfs::FramePtr           m_frame;
    QSharedPointer<QImage>  m_thumbnail;
};

typedef std::vector< HdrCreationItem > HdrCreationItemContainer;

class HdrCreationManager : public QObject
{
    Q_OBJECT
private:
    HdrCreationItemContainer m_data;

    libhdr::fusion::FusionOperator m_fusionOperator;
    libhdr::fusion::WeightFunction m_weightFunction;
    libhdr::fusion::ResponseFunction m_responseFunction;

    QString m_inputResponseCurveFile;
    QString m_outputResponseCurveFile;

public:
    HdrCreationManager(bool fromCommandLine = false);
	~HdrCreationManager();

    // ----- NEW FUNCTIONS ------
    HdrCreationItem& getFile(size_t idx)                { return m_data[idx]; }
    const HdrCreationItem& getFile(size_t idx) const    { return m_data[idx]; }

    void loadFiles(const QStringList& filenames);
    void removeFile(int idx);
    void clearFiles()                   { m_data.clear(); }
    size_t availableInputFiles() const  { return m_data.size(); }

    QStringList getFilesWithoutExif() const;
    size_t numFilesWithoutExif() const;

    // iterators
    typedef HdrCreationItemContainer::iterator          iterator;
    typedef HdrCreationItemContainer::const_iterator    const_iterator;

    iterator begin()                { return m_data.begin(); }
    iterator end()                  { return m_data.end(); }
    const_iterator begin() const    { return m_data.begin(); }
    const_iterator end() const      { return m_data.end(); }

    void setFusionOperator(libhdr::fusion::FusionOperator fo)       { m_fusionOperator = fo; }
    void setWeightFunction(libhdr::fusion::WeightFunction wf)       { m_weightFunction = wf; }
    void setResponseFunction(libhdr::fusion::ResponseFunction rf)   { m_responseFunction = rf; }

    void setInputResponseFile(const QString& filename)              { m_inputResponseCurveFile = filename; }
    void setOutputResponseFile(const QString& filename)             { m_outputResponseCurveFile = filename; }

    const QString& outputResponseFile() const                       { return m_outputResponseCurveFile; }

    void setConfig(const config_triple& cfg);

	pfs::Frame* createHdr(bool ag, int iterations);

    void set_ais_crop_flag(bool flag);
	void align_with_ais();
	void align_with_mtb();

    const HdrCreationItemContainer& getData() const         { return m_data; } 
    const QList<QImage*>& getAntiGhostingMasksList() const  { return antiGhostingMasksList; }
    const QVector<float> getExpotimes() const;

    // the configuration used to create the hdr
    // this is public so that the wizard (or the cli?) can modify it directly.
	config_triple chosen_config;

    //void applyShiftsToImageStack(const QList< QPair<int,int> >& HV_offsets);
    //void applyShiftsToMdrImageStack(const QList< QPair<int,int> >& HV_offsets);

    //void cropLDR(const QRect& ca);
    //void cropMDR(const QRect& ca);
    void applyShiftsToItems(const QList<QPair<int,int> >&);
    void cropItems(const QRect& ca);
    void cropAgMasks(const QRect& ca);

    void saveImages(const QString& prefix);
	void doAntiGhosting(int);
	void doAutoAntiGhosting(float);
	void removeTempFiles();

signals:
    // computation progress
    void progressStarted();
    void progressFinished();
    void progressCancel();
    void progressRangeChanged(int,int);
    void progressValueChanged(int);
    void finishedLoadingFiles();

    // legacy code
    void finishedLoadingInputFiles(const QStringList& filesLackingExif);
    void errorWhileLoading(const QString& message); //also for !valid size

    void fileLoaded(int index, const QString& fname, float expotime);

	void finishedAligning(int);
	void expotimeValueChanged(float,int);
	void ais_failed(QProcess::ProcessError);
    void aisDataReady(const QByteArray& data);
	void processed();
	void imagesSaved();

private:
    bool framesHaveSameSize();    
	void doAutoAntiGhostingMDR(float);
	void doAutoAntiGhostingLDR(float);

	QList<QImage*> antiGhostingMasksList;  //QImages used for manual anti ghosting
    LuminanceOptions m_luminance_options;

    // align_image_stack
	QProcess *ais;

    bool m_ais_crop_flag;
	bool fromCommandLine;

private slots:
	void ais_finished(int,QProcess::ExitStatus);
	void ais_failed_slot(QProcess::ProcessError);
	void readData();
};
#endif
