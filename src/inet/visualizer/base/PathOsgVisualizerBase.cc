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

#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/OSGScene.h"
#include "inet/common/OSGUtils.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/visualizer/base/PathOsgVisualizerBase.h"

#ifdef WITH_OSG
#include <osg/LineWidth>
#endif // ifdef WITH_OSG

namespace inet {

namespace visualizer {

#ifdef WITH_OSG

PathOsgVisualizerBase::PathOsgVisualization::PathOsgVisualization(const std::vector<int>& path, osg::Node *node) :
    PathVisualization(path),
    node(node)
{
}

PathOsgVisualizerBase::PathOsgVisualization::~PathOsgVisualization()
{
    // TODO: delete node;
}

void PathOsgVisualizerBase::refreshDisplay() const
{
    PathVisualizerBase::refreshDisplay();
    // TODO: switch to osg canvas when API is extended
    visualizerTargetModule->getCanvas()->setAnimationSpeed(pathVisualizations.empty() ? 0 : fadeOutAnimationSpeed, this);
}

const PathVisualizerBase::PathVisualization *PathOsgVisualizerBase::createPathVisualization(const std::vector<int>& path) const
{
    std::vector<Coord> points;
    for (auto id : path) {
        auto node = getSimulation()->getModule(id);
        points.push_back(getPosition(node));
    }
    auto node = inet::osg::createPolyline(points, cFigure::ARROW_NONE, cFigure::ARROW_BARBED);
    cFigure::Color color;
    if (!strcmp(par("lineColor"), "auto"))
        color = cFigure::GOOD_DARK_COLORS[pathVisualizations.size() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
    else
        color = lineColor;
    node->setStateSet(inet::osg::createLineStateSet(color, lineStyle, lineWidth));
    return new PathOsgVisualization(path, node);
}

void PathOsgVisualizerBase::addPathVisualization(std::pair<int, int> sourceAndDestination, const PathVisualization *pathVisualization)
{
    PathVisualizerBase::addPathVisualization(sourceAndDestination, pathVisualization);
    auto pathOsgVisualization = static_cast<const PathOsgVisualization *>(pathVisualization);
    auto scene = inet::osg::TopLevelScene::getSimulationScene(visualizerTargetModule);
    scene->addChild(pathOsgVisualization->node);
}

void PathOsgVisualizerBase::removePathVisualization(std::pair<int, int> sourceAndDestination, const PathVisualization *pathVisualization)
{
    PathVisualizerBase::removePathVisualization(sourceAndDestination, pathVisualization);
    auto pathOsgVisualization = static_cast<const PathOsgVisualization *>(pathVisualization);
    auto node = pathOsgVisualization->node;
    node->getParent(0)->removeChild(node);
}

void PathOsgVisualizerBase::setAlpha(const PathVisualization *pathVisualization, double alpha) const
{
    auto pathOsgVisualization = static_cast<const PathOsgVisualization *>(pathVisualization);
    auto node = pathOsgVisualization->node;
    auto stateSet = node->getOrCreateStateSet();
    auto material = static_cast<osg::Material *>(stateSet->getAttribute(osg::StateAttribute::MATERIAL));
    material->setAlpha(osg::Material::FRONT_AND_BACK, alpha);
}

#endif // ifdef WITH_OSG

} // namespace visualizer

} // namespace inet

