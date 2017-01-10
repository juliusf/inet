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

#include "inet/common/figures/SignalFigure.h"
#include "inet/common/ModuleAccess.h"
#include "inet/visualizer/physicallayer/MediumCanvasVisualizer.h"

namespace inet {

namespace visualizer {

Define_Module(MediumCanvasVisualizer);

void MediumCanvasVisualizer::initialize(int stage)
{
    MediumVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        zIndex = par("zIndex");
        const char *signalShapeString = par("signalShape");
        if (!strcmp(signalShapeString, "ring"))
            signalShape = SIGNAL_SHAPE_RING;
        else if (!strcmp(signalShapeString, "sphere"))
            signalShape = SIGNAL_SHAPE_SPHERE;
        else
            throw cRuntimeError("Unknown signalShape parameter value: '%s'", signalShapeString);
        signalOpacity = par("signalOpacity");
        signalColor = par("signalColor");
        signalRingCount = par("signalRingCount");
        signalRingSize = par("signalRingSize");
        signalFadingDistance = par("signalFadingDistance");
        signalFadingFactor = par("signalFadingFactor");
        signalWaveCount = par("signalWaveCount");
        signalWaveLength = par("signalWaveLength");
        signalWaveWidth = par("signalWaveWidth");
        signalWaveFadingAnimationSpeedFactor = par("signalWaveFadingAnimationSpeedFactor");
        cCanvas *canvas = visualizerTargetModule->getCanvas();
        if (displaySignals) {
            signalLayer = new cGroupFigure("communication");
            signalLayer->setZIndex(zIndex);
            signalLayer->insertBefore(canvas->getSubmodulesLayer());
        }
        displayCommunicationHeat = par("displayCommunicationHeat");
        if (displayCommunicationHeat) {
            communicationHeat = new HeatMapFigure(communicationHeatMapSize, "communication heat");
            communicationHeat->setZIndex(zIndex);
            communicationHeat->setTags("successful_reception heat");
            canvas->addFigure(communicationHeat, 0);
        }
        networkNodeVisualizer = getModuleFromPar<NetworkNodeCanvasVisualizer>(par("networkNodeVisualizerModule"), this);
    }
    else if (stage == INITSTAGE_LAST) {
        canvasProjection = CanvasProjection::getCanvasProjection(visualizerTargetModule->getCanvas());
        if (communicationHeat != nullptr) {
            const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
            Coord min = mediumLimitCache->getMinConstraintArea();
            Coord max = mediumLimitCache->getMaxConstraintArea();
            cFigure::Point o = canvasProjection->computeCanvasPoint(Coord::ZERO);
            cFigure::Point x = canvasProjection->computeCanvasPoint(Coord(1, 0, 0));
            cFigure::Point y = canvasProjection->computeCanvasPoint(Coord(0, 1, 0));
            double t1 = o.x;
            double t2 = o.y;
            double a = x.x - t1;
            double b = x.y - t2;
            double c = y.x - t1;
            double d = y.y - t2;
            communicationHeat->setTransform(cFigure::Transform(a, b, c, d, t1, t2));
            communicationHeat->setPosition(cFigure::Point((min.x + max.x) / 2, (min.y + max.y) / 2));
            communicationHeat->setWidth(max.x - min.x);
            communicationHeat->setHeight(max.y - min.y);
        }
    }
}

void MediumCanvasVisualizer::refreshDisplay() const
{
    if (displaySignals)
        for (auto transmission : transmissions)
            if (matchesTransmission(transmission))
                refreshSignalFigure(transmission);
    if (displayCommunicationHeat)
        communicationHeat->coolDown();
}

void MediumCanvasVisualizer::setAnimationSpeed() const
{
    double animationSpeed = DBL_MAX;
    if (displaySignals) {
        for (auto transmission : transmissions) {
            if (matchesTransmission(transmission)) {
                if (isSignalPropagationInProgress(transmission))
                    animationSpeed = std::min(animationSpeed, signalPropagationAnimationSpeed);
                else if (isSignalTransmissionInProgress(transmission))
                    animationSpeed = std::min(animationSpeed, signalTransmissionAnimationSpeed);
            }
        }
    }
    animationSpeed = animationSpeed == DBL_MAX ? 0 : animationSpeed;
    visualizerTargetModule->getCanvas()->setAnimationSpeed(animationSpeed, this);
}

cFigure *MediumCanvasVisualizer::getRadioFigure(const IRadio *radio) const
{
    auto it = radioFigures.find(radio);
    if (it == radioFigures.end())
        return nullptr;
    else
        return it->second;
}

void MediumCanvasVisualizer::setRadioFigure(const IRadio *radio, cFigure *figure)
{
    radioFigures[radio] = figure;
}

cFigure *MediumCanvasVisualizer::removeRadioFigure(const IRadio *radio)
{
    auto it = radioFigures.find(radio);
    if (it == radioFigures.end())
        return nullptr;
    else {
        radioFigures.erase(it);
        return it->second;
    }
}

cFigure *MediumCanvasVisualizer::getSignalFigure(const ITransmission *transmission) const
{
    auto it = signalFigures.find(transmission);
    if (it == signalFigures.end())
        return nullptr;
    else
        return it->second;
}

void MediumCanvasVisualizer::setSignalFigure(const ITransmission *transmission, cFigure *figure)
{
    signalFigures[transmission] = figure;
}

cFigure *MediumCanvasVisualizer::removeSignalFigure(const ITransmission *transmission)
{
    auto it = signalFigures.find(transmission);
    if (it == signalFigures.end())
        return nullptr;
    else {
        signalFigures.erase(it);
        return it->second;
    }
}

cGroupFigure* MediumCanvasVisualizer::createSignalFigure(const ITransmission* transmission) const
{
    cFigure::Point position = canvasProjection->computeCanvasPoint( transmission->getStartPosition());
    cGroupFigure* groupFigure = new cGroupFigure("signal");
    cFigure::Color color;
    if (!strcmp(signalColor, "auto"))
        color = cFigure::GOOD_DARK_COLORS[transmission->getId() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
    else
        color = cFigure::parseColor(signalColor);
    SignalFigure* signalFigure = new SignalFigure("bubble");
    signalFigure->setTags("propagating_signal");
    signalFigure->setTooltip("These rings represents a signal propagating through the medium");
    signalFigure->setRingCount(signalRingCount);
    signalFigure->setRingSize(signalRingSize);
    signalFigure->setFadingDistance(signalFadingDistance);
    signalFigure->setFadingFactor(signalFadingFactor);
    signalFigure->setWaveCount(signalWaveCount);
    signalFigure->setWaveLength(signalWaveLength);
    signalFigure->setWaveWidth(signalWaveWidth);
    signalFigure->setOpacity(signalOpacity);
    signalFigure->setColor(color);
    signalFigure->setBounds(cFigure::Rectangle(position.x, position.y, 0, 0));
    signalFigure->refresh();
    groupFigure->addFigure(signalFigure);
    cLabelFigure* nameFigure = new cLabelFigure("name");
    nameFigure->setPosition(position);
    nameFigure->setTags("propagating_signal packet_name label");
    nameFigure->setText(transmission->getMacFrame()->getName());
    nameFigure->setColor(color);
    groupFigure->addFigure(nameFigure);
    return groupFigure;
}

void MediumCanvasVisualizer::refreshSignalFigure(const ITransmission *transmission) const
{
    const IPropagation *propagation = radioMedium->getPropagation();
    cFigure *groupFigure = getSignalFigure(transmission);
    double startRadius = propagation->getPropagationSpeed().get() * (simTime() - transmission->getStartTime()).dbl();
    double endRadius = std::max(0.0, propagation->getPropagationSpeed().get() * (simTime() - transmission->getEndTime()).dbl());
    if (groupFigure) {
        SignalFigure *signalFigure = static_cast<SignalFigure *>(groupFigure->getFigure(0));
        cLabelFigure *labelFigure = static_cast<cLabelFigure *>(groupFigure->getFigure(1));
        double phi = transmission->getId();
        labelFigure->setTransform(cFigure::Transform().translate(endRadius * sin(phi), endRadius * cos(phi)));
        const Coord transmissionStart = transmission->getStartPosition();
        // KLUDGE: to workaround overflow bugs in drawing
        double offset = std::fmod(startRadius, signalFigure->getWaveLength());
        if (startRadius > 10000)
            startRadius = 10000;
        if (endRadius > 10000)
            endRadius = 10000;
        switch (signalShape) {
            case SIGNAL_SHAPE_RING: {
                // determine the rotated 2D canvas points by computing the 2D affine trasnformation from the 3D transformation of the environment
                cFigure::Point o = canvasProjection->computeCanvasPoint(transmissionStart);
                cFigure::Point x = canvasProjection->computeCanvasPoint(transmissionStart + Coord(1, 0, 0));
                cFigure::Point y = canvasProjection->computeCanvasPoint(transmissionStart + Coord(0, 1, 0));
                double t1 = o.x;
                double t2 = o.y;
                double a = x.x - t1;
                double b = x.y - t2;
                double c = y.x - t1;
                double d = y.y - t2;
                signalFigure->setTransform(cFigure::Transform(a, b, c, d, t1, t2));
                signalFigure->setBounds(cFigure::Rectangle(-startRadius, -startRadius, startRadius * 2, startRadius * 2));
                signalFigure->setInnerRx(endRadius);
                signalFigure->setInnerRy(endRadius);
                signalFigure->setWaveOffset(offset);
                signalFigure->setWaveOpacityFactor(std::min(1.0, signalPropagationAnimationSpeed / getSimulation()->getEnvir()->getAnimationSpeed() / signalWaveFadingAnimationSpeedFactor) / 2);
                signalFigure->refresh();
                break;
            }
            case SIGNAL_SHAPE_SPHERE: {
                // a sphere looks like a circle from any view angle
                cFigure::Point center = canvasProjection->computeCanvasPoint(transmissionStart);
                signalFigure->setBounds(cFigure::Rectangle(center.x - startRadius, center.y - startRadius, 2 * startRadius, 2 * startRadius));
                signalFigure->setInnerRx(endRadius);
                signalFigure->setInnerRy(endRadius);
                break;
            }
            default:
                throw cRuntimeError("Unimplemented signal shape");
        }
    }
}

void MediumCanvasVisualizer::radioAdded(const IRadio *radio)
{
    Enter_Method_Silent();
    auto module = check_and_cast<const cModule *>(radio);
    auto networkNode = getContainingNode(module);
    if (networkNodeFilter.matches(networkNode)) {
        if (displayInterferenceRanges || (module->hasPar("displayInterferenceRange") && module->par("displayInterferenceRange"))) {
            auto networkNodeVisualization = networkNodeVisualizer->getNeworkNodeVisualization(networkNode);
            auto interferenceRangeFigure = new cOvalFigure("interferenceRange");
            m maxInterferenceRange = check_and_cast<const IRadioMedium *>(radio->getMedium())->getMediumLimitCache()->getMaxInterferenceRange(radio);
            interferenceRangeFigure->setTags("interference_range");
            interferenceRangeFigure->setTooltip("This circle represents the interference range of a wireless interface");
            interferenceRangeFigure->setBounds(cFigure::Rectangle(-maxInterferenceRange.get(), -maxInterferenceRange.get(), 2 * maxInterferenceRange.get(), 2 * maxInterferenceRange.get()));
            interferenceRangeFigure->setLineColor(interferenceRangeLineColor);
            interferenceRangeFigure->setLineStyle(interferenceRangeLineStyle);
            interferenceRangeFigure->setLineWidth(interferenceRangeLineWidth);
            networkNodeVisualization->addFigure(interferenceRangeFigure);
        }
        if (displayCommunicationRanges || (module->hasPar("displayCommunicationRange") && module->par("displayCommunicationRange"))) {
            auto networkNodeVisualization = networkNodeVisualizer->getNeworkNodeVisualization(networkNode);
            auto communicationRangeFigure = new cOvalFigure("communicationRange");
            m maxCommunicationRange = check_and_cast<const IRadioMedium *>(radio->getMedium())->getMediumLimitCache()->getMaxCommunicationRange(radio);
            communicationRangeFigure->setTags("communication_range");
            communicationRangeFigure->setTooltip("This circle represents the communication range of a wireless interface");
            communicationRangeFigure->setBounds(cFigure::Rectangle(-maxCommunicationRange.get(), -maxCommunicationRange.get(), 2 * maxCommunicationRange.get(), 2 * maxCommunicationRange.get()));
            communicationRangeFigure->setLineColor(communicationRangeLineColor);
            communicationRangeFigure->setLineStyle(communicationRangeLineStyle);
            communicationRangeFigure->setLineWidth(communicationRangeLineWidth);
            networkNodeVisualization->addFigure(communicationRangeFigure);
        }
        if (displayTransmissions || displayReceptions) {
            auto networkNodeVisualization = networkNodeVisualizer->getNeworkNodeVisualization(networkNode);
            auto group = new cGroupFigure();
            cFigure::Rectangle bounds;
            if (displayTransmissions) {
                std::string imageName = par("transmissionImage");
                auto transmissionImage = new cIconFigure();
                transmissionImage->setTags("transmission");
                transmissionImage->setTooltip("This icon represents an ongoing transmission in a wireless interface");
                transmissionImage->setImageName(imageName.substr(0, imageName.find_first_of(".")).c_str());
                transmissionImage->setAnchor(cFigure::ANCHOR_NW);
                transmissionImage->setVisible(false);
                group->addFigure(transmissionImage);
                bounds = transmissionImage->getBounds();
            }
            if (displayReceptions) {
                std::string imageName = par("receptionImage");
                auto receptionImage = new cIconFigure();
                receptionImage->setTags("reception");
                receptionImage->setTooltip("This icon represents an ongoing reception in a wireless interface");
                receptionImage->setImageName(imageName.substr(0, imageName.find_first_of(".")).c_str());
                receptionImage->setAnchor(cFigure::ANCHOR_NW);
                receptionImage->setVisible(false);
                group->addFigure(receptionImage);
                bounds = receptionImage->getBounds();
            }
            networkNodeVisualization->addAnnotation(group, bounds.getSize());
            setRadioFigure(radio, group);
        }
    }
}

void MediumCanvasVisualizer::radioRemoved(const IRadio *radio)
{
    Enter_Method_Silent();
    auto figure = removeRadioFigure(radio);
    if (figure != nullptr) {
        auto module = const_cast<cModule *>(check_and_cast<const cModule *>(radio));
        auto networkNodeVisualization = networkNodeVisualizer->getNeworkNodeVisualization(getContainingNode(module));
        networkNodeVisualization->removeAnnotation(figure);
    }
}

void MediumCanvasVisualizer::transmissionAdded(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (displaySignals && matchesTransmission(transmission)) {
        transmissions.push_back(transmission);
        cGroupFigure *signalFigure = createSignalFigure(transmission);
        signalLayer->addFigure(signalFigure);
        setSignalFigure(transmission, signalFigure);
        setAnimationSpeed();
    }
}

void MediumCanvasVisualizer::transmissionRemoved(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (displaySignals && matchesTransmission(transmission)) {
        transmissions.erase(std::remove(transmissions.begin(), transmissions.end(), transmission));
        cFigure *signalFigure = getSignalFigure(transmission);
        removeSignalFigure(transmission);
        if (signalFigure != nullptr)
            delete signalLayer->removeFigure(signalFigure);
        setAnimationSpeed();
    }
}

void MediumCanvasVisualizer::transmissionStarted(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (matchesTransmission(transmission)) {
        if (displaySignals)
            setAnimationSpeed();
        if (displayTransmissions) {
            auto transmitter = transmission->getTransmitter();
            auto figure = getRadioFigure(transmitter);
            figure->getFigure(0)->setVisible(true);
        }
    }
}

void MediumCanvasVisualizer::transmissionEnded(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (matchesTransmission(transmission)) {
        if (displaySignals)
            setAnimationSpeed();
        if (displayTransmissions) {
            auto transmitter = transmission->getTransmitter();
            auto figure = getRadioFigure(transmitter);
            figure->getFigure(0)->setVisible(false);
        }
    }
}

void MediumCanvasVisualizer::receptionStarted(const IReception *reception)
{
    Enter_Method_Silent();
    if (matchesTransmission(reception->getTransmission())) {
        if (displaySignals)
            setAnimationSpeed();
        if (displayReceptions) {
            auto receiver = reception->getReceiver();
            auto figure = getRadioFigure(receiver);
            figure->getFigure(1)->setVisible(true);
        }
        if (displayCommunicationHeat) {
            const ITransmission *transmission = reception->getTransmission();
            const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
            Coord min = mediumLimitCache->getMinConstraintArea();
            Coord max = mediumLimitCache->getMaxConstraintArea();
            Coord delta = max - min;
            int x1 = std::round((communicationHeatMapSize - 1) * ((transmission->getStartPosition().x - min.x) / delta.x));
            int y1 = std::round((communicationHeatMapSize - 1) * ((transmission->getStartPosition().y - min.x) / delta.y));
            int x2 = std::round((communicationHeatMapSize - 1) * ((reception->getStartPosition().x - min.x) / delta.x));
            int y2 = std::round((communicationHeatMapSize - 1) * ((reception->getStartPosition().y - min.y) / delta.y));
            communicationHeat->heatLine(x1, y1, x2, y2);
        }
    }
}

void MediumCanvasVisualizer::receptionEnded(const IReception *reception)
{
    Enter_Method_Silent();
    if (matchesTransmission(reception->getTransmission())) {
        if (displaySignals)
            setAnimationSpeed();
        if (displayReceptions) {
            auto receiver = reception->getReceiver();
            auto figure = getRadioFigure(receiver);
            figure->getFigure(1)->setVisible(false);
        }
    }
}

} // namespace visualizer

} // namespace inet

