#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <QMainWindow>
#include <Projects/Inspector/ui_mainwindow.h>

class ezMainWindow : public QMainWindow, public Ui_MainWindow
{
public:
  Q_OBJECT

public:
  ezMainWindow();

  static ezMainWindow* s_pWidget;

  void paintEvent(QPaintEvent* event) EZ_OVERRIDE;

  void SaveLayout (const char* szFile) const;
  void LoadLayout (const char* szFile);

  void Log(const char* szMsg);

private slots:

private:

  struct LogMessage
  {
    ezString m_sMsg;
  };

  ezDeque<LogMessage> m_LogList;
};


