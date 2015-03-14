#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDateTime>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
	Q_OBJECT

public:
	explicit Widget(QWidget *parent = 0);
	~Widget();

private:
	Ui::Widget *ui;
	void readCmdrLog();
	void scanDirectoryLogs();
	void parseLog(QString elite_path);
	void writeCmdrLog();
	void checkUniqueSystem(QString MySystem);
	void updateSystemsVisited();
	void getLogDirectory();
	QString timeUTCtoString();
	QString extractSystemName(QString line);
	QString MySystem,CurrentLogName;
	QStringList uniqueSystems;
	int timerId;
	bool fileChangedOrNot(QString elite_file);
	QDateTime created,oldfiletime;
	int savedHammers,numSessionSystems;
	qint64 sizeOfLog,sizeOfOldLog,filePos;
	QString logDirectory;

protected:
	void timerEvent(QTimerEvent *event);
private slots:
	void on_pushButton_clicked();
};

#endif // WIDGET_H
