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
#include "inet/visualizer/transportlayer/TransportConnectionCanvasVisualizer.h"

namespace inet {

namespace visualizer {

Define_Module(TransportConnectionCanvasVisualizer);

TransportConnectionCanvasVisualizer::TransportConnectionCanvasVisualization::TransportConnectionCanvasVisualization(LabeledIcon *sourceFigure, LabeledIcon *destinationFigure, int sourceModuleId, int destinationModuleId, int count) :
    TransportConnectionVisualization(sourceModuleId, destinationModuleId, count),
    sourceFigure(sourceFigure),
    destinationFigure(destinationFigure)
{
}

void TransportConnectionCanvasVisualizer::initialize(int stage)
{
    TransportConnectionVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        zIndex = par("zIndex");
        auto canvas = visualizerTargetModule->getCanvas();
        canvasProjection = CanvasProjection::getCanvasProjection(canvas);
        networkNodeVisualizer = getModuleFromPar<NetworkNodeCanvasVisualizer>(par("networkNodeVisualizerModule"), this);
    }
}

LabeledIcon *TransportConnectionCanvasVisualizer::createConnectionEndFigure(tcp::TCPConnection *tcpConnection) const
{
    std::string icon(this->icon);
    auto figure = new LabeledIcon();
    figure->setZIndex(zIndex);
    figure->setTags("transport_connection");
    auto iconFigure = figure->getIconFigure();
    iconFigure->setTooltip("This icon represents a transport connection between two network nodes");
    iconFigure->setAssociatedObject(tcpConnection);
    iconFigure->setAnchor(cFigure::ANCHOR_NW);
    iconFigure->setImageName(icon.substr(0, icon.find_first_of(".")).c_str());
    iconFigure->setTintAmount(1);
    auto darkColorsLength = sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color);
    iconFigure->setTintColor(cFigure::GOOD_DARK_COLORS[connectionVisualizations.size() % darkColorsLength]);
    auto labelFigure = figure->getLabelFigure();
    labelFigure->setTooltip("This icon represents a transport connection between two network nodes");
    labelFigure->setAssociatedObject(tcpConnection);
    labelFigure->setPosition(iconFigure->getBounds().getSize() / 2);
    labelFigure->setAnchor(cFigure::ANCHOR_CENTER);
    labelFigure->setFont(labelFont);
    labelFigure->setColor(labelColor);
    char label[32];
    sprintf(label, "%ld", 1 + connectionVisualizations.size() / darkColorsLength);
    labelFigure->setText(label);
    return figure;
}

const TransportConnectionVisualizerBase::TransportConnectionVisualization *TransportConnectionCanvasVisualizer::createConnectionVisualization(cModule *source, cModule *destination, tcp::TCPConnection *tcpConnection) const
{
    auto sourceFigure = createConnectionEndFigure(tcpConnection);
    auto destinationFigure = createConnectionEndFigure(tcpConnection);
    return new TransportConnectionCanvasVisualization(sourceFigure, destinationFigure, source->getId(), destination->getId(), 1);
}

void TransportConnectionCanvasVisualizer::addConnectionVisualization(const TransportConnectionVisualization *connectionVisualization)
{
    TransportConnectionVisualizerBase::addConnectionVisualization(connectionVisualization);
    auto connectionCanvasVisualization = static_cast<const TransportConnectionCanvasVisualization *>(connectionVisualization);
    auto sourceModule = getSimulation()->getModule(connectionVisualization->sourceModuleId);
    auto sourceVisualization = networkNodeVisualizer->getNeworkNodeVisualization(getContainingNode(sourceModule));
    sourceVisualization->addAnnotation(connectionCanvasVisualization->sourceFigure, connectionCanvasVisualization->sourceFigure->getBounds().getSize());
    auto destinationModule = getSimulation()->getModule(connectionVisualization->destinationModuleId);
    auto destinationVisualization = networkNodeVisualizer->getNeworkNodeVisualization(getContainingNode(destinationModule));
    destinationVisualization->addAnnotation(connectionCanvasVisualization->destinationFigure, connectionCanvasVisualization->destinationFigure->getBounds().getSize());
}

void TransportConnectionCanvasVisualizer::removeConnectionVisualization(const TransportConnectionVisualization *connectionVisualization)
{
    TransportConnectionVisualizerBase::removeConnectionVisualization(connectionVisualization);
    auto connectionCanvasVisualization = static_cast<const TransportConnectionCanvasVisualization *>(connectionVisualization);
    auto sourceModule = getSimulation()->getModule(connectionVisualization->sourceModuleId);
    auto sourceVisualization = networkNodeVisualizer->getNeworkNodeVisualization(getContainingNode(sourceModule));
    sourceVisualization->removeAnnotation(connectionCanvasVisualization->sourceFigure);
    auto destinationModule = getSimulation()->getModule(connectionVisualization->destinationModuleId);
    auto destinationVisualization = networkNodeVisualizer->getNeworkNodeVisualization(getContainingNode(destinationModule));
    destinationVisualization->removeAnnotation(connectionCanvasVisualization->destinationFigure);
}

} // namespace visualizer

} // namespace inet

