#pragma once

#include "QGCCorePlugin.h"

/// @brief Custom QGC plugin implementation
class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

    public:
        CustomPlugin();

        // QGCCorePlugin interface
        QString brandImageIndoor() const override;
        QString brandImageOutdoor() const override;

    protected:
        bool _showAdvancedUI = false;
};