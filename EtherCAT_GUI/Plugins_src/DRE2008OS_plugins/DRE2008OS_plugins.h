﻿#ifndef DRE2008OS_PLUGIN_H
#define DRE2008OS_PLUGIN_H

#include <QObject>
#include "../../Qt_VS_Dock/Plugin_common/EtherCAT_UserApp.h"
#include "form_plot.h"
#include "DRE2008_OS_Callback.h"

class DRE2008OS_plugin : public QObject,EtherCAT_UserApp
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID UserApp_iid FILE "DRE2008OS_plugins.json")
#endif // QT_VERSION >= 0x050000

    Q_INTERFACES(EtherCAT_UserApp)
private:  
    ~DRE2008OS_plugin();

    Form_plot *m_userWidget;
    DRE2008_OS_Callback *m_userCallback;

    QTimer *m_timePlot;
    int m_plotDisplay_Num;
    bool m_isMaster_Run;
public:
    DRE2008OS_plugin(QObject *parent = 0);

    void Init_Cores();
    void Destroy_Cores();
private slots:
    void plot_pushButton_PlotStart_clicked();
    void plot_pushButton_PlotStop_clicked();
    void user_timeout_handle();

    void plot_dial_SampleRate_valueChanged(int value);
    void plot_dial_DisplayNum_valueChanged(int value);
    void plot_combobox_ADchannel_currentIndexChanged(int index);
    void plot_combobox_OSchannel_currentIndexChanged(int index);

    void callback_Master_RunStateChanged(bool isRun);

};

#endif // USERAPP_PLUGIN_H
