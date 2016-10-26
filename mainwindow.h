#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow *ui;
	void readCmdrLog();
	void scanDirectoryLogs();
	void parseLog(QString elite_path);
	void writeCmdrLog();
	void checkUniqueSystem(QString MySystem);
	void updateSystemsVisited();
	void readEliteCFG();
	void saveEliteCFG();
	void parseSystemsJSON(QByteArray line);
	QString timeUTCtoString();
	QString extractSystemName(QString line);
	QString extractStationName(QString line);
	QString MySystem,MyStation,CurrentLogName;
	QString JumpDist,FuelUsed,FuelLevel;
	QStringList uniqueSystems;
	int timerId;
	bool fileChangedOrNot(QString elite_file);
	QDateTime created,oldfiletime;
	int savedHammers,numSessionSystems,numSessionSystemsRecord,numAllSystems,deaths,fuelLevelLowest;
	double scoopedTotal,JumpDistShortest,JumpDistLongest,DistanceFromArrivalLSMin,DistanceFromArrivalLSMax;
	double planetRadiusSmallest,planetRadiusLargest,surfaceGravityLowest,surfaceGravityHighest;
	double landableRadiusSmallest,landableRadiusLargest,landableGravityLowest,landableGravityHighest;
	double stellarMassLowest,stellarMassHighest,stellarRadiusSmallest,stellarRadiusLargest;
	double age_MYyoungest,age_MYoldest;
	qint64 sizeOfLog,sizeOfOldLog,filePos;
	QString logDirectory,eliteLogVersion,cmdrLogFileName,numSessionSystemsRecordDate;

protected:
	void timerEvent(QTimerEvent *event);
private slots:
	void on_pushButton_clicked();
	void on_pushButton_2_clicked();
	void on_pushButton_3_clicked();
};

#endif // MAINWINDOW_H
