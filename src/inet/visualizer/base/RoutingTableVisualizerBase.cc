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
#include "inet/common/NotifierConsts.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/visualizer/base/RoutingTableVisualizerBase.h"

namespace inet {

namespace visualizer {

RoutingTableVisualizerBase::RouteVisualization::RouteVisualization(int nodeModuleId, int nextHopModuleId) :
    ModuleLine(nodeModuleId, nextHopModuleId),
    nodeModuleId(nodeModuleId),
    nextHopModuleId(nextHopModuleId)
{
}

RoutingTableVisualizerBase::~RoutingTableVisualizerBase()
{
    if (displayRoutingTables)
        unsubscribe();
}

void RoutingTableVisualizerBase::initialize(int stage)
{
    VisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        subscriptionModule = getModuleFromPar<cModule>(par("subscriptionModule"), this);
        displayRoutingTables = par("displayRoutingTables");
        destinationFilter.setPattern(par("destinationFilter"));
        nodeFilter.setPattern(par("nodeFilter"));
        lineColor = cFigure::Color(par("lineColor"));
        lineStyle = cFigure::parseLineStyle(par("lineStyle"));
        lineWidth = par("lineWidth");
        lineShift = par("lineShift");
        lineShiftMode = par("lineShiftMode");
        lineContactSpacing = par("lineContactSpacing");
        lineContactMode = par("lineContactMode");
        lineManager = LineManager::getLineManager(visualizerTargetModule->getCanvas());
        if (displayRoutingTables)
            subscribe();
    }
}

void RoutingTableVisualizerBase::subscribe()
{
    subscriptionModule->subscribe(NF_ROUTE_ADDED, this);
    subscriptionModule->subscribe(NF_ROUTE_DELETED, this);
    subscriptionModule->subscribe(NF_ROUTE_CHANGED, this);
    subscriptionModule->subscribe(NF_INTERFACE_IPv4CONFIG_CHANGED, this);
}

void RoutingTableVisualizerBase::unsubscribe()
{
    // NOTE: lookup the module again because it may have been deleted first
    subscriptionModule = getModuleFromPar<cModule>(par("subscriptionModule"), this, false);
    if (subscriptionModule != nullptr) {
        subscriptionModule->unsubscribe(NF_ROUTE_ADDED, this);
        subscriptionModule->unsubscribe(NF_ROUTE_DELETED, this);
        subscriptionModule->unsubscribe(NF_ROUTE_CHANGED, this);
        subscriptionModule->unsubscribe(NF_INTERFACE_IPv4CONFIG_CHANGED, this);
    }
}

void RoutingTableVisualizerBase::receiveSignal(cComponent *source, simsignal_t signal, cObject *object, cObject *details)
{
    Enter_Method_Silent();
    if (signal == NF_ROUTE_ADDED || signal == NF_ROUTE_DELETED || signal == NF_ROUTE_CHANGED) {
        auto routingTable = check_and_cast<IIPv4RoutingTable *>(source);
        auto networkNode = getContainingNode(check_and_cast<cModule *>(source));
        if (nodeFilter.matches(networkNode))
            updateRoutes(routingTable);
    }
    else if (signal == NF_INTERFACE_IPv4CONFIG_CHANGED) {
        for (cModule::SubmoduleIterator it(getSystemModule()); !it.end(); it++) {
            auto networkNode = *it;
            if (isNetworkNode(networkNode) && destinationFilter.matches(networkNode)) {
                L3AddressResolver addressResolver;
                auto routingTable = addressResolver.findIPv4RoutingTableOf(networkNode);
                if (routingTable != nullptr)
                    updateRoutes(routingTable);
            }
        }
    }
    else
        throw cRuntimeError("Unknown signal");
}

const RoutingTableVisualizerBase::RouteVisualization *RoutingTableVisualizerBase::getRouteVisualization(std::pair<int, int> route)
{
    auto it = routeVisualizations.find(route);
    return it == routeVisualizations.end() ? nullptr : it->second;
}

void RoutingTableVisualizerBase::addRouteVisualization(const RouteVisualization *routeVisualization)
{
    auto key = std::pair<int, int>(routeVisualization->nodeModuleId, routeVisualization->nextHopModuleId);
    routeVisualizations[key] = routeVisualization;
}

void RoutingTableVisualizerBase::removeRouteVisualization(const RouteVisualization *routeVisualization)
{
    routeVisualizations.erase(routeVisualizations.find(std::pair<int, int>(routeVisualization->nodeModuleId, routeVisualization->nextHopModuleId)));
}

std::vector<IPv4Address> RoutingTableVisualizerBase::getDestinations()
{
    L3AddressResolver addressResolver;
    std::vector<IPv4Address> destinations;
    for (cModule::SubmoduleIterator it(getSystemModule()); !it.end(); it++) {
        auto networkNode = *it;
        if (isNetworkNode(networkNode) && destinationFilter.matches(networkNode)) {
            auto interfaceTable = addressResolver.findInterfaceTableOf(networkNode);
            for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
                auto interface = interfaceTable->getInterface(i);
                if (interface->ipv4Data() != nullptr) {
                    auto address = interface->ipv4Data()->getIPAddress();
                    if (!address.isUnspecified())
                        destinations.push_back(address);
                }
            }
        }
    }
    return destinations;
}

void RoutingTableVisualizerBase::addRoutes(IIPv4RoutingTable *routingTable)
{
    L3AddressResolver addressResolver;
    auto node = getContainingNode(check_and_cast<cModule *>(routingTable));
    for (auto destination : getDestinations()) {
        if (!routingTable->isLocalAddress(destination)) {
            auto route = routingTable->findBestMatchingRoute(destination);
            if (route != nullptr) {
                auto gateway = route->getGateway();
                auto nextHop = addressResolver.findHostWithAddress(gateway.isUnspecified() ? destination : gateway);
                if (nextHop != nullptr) {
                    auto key = std::pair<int, int>(node->getId(), nextHop->getId());
                    if (routeVisualizations.find(key) == routeVisualizations.end())
                        addRouteVisualization(createRouteVisualization(route, node, nextHop));
                }
            }
        }
    }
}

void RoutingTableVisualizerBase::removeRoutes(IIPv4RoutingTable *routingTable)
{
    auto node = getContainingNode(check_and_cast<cModule *>(routingTable));
    std::vector<const RouteVisualization*> removedRoutes;
    for (auto it : routeVisualizations)
        if (it.first.first == node->getId() && it.second)
            removedRoutes.push_back(it.second);
    for (auto it : removedRoutes) {
        removeRouteVisualization(it);
        delete it;
    }
}

void RoutingTableVisualizerBase::updateRoutes(IIPv4RoutingTable *routingTable)
{
    removeRoutes(routingTable);
    addRoutes(routingTable);
}

} // namespace visualizer

} // namespace inet

