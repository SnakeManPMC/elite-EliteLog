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
	explicit MainWindow(QWidget *parent = nullptr);
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
	QStringList uniqueSystems,sessionScanBodies;
	int timerId,cargoValue,lastTrade,sessionTrades;
	bool fileChangedOrNot(QString elite_file);
	QDateTime created,oldfiletime;
	int savedHammers,numSessionSystems,numSessionSystemsRecord,numAllSystems,deaths,numSessionScans,numSessionAmmonia,numSessionEarth,numSessionWater,credits;
	float scoopedTotal,JumpDistShortest,JumpDistLongest,DistanceFromArrivalLSMin,DistanceFromArrivalLSMax;
	float planetRadiusSmallest,planetRadiusLargest,surfaceGravityLowest,surfaceGravityHighest;
	float landableRadiusSmallest,landableRadiusLargest,landableGravityLowest,landableGravityHighest;
	float stellarMassLowest,stellarMassHighest,stellarRadiusSmallest,stellarRadiusLargest;
	float age_MYyoungest,age_MYoldest,fuelLevelLowest,sessionPlanetSmallest,sessionPlanetLargest;
	qint64 sizeOfLog,sizeOfOldLog,filePos;
	QString logDirectory,eliteLogVersion,cmdrLogFileName,numSessionSystemsRecordDate;

	void setupTableWidget();
	void updateTableView(const QString& date, const QString& event, const QString& details);
	QString ttime, tevent, tdetails;
	void addSpecialPlanets(QString planetClass, QString bodyname, float distancels, float radius);

protected:
	void timerEvent(QTimerEvent *event);
private slots:
	void on_pushButton_clicked();
	void on_pushButton_2_clicked();
	void on_pushButton_3_clicked();
	void on_pushButton_UTCArrivedAtSystem_clicked();
	void on_pushButton_CreditsToClipboard_clicked();
};

#endif // MAINWINDOW_H
