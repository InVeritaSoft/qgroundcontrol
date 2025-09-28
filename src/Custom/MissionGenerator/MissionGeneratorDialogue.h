#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class MissionService;
class VehicleService;

class MissionGeneratorDialogue : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MissionGeneratorDialogue(QObject* parent = nullptr);
    ~MissionGeneratorDialogue();

    Q_INVOKABLE void generateMission(const QString& missionType, 
                                   int areaSize, 
                                   int altitude, 
                                   double speed, 
                                   const QString& description);

    Q_INVOKABLE void getAllVehicles();

signals:
    void missionGenerated(const QString& missionType, 
                         int areaSize, 
                         int altitude, 
                         double speed, 
                         const QString& description);

    void vehicleInfoReady(const QString& vehicleInfo);
    void vehicleDataReady(const QVariantList& vehicleList);
    
    // New signals for better UI feedback
    void missionGenerationStarted();
    void missionGenerationProgress(int current, int total);
    void missionGenerationCompleted(bool success, const QString& message);

private slots:
    void onMissionGenerationCompleted(bool success, const QString& message);
    void onMissionGenerationProgress(int current, int total);

private:
    MissionService* m_missionService;
    VehicleService* m_vehicleService;
};
