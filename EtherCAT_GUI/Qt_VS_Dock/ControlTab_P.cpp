﻿#include "ControlTab_P.h"

#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QFileDialog>
#include <QKeyEvent>
#include <QSettings>

//用于解析G代码
#define PROGRESSMINLINES 10000 //G代码文件的最大行数
#define PROGRESSSTEP     1000

ControlTab_P::ControlTab_P(QObject *parent) : QObject(parent)
{
   user_form_controlTab = new Form_ControlTab();
   m_motorApp_callback = new My_MotorApp_Callback();

   set_UIWidgetPtr(user_form_controlTab);
   set_CallbackPtr(m_motorApp_callback);

}

void ControlTab_P::Init_Cores()
{
    m_settingPath = "./config_User.ini";
    Load_setting(m_settingPath);//加载设置
    m_motorApp_callback->m_slaveCount = 0;//赋初值

    controlTab_isTheta_display = user_form_controlTab->get_CheckBoxPtr(Form_ControlTab::check_isThetaDis_c)->checkState();

    connect(user_form_controlTab->get_ButtonGcode(Form_ControlTab::Gcode_openFile_b),SIGNAL(clicked(bool)),this,SLOT(Control_OpenGcode_clicked()));
    connect(user_form_controlTab->get_ButtonGcode(Form_ControlTab::Gcode_reloadFile_b),SIGNAL(clicked(bool)),this,SLOT(Control_ReloadGcode_clicked()));
    connect(user_form_controlTab->get_ButtonGcode(Form_ControlTab::Gcode_sendFile_b),SIGNAL(clicked(bool)),this,SLOT(Control_SendGcode_clicked()));
    connect(user_form_controlTab->get_CheckBoxPtr(Form_ControlTab::check_isThetaDis_c),SIGNAL(stateChanged(int)),this,SLOT(ControlTab_checkThetaDis_stateChange(int)));

    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(tr("280.799"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setPlaceholderText(tr("X轴"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(tr("0"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setPlaceholderText(tr("Y轴"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(tr("155"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setPlaceholderText(tr("Z轴"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_Speed_e)->setText(tr("1000"));
    user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_Speed_e)->setPlaceholderText(tr("速度"));

    m_GcodeSegment_Q = nullptr;
    gp_t = new GcodeParser();
//    m_GcodeSegment_Q = new QQueue<Gcode_segment>();
    GcodeSendThread = new QThread();
    m_motorApp_callback->moveToThread(GcodeSendThread);
    connect(GcodeSendThread,SIGNAL(started()),m_motorApp_callback,SLOT(GcodeSendThread_run()));//Gcode发送线程
    connect(m_motorApp_callback,SIGNAL(Gcode_lineChange(int)),this,SLOT(MotorCallback_GcodeLineChange(int)));//实现Gcode滚动效果

    connect(user_form_controlTab,SIGNAL(Jog_ButtonDown(int)),this,SLOT(ControlTab_jog_clicked(int)));//jog按钮组响应
    connect(m_motorApp_callback,SIGNAL(Gcode_PositionChange(QVector3D)),this,SLOT(MotorCallback_GcodePositionChange(QVector3D)));//设置显示的按钮
    connect(m_motorApp_callback,SIGNAL(Gcode_ThetaChange(QVector3D)),this,SLOT(MotorCallback_GcodeThetaChange(QVector3D)));//设置显示的按钮

    connect(m_motorApp_callback,SIGNAL(Master_QuitSignal(bool)),this,SLOT(MotorCallback_MasterQuit_sig(bool)));
    connect(user_form_controlTab,SIGNAL(Key_EventSignal(QKeyEvent*)),this,SLOT(ControlTab_keyPressEvent(QKeyEvent*)));
}

void ControlTab_P::Destroy_Cores()
{
   Save_setting(m_settingPath);//保存设置
}

/*********************** Operation *************************/
void ControlTab_P::Set_StatusMessage(QString message, int interval)
{
    emit StatusMessage_change(message,interval);//发出自定义信号
}

void ControlTab_P::Set_BottomMessage(QString message)
{
    emit BottomMessage_change(message);//发出自定义信号
}

void ControlTab_P::Set_MasterStop()
{
    emit MasterStop_Signal();//发出自定义信号
}

int ControlTab_P::Load_setting(const QString &path){

//    QFile file("./config.ini");
    QFile file(path);
    if(file.exists()){
        QSettings setting(path,QSettings::IniFormat);//读配置文件

//        QString str_3=setting.value("Login/account").toString();
//        qDebug() << str_3;
        QString setting_GcodePath = setting.value("Path/GcodePath").toString();
//        QString setting_pluginDir =  setting.value("Path/PluginPath").toString();
        QDir dir;
        dir= QDir(setting_GcodePath);
        if(dir.exists()){
            m_GcodePath = setting_GcodePath;
        }
        else{
            QMessageBox::warning(get_UIWidgetPtr(),tr("Path Error!"),"GcodePath is Invalid,loading default path..");
        }

//        m_GcodePath = setting.value("Path/GcodePath").toString();
//        m_pluginDir = setting.value("Path/PluginPath").toString();

//        QString master_adapterName = setting.value("EtherCAT/Adapter_Name").toString();
//        QString master_adapterDesc = setting.value("EtherCAT/Adapter_Desc").toString();
//        user_form_generalTab->setMaster_adapterName(master_adapterName);
//        user_form_generalTab->setMaster_adapterDesc(master_adapterDesc);
//        //bind to master
//        user_form_generalTab->master->m_adapterDescSelect = master_adapterDesc;
//        user_form_generalTab->master->m_adapterNameSelect = master_adapterName;
//        qDebug() << m_GcodePath<<m_pluginDir;
    }
    else{
        m_GcodePath = "./";
//        qDebug() << "Load default setting!";
        Set_StatusMessage(tr("User:Load default setting!"),3000);
    }

    return 0;
}

int ControlTab_P::Save_setting(const QString &path){

   QSettings setting(path,QSettings::IniFormat);//读配置文件

   setting.beginGroup(tr("Path"));
   setting.setValue("GcodePath",m_GcodePath);//设置key和value，也就是参数和值
//   setting.setValue("PluginPath",m_pluginDir);
//   setting.setValue("remeber",true);
   setting.endGroup();//节点结束

    return 0;
}

int ControlTab_P::Gcode_load(QString &fileName){
    if(!fileName.isEmpty()){
        //m_pluginDir = dir;
  //        qDebug() << m_pluginDir;
//        qDebug() << dir;
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(get_UIWidgetPtr(), tr("ControlTab_P"), tr("Can't open file:\n") + fileName);
            return -1;
        }




//        // Set filename
//        m_programFileName = fileName;

        // Prepare text stream
        QTextStream textStream(&file);

        // Read lines
        QList<QString> data;
        while (!textStream.atEnd()) data.append(textStream.readLine());

//        qDebug() << data;

        //读取信息后，进行G代码的解析
        // Prepare parser
//        GcodeParser gp;
//        GcodeParser *gp_t = new GcodeParser();

        QProgressDialog progress(tr("Opening file..."), tr("Abort"), 0, data.count(), get_UIWidgetPtr());
        progress.setWindowModality(Qt::WindowModal);
        progress.setFixedSize(progress.sizeHint());
        if (data.count() > PROGRESSMINLINES) {//如果行数较大，就进行拆分
            progress.show();
            progress.setStyleSheet("QProgressBar {text-align: center; qproperty-format: \"\"}");
        }

        QString command;
        QString stripped;
        QString trimmed;
        QList<QString> args;

//        GCodeItem item;
        gp_t->clearQueue();
        user_form_controlTab->get_TableGcode()->clearContents();
        int line_num = 0;
        QTableWidgetItem *tableItem;
        bool isCommentLine = false;
        while (!data.isEmpty())
        {
            command = data.takeFirst();

            // Trim command
            trimmed = command.trimmed();//移除字符串两端的空白字符

            if (!trimmed.isEmpty()) {
                // Split command
                stripped = GcodePreprocessorUtils::removeComment(command);
                if(stripped.isEmpty()){
                    isCommentLine = true;
                }
                else{
                    isCommentLine = false;
                }
//                qDebug() << stripped;
                args = GcodePreprocessorUtils::splitCommand(stripped);

//                gp.addCommand(args);//里面包含了handle
                gp_t->addCommand(args);//处理命令
//                m_GcodeSegment_Q = gp_t->getGodeQueue();
                if(isCommentLine){//空的表示是注释行
                    Gcode_segment segment;//插入M代码，让行对应
                    segment.line = gp_t->getGodeQueue()->size();
                    segment.data_xyz = QVector3D(0,0,0);//注释的M代码位置无效
                    segment.Mcode = Gcode_segment::COMMENT_CODE;
                    gp_t->getGodeQueue()->enqueue(segment);

                }
//                while(!gp_t->getGodeQueue()->empty())
//                qDebug() << gp_t->getGodeQueue()->dequeue().data_xyz;
                  //添加到table中
                user_form_controlTab->get_TableGcode()->setRowCount(1+line_num);
                tableItem = new QTableWidgetItem(QString::number(line_num));
                tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                user_form_controlTab->get_TableGcode()->setItem(line_num,0,tableItem);

                tableItem = new QTableWidgetItem(trimmed);
                tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                user_form_controlTab->get_TableGcode()->setItem(line_num,1,tableItem);

                tableItem = new QTableWidgetItem(tr("Ready"));
                tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                user_form_controlTab->get_TableGcode()->setItem(line_num++,2,tableItem);

//                item.command = trimmed;
//                item.state = GCodeItem::InQueue;
//                item.line = gp.getCommandNumber();
//                item.args = args;

//                m_programModel.data().append(item);
            }

            if (progress.isVisible() && (data.count() % PROGRESSSTEP == 0)) {
                progress.setValue(progress.maximum() - data.count());
                qApp->processEvents();
                if (progress.wasCanceled()) break;
            }
        }
        progress.close();

        m_GcodeSegment_Q = gp_t->getGodeQueue();//传递指针
        //加一行，表示end
        user_form_controlTab->get_TableGcode()->setRowCount(1+line_num);
        tableItem = new QTableWidgetItem(QString::number(line_num));
        tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        user_form_controlTab->get_TableGcode()->setItem(line_num,0,tableItem);

        tableItem = new QTableWidgetItem(tr("End"));
        tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        user_form_controlTab->get_TableGcode()->setItem(line_num,1,tableItem);

        tableItem = new QTableWidgetItem(tr("Ready"));
        tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        user_form_controlTab->get_TableGcode()->setItem(line_num++,2,tableItem);

        Gcode_segment segment;//插入M代码，让行对应
        segment.line = gp_t->getGodeQueue()->size();
        segment.data_xyz = QVector3D(0,0,0);//M代码位置无效
        segment.Mcode = Gcode_segment::EndParse_CODE;
        gp_t->getGodeQueue()->enqueue(segment);


////实现滚动效果
//m_table_Gcode->selectRow(250);
//m_table_Gcode->scrollTo(m_table_Gcode->model()->index(250,1));

//        // Load lines
//        loadFile(data);
//        while(!gp_t->getGodeQueue()->empty())
//        qDebug() << "xx:"<<gp_t->getGodeQueue()->dequeue().data_xyz;
//        if(gp_t->getGodeQueue()->empty()){
//            qDebug() << "mainview_center Q empty!";
//        }
//        else{
//            qDebug() << "mainview_center Q No empty!";
//        }
    }

    return 0;
}

/************  Slots *******************/

void ControlTab_P::Control_OpenGcode_clicked(){
//    if(user_form_generalTab->master->Master_getSlaveCount()>0){
//       Master_stop();//防止界面卡死
//       StatusMessage_change(tr("Stop Master..."),3000);
//    }
//    StatusMessage_change(tr("Stop Master..."),3000);
    if(m_motorApp_callback->m_slaveCount > 0){
        Set_MasterStop();
        Set_StatusMessage(tr("Stop Master..."),3000);
    }

    //WARNING:看看是否需要延时

    QString fileName = "";
    fileName  = QFileDialog::getOpenFileName(get_UIWidgetPtr(), tr("Open Gcode"), m_GcodePath,
                               tr("G-Code files (*.nc *.ncc *.ngc *.tap *.txt);;All files (*.*)"));//如果没有选择路径就会为空
    if(!fileName.isEmpty()){
        m_GcodePath_full = fileName;
        //分离path和fileName
        QFileInfo fileInfo = QFileInfo(fileName);
//        qDebug() << fileInfo.fileName() <<fileInfo.absolutePath();
        m_GcodePath = fileInfo.absolutePath();

        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Gcode_filePath_e)->setText(fileName);
        Set_BottomMessage(tr("OPen Gcode file OK!"));
    }

    Gcode_load(m_GcodePath_full);
}

void ControlTab_P::Control_ReloadGcode_clicked(){
//    if(user_form_generalTab->master->Master_getSlaveCount()>0){
//       Master_stop();//防止界面卡死
//       StatusMessage_change(tr("Stop Master..."),3000);
//    }

    if(m_motorApp_callback->m_slaveCount > 0){
        Set_MasterStop();
        Set_StatusMessage(tr("Stop Master..."),3000);
    }

    int ret = Gcode_load(m_GcodePath_full);
    if(ret ==0){
        Set_BottomMessage(tr("Reload Gcode file OK!"));
    }
}

void ControlTab_P::Control_SendGcode_clicked(){
   GcodeSendThread->start();//开始解析G代码线程
   m_motorApp_callback->m_RenewST_ready = true;
}

void ControlTab_P::MotorCallback_GcodeLineChange(int line){
//    qDebug() << line;
    //实现滚动效果
    user_form_controlTab->get_TableGcode()->selectRow(line);
    user_form_controlTab->get_TableGcode()->scrollTo(user_form_controlTab->get_TableGcode()->model()->index(line,QTableView::EnsureVisible));
    //改变状态
    int current_row = user_form_controlTab->get_TableGcode()->currentRow();
    QTableWidgetItem *tableItem = new QTableWidgetItem(tr("OK"));
    tableItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    user_form_controlTab->get_TableGcode()->setItem(current_row,2,tableItem);

}

void ControlTab_P::MotorCallback_GcodePositionChange(QVector3D pos){
    if(!controlTab_isTheta_display){
        user_form_controlTab->set_LCDnumber_Display(Form_ControlTab::Axis_X,pos.x());
        user_form_controlTab->set_LCDnumber_Display(Form_ControlTab::Axis_Y,pos.y());
        user_form_controlTab->set_LCDnumber_Display(Form_ControlTab::Axis_Z,pos.z());
    }
}

void ControlTab_P::MotorCallback_GcodeThetaChange(QVector3D theta){
    if(controlTab_isTheta_display){
        user_form_controlTab->set_LCDnumber_Display(Form_ControlTab::Axis_X,theta.x());
        user_form_controlTab->set_LCDnumber_Display(Form_ControlTab::Axis_Y,theta.y());
        user_form_controlTab->set_LCDnumber_Display(Form_ControlTab::Axis_Z,theta.z());
    }
}

void ControlTab_P::ControlTab_checkThetaDis_stateChange(int arg){
    controlTab_isTheta_display = arg;
}

void ControlTab_P::ControlTab_jog_clicked(int button){
    QVector3D coor_temp;

    switch(button){
       case Form_ControlTab::Jog_AxisX_P_b://左
        coor_temp.setY(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->text().toFloat()-user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(QString("%1").arg(coor_temp.y()));

        break;
        case Form_ControlTab::Jog_AxisX_N_b://右
        coor_temp.setY(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->text().toFloat()+user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(QString("%1").arg(coor_temp.y()));
         break;
        case Form_ControlTab::Jog_AxisY_P_b://前
        coor_temp.setX(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->text().toFloat()+user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(QString("%1").arg(coor_temp.x()));
         break;
        case Form_ControlTab::Jog_AxisY_N_b://后
        coor_temp.setX(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->text().toFloat()-user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(QString("%1").arg(coor_temp.x()));
         break;
        case Form_ControlTab::Jog_AxisZ_P_b://上
        coor_temp.setZ(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->text().toFloat()+user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(QString("%1").arg(coor_temp.z()));
         break;
        case Form_ControlTab::Jog_AxisZ_N_b://下
        coor_temp.setZ(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->text().toFloat()-user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(QString("%1").arg(coor_temp.z()));
         break;
        case Form_ControlTab::Jog_Home_b:
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(QString("%1").arg(280.80));
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(QString("%1").arg(0));
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(QString("%1").arg(155));
        break;
        case Form_ControlTab::Jog_Halt_b:
        memset(m_motorApp_callback->loop_count,0,sizeof(m_motorApp_callback->loop_count));//stop
        m_motorApp_callback->m_Stepper_block_Q->clear();
        m_motorApp_callback->m_sys_reset = true;
        return;

        break;
        default:
            break;
    }

    m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_X] = user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->text().toFloat();//300;
    m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Y] = user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->text().toFloat();//200;
    m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Z] = user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->text().toFloat();//100;
    m_motorApp_callback->Planner_BufferLine(m_motorApp_callback->m_ARM_Motion_test.arm,Gcode_segment::No_Mcode);
    m_motorApp_callback->m_sys_reset = false;
}

void ControlTab_P::MotorCallback_MasterQuit_sig(bool isQuit){
    if(isQuit){
        GcodeSendThread->quit();
        GcodeSendThread->wait();
        user_form_controlTab->get_GroupPtr(Form_ControlTab::Groups_jog_g)->setEnabled(false);
        user_form_controlTab->get_ButtonGcode(Form_ControlTab::Gcode_sendFile_b)->setEnabled(false);
        m_motorApp_callback->Gcode_ReleaseAddress();
    }
    else{
        user_form_controlTab->get_GroupPtr(Form_ControlTab::Groups_jog_g)->setEnabled(true);
        user_form_controlTab->get_ButtonGcode(Form_ControlTab::Gcode_sendFile_b)->setEnabled(true);
        m_motorApp_callback->Gcode_setAddress(m_GcodeSegment_Q);

        get_UIWidgetPtr()->setFocus();//如果用键盘测试的话，需要设置焦点
    }

}

void ControlTab_P::ControlTab_keyPressEvent(QKeyEvent *event){
    if(m_motorApp_callback->input_ptr == NULL){
        return;
    }

    QVector3D coor_temp;

    switch(event->key()){
    case Qt::Key_Space:
        memset(m_motorApp_callback->loop_count,0,sizeof(m_motorApp_callback->loop_count));//stop
        m_motorApp_callback->m_Stepper_block_Q->clear();
        m_motorApp_callback->m_sys_reset = true;
        break;
    case Qt::Key_Control:
        m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_X] = user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->text().toFloat();//300;
        m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Y] = user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->text().toFloat();//200;
        m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Z] = user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->text().toFloat();//100;
        m_motorApp_callback->Planner_BufferLine(m_motorApp_callback->m_ARM_Motion_test.arm,Gcode_segment::No_Mcode);
        m_motorApp_callback->m_sys_reset = false;
//        m_motorApp_callback->start();
        break;
     case Qt::Key_R:
            m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_X] = 280.80;
            m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Y] = 0;
            m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Z] = 155;
//            m_motorApp_callback->start();
            m_motorApp_callback->Planner_BufferLine(m_motorApp_callback->m_ARM_Motion_test.arm,Gcode_segment::No_Mcode);
            m_motorApp_callback->m_sys_reset = false;

        break;
     case Qt::Key_S:
//        if(m_motorApp_callback->m_sys_reset){
//            while(!m_GcodeSegment_Q->empty()){
//                QVector3D data = m_GcodeSegment_Q->dequeue().data_xyz;
//                data_last = data;
//                qDebug() << data;
//                m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_X] = data.x();
//                m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Y] = data.y();
//                m_motorApp_callback->m_ARM_Motion_test.arm[My_MotorApp_Callback::AXIS_Z] = data.z();
//                m_motorApp_callback->Planner_BufferLine(m_motorApp_callback->m_ARM_Motion_test.arm,0);
//            }
//            m_motorApp_callback->m_sys_reset = false;
//            m_motorApp_callback->start();
//            m_motorApp_callback->loop_count[0] = 1;
//        }
            GcodeSendThread->start();
        break;
   case Qt::Key_Q:
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(tr("280.799"));
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(tr("0"));
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(tr("155"));
        break;
    case Qt::Key_A:
        m_motorApp_callback->m_RenewST_ready = true;
        break;
    case Qt::Key_Left://左
        coor_temp.setY(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->text().toFloat()-user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(QString("%1").arg(coor_temp.y()));
        break;
    case Qt::Key_Right://右
        coor_temp.setY(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->text().toFloat()+user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosY_e)->setText(QString("%1").arg(coor_temp.y()));

        break;
    case Qt::Key_Up://前
        coor_temp.setX(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->text().toFloat()+user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(QString("%1").arg(coor_temp.x()));

        break;
    case Qt::Key_Down://后
        coor_temp.setX(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->text().toFloat()-user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosX_e)->setText(QString("%1").arg(coor_temp.x()));

        break;
    case Qt::Key_Z://上
        coor_temp.setZ(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->text().toFloat()+user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(QString("%1").arg(coor_temp.z()));

        break;
    case Qt::Key_X://下
        coor_temp.setZ(user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->text().toFloat()-user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_step_e)->text().toInt());
        user_form_controlTab->get_LineEditGcode(Form_ControlTab::Jog_PosZ_e)->setText(QString("%1").arg(coor_temp.z()));

        break;
    default:
        qDebug() << event->key();
        break;
    }
}

/************  Slots End ***************/
