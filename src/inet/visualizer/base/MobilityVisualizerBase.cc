//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/common/ModuleAccess.h"
#include "inet/visualizer/base/MobilityVisualizerBase.h"

namespace inet {

namespace visualizer {

MobilityVisualizerBase::MobilityVisualization::MobilityVisualization(IMobility *mobility) :
    mobility(mobility)
{
}

MobilityVisualizerBase::~MobilityVisualizerBase()
{
    if (displayMovement)
        unsubscribe();
}

void MobilityVisualizerBase::initialize(int stage)
{
    VisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        subscriptionModule = getModuleFromPar<cModule>(par("subscriptionModule"), this);
        displayMovement = par("displayMovement");
        moduleFilter.setPattern(par("moduleFilter"));
        // orientation
        displayOrientation = par("displayOrientation");
        orientationArcSize = par("orientationArcSize");
        orientationLineColor = cFigure::parseColor(par("orientationLineColor"));
        orientationLineWidth = par("orientationLineWidth");
        // velocity
        displayVelocity = par("displayVelocity");
        velocityArrowScale = par("velocityArrowScale");
        velocityLineColor = cFigure::parseColor(par("velocityLineColor"));
        velocityLineWidth = par("velocityLineWidth");
        velocityLineStyle = cFigure::parseLineStyle(par("velocityLineStyle"));
        // movement trail
        displayMovementTrail = par("displayMovementTrail");
        const char *movementTrailLineColorString = par("movementTrailLineColor");
        autoMovementTrailLineColor = !strcmp("auto", movementTrailLineColorString);
        if (!autoMovementTrailLineColor)
            movementTrailLineColor = cFigure::parseColor(movementTrailLineColorString);
        movementTrailLineWidth = par("movementTrailLineWidth");
        trailLength = par("trailLength");
        if (displayMovement)
            subscribe();
    }
}

void MobilityVisualizerBase::subscribe()
{
    subscriptionModule->subscribe(IMobility::mobilityStateChangedSignal, this);
}

void MobilityVisualizerBase::unsubscribe()
{
    // NOTE: lookup the module again because it may have been deleted first
    subscriptionModule = getModuleFromPar<cModule>(par("subscriptionModule"), this, false);
    if (subscriptionModule != nullptr)
        subscriptionModule->unsubscribe(IMobility::mobilityStateChangedSignal, this);
}

} // namespace visualizer

} // namespace inet

